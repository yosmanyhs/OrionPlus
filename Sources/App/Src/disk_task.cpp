#include <stm32f4xx_hal.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"



#include "task_settings.h"
#include "disk_task.h"
#include "settings_manager.h"

TaskHandle_t disk_task_handle;

void DiskTask_Entry(void * pvParam)
{
    // Initialize FAT Stack [SDCard Device]
    
    for ( ; ; )
    {
        vTaskSuspend(NULL); 
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

