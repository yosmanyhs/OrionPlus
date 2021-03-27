#include <stm32f4xx_hal.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"



#include "task_settings.h"
#include "usb_task.h"
#include "settings_manager.h"

#include "tusb.h"
#include "pins.h"

TaskHandle_t usb_task_handle;

static void usb_low_level_init(void);


void USBTask_Entry(void * pvParam)
{   
    /* Initialize USB low level components */
    usb_low_level_init();
    
    /* Initialize USB Stack [CDC Device] */
    tusb_init();
    
    // Wait for USB events (signaled from Interrupt to Task Notification)
    for ( ; ; )
    {
        /* This function never returns (unless fails wait on queue using portMAX_DELAY) */
        tud_task();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

static void usb_low_level_init(void)
{
    GPIO_InitTypeDef gpio;
    
    /* Low level USB peripheral init (clocks, gpio, etc.) */
    //__HAL_RCC_GPIOA_CLK_ENABLE();     /* Already done */
    
    gpio.Pin = USB_D_PLUS_Pin | USB_D_MINUS_Pin;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_HIGH;
    gpio.Alternate = GPIO_AF10_OTG_FS;
    
    HAL_GPIO_Init(GPIOA, &gpio);
    
    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
    HAL_NVIC_SetPriority(OTG_FS_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" void OTG_FS_IRQHandler(void)
{
    tud_int_handler();
}


