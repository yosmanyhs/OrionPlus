#ifndef STEP_TICKER_H
#define STEP_TICKER_H

#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

#include "Block.h"
#include "Conveyor.h"
#include "GCodeParser.h"

///////////////////////////////////////////////////////////////////////////////

// handle 2.62 Fixed point
#define STEPTICKER_FPSCALE (1LL<<62)
#define STEPTICKER_FROMFP(x) ((float)(x)/STEPTICKER_FPSCALE)

class StepTicker
{
public:
    StepTicker();
    ~StepTicker();
    
    void set_frequency( float frequency );
    void set_unstep_time( uint8_t microseconds );
    
    float get_frequency() const { return frequency; }
    void unstep_tick();
    
    inline const int32_t* GetSteppersPosition() { return this->m_stepper_positions; };
    void ResetSteppersPosition() { memset((void*)&m_stepper_positions[0], 0, sizeof(m_stepper_positions)); } 

    void step_tick (void);
    void start();
    
    void Associate_Conveyor(Conveyor* conv) { m_conveyor = conv; }
    inline void EnableMotor(uint8_t axis) { this->motor_enable_bits |= (1 << axis); }
    inline void DisableMotor(uint8_t axis) { this->motor_enable_bits &= (~(1 << axis)); }
    inline void DisableAllMotors() { this->motor_enable_bits = 0; }
    
    inline bool AreMotorsStillMoving() { return (this->motor_enable_bits != 0) ? true : false; }
    inline bool AreMotorsRunning() { return this->running; }
    
    void ApplyUpdatedInversionMasks();
    void ResetStepperDrivers(bool reset);
    void EnableStepperDrivers(bool enable);

    static StepTicker *getInstance() { return instance; }

private:
    static StepTicker *instance;

    bool start_next_block();

    float frequency;
    uint32_t period;

    uint8_t inversion_mask_bits_steps;
    uint8_t inversion_mask_bits_dirs;

    uint8_t unstep_bits;

    uint8_t motor_enable_bits;

    Block *current_block;
    uint32_t current_tick;

    Conveyor* m_conveyor;

    volatile bool running;

    int32_t m_stepper_positions[TOTAL_AXES_COUNT];
};



#endif
