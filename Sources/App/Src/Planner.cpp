#include "Planner.h"

#include <ctype.h>
#include <string.h>
#include <math.h>

#include "settings_manager.h"
#include "Conveyor.h"


Planner::Planner(void)
{
    memset((void*)&this->m_previous_unit_vector[0], 0, sizeof(this->m_previous_unit_vector));
    m_junction_deviation = Settings_Manager::GetJunctionDeviation_mm();
    
    memset((void*)&this->m_position_steps[0], 0, sizeof(this->m_position_steps));
    
    m_conveyor = NULL;
}


Planner::~Planner(void)
{
}

int Planner::AppendLine(const float* target_mm, float spindle_speed, float rate_mm_s, bool inverseTimeRate)
{
    uint32_t index;
    int32_t target_steps[TOTAL_AXES_COUNT];
    
    float unit_vec[TOTAL_AXES_COUNT];
    float distance = 0.0f;
    float delta_mm = 0.0f;
    
    float vmax_junction = 0.0f;
    
    Block* block = m_conveyor->queue.head_ref();
    
    
    for (index = COORD_X; index < TOTAL_AXES_COUNT; index++)
    {
        // Calculate how many steps from mm and steps per mm settings
        target_steps[index] = lround(target_mm[index] * Settings_Manager::GetStepsPer_mm_Axis(index));  // Check to use lroundf
        
        // Calculate absolute count of steps to reach from current position to target position
        block->steps[index] = labs(target_steps[index] - this->m_position_steps[index]);
        
        // Find maximum step count
        block->steps_event_count = std::max(block->steps_event_count, block->steps[index]);
        
        // Calculate distance to move each axis in mm
        delta_mm = (target_steps[index] - this->m_position_steps[index]) / Settings_Manager::GetStepsPer_mm_Axis(index);

        // Copy this delta_mm to unit vector's numerator
        unit_vec[index] = delta_mm;
        
        // Update direction bits [1 for negative, 0 for positive]
        if (delta_mm < 0.0f)
            block->direction_bits |= (1 << index);
        
        // Accumulate delta_mm^2 to calculate distance
        distance += (delta_mm * delta_mm);
    }
    
    // Check if any move 
    if (distance == 0.0f)
        return PLANNER_OK;  // Nothing to do
    
    // Calculate square root of the sum of squares obtained previously
    distance = sqrtf(distance);
    block->millimeters = distance;
    
    // Complete the calculation of unit vector
    for (index = COORD_X; index < TOTAL_AXES_COUNT; index++)
        unit_vec[index] /= distance;
    
    // Limit rate_mm_s value to maximum allowed    
    rate_mm_s = limit_value_by_axis_maximum(rate_mm_s, Settings_Manager::GetMaxSpeed_mm_sec_all_axes(), unit_vec);
    
    // Limit acceleration value to maximum allowed
    block->acceleration = limit_value_by_axis_maximum(SOME_LARGE_VALUE, Settings_Manager::GetAcceleration_mm_sec2_all_axes(), unit_vec);
    
    // Determine nominal speeds/rates
    if (distance > 0.0f)
    {
        block->nominal_speed = rate_mm_s;
        block->nominal_rate = block->steps_event_count * rate_mm_s / distance; // steps/sec
    }
    else
    {
        block->nominal_speed = 0.0f;
        block->nominal_rate = 0.0f;
    }        
    
    // Calculate junction deviation speeds
    if (m_conveyor->is_queue_empty() == false)
    {
        // We have a previous block. Get it
        Block * prev_block = m_conveyor->queue.item_ref( m_conveyor->queue.prev( m_conveyor->queue.head_i ) );
        float previous_nominal_speed = prev_block->nominal_speed;
        
        if (this->m_junction_deviation > 0.0f && previous_nominal_speed > 0.0f)
        {
            // Compute cosine of angle between previous and current path. (prev_unit_vec is negative)
            // NOTE: Max junction velocity is computed without sin() or acos() by trig half angle identity.
            float cos_theta = 0.0f;
                              
            for (index = COORD_X; index < TOTAL_AXES_COUNT; index++)
                cos_theta -= (this->m_previous_unit_vector[index] * unit_vec[index]);

            // Skip and use default max junction speed for 0 degree acute junction.
            if (cos_theta <= 0.9999f) 
            {
                vmax_junction = std::min(previous_nominal_speed, block->nominal_speed);
                
                // Skip and avoid divide by zero for straight junctions at 180 degrees. Limit to min() of nominal speeds.
                if (cos_theta >= -0.9999f) 
                {
                    // Compute maximum junction velocity based on maximum acceleration and junction deviation
                    float sin_theta_d2 = sqrtf(0.5f * (1.0f - cos_theta)); // Trig half angle identity. Always positive.
                    
                    vmax_junction = std::min(vmax_junction, sqrtf(block->acceleration * m_junction_deviation * sin_theta_d2 / (1.0f - sin_theta_d2)));
                }
            }
        }
    }
    
    block->max_entry_speed = vmax_junction;
    
    // Initialize block entry speed. Compute based on deceleration to user-defined minimum_planner_speed.
    float v_allowable = max_allowable_speed(-block->acceleration, 0.0f, block->millimeters);
    
    block->entry_speed = std::min(vmax_junction, v_allowable);
    
    
    // Initialize planner efficiency flags
    // Set flag if block will always reach maximum junction speed regardless of entry/exit speeds.
    // If a block can de/ac-celerate from nominal speed to zero within the length of the block, then
    // the current block and next block junction speeds are guaranteed to always be at their maximum
    // junction speeds in deceleration and acceleration, respectively. This is due to how the current
    // block nominal speed limits both the current and next maximum junction speeds. Hence, in both
    // the reverse and forward planners, the corresponding block junction speed will always be at the
    // the maximum junction speed and may always be ignored for any speed reduction checks.
    if (block->nominal_speed <= v_allowable) 
        block->nominal_length_flag = true; 
    else 
        block->nominal_length_flag = false; 

    // Always calculate trapezoid for new block
    block->recalculate_flag = true;

    // Update previous path unit_vector and position in steps
    memcpy(m_previous_unit_vector, unit_vec, sizeof(m_previous_unit_vector)); // previous_unit_vec[] = unit_vec[]
    memcpy(m_position_steps, target_steps, sizeof(m_position_steps));
    
    // Math-heavy re-computing of the whole queue to take the new
    this->recalculate();

    // The block can now be used
    block->ready();

    m_conveyor->queue_head_block();
    
    return PLANNER_OK;
}

float Planner::limit_value_by_axis_maximum(float limit_value, const float * max_values, const float * unit_vector)
{
    uint32_t idx;
    
    for (idx = 0; idx < TOTAL_AXES_COUNT; idx++) 
    {
        if (unit_vector[idx] != 0.0f) 
        {  
            // Avoid divide by zero.
            limit_value = std::min(limit_value, fabsf(max_values[idx] / unit_vector[idx]));
        }
    }
  
    return (limit_value);
}

void Planner::recalculate()
{
    unsigned int block_index;

    Block* previous;
    Block* current;

    /*
     * a newly added block is decel limited
     *
     * we find its max entry speed given its exit speed
     *
     * for each block, walking backwards in the queue:
     *
     * if max entry speed == current entry speed
     * then we can set recalculate to false, since clearly adding another block didn't allow us to enter faster
     * and thus we don't need to check entry speed for this block any more
     *
     * once we find an accel limited block, we must find the max exit speed and walk the queue forwards
     *
     * for each block, walking forwards in the queue:
     *
     * given the exit speed of the previous block and our own max entry speed
     * we can tell if we're accel or decel limited (or coasting)
     *
     * if prev_exit > max_entry
     *     then we're still decel limited. update previous trapezoid with our max entry for prev exit
     * if max_entry >= prev_exit
     *     then we're accel limited. set recalculate to false, work out max exit speed
     *
     * finally, work out trapezoid for the final (and newest) block.
     */

    /*
     * Step 1:
     * For each block, given the exit speed and acceleration, find the maximum entry speed
     */

    float entry_speed = m_minimum_planner_speed;

    block_index = m_conveyor->queue.head_i;
    current     = m_conveyor->queue.item_ref(block_index);

    if (!m_conveyor->queue.is_empty()) 
    {
        while ((block_index != m_conveyor->queue.tail_i) && current->recalculate_flag) 
        {
            entry_speed = current->reverse_pass(entry_speed);

            block_index = m_conveyor->queue.prev(block_index);
            current     = m_conveyor->queue.item_ref(block_index);
        }

        /*
         * Step 2:
         * now current points to either tail or first non-recalculate block
         * and has not had its reverse_pass called
         * or its calculate_trapezoid
         * entry_speed is set to the *exit* speed of current.
         * each block from current to head has its entry speed set to its max entry speed- limited by decel or nominal_rate
         */

        float exit_speed = current->max_exit_speed();

        while (block_index != m_conveyor->queue.head_i) 
        {
            previous    = current;
            block_index = m_conveyor->queue.next(block_index);
            current     = m_conveyor->queue.item_ref(block_index);

            // we pass the exit speed of the previous block
            // so this block can decide if it's accel or decel limited and update its fields as appropriate
            exit_speed = current->forward_pass(exit_speed);

            previous->calculate_trapezoid(previous->entry_speed, current->entry_speed);
        }
    }

    /*
     * Step 3:
     * work out trapezoid for final (and newest) block
     */

    // now current points to the head item
    // which has not had calculate_trapezoid run yet
    current->calculate_trapezoid(current->entry_speed, m_minimum_planner_speed);
}


// Calculates the maximum allowable speed at this point when you must be able to reach target_velocity using the
// acceleration within the allotted distance.
float Planner::max_allowable_speed(float acceleration, float target_velocity, float distance)
{
    // Was acceleration*60*60*distance, in case this breaks, but here we prefer to use seconds instead of minutes
    return(sqrtf(target_velocity * target_velocity - 2.0F * acceleration * distance));
}




const char* Planner::GetErrorText(uint32_t error_code)
{
    switch (error_code)
    {
    case PLANNER_OK:
        return("OK");

    default:
        return("Unknown planner error code");
    }
}

