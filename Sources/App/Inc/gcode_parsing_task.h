#ifndef GCODE_PARSING_TASK_H
#define GCODE_PARSING_TASK_H

#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t gcode_task_handle;

void GCodeParsingTask_Entry(void * pvParam);

#endif
