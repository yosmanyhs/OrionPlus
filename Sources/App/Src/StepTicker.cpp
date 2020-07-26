#include "StepTicker.h"
#include <string.h>
#include <stm32f4xx_hal.h>
#include <math.h>

#include "settings_manager.h"
#include "hw_timers.h"
#include "pins.h"

#include "user_tasks.h"
#include "MachineCore.h"


StepTicker *StepTicker::instance;

StepTicker::StepTicker()
{
    uint8_t pulse_us;
    
    instance = this; // setup the Singleton instance of the stepticker

    // Default start values
    this->set_frequency(100000);
    
    pulse_us = (uint8_t)Settings_Manager::GetPulseLenTime_us();
    
    if (pulse_us >= 10)
        this->set_unstep_time(pulse_us);
    else
        this->set_unstep_time(10);

    this->unstep_bits = 0x00;
    this->running = false;
    this->current_block = NULL;    
    this->current_tick = 0;
    
    this->motor_enable_bits = 0;
    this->inversion_mask_bits_steps = ((uint8_t)(Settings_Manager::GetSignalInversionMasks() & SIGNAL_INVERT_STEP_PINS_MASK));  
    this->inversion_mask_bits_dirs =  ((uint8_t)(Settings_Manager::GetSignalInversionMasks() & SIGNAL_INVERT_DIR_PINS_MASK));  
}

StepTicker::~StepTicker()
{
}

//called when everything is setup and interrupts can start
void StepTicker::start()
{
    uint32_t off_mask32;
    
    current_tick = 0;
    
    // Setup GPIO pins to default "off" values for STEP/DIR/RESET/ENABLE 
    off_mask32 = (this->inversion_mask_bits_steps | this->inversion_mask_bits_dirs);
    off_mask32 ^= (SIGNAL_INVERT_STEP_PINS_MASK | SIGNAL_INVERT_DIR_PINS_MASK);  // Invert polarity selection bits
    off_mask32 <<= 16;
    off_mask32 |= ((uint32_t)(this->inversion_mask_bits_steps | this->inversion_mask_bits_dirs));
    
    STEP_PINS_GPIO_PORT->BSRR = off_mask32;
    
    // Start with stepper motors disabled
    this->EnableStepperDrivers(false);
    
    __HAL_TIM_ENABLE_IT(&step_timer_handle, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE_IT(&unstep_timer_handle, TIM_IT_UPDATE);
}


void StepTicker::ApplyUpdatedInversionMasks()
{
    // Update signal inversion mask
    this->inversion_mask_bits_steps = ((uint8_t)(Settings_Manager::GetSignalInversionMasks() & 
                                      (SIGNAL_INVERT_STEP_PINS_MASK)));
    
    this->inversion_mask_bits_dirs =  ((uint8_t)(Settings_Manager::GetSignalInversionMasks() & 
                                      (SIGNAL_INVERT_DIR_PINS_MASK))); 
}

void StepTicker::ResetStepperDrivers(bool reset)
{
    bool reset_high = ((Settings_Manager::GetSignalInversionMasks() & SIGNAL_INVERT_STP_RST) != 0);
    uint32_t bsrr_val = 0;
    
    if (reset == true)
    {
        if (reset_high == true)
            bsrr_val = (STEP_RESET_Pin);
        else
            bsrr_val = (STEP_RESET_Pin << 16);
    }
    else
    {
        if (reset_high == true)
            bsrr_val = (STEP_RESET_Pin << 16);
        else
            bsrr_val = (STEP_RESET_Pin);
    }
    
    STEP_RESET_GPIO_Port->BSRR = bsrr_val;
}

void StepTicker::EnableStepperDrivers(bool enable)
{
    bool enable_high = ((Settings_Manager::GetSignalInversionMasks() & SIGNAL_INVERT_STP_ENA) != 0);
    uint32_t bsrr_val = 0;
    
    if (enable == true)
    {
        if (enable_high == true)
            bsrr_val = (STEP_ENABLE_Pin);
        else
            bsrr_val = (STEP_ENABLE_Pin << 16);
    }
    else
    {
        if (enable_high == true)
            bsrr_val = (STEP_ENABLE_Pin << 16);
        else
            bsrr_val = (STEP_ENABLE_Pin);
    }
    
    STEP_ENABLE_GPIO_Port->BSRR = bsrr_val;
}

// Set the base stepping frequency
void StepTicker::set_frequency( float frequency )
{
    if (frequency > 0.0f)
    {
        this->frequency = frequency;    
        this->period = floorf((SystemCoreClock / 2.0f) / frequency); // SystemCoreClock/4 = Timer increments in a second
        
        __HAL_TIM_SET_AUTORELOAD(&step_timer_handle, (uint32_t)(this->period - 1));
    }
}

// Set the reset delay, must be called after set_frequency
void StepTicker::set_unstep_time( uint8_t microseconds )
{
    __HAL_TIM_SET_AUTORELOAD(&unstep_timer_handle, (uint32_t)(microseconds - 1));
}

/*
 *  Step Z -> PC0
 *  Step Y -> PC2
 *  Step X -> PC4
 *
 *  Dir Z  -> PC1
 *  Dir Y  -> PC3
 *  Dir X  -> PC5
 */


// Reset step pins on any motor that was stepped
void StepTicker::unstep_tick()
{
    uint32_t bits_to_update_bsrr;
    uint32_t off_mask32;
    uint32_t move32;
    
    // Revert modified bits in unstep_bits to 'default' values (mask values)    
    // Generate mask
    off_mask32 = ((this->inversion_mask_bits_steps ^ SIGNAL_INVERT_STEP_PINS_MASK) << 16);  // Invert polarity selection bits
    off_mask32 |= ((uint32_t)(this->inversion_mask_bits_steps));
    
    // Generate move bits
    move32 = ((uint32_t)(this->unstep_bits << 16)) | (this->unstep_bits);
    
    // Finally combine desired bits to change with polarity selection mask
    bits_to_update_bsrr = off_mask32 & move32;

    STEP_PINS_GPIO_PORT->BSRR = bits_to_update_bsrr;
    this->unstep_bits = 0;
}

// step clock
void StepTicker::step_tick (void)
{
    uint8_t execute_this_steps = 0;
    
    // if nothing has been setup we ignore the ticks
    if (!running)
    {
        // check if anything new available
        if(m_conveyor->get_next_block(&current_block)) 
        {
            // returns false if no new block is available
            running = start_next_block(); // returns true if there is at least one motor with steps to issue
            
            if (!running) 
            {
                __HAL_TIM_DISABLE(&step_timer_handle);
                
                // Turn Off Activity LED [Write 1]
                HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
                return;
            }
        }
        else
        {
            __HAL_TIM_DISABLE(&step_timer_handle);
            
            // Turn Off Activity LED [Write 1]
            HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_SET);
            return;
        }
    }

    if (machine->IsHalted())
    {
        running = false;
        current_tick = 0;
        current_block = NULL;
        return;
    }

    bool still_moving = false;
    
    // foreach motor, if it is active see if time to issue a step to that motor
    for (uint8_t motor_idx = 0; motor_idx < TOTAL_AXES_COUNT; motor_idx++) 
    {
        if (current_block->tick_info[motor_idx].steps_to_move == 0) 
            continue; // not active

        current_block->tick_info[motor_idx].steps_per_tick += current_block->tick_info[motor_idx].acceleration_change;

        // Speed curve state management [Acceleration, Plateau, Deceleration]
        if (current_tick == current_block->tick_info[motor_idx].next_accel_event) 
        {
            if (current_tick == current_block->accelerate_until) 
            {
                // We are done accelerating, acceleration becomes 0 : plateau
                current_block->tick_info[motor_idx].acceleration_change = 0;
                
                if (current_block->decelerate_after < current_block->total_move_ticks)
                {
                    current_block->tick_info[motor_idx].next_accel_event = current_block->decelerate_after;
                    
                    if (current_tick != current_block->decelerate_after) 
                    { 
                        // We are plateauing
                        // steps/sec / tick frequency to get steps per tick
                        current_block->tick_info[motor_idx].steps_per_tick = current_block->tick_info[motor_idx].plateau_rate;
                    }
                }
            }

            if (current_tick == current_block->decelerate_after) 
            {
                // We start decelerating
                current_block->tick_info[motor_idx].acceleration_change = current_block->tick_info[motor_idx].deceleration_change;
            }
        }

        // protect against rounding errors and such
        if (current_block->tick_info[motor_idx].steps_per_tick <= 0)
        {
            current_block->tick_info[motor_idx].counter = STEPTICKER_FPSCALE; // we force completion of this step by setting to 1.0
            current_block->tick_info[motor_idx].steps_per_tick = 0;
        }

        current_block->tick_info[motor_idx].counter += current_block->tick_info[motor_idx].steps_per_tick;

        if (current_block->tick_info[motor_idx].counter >= STEPTICKER_FPSCALE) 
        {
            // >= 1.0 step time
            current_block->tick_info[motor_idx].counter -= STEPTICKER_FPSCALE; // -= 1.0F;
            ++current_block->tick_info[motor_idx].step_count;

            
            bool ismoving = false;
            
            // Check if current motor is allowed to move
            if (((1 << motor_idx) & this->motor_enable_bits) != 0)
            {
                execute_this_steps |= (1 << (4 - (motor_idx * 2)));     // Swap/Move bits ZYX -> 0X0Y0Z
                ismoving = true;
            }

            if (!ismoving || current_block->tick_info[motor_idx].step_count == current_block->tick_info[motor_idx].steps_to_move) 
            {
                // done
                current_block->tick_info[motor_idx].steps_to_move = 0;
                this->motor_enable_bits &= ~(1 << motor_idx); // let motor know it is no longer moving
            }
        }

        // see if any motors are still moving after this tick
        if (((1 << motor_idx) & this->motor_enable_bits) != 0)
            still_moving = true;
    }   // end for    
    
    if (execute_this_steps != 0)
    {
        uint32_t bits_to_update_bsrr;
        uint32_t mask32;
        uint32_t move32;
        
        // Update which bits need to be restored
        this->unstep_bits = execute_this_steps;
                
        // Generate mask
        mask32 = ((this->inversion_mask_bits_steps ^ SIGNAL_INVERT_STEP_PINS_MASK));  // Invert polarity selection bits
        mask32 |= ((uint32_t)(this->inversion_mask_bits_steps << 16));
        
        // Generate move bits
        move32 = ((uint32_t)(execute_this_steps << 16)) | (execute_this_steps);
        
        // Finally combine desired bits to change with polarity selection mask
        bits_to_update_bsrr = mask32 & move32;
        
        STEP_PINS_GPIO_PORT->BSRR = bits_to_update_bsrr;
        
        // If activated any step signal then start unstep timer
        __HAL_TIM_ENABLE(&unstep_timer_handle);
    }

    // do this after so we start at tick 0
    current_tick++; // count number of ticks

    // see if any motors are still moving
    if(!still_moving) 
    {
        //SET_STEPTICKER_DEBUG_PIN(0);

        // all moves finished
        current_tick = 0;

        // get next block
        // do it here so there is no delay in ticks
        m_conveyor->block_finished();

        if (m_conveyor->get_next_block(&current_block)) 
        { 
            // returns false if no new block is available
            running = start_next_block(); // returns true if there is at least one motor with steps to issue
        }
        else
        {
            current_block = NULL;
            running = false;
        }
    }
}

// only called from the step tick ISR (single consumer)
bool StepTicker::start_next_block()
{
    bool ok = false;
    uint8_t direction_bits_value;
    uint8_t mask;
    uint32_t bits_to_update_bsrr;
    
    if (current_block == NULL) 
        return false;    
    
    direction_bits_value = 0;
    
    // need to prepare each active motor
    for (uint8_t motor_idx = 0; motor_idx < TOTAL_AXES_COUNT; motor_idx++) 
    {
        if (current_block->tick_info[motor_idx].steps_to_move == 0)
            continue;

        ok = true; // mark at least one motor is moving
        
        // Only set direction bits for backward movements 
        if ((current_block->direction_bits & (1 << motor_idx)) != 0)
            direction_bits_value |= (1 << (5 - (motor_idx * 2)));
        
        this->motor_enable_bits |= (1 << motor_idx);
    }
    
    // Generate mask
    mask = this->inversion_mask_bits_dirs ^ direction_bits_value;
    bits_to_update_bsrr = ((mask ^ SIGNAL_INVERT_DIR_PINS_MASK) << 16) | (mask);
    
    current_tick = 0;

    if (ok == true) 
    {   
        STEP_PINS_GPIO_PORT->BSRR = bits_to_update_bsrr;
        return true;
    }
    else
    {
        // this is an edge condition that should never happen, but we need to discard this block if it ever does
        // basically it is a block that has zero steps for all motors
        m_conveyor->block_finished();
    }

    return false;
}


extern "C" void TIM6_DAC_IRQHandler(void)
{
    __HAL_TIM_CLEAR_IT(&unstep_timer_handle, TIM_IT_UPDATE);
    StepTicker::getInstance()->unstep_tick();
}

// The actual interrupt handler where we do all the work
extern "C" void TIM2_IRQHandler(void)
{
    // Reset interrupt register
    __HAL_TIM_CLEAR_IT(&step_timer_handle, TIM_IT_UPDATE);
    StepTicker::getInstance()->step_tick();
}
