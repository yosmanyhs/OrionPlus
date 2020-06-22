#ifndef USB_TASK_H
#define USB_TASK_H

#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t usb_task_handle;

void USBTask_Entry(void * pvParam);

#endif
