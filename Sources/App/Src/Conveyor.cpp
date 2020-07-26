/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl) with additions from Sungeun K. Jeon (https://github.com/chamnit/grbl)
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Block.h"
#include "Conveyor.h"
#include "Planner.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stm32f4xx_hal.h>

#include "hw_timers.h"
#include "gpio.h"
#include "pins.h"

#include "user_tasks.h"
#include "MachineCore.h"

/*
 * The conveyor holds the queue of blocks, takes care of creating them, and starting the executing chain of blocks
 *
 * The Queue is implemented as a ringbuffer- with a twist
 *
 * Since delete() is not thread-safe, we must marshall deletable items out of ISR context
 *
 * To do this, we have implmented a *double* ringbuffer- two ringbuffers sharing the same ring, and one index pointer
 *
 * as in regular ringbuffers, HEAD always points to a clean, free block. We are free to prepare it as we see fit, at our leisure.
 * When the block is fully prepared, we increment the head pointer, and from that point we must not touch it anymore.
 *
 * also, as in regular ringbuffers, we can 'use' the TAIL block, and increment tail pointer when we're finished with it
 *
 * Both of these are implemented here- see queue_head_block() (where head is pushed) and on_idle() (where tail is consumed)
 *
 * The double ring is implemented by adding a third index pointer that lives in between head and tail. We call it isr_tail_i.
 *
 * in ISR context, we use HEAD as the head pointer, and isr_tail_i as the tail pointer.
 * As HEAD increments, ISR context can consume the new blocks which appear, and when we're finished with a block, we increment isr_tail_i to signal that they're finished, and ready to be cleaned
 *
 * in IDLE context, we use isr_tail_i as the head pointer, and TAIL as the tail pointer.
 * When isr_tail_i != tail, we clean up the tail block (performing ISR-unsafe delete operations) and consume it (increment tail pointer), returning it to the pool of clean, unused blocks which HEAD is allowed to prepare for queueing
 *
 * Thus, our two ringbuffers exist sharing the one ring of blocks, and we safely marshall used blocks from ISR context to IDLE context for safe cleanup.
 */
 

Conveyor::Conveyor()
{
    running = false;
    allow_fetch = false;
    flush= false;
    current_feedrate = 0;
}

// we allocate the queue here after config is completed so we do not run out of memory during config
void Conveyor::start()
{
    queue.resize(32);
    queue_delay_time_ms = (100);
    running = true;
}

void Conveyor::on_idle()
{
    if (running)
        check_queue();

    // we can garbage collect the block queue here
    if (queue.tail_i != queue.isr_tail_i) 
    {
        if (queue.is_empty()) 
        {
            // This should not happen
            configASSERT(0);
        } 
        else 
        {
            // Cleanly delete block
            Block* block = queue.tail_ref();
        
            block->clear();
            queue.consume_tail();
        }
    }
}

// see if we are idle
// this checks the block queue is empty, and that the step queue is empty and
// checks that all motors are no longer moving
bool Conveyor::is_idle() const
{
    if (queue.is_empty() == false || machine->AreMotorsStillMoving() == true)
        return false;
    
    return true;
}

// Wait for the queue to be empty and for all the jobs to finish in step ticker
void Conveyor::wait_for_idle(bool wait_for_motors)
{
    // wait for the job queue to empty, this means cycling everything on the block queue into the job queue
    // forcing them to be jobs
    running = false; // stops on_idle calling check_queue
    
    while (!queue.is_empty()) 
    {
        check_queue(true); // forces queue to be made available to stepticker
    }

    if (wait_for_motors) 
    {
        // now we wait for all motors to stop moving
        while(!is_idle()) 
        {
            //THEKERNEL->call_event(ON_IDLE, this);
        }
    }

    running = true;
    // returning now means that everything has totally finished
}

/*
 * push the pre-prepared head block onto the queue
 */
void Conveyor::queue_head_block()
{
    // upstream caller will block on this until there is room in the queue
    while (queue.is_full() && machine->IsHalted() == false) 
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (machine->IsHalted())
    {
        // we do not want to stick more stuff on the queue if we are in halt state
        // clear and release the block on the head
        queue.head_ref()->clear();
        return; // if we got a halt then we are done here
    }

    queue.produce_head();

    // not sure if this is the correct place but we need to turn on the motors if they were not already on    
    // turn all enable pins on
    machine->EnableSteppers();
}

void Conveyor::check_queue(bool force)
{
    static uint32_t last_time_check = xTaskGetTickCount();
    static bool idle_timer_running = false;

    if (queue.is_empty()) 
    {
        allow_fetch = false;
        last_time_check = xTaskGetTickCount(); // reset timeout
        
        if (idle_timer_running == false)
        {
            machine->StartStepperIdleTimer();
            idle_timer_running = true;
        }
        
        return;
    }

    // if we have been waiting for more than the required waiting time and the queue is not empty, or the queue is full, then allow stepticker to get the tail
    // we do this to allow an idle system to pre load the queue a bit so the first few blocks run smoothly.
    if (force || queue.is_full() || (xTaskGetTickCount() - last_time_check) >= (queue_delay_time_ms)) 
    {
        last_time_check = xTaskGetTickCount(); // reset timeout
        
        if (!flush) 
        {
            allow_fetch = true;
            __HAL_TIM_ENABLE(&step_timer_handle);
            
            // Turn On Activity LED [Write 0]
            HAL_GPIO_WritePin(LED_0_GPIO_Port, LED_0_Pin, GPIO_PIN_RESET);
            
            if (idle_timer_running == true)
            {
                // Stop and reset idling timer
                machine->StopStepperIdleTimer();
                idle_timer_running = false;
            }
        }
        
        return;
    }
}

// called from step ticker ISR
bool Conveyor::get_next_block(Block **block)
{
    // mark entire queue for GC if flush flag is asserted
    if (flush)
    {
        while (queue.isr_tail_i != queue.head_i) 
        {
            queue.isr_tail_i = queue.next(queue.isr_tail_i);
        }
    }

    // default the feerate to zero if there is no block available
    this->current_feedrate = 0;

    if (machine->IsHalted() == true || queue.isr_tail_i == queue.head_i) 
        return false; // we do not have anything to give

    // wait for queue to fill up, optimizes planning
    if (!allow_fetch) 
        return false;

    Block *b = queue.item_ref(queue.isr_tail_i);
    
    // we cannot use this now if it is being updated
    if (!b->locked) 
    {
        if (!b->is_ready) 
        { 
            while (1); // __debugbreak(); // should never happen
        }
        
        b->is_ticking = true;
        b->recalculate_flag = false;
        this->current_feedrate = b->nominal_speed;
        *block = b;
        return true;
    }

    return false;
}

// called from step ticker ISR when block is finished, do not do anything slow here
void Conveyor::block_finished()
{
    // we increment the isr_tail_i so we can get the next block
    queue.isr_tail_i = queue.next(queue.isr_tail_i);
}

/*
    In most cases this will not totally flush the queue, as when streaming
    gcode there is one stalled waiting for space in the queue, in
    queue_head_block() so after this flush, once main_loop runs again one more
    gcode gets stuck in the queue, this is bad. Current work around is to call
    this when the queue in not full and streaming has stopped
*/
void Conveyor::flush_queue()
{
    allow_fetch = false;
    flush = true;

    // TODO force deceleration of last block

    // now wait until the block queue has been flushed
    wait_for_idle(false);
    flush = false;
}
