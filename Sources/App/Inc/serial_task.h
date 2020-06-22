#ifndef SERIAL_TASK
#define SERIAL_TASK

#include "FreeRTOS.h"
#include "task.h"

#define RX_BUFFER_SIZE  256
#define TX_BUFFER_SIZE  128

extern TaskHandle_t serial_task_handle;

void SerialTask_Entry(void * pvParam);

#endif
