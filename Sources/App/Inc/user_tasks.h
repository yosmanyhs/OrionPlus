#ifndef USER_TASKS_H
#define USER_TASKS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "timers.h"

#include "MachineCore.h"

#include "GCodeParser.h"
#include "Planner.h"
#include "Conveyor.h"
#include "StepTicker.h"

#define GCODE_STRING_QUEUE_ITEM_COUNT   1

extern MachineCore * machine;   // System Global Controller class

void Init_UserTasks_and_Objects(void);

#endif
