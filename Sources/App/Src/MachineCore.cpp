#include "MachineCore.h"

#include <stm32f4xx_hal.h>

#include <ctype.h>
#include <string.h>

#include "settings_manager.h"

#include "FreeRTOS.h"
#include "timers.h"

#include "pins.h"
#include "gpio.h"

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

void MachineCore::OnIdle()
{
    if (this->m_startup_finished == false)
        return;
    
    m_conveyor->on_idle();
}

bool MachineCore::StartStepperIdleTimer()
{
    // Only start idling timer if not dwelling
    if (m_dwell_active == false)
    {
        xTimerStart(m_stepper_idle_timer, 0);
        return true;
    }
    
    return false;
}

void MachineCore::StopStepperIdleTimer()
{
    xTimerReset(m_stepper_idle_timer, 0);
    xTimerStop(m_stepper_idle_timer, 0);
}

void MachineCore::Halt() 
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
        else if (HAL_GPIO_ReadPin(LIM_X_GPIO_Port, LIM_X_Pin) != GPIO_PIN_RESET)
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
    
    return pdFALSE;
}

int MachineCore::GoHome(float* target, uint32_t spec_value_mask, bool isG28) 
{ 
    return 0; 
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
