#include "StepTicker.h"
#include <string.h>
#include <stm32f4xx_hal.h>
#include <math.h>

#include "settings_manager.h"
#include "hw_timers.h"
#include "pins.h"


StepTicker *StepTicker::instance;

StepTicker::StepTicker()
{
    instance = this; // setup the Singleton instance of the stepticker

    // Default start values
    this->set_frequency(100000);
    this->set_unstep_time(10);

    this->unstep_bits = 0x00;
    this->running = false;
    this->current_block = NULL;    
    this->current_tick = 0;
    
    this->motor_enable_bits = 0;
}

StepTicker::~StepTicker()
{
}

//called when everything is setup and interrupts can start
void StepTicker::start()
{
    current_tick = 0;
    
    __HAL_TIM_ENABLE_IT(&step_timer_handle, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE_IT(&unstep_timer_handle, TIM_IT_UPDATE);
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
void StepTicker::set_unstep_time( float microseconds )
{
    __HAL_TIM_SET_AUTORELOAD(&unstep_timer_handle, (uint32_t)(microseconds - 1.0f));
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
    uint32_t bit_clear_mask = 0;
    
    // 31 30 29 28  27 26 25 24  23 22 21 20  19 18 17 16  bit clear in BSRR
    // 15 14 13 12  11 10 09 08  07 06 05 04  03 02 01 00  bit set   in BSRR
    // 
    //                                 DX SX  DY SY DZ SZ 
        
    // i = 0 -> X axis -> (1 << 4) << 16 -> (1 << 20)   to clear X step pin [0x0010 0000]
    // i = 1 -> Y axis -> (1 << 2) << 16 -> (1 << 18)   to clear Y step pin [0x0004 0000]
    // i = 2 -> Z axis -> (1 << 0) << 16 -> (1 << 16)   to clear Z step pin [0x0001 0000]
    
    for (int i = 0; i < 3; i++) 
    {
        if (((1 << i) & unstep_bits) != 0)
            bit_clear_mask |= (1 << (20 - (i * 2)));
    }
    
    STEP_PINS_GPIO_PORT->BSRR = bit_clear_mask;
    this->unstep_bits = 0;
}

// step clock
void StepTicker::step_tick (void)
{
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
                return;
            }
        }
        else
        {
            __HAL_TIM_DISABLE(&step_timer_handle);
            return;
        }
    }

//    if (THEKERNEL->is_halted()) 
//    {
//        running = false;
//        current_tick = 0;
//        current_block = NULL;
//        return;
//    }

    bool still_moving = false;
    
    // foreach motor, if it is active see if time to issue a step to that motor
    for (uint8_t m = 0; m < TOTAL_AXES_COUNT; m++) 
    {
        if (current_block->tick_info[m].steps_to_move == 0) 
            continue; // not active

        current_block->tick_info[m].steps_per_tick += current_block->tick_info[m].acceleration_change;

        if (current_tick == current_block->tick_info[m].next_accel_event) 
        {
            if (current_tick == current_block->accelerate_until) 
            { 
                // We are done accelerating, deceleration becomes 0 : plateau
                current_block->tick_info[m].acceleration_change = 0;
                
                if (current_block->decelerate_after < current_block->total_move_ticks) 
                {
                    current_block->tick_info[m].next_accel_event = current_block->decelerate_after;
                    
                    if (current_tick != current_block->decelerate_after) 
                    { 
                        // We are plateauing
                        // steps/sec / tick frequency to get steps per tick
                        current_block->tick_info[m].steps_per_tick = current_block->tick_info[m].plateau_rate;
                    }
                }
            }

            if (current_tick == current_block->decelerate_after) 
            { 
                // We start decelerating
                current_block->tick_info[m].acceleration_change = current_block->tick_info[m].deceleration_change;
            }
        }

        // protect against rounding errors and such
        if (current_block->tick_info[m].steps_per_tick <= 0) 
        {
            current_block->tick_info[m].counter = STEPTICKER_FPSCALE; // we force completion this step by setting to 1.0
            current_block->tick_info[m].steps_per_tick = 0;
        }

        current_block->tick_info[m].counter += current_block->tick_info[m].steps_per_tick;

        if (current_block->tick_info[m].counter >= STEPTICKER_FPSCALE) 
        {
            // >= 1.0 step time
            current_block->tick_info[m].counter -= STEPTICKER_FPSCALE; // -= 1.0F;
            ++current_block->tick_info[m].step_count;

            
            bool ismoving = false; // motor[m]->step(); // returns false if the moving flag was set to false externally (probes, endstops etc)
            
            if (((1 << m) & this->motor_enable_bits) != 0)
            {
                uint32_t bit_set_mask = 0;
                
                // step the motor
                // 31 30 29 28  27 26 25 24  23 22 21 20  19 18 17 16  bit clear in BSRR
                // 15 14 13 12  11 10 09 08  07 06 05 04  03 02 01 00  bit set   in BSRR
                // 
                //                                 DX SX  DY SY DZ SZ 
                    
                // m = 0 -> X axis -> (1 << 4) -> to clear X step pin [0x0000 0010]
                // m = 1 -> Y axis -> (1 << 2) -> to clear Y step pin [0x0000 0004]
                // m = 2 -> Z axis -> (1 << 0) -> to clear Z step pin [0x0000 0001]
    
                bit_set_mask |= (1 << (4 - (m * 2)));
                STEP_PINS_GPIO_PORT->BSRR = bit_set_mask;
                ismoving = true;
            }
            
            // we stepped so schedule an unstep
            unstep_bits |= (1 << m);

            if (!ismoving || current_block->tick_info[m].step_count == current_block->tick_info[m].steps_to_move) 
            {
                // done
                current_block->tick_info[m].steps_to_move = 0;
                this->motor_enable_bits &= ~(1 << m); // let motor know it is no longer moving
            }
        }

        // see if any motors are still moving after this tick
        if (((1 << m) & this->motor_enable_bits) != 0)
            still_moving = true;
    }

    // do this after so we start at tick 0
    current_tick++; // count number of ticks

    // We may have set a pin on in this tick, now we reset the timer to set it off
    // Note there could be a race here if we run another tick before the unsteps have happened,
    // right now it takes about 3-4us but if the unstep were near 10uS or greater it would be an issue
    // also it takes at least 2us to get here so even when set to 1us pulse width it will still be about 3us
    
    if ( unstep_bits != 0) 
    {
        __HAL_TIM_ENABLE(&unstep_timer_handle);
    }


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

        // all moves finished
        // we delegate the slow stuff to the pendsv handler which will run as soon as this interrupt exits
        //NVIC_SetPendingIRQ(PendSV_IRQn); this doesn't work
        //SCB->ICSR = 0x10000000; // SCB_ICSR_PENDSVSET_Msk;
    }
}

// only called from the step tick ISR (single consumer)
bool StepTicker::start_next_block()
{
    uint32_t bit_mask = 0;
    
    if (current_block == NULL) 
        return false;

    bool ok = false;
    
    // need to prepare each active motor
    for (uint8_t m = 0; m < 3; m++) 
    {
        if (current_block->tick_info[m].steps_to_move == 0) 
            continue;

        ok = true; // mark at least one motor is moving
        
        // set direction bit here
        // NOTE this would be at least 10us before first step pulse.
        // TODO does this need to be done sooner, if so how without delaying next tick
        //motor[m]->start_moving(); // also let motor know it is moving now
        
        // 31 30 29 28  27 26 25 24  23 22 21 20  19 18 17 16  bit clear in BSRR
        // 15 14 13 12  11 10 09 08  07 06 05 04  03 02 01 00  bit set   in BSRR
        // 
        //                                 DX SX  DY SY DZ SZ      
        
        if (((1 << m) & current_block->direction_bits) != 0)
        {
            // m = 0 -> X axis -> (1 << 5) to clear X dir pin [0x0000 0020]
            // m = 1 -> Y axis -> (1 << 3) to clear Y dir pin [0x0000 0008]
            // m = 2 -> Z axis -> (1 << 1) to clear Z dir pin [0x0000 0002]
            
            // Backward. Set direction bit [15 .. 00]
            bit_mask |= (1 << (5 - (m * 2)));
        }
        else
        {
            // m = 0 -> X axis -> (1 << 5) << 16 -> (1 << 21)   to clear X dir pin [0x0020 0000]
            // m = 1 -> Y axis -> (1 << 3) << 16 -> (1 << 19)   to clear Y dir pin [0x0008 0000]
            // m = 2 -> Z axis -> (1 << 1) << 16 -> (1 << 17)   to clear Z dir pin [0x0002 0000]
            
            // Forward. Clear direction bit [31 .. 16]
            bit_mask |= (1 << (21 - (m * 2)));
        }
        
        STEP_PINS_GPIO_PORT->BSRR = bit_mask;
        motor_enable_bits |= (1 << m);
    }

    current_tick = 0;

    if (ok == true) 
    {   
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
