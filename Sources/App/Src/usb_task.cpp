#include <stm32f4xx_hal.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"



#include "task_settings.h"
#include "usb_task.h"
#include "settings_manager.h"

TaskHandle_t usb_task_handle;

void USBTask_Entry(void * pvParam)
{
    // Initialize USB Stack [CDC Device]
    
    // Wait for USB events (signaled from Interrupt to Task Notification)
    for ( ; ; )
    {
         if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)!= 0)
         {
             NVIC_EnableIRQ(OTG_FS_IRQn);
         }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void OTG_FS_IRQHandler(void)
{
    BaseType_t high_prio_task_woken = pdFALSE;
    
    xTaskNotifyFromISR(usb_task_handle, 1, eSetBits, &high_prio_task_woken);
    NVIC_DisableIRQ(OTG_FS_IRQn);
    
    portYIELD_FROM_ISR(high_prio_task_woken);
}
