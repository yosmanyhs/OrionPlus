#ifndef USER_TASKS_H
#define USER_TASKS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"

#include "GCodeParser.h"
#include "Planner.h"
#include "Conveyor.h"
#include "StepTicker.h"

#define GCODE_STRING_QUEUE_ITEM_COUNT   1

typedef struct GLOBAL_TASK_INFO_STRUCT_TYPE
{
    bool                ready_to_use;

    class GCodeParser*  gcode_parser_ptr;
    class Planner*      planner_ptr;
    class Conveyor*     conveyor_ptr;
    class StepTicker*   step_ticker_ptr;
    
    QueueHandle_t       gcode_str_queue_handle;
    QueueHandle_t       gcode_result_queue_handle;
    EventGroupHandle_t  input_events_handle;

}GLOBAL_TASK_INFO_STRUCT_TYPE, *GLOBAL_TASK_INFO_TYPE;

void Init_UserTasks_and_Objects(void);

#endif
