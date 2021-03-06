/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

#include <cmath>
#include <string>
#include <math.h>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "Block.h"

#define STEP_TICKER_FREQUENCY 100000

// A block represents a movement, it's length for each stepper motor, and the corresponding acceleration curves.
// It's stacked on a queue, and that queue is then executed in order, to move the motors.
// Most of the accel math is also done in this class
// And GCode objects for use in on_gcode_execute are also help in here

Block::Block()
{
    tick_info = NULL;
    clear();
}

void Block::clear()
{
    is_ready            = false;

    memset((void*)steps, 0, sizeof(steps));

    steps_event_count   = 0;
    nominal_rate        = 0.0F;
    nominal_speed       = 0.0F;
    millimeters         = 0.0F;
    entry_speed         = 0.0F;
    exit_speed          = 0.0F;
    acceleration        = 100.0F; // we don't want to get divide by zeroes if this is not set
    initial_rate        = 0.0F;
    accelerate_until    = 0;
    decelerate_after    = 0;
    direction_bits      = 0;
    recalculate_flag    = false;
    nominal_length_flag = false;
    max_entry_speed     = 0.0F;
    is_ticking          = false;
    is_g123             = false;
    locked              = false;
    //s_value             = 0.0F;

    total_move_ticks = 0;
    
    if (tick_info == NULL) 
    {
        // we create this once for this block
        tick_info= new tickinfo_t[TOTAL_AXES_COUNT]; //(tickinfo_t *)malloc(sizeof(tickinfo_t) * n_actuators);
    }
    
    memset(tick_info, 0, sizeof(tickinfo_t) * TOTAL_AXES_COUNT);
}


/* Calculates trapezoid parameters so that the entry- and exit-speed is compensated by the provided factors.
// The factors represent a factor of braking and must be in the range 0.0-1.0.
//                                +--------+ <- nominal_rate
//                               /          \
// nominal_rate*entry_factor -> +            \
//                              |             + <- nominal_rate*exit_factor
//                              +-------------+
//                                  time -->
*/
void Block::calculate_trapezoid(float entryspeed, float exitspeed)
{
    // if block is currently executing, don't touch anything!
    if (is_ticking) 
        return;

    float initial_rate = this->nominal_rate * (entryspeed / this->nominal_speed); // steps/sec
    float final_rate = this->nominal_rate * (exitspeed / this->nominal_speed);
    
    // How many steps ( can be fractions of steps, we need very precise values ) to accelerate and decelerate
    // This is a simplification to get rid of rate_delta and get the steps/s² accel directly from the mm/s² accel
    float acceleration_per_second = (this->acceleration * this->steps_event_count) / this->millimeters;

    float maximum_possible_rate = sqrtf( ( this->steps_event_count * acceleration_per_second ) + ( ( (initial_rate * initial_rate) + (final_rate * final_rate) ) / 2.0F ) );

    // Now this is the maximum rate we'll achieve this move, either because
    // it's the higher we can achieve, or because it's the higher we are
    // allowed to achieve
    this->maximum_rate = std::min(maximum_possible_rate, this->nominal_rate);

    // Now figure out how long it takes to accelerate in seconds
    float time_to_accelerate = ( this->maximum_rate - initial_rate ) / acceleration_per_second;

    // Now figure out how long it takes to decelerate
    float time_to_decelerate = ( final_rate -  this->maximum_rate ) / -acceleration_per_second;

    // Now we know how long it takes to accelerate and decelerate, but we must
    // also know how long the entire move takes so we can figure out how long
    // is the plateau if there is one
    float plateau_time = 0;

    // Only if there is actually a plateau ( we are limited by nominal_rate )
    if (maximum_possible_rate > this->nominal_rate) 
    {
        // Figure out the acceleration and deceleration distances ( in steps )
        float acceleration_distance = ( ( initial_rate + this->maximum_rate ) / 2.0F ) * time_to_accelerate;
        float deceleration_distance = ( ( this->maximum_rate + final_rate ) / 2.0F ) * time_to_decelerate;

        // Figure out the plateau steps
        float plateau_distance = this->steps_event_count - acceleration_distance - deceleration_distance;

        // Figure out the plateau time in seconds
        plateau_time = plateau_distance / this->maximum_rate;
    }

    // Figure out how long the move takes total ( in seconds )
    float total_move_time = time_to_accelerate + time_to_decelerate + plateau_time;

    // We now have the full timing for acceleration, plateau and deceleration,
    // yay \o/ Now this is very important these are in seconds, and we need to
    // round them into ticks. This means instead of accelerating in 100.23
    // ticks we'll accelerate in 100 ticks. Which means to reach the exact
    // speed we want to reach, we must figure out a new/slightly different
    // acceleration/deceleration to be sure we accelerate and decelerate at
    // the exact rate we want

    // First off round total time, acceleration time and deceleration time in ticks
    uint32_t acceleration_ticks = floorf( time_to_accelerate * STEP_TICKER_FREQUENCY ); // time_to_accel * 100 kHz
    uint32_t deceleration_ticks = floorf( time_to_decelerate * STEP_TICKER_FREQUENCY );
    uint32_t total_move_ticks   = floorf( total_move_time    * STEP_TICKER_FREQUENCY );

    // Now deduce the plateau time for those new values expressed in tick
    //uint32_t plateau_ticks = total_move_ticks - acceleration_ticks - deceleration_ticks;

    // Now we figure out the acceleration value to reach EXACTLY maximum_rate(steps/s) in EXACTLY acceleration_ticks(ticks) amount of time in seconds
    float acceleration_time = ((float)(acceleration_ticks)) / STEP_TICKER_FREQUENCY;  // This can be moved into the operation below, separated for clarity, note we need to do this instead of using time_to_accelerate(seconds) directly because time_to_accelerate(seconds) and acceleration_ticks(seconds) do not have the same value anymore due to the rounding
    float deceleration_time = ((float)(deceleration_ticks)) / STEP_TICKER_FREQUENCY;

    float acceleration_in_steps = (acceleration_time > 0.0F ) ? ( this->maximum_rate - initial_rate ) / acceleration_time : 0;
    float deceleration_in_steps =  (deceleration_time > 0.0F ) ? ( this->maximum_rate - final_rate ) / deceleration_time : 0;

    // we have a potential race condition here as we could get interrupted anywhere in the middle of this call, we need to lock
    // the updates to the blocks to get around it
    this->locked= true;
    
    // Now figure out the two acceleration ramp change events in ticks
    this->accelerate_until = acceleration_ticks;
    this->decelerate_after = total_move_ticks - deceleration_ticks;

    // We now have everything we need for this block to call a Steppermotor->move method !!!!
    // Theorically, if accel is done per tick, the speed curve should be perfect.
    this->total_move_ticks = total_move_ticks;

    this->initial_rate = initial_rate;
    this->exit_speed = exitspeed;

    // prepare the block for stepticker
    this->prepare(acceleration_in_steps, deceleration_in_steps);

    this->locked= false;
}

// Calculates the maximum allowable speed at this point when you must be able to reach target_velocity using the
// acceleration within the allotted distance.
float Block::max_allowable_speed(float acceleration, float target_velocity, float distance)
{
    return sqrtf(target_velocity * target_velocity - 2.0f * acceleration * distance);
}

// Called by Planner::recalculate() when scanning the plan from last to first entry.
float Block::reverse_pass(float exit_speed)
{
    // If entry speed is already at the maximum entry speed, no need to recheck. Block is cruising.
    // If not, block in state of acceleration or deceleration. Reset entry speed to maximum and
    // check for maximum allowable speed reductions to ensure maximum possible planned speed.
    if (this->entry_speed != this->max_entry_speed) 
    {
        // If nominal length true, max junction speed is guaranteed to be reached. Only compute
        // for max allowable speed if block is decelerating and nominal length is false.
        if ((!this->nominal_length_flag) && (this->max_entry_speed > exit_speed)) 
        {
            float max_entry_speed = max_allowable_speed(-this->acceleration, exit_speed, this->millimeters);

            this->entry_speed = std::min(max_entry_speed, this->max_entry_speed);

            return this->entry_speed;
        } 
        else
            this->entry_speed = this->max_entry_speed;
    }

    return this->entry_speed;
}


// Called by Planner::recalculate() when scanning the plan from first to last entry.
// returns maximum exit speed of this block
float Block::forward_pass(float prev_max_exit_speed)
{
    // If the previous block is an acceleration block, but it is not long enough to complete the
    // full speed change within the block, we need to adjust the entry speed accordingly. Entry
    // speeds have already been reset, maximized, and reverse planned by reverse planner.
    // If nominal length is true, max junction speed is guaranteed to be reached. No need to recheck.

    // TODO: find out if both of these checks are necessary
    if (prev_max_exit_speed > nominal_speed)
        prev_max_exit_speed = nominal_speed;
    
    if (prev_max_exit_speed > max_entry_speed)
        prev_max_exit_speed = max_entry_speed;

    if (prev_max_exit_speed <= entry_speed) 
    {
        // accel limited
        entry_speed = prev_max_exit_speed;
        // since we're now acceleration or cruise limited
        // we don't need to recalculate our entry speed anymore
        recalculate_flag = false;
    }
    // else
    // // decel limited, do nothing

    return max_exit_speed();
}

float Block::max_exit_speed()
{
    // if block is currently executing, return cached exit speed from calculate_trapezoid
    // this ensures that a block following a currently executing block will have correct entry speed
    if (is_ticking)
        return this->exit_speed;

    // if nominal_length_flag is asserted
    // we are guaranteed to reach nominal speed regardless of entry speed
    // thus, max exit will always be nominal
    if (nominal_length_flag)
        return nominal_speed;

    // otherwise, we have to work out max exit speed based on entry and acceleration
    float max = max_allowable_speed(-this->acceleration, this->entry_speed, this->millimeters);

    return std::min(max, nominal_speed);
}

// prepare block for the step ticker, called everytime the block changes
// this is done during planning so does not delay tick generation and step ticker can simply grab the next block during the interrupt
void Block::prepare(float acceleration_in_steps, float deceleration_in_steps)
{
    float inv = 1.0f / this->steps_event_count;

    // Now figure out the acceleration PER TICK, this should ideally be held as a double as it's very critical to the block timing
    // steps/tick^2
    // was....
    // float acceleration_per_tick = acceleration_in_steps / STEP_TICKER_FREQUENCY_2; // that is 100,000² too big for a float
    // float deceleration_per_tick = deceleration_in_steps / STEP_TICKER_FREQUENCY_2;
    double acceleration_per_tick = acceleration_in_steps * fp_scale; // this is now scaled to fit a 2.30 fixed point number
    double deceleration_per_tick = deceleration_in_steps * fp_scale;

    for (uint8_t m = 0; m < TOTAL_AXES_COUNT; m++) 
    {
        uint32_t steps = this->steps[m];
        
        this->tick_info[m].steps_to_move = steps;
        
        if (steps == 0) 
            continue;

        float aratio = inv * steps; // steps[m] / steps_event_count

        this->tick_info[m].steps_per_tick = (int64_t)round((((double)this->initial_rate * aratio) / STEP_TICKER_FREQUENCY) * STEPTICKER_FPSCALE); // steps/sec / tick frequency to get steps per tick in 2.62 fixed point
        this->tick_info[m].counter = 0; // 2.62 fixed point
        this->tick_info[m].step_count = 0;
        this->tick_info[m].next_accel_event = this->total_move_ticks + 1;

        double acceleration_change = 0;
        
        if (this->accelerate_until != 0) 
        { 
            // If the next accel event is the end of accel
            this->tick_info[m].next_accel_event = this->accelerate_until;
            acceleration_change = acceleration_per_tick;

        } 
        else if (this->decelerate_after == 0 /*&& this->accelerate_until == 0*/) 
        {
            // we start off decelerating
            acceleration_change = -deceleration_per_tick;
        } 
        else if (this->decelerate_after != this->total_move_ticks /*&& this->accelerate_until == 0*/) 
        {
            // If the next event is the start of decel ( don't set this if the next accel event is accel end )
            this->tick_info[m].next_accel_event = this->decelerate_after;
        }

        // already converted to fixed point just needs scaling by ratio
        //#define STEPTICKER_TOFP(x) ((int64_t)round((double)(x)*STEPTICKER_FPSCALE))
        this->tick_info[m].acceleration_change= (int64_t)round(acceleration_change * aratio);
        this->tick_info[m].deceleration_change= -(int64_t)round(deceleration_per_tick * aratio);
        this->tick_info[m].plateau_rate= (int64_t)round(((this->maximum_rate * aratio) / STEP_TICKER_FREQUENCY) * STEPTICKER_FPSCALE);
    }
}

// returns current rate (steps/sec) for the given actuator
float Block::get_trapezoid_rate(int i) const
{
    // convert steps per tick from fixed point to float and convert to steps/sec
    // FIXME steps_per_tick can change at any time, potential race condition if it changes while being read here
    return STEPTICKER_FROMFP(tick_info[i].steps_per_tick) * STEP_TICKER_FREQUENCY;
}
