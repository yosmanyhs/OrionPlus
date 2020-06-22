#ifndef PLANNER_H
#define PLANNER_H

#include <stdint.h>
#include "GCodeParser.h"
#include "BlockQueue.h"

#include "Block.h"
#include "Conveyor.h"

///////////////////////////////////////////////////////////////////////////////

enum PLANNER_STATUS_RESULTS 
{
	PLANNER_OK = 0,
	
};

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Function pointers for callback functions (must be set to proper values pointing to user functions)
// Included default dummy pointers

///////////////////////////////////////////////////////////////////////////////

class Block;


class Planner
{
    friend class Conveyor;
    friend class BlockQueue;
    
public:
        Planner();
        ~Planner();

        void AssociateConveyor(Conveyor * conv) { m_conveyor = conv; }

        int AppendLine(const float* target_mm, float spindle_speed, float rate_mm_s, bool inverseTimeRate = false);
        
        float max_allowable_speed( float acceleration, float target_velocity, float distance);
    
        static const char*  GetErrorText(uint32_t error_code);

    protected:
    
        float m_previous_unit_vector[TOTAL_AXES_COUNT];
        float m_junction_deviation;
    
        float m_minimum_planner_speed; // Setting
    
        int32_t m_position_steps[TOTAL_AXES_COUNT];
    
        Conveyor * m_conveyor;

        ///////////////////////////////////////////////////////////////////////////////////////////
        
        float limit_value_by_axis_maximum(float limit_value, const float * max_values, const float * unit_vector);
    
        void recalculate();
};

#endif
