#include <stm32f4xx_hal.h>

static IWDG_HandleTypeDef  watchdog_handle;

void Init_WatchDog()
{
    watchdog_handle.Instance = IWDG;
    watchdog_handle.Init.Prescaler = IWDG_PRESCALER_64;
    watchdog_handle.Init.Reload = 0x0FFF;
    
    HAL_IWDG_Init(&watchdog_handle);
    
    // Freeze IWDG during debugging halt
    __HAL_DBGMCU_FREEZE_IWDG();
    
    HAL_IWDG_Refresh(&watchdog_handle);
}

void Refresh_WatchDog()
{
    HAL_IWDG_Refresh(&watchdog_handle);
}
