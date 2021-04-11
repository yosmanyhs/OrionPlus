#ifndef USB_TASK_H
#define USB_TASK_H

#include "FreeRTOS.h"
#include "task.h"


#define USB_EVENT_CONNECTED         (1 << 0)
#define USB_EVENT_DISCONNECTED      (1 << 1)
#define USB_EVENT_SUSPENDED         (1 << 2)
#define USB_EVENT_RESUMED           (1 << 3)

extern TaskHandle_t usb_task_handle;

void USBTask_Entry(void * pvParam);

#endif
