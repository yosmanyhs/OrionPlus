#ifndef MACHINE_CORE_H
#define MACHINE_CORE_H

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////

typedef enum GCODE_SOURCE_OPTIONS
{
    GCODE_SOURCE_SERIAL_CONSOLE,
    GCODE_SOURCE_SD_STORAGE,
    GCODE_SOURCE_JOG,
    
    GCODE_SOURCE_MAX_VALUE    
}GCODE_SOURCE_OPTIONS;

///////////////////////////////////////////////////////////////////////////////

typedef enum HOMING_STATE_VALUES
{
    HOMING_IDLE,
    
}HOMING_STATE_VALUES;

///////////////////////////////////////////////////////////////////////////////

typedef enum PROBING_STATE_VALUES
{
    PROBING_IDLE,
    
}PROBING_STATE_VALUES;

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////
#include "GCodeParser.h"
#include "Planner.h"
#include "Conveyor.h"
#include "StepTicker.h"
#include "CoolantController.h"
#include "SpindleController.h"
///////////////////////////////////////////////////////////////////////////////

class GCodeParser;
class Planner;
class Conveyor;
class StepTicker;
class CoolantController;
class SpindleController;

class MachineCore
{
public:
    MachineCore();
    ~MachineCore();

    bool Initialize();    
    void OnIdle();
    
    void StartStepperIdleTimer();
    void StopStepperIdleTimer();
    void Halt();
    
    // Called from EXTI interrupt context
    BaseType_t NotifyOfEvent(uint32_t it_evt_src);
    
    inline void EnterFeedHold() { m_feed_hold = true; }
    inline void ExitFeedHold() { m_feed_hold = false; }
    
    inline bool IsFeedHoldActive() { return m_feed_hold; }
    
    
    inline bool IsHalted() { return m_system_halted; }
    
    inline void EnableSteppers() { m_step_ticker->EnableStepperDrivers(true); }
    
    inline bool IsHomingNow() { return (m_axes_homing_now != 0) ? true : false; }
    inline bool IsAxisHomingNow(uint8_t axis) { return (((1 << axis) & m_axes_homing_now) != 0) ? true : false; }
    inline bool IsAxisHomed(uint8_t axis) { return (((1 << axis) & m_axes_already_homed) != 0) ? true : false; }
    
    inline bool AreMotorsStillMoving() { return m_step_ticker->AreMotorsStillMoving(); }
    
    int ParseGCodeLine(char* line) { return m_gcode_parser->ParseLine(line); }
    const char* GetGCodeErrorText(uint32_t code) { return GCodeParser::GetErrorText(code); } 
        
protected:
    
    bool                        m_startup_finished;    
    bool                        m_system_halted;
    bool                        m_feed_hold;

    class GCodeParser*          m_gcode_parser;
    class Planner*              m_planner;
    class Conveyor*             m_conveyor;
    class StepTicker*           m_step_ticker;
    class CoolantController*    m_coolant;
    class SpindleController*    m_spindle;

    GCODE_SOURCE_OPTIONS        m_gcode_source;

    // Homing Control
    HOMING_STATE_VALUES         m_homing_state;
    uint32_t                    m_axes_homing_now;
    uint32_t                    m_axes_already_homed;

    // Probing Control
    PROBING_STATE_VALUES        m_probe_state;
    float                       m_probe_position[COORDINATE_LINEAR_AXES_COUNT];

    // FreeRTOS objects
    TimerHandle_t               m_stepper_idle_timer;
    TimerHandle_t               m_user_btn_read_timer;

    EventGroupHandle_t          m_input_events_group;
    
    ///////////////////////////////////////////////////////////////////////////////////////////

    // Static callbacks to associate to software timers. The pvTimerId parameter contains the 
    // 'this' pointer referring to the current instance (only one allowed)
    static void steppers_idle_timeout_callback(TimerHandle_t xTimer);
    static void delayed_startup_callback(TimerHandle_t xTimer);
    static void read_user_buttons_callback(TimerHandle_t xTimer);
};

#endif
