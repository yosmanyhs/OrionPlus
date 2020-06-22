#include <stm32f4xx_hal.h>
#include "FreeRTOS.h"
#include "task.h"

// This file overrides the default implementation of HAL_Tick and HAL_Delay support functions
// the functions call the underlying FreeRTOS functions


HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    /* Return function status */
    return HAL_OK;
}

void HAL_IncTick(void)
{
}

uint32_t HAL_GetTick(void)
{
    return xTaskGetTickCount();
}

void HAL_Delay(uint32_t Delay)
{
    vTaskDelay(Delay);
}

void HAL_SuspendTick(void)
{
}

void HAL_ResumeTick(void)
{
}

