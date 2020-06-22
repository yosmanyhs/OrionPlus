#include <stm32f4xx_hal.h>
#include "pins.h"
#include "gpio.h"
#include "hw_timers.h"

#include "FreeRTOS.h"
#include "task.h"


TIM_HandleTypeDef step_timer_handle;
TIM_HandleTypeDef unstep_timer_handle;

TIM_HandleTypeDef pwm3_timer_handle;
TIM_HandleTypeDef pwm12_timer_handle;
TIM_HandleTypeDef tft_backlight_timer_handle;

static void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle);

/* TIM2 init function */
void Init_Stepper_Timer2(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    step_timer_handle.Instance = TIM2;
    step_timer_handle.Init.Prescaler = 0; // 84 MHz / 1 = 84 MHz
    step_timer_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    step_timer_handle.Init.Period = 840-1; // 100 kHz
    step_timer_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    step_timer_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    HAL_TIM_Base_Init(&step_timer_handle);
    
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  
    HAL_TIM_ConfigClockSource(&step_timer_handle, &sClockSourceConfig);
    
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    
    HAL_TIMEx_MasterConfigSynchronization(&step_timer_handle, &sMasterConfig);
    
    __HAL_DBGMCU_FREEZE_TIM2();
    __HAL_TIM_CLEAR_IT(&step_timer_handle, TIM_IT_UPDATE);
}

void Init_Unstep_Timer6(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    unstep_timer_handle.Instance = TIM6;
    unstep_timer_handle.Init.Prescaler = 84 - 1; // 84 MHz / 84 = 1 MHz (ticks of 1us)
    unstep_timer_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    unstep_timer_handle.Init.Period = 10 - 1; // 10 us period by default
    unstep_timer_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    unstep_timer_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    HAL_TIM_Base_Init(&unstep_timer_handle);
    
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    
    HAL_TIMEx_MasterConfigSynchronization(&unstep_timer_handle, &sMasterConfig);
    
    __HAL_DBGMCU_FREEZE_TIM6();
    __HAL_TIM_CLEAR_IT(&unstep_timer_handle, TIM_IT_UPDATE);
    
    HAL_TIM_OnePulse_Init(&unstep_timer_handle, TIM_OPMODE_SINGLE);
}

/* TIM4 init function */
void Init_Pwm3_Timer4(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    pwm3_timer_handle.Instance = TIM4;
    pwm3_timer_handle.Init.Prescaler = 0;
    pwm3_timer_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    pwm3_timer_handle.Init.Period = 0;
    pwm3_timer_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    pwm3_timer_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    HAL_TIM_Base_Init(&pwm3_timer_handle);
    
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  
    HAL_TIM_ConfigClockSource(&pwm3_timer_handle, &sClockSourceConfig);
  
    HAL_TIM_PWM_Init(&pwm3_timer_handle);
  
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  
    HAL_TIMEx_MasterConfigSynchronization(&pwm3_timer_handle, &sMasterConfig);
    
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    HAL_TIM_PWM_ConfigChannel(&pwm3_timer_handle, &sConfigOC, TIM_CHANNEL_2);
    
    HAL_TIM_MspPostInit(&pwm3_timer_handle);
}

/* TIM9 init function */
void Init_Pwm12_Timer9(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    pwm12_timer_handle.Instance = TIM9;
    pwm12_timer_handle.Init.Prescaler = 0;
    pwm12_timer_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    pwm12_timer_handle.Init.Period = 0;
    pwm12_timer_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    pwm12_timer_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    
    HAL_TIM_Base_Init(&pwm12_timer_handle);
    
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

    HAL_TIM_ConfigClockSource(&pwm12_timer_handle, &sClockSourceConfig);
    
    HAL_TIM_PWM_Init(&pwm12_timer_handle);
  
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    HAL_TIM_PWM_ConfigChannel(&pwm12_timer_handle, &sConfigOC, TIM_CHANNEL_1);
  
    HAL_TIM_PWM_ConfigChannel(&pwm12_timer_handle, &sConfigOC, TIM_CHANNEL_2);
 
    HAL_TIM_MspPostInit(&pwm12_timer_handle);
}

/* TIM12 init function (TFT Backlight Control)*/
void Init_BacklightPwm_Timer12(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    tft_backlight_timer_handle.Instance = TIM12;
    tft_backlight_timer_handle.Init.Prescaler = 4199;
    tft_backlight_timer_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    tft_backlight_timer_handle.Init.Period = 99;
    tft_backlight_timer_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tft_backlight_timer_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&tft_backlight_timer_handle);

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;

    HAL_TIM_ConfigClockSource(&tft_backlight_timer_handle, &sClockSourceConfig);

    HAL_TIM_PWM_Init(&tft_backlight_timer_handle);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    HAL_TIM_PWM_ConfigChannel(&tft_backlight_timer_handle, &sConfigOC, TIM_CHANNEL_2);
    
    HAL_TIM_MspPostInit(&tft_backlight_timer_handle);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{
    if (tim_baseHandle->Instance == TIM2)
    {
        /* TIM2 clock enable */
        __HAL_RCC_TIM2_CLK_ENABLE();

        /* TIM2 interrupt Init */
        HAL_NVIC_SetPriority(TIM2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn); 
    }
    else if (tim_baseHandle->Instance == TIM4)
    {
        /* TIM4 clock enable */
        __HAL_RCC_TIM4_CLK_ENABLE();
    }
    else if(tim_baseHandle->Instance == TIM6)
    {
        /* TIM6 clock enable */
        __HAL_RCC_TIM6_CLK_ENABLE();

        /* TIM6 interrupt Init */
        HAL_NVIC_SetPriority(TIM6_DAC_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    }
    else if (tim_baseHandle->Instance == TIM9)
    {  
        /* TIM9 clock enable */
        __HAL_RCC_TIM9_CLK_ENABLE();
    }
    else if (tim_baseHandle->Instance == TIM12)
    {
        /* TIM12 clock enable */
        __HAL_RCC_TIM12_CLK_ENABLE();
    }
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
  
    if (timHandle->Instance == TIM4)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /**TIM4 GPIO Configuration    
        PB7     ------> TIM4_CH2 
        */
        GPIO_InitStruct.Pin = PWM_3_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
        HAL_GPIO_Init(PWM_3_GPIO_Port, &GPIO_InitStruct);
    }
    else if (timHandle->Instance == TIM9)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();

        /**TIM9 GPIO Configuration    
        PE5     ------> TIM9_CH1
        PE6     ------> TIM9_CH2 
        */
        GPIO_InitStruct.Pin = PWM_1_Pin | PWM_2_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF3_TIM9;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    }
    else if (timHandle->Instance == TIM12)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    
        /**TIM12 GPIO Configuration    
        PB15     ------> TIM12_CH2 
        */
        GPIO_InitStruct.Pin = TFT_BACKLIGHT_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF9_TIM12;
        HAL_GPIO_Init(TFT_BACKLIGHT_GPIO_Port, &GPIO_InitStruct);
    }
}
