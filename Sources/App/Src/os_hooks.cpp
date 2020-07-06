#include <stm32f4xx_hal.h>
#include "FreeRTOS.h"
#include "task.h"
#include "pins.h"

#include "gpio.h"
#include "lvgl.h"

#include "user_tasks.h"

extern GLOBAL_TASK_INFO_STRUCT_TYPE global_task_info;

/* FreeRTOS Hooks */
extern "C" void vApplicationIdleHook( void )
{
    static uint32_t counter = 0;
    
    if (++counter == 500000)
    {
        counter = 0;
        HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);
    }
    
    if (global_task_info.ready_to_use == true)
    {
        if (counter == 0)
            global_task_info.conveyor_ptr->on_idle();
    }
}

extern "C" void vApplicationTickHook( void )
{
    lv_tick_inc(1);
    user_button_read_function();
}

extern "C" void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    configASSERT(0);
}



extern "C" void vApplicationMallocFailedHook( void )
{
    configASSERT(0);
}

