#include <stm32f4xx_hal.h>
#include "FreeRTOS.h"
#include "task.h"
#include "pins.h"
#include "watchdog.h"

#include "gpio.h"
#include "lvgl.h"

#include "user_tasks.h"


/* FreeRTOS Hooks */
extern "C" void vApplicationIdleHook( void )
{
    static uint32_t counter = 0;
    
    if (++counter == 500000)
    {
        counter = 0;
        HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);
    }
    
    if (counter == 0)
        machine->OnIdle();    
    
    // Keep system alive
    //Refresh_WatchDog();
}

extern "C" void vApplicationTickHook( void )
{
    lv_tick_inc(1);
}

extern "C" void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    configASSERT(0);
}



extern "C" void vApplicationMallocFailedHook( void )
{
    configASSERT(0);
}

