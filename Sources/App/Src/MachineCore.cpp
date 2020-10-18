#include "MachineCore.h"

#include <stm32f4xx_hal.h>

#include <ctype.h>
#include <string.h>

#include <algorithm>

#include "settings_manager.h"

#include "FreeRTOS.h"
#include "timers.h"

#include "pins.h"
#include "gpio.h"

#include "task_settings.h"

MachineCore::MachineCore(void)
{
    TimerHandle_t delayed_startup_timer;
    
    m_startup_finished = false;
    m_system_halted = false;
    m_feed_hold = false;
    m_dwell_active = false;
    
    m_gcode_source = GCODE_SOURCE_SERIAL_CONSOLE;
    
    m_homing_state = HOMING_IDLE;
    m_axes_homing_now = 0;
    m_axes_already_homed = 0;
    
    m_probe_state = PROBING_IDLE;
    memset((void*)this->m_probe_position, 0, sizeof(this->m_probe_position));
    
    // Create objects
    m_gcode_parser = new GCodeParser();
    m_planner = new Planner();
    m_conveyor = new Conveyor();
    m_step_ticker = new StepTicker();
    m_coolant = new CoolantController();
    m_spindle = new SpindleController();
    
    // Associate them
    m_step_ticker->Associate_Conveyor(m_conveyor);
    m_planner->AssociateConveyor(m_conveyor);
    m_gcode_parser->AssociatePlanner(m_planner);
    
    // Create FreeRTOS objects
    m_stepper_idle_timer = xTimerCreate(NULL, 
                                        pdMS_TO_TICKS(Settings_Manager::GetIdleLockTime_secs() * 1000), 
                                        pdFALSE,  
                                        (void*)this, 
                                        (TimerCallbackFunction_t)MachineCore::steppers_idle_timeout_callback);

    // Create delayed startup timer
    delayed_startup_timer = xTimerCreate(NULL, 
                                         1, 
                                         pdFALSE, 
                                         (void*)this, 
                                         (TimerCallbackFunction_t)MachineCore::delayed_startup_callback);

    xTimerStart(delayed_startup_timer, 0);
                            
    // Create timer to read user buttons (and debounce too)
    m_user_btn_read_timer = xTimerCreate(NULL, 
                                         pdMS_TO_TICKS(20), 
                                         pdTRUE, 
                                         (void*)this, 
                                         (TimerCallbackFunction_t)MachineCore::read_user_buttons_callback);
                                         
    xTimerStart(m_user_btn_read_timer, 0);
                                         
    // Create event group to register/detect input events to the machine
    m_input_events_group = xEventGroupCreate();
                                         
    m_fault_event_conditions = 0;
                                         
    xTaskCreate(safety_task_entry, NULL, SAFETY_TASK_STACK_SIZE, (void*)this, SAFETY_TASK_PRIORITY, &m_safety_task_handle);
}


MachineCore::~MachineCore(void)
{
    delete m_step_ticker;
    delete m_conveyor;
    delete m_planner;
    delete m_gcode_parser;
}

bool MachineCore::Initialize()
{
    m_conveyor->start();
    m_step_ticker->start();
    
    // Reset stepper drivers [reset removed after expiration of startup timer]
    m_step_ticker->ResetStepperDrivers(true);
    return true;
}

void MachineCore::OnIdle()  // [Called from the IDLE task]
{
    if (this->m_startup_finished == false)
        return;
    
    m_conveyor->on_idle();
    
    // Update current position
    const int32_t* stepper_pos = m_step_ticker->GetSteppersPosition();
    
    m_current_stepper_pos[COORD_X] = (float)(stepper_pos[COORD_X]) / Settings_Manager::GetStepsPer_mm_Axis(COORD_X);
    m_current_stepper_pos[COORD_Y] = (float)(stepper_pos[COORD_Y]) / Settings_Manager::GetStepsPer_mm_Axis(COORD_Y);
    m_current_stepper_pos[COORD_Z] = (float)(stepper_pos[COORD_Z]) / Settings_Manager::GetStepsPer_mm_Axis(COORD_Z);
}

bool MachineCore::StartStepperIdleTimer()   // [Called from GCode parsing task]
{
    // Only start idling timer if not dwelling
    if (m_dwell_active == false)
    {
        xTimerStart(m_stepper_idle_timer, 0);
        return true;
    }
    
    return false;
}

void MachineCore::StopStepperIdleTimer()    // [Called from GCode parsing task]
{
    xTimerReset(m_stepper_idle_timer, 0);
    xTimerStop(m_stepper_idle_timer, 0);
}

void MachineCore::Halt()                    // [Called from GCode parsing task]
{ 
    m_system_halted = true;
    
    m_spindle->InmediateStop();
    m_coolant->Stop();
    
    m_step_ticker->EnableStepperDrivers(false);
    m_step_ticker->DisableAllMotors();
    
    m_conveyor->flush_queue();
}

// Called from EXTI interrupt context
BaseType_t MachineCore::NotifyOfEvent(uint32_t it_evt_src)
{
    BaseType_t should_yield = pdFALSE;
    
    // Check each bit [EXTI6, EXTI7, EXTI8]
    // EXTI6
    if ((it_evt_src & (1 << 6)) != 0)
    {
        // EXTI6 can be from PC6 [Limit X] / PG6 [Global Stepper Fault]
        if (HAL_GPIO_ReadPin(GLOBAL_FAULT_GPIO_Port, GLOBAL_FAULT_Pin) != GPIO_PIN_SET)
        {
            // Global Fault = 0. Stepper motors fault condition
            m_fault_event_conditions |= STEPPER_FAULT_EVENT;
        }
        
        if (HAL_GPIO_ReadPin(LIM_X_GPIO_Port, LIM_X_Pin) != GPIO_PIN_RESET)
        {
            // Limit X = 1
            m_fault_event_conditions |= LIMIT_X_MIN_EVENT;
        }
    }
    
    // EXTI7
    if ((it_evt_src & (1 << 7)) != 0)
    {
        // Limit Y
        m_fault_event_conditions |= LIMIT_Y_MIN_EVENT;
    }
    
    // EXTI8
    if ((it_evt_src & (1 << 8)) != 0)
    {
        // Limit Z
        m_fault_event_conditions |= LIMIT_Z_MIN_EVENT;
    }
    
    if (m_fault_event_conditions != 0)
    {
        xTaskNotifyFromISR(this->m_safety_task_handle, m_fault_event_conditions, eSetBits, &should_yield);
    }
    
    return should_yield;
}

int MachineCore::GoHome(float* target, uint32_t spec_value_mask, bool isG28) 
{
    int status = GCODE_OK;
    float move_tgt[TOTAL_AXES_COUNT];
    uint16_t limit_read_value;
    bool idle_condition;
    
    // Notify we're homing now
    this->m_axes_homing_now = true;
        
    do
    {
        // Always read current status
        limit_read_value = (LIMITS_GPIO_Port->IDR & LIMITS_Mask);
        idle_condition = m_conveyor->is_idle();
            
        switch (m_homing_state)
        {
            case HOMING_IDLE:
            {
                // Disable limit interrupts
                disable_limit_interrupts();

                memset((void*)&move_tgt[0], 0, sizeof(move_tgt));

                // Check if we're already in the limits. In this case don't schedule any seeking move
                if (limit_read_value == LIMITS_Mask)
                {
                    // All the switches are engaged. Skip the first seeking move.
                    // Schedule the release move directly
                    move_tgt[0] = Settings_Manager::GetHomingData().home_pull_off_distance_mm;
                    move_tgt[1] = move_tgt[2] = move_tgt[0];
                        
                    m_planner->ResetPosition();
                    
                    // Schedule movement
                    m_planner->AppendLine(move_tgt, 0.0f, Settings_Manager::GetHomingData().home_seek_rate_mm_sec);
                    m_homing_state = HOMING_RELEASE_LIMITS;
                }
                else
                {
                    // Determine the required travel distance (max value * 1.5) [Only linear axes]
                    for (uint32_t idx = COORD_X; idx < COORDINATE_LINEAR_AXES_COUNT; idx++)
                        move_tgt[0] = std::max(move_tgt[0], Settings_Manager::GetMaxTravel_mm_Axis(idx));
                    
                    // Apply -1.5 factor and copy to remaining axes
                    move_tgt[0] = -1.5f * move_tgt[0];
                    move_tgt[1] = move_tgt[2] = move_tgt[0];
                    
                    // Schedule movement
                    m_planner->AppendLine(move_tgt, 0.0f, Settings_Manager::GetHomingData().home_seek_rate_mm_sec);
                    m_homing_state = HOMING_SEEK_LIMITS;
                }
            }
            break;
            
            case HOMING_SEEK_LIMITS:
            {
                // Check if any motor has reached the limit switch
                if (limit_read_value != 0)
                {
                    // Determine specific axis
                    if ((limit_read_value & LIM_X_Pin) != 0)
                        m_step_ticker->DisableMotor(COORD_X);
                    
                    if ((limit_read_value & LIM_Y_Pin) != 0)
                        m_step_ticker->DisableMotor(COORD_Y);
                    
                    if ((limit_read_value & LIM_Z_Pin) != 0)
                        m_step_ticker->DisableMotor(COORD_Z);
                }

                // Determine if all motors have reached the limit switches or the move has ended
                if ((limit_read_value & LIMITS_Mask) == LIMITS_Mask)
                {
                    // All switches are engaged before the completion of the move. 
                    // Prepare to release all of them
                    m_planner->ResetPosition();
                    
                    // Move a little to release all switches
                    move_tgt[0] = Settings_Manager::GetHomingData().home_pull_off_distance_mm;
                    move_tgt[1] = move_tgt[2] = move_tgt[0];
                    
                    // Schedule movement
                    m_planner->AppendLine(move_tgt, 0.0f, Settings_Manager::GetHomingData().home_seek_rate_mm_sec);
                    m_homing_state = HOMING_RELEASE_LIMITS;
                }
                else if (idle_condition == true)
                {
                    // In any case the move cannot end before reaching the next state.
                    // Report the error and exit
                    status = GCODE_ERROR_HOMING_LOCATE_LIMITS;
                    m_homing_state = HOMING_ERROR;
                }
                else 
                {
                    // Just wait for 1 tick (to allow other tasks to execute)
                    vTaskDelay(1);
                }
            }
            break;
            
            case HOMING_RELEASE_LIMITS:
            {
                // Wait for the move completion
                m_conveyor->wait_for_idle(true);
                
                limit_read_value = (LIMITS_GPIO_Port->IDR & LIMITS_Mask);
                
                // Determine if all motors have released the limit switches or the move has ended
                if (limit_read_value == 0)
                {
                    // All switches are engaged before the completion of the move. 
                    // Prepare to release all of them
                    m_planner->ResetPosition();
                    
                    // Move a little to release all switches
                    move_tgt[0] = -(Settings_Manager::GetHomingData().home_pull_off_distance_mm);
                    move_tgt[1] = move_tgt[2] = move_tgt[0];
                    
                    // Schedule movement
                    m_planner->AppendLine(move_tgt, 0.0f, Settings_Manager::GetHomingData().home_feed_rate_mm_sec);
                    m_homing_state = HOMING_CONFIRM_LIMITS;
                }
                else
                {
                    // In any case the move cannot end before reaching the next state.
                    // Report the error and exit
                    status = GCODE_ERROR_HOMING_RELEASE_LIMITS;
                    m_homing_state = HOMING_ERROR;
                }
            }
            break;
            
            case HOMING_CONFIRM_LIMITS:
            {
                // Check if any motor has reached the limit switch
                if (limit_read_value != 0)
                {
                    // Determine specific axis
                    if ((limit_read_value & LIM_X_Pin) != 0)
                        m_step_ticker->DisableMotor(COORD_X);
                    
                    if ((limit_read_value & LIM_Y_Pin) != 0)
                        m_step_ticker->DisableMotor(COORD_Y);
                    
                    if ((limit_read_value & LIM_Z_Pin) != 0)
                        m_step_ticker->DisableMotor(COORD_Z);
                }

                // Determine if all motors have reached the limit switches or the move has ended
                if ((limit_read_value & LIMITS_Mask) == LIMITS_Mask)
                {
                    // All switches are engaged before the completion of the move. 
                    // Prepare to release all of them
                    m_planner->ResetPosition();
                    
                    // Move a little to release all switches
                    move_tgt[0] = Settings_Manager::GetHomingData().home_pull_off_distance_mm;
                    move_tgt[1] = move_tgt[2] = move_tgt[0];
                    
                    // Schedule movement
                    m_planner->AppendLine(move_tgt, 0.0f, Settings_Manager::GetHomingData().home_feed_rate_mm_sec);
                    m_homing_state = HOMING_FINISH;
                }
                else if (idle_condition == true)
                {
                    // In any case the move cannot end before reaching the next state.
                    // Report the error and exit
                    status = GCODE_ERROR_HOMING_CONFIRM_LIMITS;
                    m_homing_state = HOMING_ERROR;
                }
                else 
                {
                    // Just wait for 1 tick (to allow other tasks to execute)
                    vTaskDelay(1);
                }
            }
            break;
            
            case HOMING_FINISH:
            {
                // Wait for move completion
                m_conveyor->wait_for_idle(true);
                    
                limit_read_value = (LIMITS_GPIO_Port->IDR & LIMITS_Mask);
                
                if (limit_read_value == 0)
                {
                    // All switches are released after the completion of the move. 
                                          
                    m_homing_state = HOMING_DONE;
                }
                else
                {
                    // In any case the move cannot end before reaching the next state.
                    // Report the error and exit
                    status = GCODE_ERROR_HOMING_FINISH_RELEASE;
                    m_homing_state = HOMING_ERROR;
                }
            }
            break;
                
            default:
            {
                m_homing_state = HOMING_ERROR;
            }
            break;
        }
    }
    while ((m_homing_state != HOMING_DONE) && (m_homing_state != HOMING_ERROR));
        
    if (HOMING_DONE == m_homing_state)
    {
        this->m_axes_already_homed = 0x07;
    }
    else
    {
        this->m_axes_already_homed = 0;
    }
        
    m_planner->ResetPosition();
    m_gcode_parser->ResetParser();
    m_step_ticker->ResetSteppersPosition();
    
    m_homing_state = HOMING_IDLE;
    this->m_axes_homing_now = false;
    enable_limit_interrupts();
    
    if (status == GCODE_OK && (spec_value_mask & VALUE_SET_ANY_AXES_BITS) != 0)
    {
        // If homing finished successfully then go to specified target position (if specified too)
        status = m_planner->AppendLine(target, 0.0f, SOME_LARGE_VALUE);
    }
    
    return status;
}

int MachineCore::DoProbe(float* target, uint32_t spec_value_mask)
{
    return 0;
}
    
int MachineCore::SendSpindleCommand(GCODE_MODAL_SPINDLE_MODES mode, float spindle_rpm) 
{
    return 0;
}

int MachineCore::SendCoolantCommand(GCODE_MODAL_COOLANT_MODES mode) 
{ 
    switch (mode)
    {
        case MODAL_COOLANT_FLOOD:
            m_coolant->EnableFlood();
            break;
        
        case MODAL_COOLANT_MIST:
            m_coolant->EnableMist();
            break;
        
        default:
            m_coolant->Stop();
            break;
    }

    return 0;
}
    
int MachineCore::Dwell(float p_time_secs) 
{ 
    int ms = (int)(p_time_secs * 1000);
    
    // During check mode this method does nothing
    if (m_gcode_parser->IsCheckModeActive())
        return 0;
    
    // Notify the system is dwelling
    m_dwell_active = true;
    
    while (ms >= 0)
    {
        if (this->m_system_halted != false)
        {
            m_dwell_active = false;
            return -1;  // TODO: Modify to proper value indicating Halt condition
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
        ms -= 500;
    }
    
    m_dwell_active = false;
    return 0; 
}
    
int MachineCore::WaitForIdleCondition() 
{ 
    m_conveyor->wait_for_idle(); 
    return 0; 
}

void MachineCore::GetGlobalStatusReport(GLOBAL_STATUS_REPORT_DATA & outData)
{
    outData.SystemHalted = this->m_system_halted;
    outData.FaultConditions = this->m_fault_event_conditions;
    outData.FeedHoldActive = this->m_feed_hold;
    outData.SteppersEnabled = this->m_step_ticker->AreMotorsRunning();
    outData.CheckModeEnabled = this->m_gcode_parser->IsCheckModeActive();
    
    outData.OriginCoordinates = this->m_gcode_parser->ReadOriginCoords();
    outData.OffsetCoordinates = this->m_gcode_parser->ReadOffsetCoords();
    
    outData.CurrentPositions = this->m_current_stepper_pos;
    
    // TODO: Fix this
    outData.TargetPositions = NULL;
    
    outData.ProbingPositions = this->m_probe_position;
    outData.HomingState = this->m_homing_state;
    outData.ProbingState = this->m_probe_state;
    
    this->m_gcode_parser->ReadParserModalState(&outData.ModalState);
    
    outData.CurrentFeedRate = this->m_conveyor->get_current_feedrate();
    outData.CurrentSpindleRPM = this->m_spindle->GetCurrentRPM();
    outData.CurrentSpindleTool = this->m_spindle->GetCurrentToolNumber();
    
    outData.FileParsingPercent = 0;
    outData.CurrentFileName = NULL;
    outData.GCodeSource = GCODE_SOURCE_SERIAL_CONSOLE;    
}

void MachineCore::disable_limit_interrupts()
{
    NVIC_DisableIRQ(EXTI9_5_IRQn);
}

void MachineCore::enable_limit_interrupts()
{
    // Clear EXTI pending register
    uint32_t pr_val = EXTI->PR;
    
    EXTI->PR = pr_val;
    NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
}

// Static timer callback functions
void MachineCore::steppers_idle_timeout_callback(TimerHandle_t xTimer)
{
    MachineCore* instance = (MachineCore*)pvTimerGetTimerID(xTimer);

    // Disable stepper motors after a while of idling
    instance->m_step_ticker->EnableStepperDrivers(false);
}

void MachineCore::delayed_startup_callback(TimerHandle_t xTimer)
{
    MachineCore* instance = (MachineCore*)pvTimerGetTimerID(xTimer);

    // Release stepper drivers reset
    instance->m_step_ticker->ResetStepperDrivers(false);
    
    // Signal the machine is ready to use
    instance->m_startup_finished = true;
    
    // Release calling timer
    xTimerDelete(xTimer, 0);
}

void MachineCore::read_user_buttons_callback(TimerHandle_t xTimer)
{
    MachineCore* instance = (MachineCore*)pvTimerGetTimerID(xTimer);
    
    static uint32_t debounce_cntrs[3];
    
    if (instance->m_startup_finished == false)
        return;
    
    // Read start button
    if ((BTN_START_GPIO_Port->IDR & BTN_START_Pin) == 0)
    {
        if (debounce_cntrs[0] < 2) // ~100 ms 
            debounce_cntrs[0]++;
        else
        {
            // Assume the button was pressed
            xEventGroupSetBits(instance->m_input_events_group, BTN_START_EVENT);
            debounce_cntrs[0] = 0;
        }
    }
    
    if ((BTN_HOLD_GPIO_Port->IDR & BTN_HOLD_Pin) == 0)
    {
        if (debounce_cntrs[1] < 2) // ~100 ms 
            debounce_cntrs[1]++;
        else
        {
            // Assume the button was pressed
            xEventGroupSetBits(instance->m_input_events_group, BTN_HOLD_EVENT);
            debounce_cntrs[1] = 0;
        }
    }
    
    if ((BTN_ABORT_GPIO_Port->IDR & BTN_ABORT_Pin) == 0)
    {
        if (debounce_cntrs[2] < 2) // ~100 ms 
            debounce_cntrs[2]++;
        else
        {
            // Assume the button was pressed
            xEventGroupSetBits(instance->m_input_events_group, BTN_ABORT_EVENT);
            debounce_cntrs[2] = 0;
        }
    }
}

void MachineCore::safety_task_entry(void* param)
{
    MachineCore* instance = (MachineCore*)param;
    uint32_t notify_value;
    
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn); 
    
    for ( ; ; )
    {
        if (pdTRUE == xTaskNotifyWait((uint32_t)-1, (uint32_t)-1, &notify_value, portMAX_DELAY))
        {
            if (notify_value != 0)
                instance->Halt();
        }
    }
}
