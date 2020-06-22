#include <stm32f4xx_hal.h>
#include <stdint.h>

#include "gcode_parsing_task.h"
#include "user_tasks.h"
#include "task_settings.h"
#include "settings_manager.h"

#include "GCodeParser.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

TaskHandle_t gcode_task_handle;



void GCodeParsingTask_Entry(void * pvParam)
{    
    GCodeParser* parser = ((GLOBAL_TASK_INFO_TYPE)pvParam)->gcode_parser_ptr;
    char* gcode_string = NULL;
    int32_t parse_result;
    
    for ( ; ; )
    {        
        // Try to receive some gcode string from queue
        if (pdPASS == xQueueReceive(((GLOBAL_TASK_INFO_TYPE)pvParam)->gcode_str_queue_handle, (void*)&gcode_string, portMAX_DELAY))
        {
            // Process received data
            if (gcode_string != NULL)
            {
                parse_result = parser->ParseLine(gcode_string);
                
                // Send back response
                xQueueSendToBack(((GLOBAL_TASK_INFO_TYPE)pvParam)->gcode_result_queue_handle, (const void*)&parse_result, portMAX_DELAY);                
                gcode_string = NULL;
            }
        }
    }
}
