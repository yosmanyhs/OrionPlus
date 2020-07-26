#include <stm32f4xx_hal.h>
#include <stdint.h>

#include "user_tasks.h"

#include "gcode_parsing_task.h"
#include "task_settings.h"
#include "settings_manager.h"

#include "GCodeParser.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

TaskHandle_t gcode_task_handle;



void GCodeParsingTask_Entry(void * pvParam)
{
    for ( ; ; )
    {
        vTaskSuspend(NULL);
    }
}
