#ifndef DISK_TASK_H
#define DISK_TASK_H

#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t disk_task_handle;

void DiskTask_Entry(void * pvParam);

#endif
