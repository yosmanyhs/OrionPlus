#ifndef MACHINE_CORE_H
#define MACHINE_CORE_H

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"

///////////////////////////////////////////////////////////////////////////////

#include "GCodeParser.h"
#include "Planner.h"
#include "Conveyor.h"
#include "StepTicker.h"
#include "CoolantController.h"
#include "SpindleController.h"

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
    HOMING_SEEK_LIMITS,
    HOMING_RELEASE_LIMITS,
    HOMING_CONFIRM_LIMITS,
    HOMING_FINISH,
    HOMING_DONE,
    HOMING_ERROR
    
}HOMING_STATE_VALUES;

///////////////////////////////////////////////////////////////////////////////

typedef enum PROBING_STATE_VALUES
{
    PROBING_IDLE,
    
}PROBING_STATE_VALUES;

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////

typedef struct GLOBAL_STATUS_REPORT_DATA
{
    bool SystemHalted;
    uint32_t FaultConditions;
    
    bool FeedHoldActive;
    bool SteppersEnabled;
    bool CheckModeEnabled;
    
    const float* OriginCoordinates;
    const float* OffsetCoordinates;
    
    const float* CurrentPositions;
    const float* TargetPositions;
    const float* ProbingPositions;
    
    HOMING_STATE_VALUES HomingState;
    PROBING_STATE_VALUES ProbingState;
    GCodeModalData ModalState;
    
    float CurrentFeedRate;
    uint32_t CurrentSpindleRPM;
    uint8_t CurrentSpindleTool;
    
    uint8_t FileParsingPercent;  
    const char* CurrentFileName;
    
    GCODE_SOURCE_OPTIONS    GCodeSource;

}GLOBAL_STATUS_REPORT_DATA;

///////////////////////////////////////////////////////////////////////////////






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
    
    bool StartStepperIdleTimer();
    void StopStepperIdleTimer();
    void Halt();

    inline void ClearHalt() { m_system_halted = false; } 
    
    // Called from EXTI interrupt context
    BaseType_t NotifyOfEvent(uint32_t it_evt_src);
    
    inline void EnterFeedHold() { m_feed_hold = true; }
    inline void ExitFeedHold() { m_feed_hold = false; }
    
    inline bool IsFeedHoldActive() { return m_feed_hold; }
    
    
    inline bool IsHalted() { return m_system_halted; }
    inline bool IsDwelling() { return m_dwell_active; } 
    
    inline void EnableSteppers() { m_step_ticker->EnableStepperDrivers(true); }
    
    inline bool IsHomingNow() { return (m_axes_homing_now != 0) ? true : false; }
    inline bool IsAxisHomingNow(uint8_t axis) { return (((1 << axis) & m_axes_homing_now) != 0) ? true : false; }
    inline bool IsAxisHomed(uint8_t axis) { return (((1 << axis) & m_axes_already_homed) != 0) ? true : false; }
    
    inline bool AreMotorsStillMoving() { return m_step_ticker->AreMotorsStillMoving(); }
    
    inline float GetCurrentFeedrate() { return m_conveyor->get_current_feedrate(); }
    inline const float* GetCurrentPosition() { return m_current_stepper_pos; }
    
    int ParseGCodeLine(char* line) { return m_gcode_parser->ParseLine(line); }
    const char* GetGCodeErrorText(uint32_t code) { return GCodeParser::GetErrorText(code); } 
    
    int GoHome(float* target, uint32_t spec_value_mask, bool isG28);
    int DoProbe(float* target, uint32_t spec_value_mask);
    int SendSpindleCommand(GCODE_MODAL_SPINDLE_MODES mode, float spindle_rpm);
    int SendCoolantCommand(GCODE_MODAL_COOLANT_MODES mode);
    int Dwell(float p_time_secs);
    int WaitForIdleCondition();
    
    void GetGlobalStatusReport(GLOBAL_STATUS_REPORT_DATA & outData);
        
protected:
    
    bool                        m_startup_finished;    
    bool                        m_system_halted;
    bool                        m_feed_hold;

    bool                        m_dwell_active;

    class GCodeParser*          m_gcode_parser;
    class Planner*              m_planner;
    class Conveyor*             m_conveyor;
    class StepTicker*           m_step_ticker;
    class CoolantController*    m_coolant;
    class SpindleController*    m_spindle;

    GCODE_SOURCE_OPTIONS        m_gcode_source;

    float                       m_current_stepper_pos[TOTAL_AXES_COUNT];

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
    uint32_t                    m_fault_event_conditions;
    TaskHandle_t                m_safety_task_handle;
    
    ///////////////////////////////////////////////////////////////////////////////////////////
    
    void disable_limit_interrupts();
    void enable_limit_interrupts();
    
    ///////////////////////////////////////////////////////////////////////////////////////////

    // Static callbacks to associate to software timers. The pvTimerId parameter contains the 
    // 'this' pointer referring to the current instance (only one allowed)
    static void steppers_idle_timeout_callback(TimerHandle_t xTimer);
    static void delayed_startup_callback(TimerHandle_t xTimer);
    static void read_user_buttons_callback(TimerHandle_t xTimer);
    
    static void safety_task_entry(void* param);
};

#endif
