#ifndef SPINDLE_CONTROLLER_H
#define SPINDLE_CONTROLLER_H

#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////

#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////
#include "GCodeParser.h"
#include "Planner.h"
#include "Conveyor.h"
#include "StepTicker.h"
///////////////////////////////////////////////////////////////////////////////

class SpindleController
{
public:
    SpindleController();
    ~SpindleController();

    bool InmediateStop();   // For halt cases
        
protected:
    ///////////////////////////////////////////////////////////////////////////////////////////

};

#endif
