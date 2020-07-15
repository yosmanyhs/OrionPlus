#include <stm32f4xx_hal.h>
#include "pins.h"
#include "gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "user_tasks.h"

extern GLOBAL_TASK_INFO_STRUCT_TYPE global_task_info;

void Init_GPIO_Pins(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint32_t exti_pr = 0;

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOC, CTOUCH_RESET_Pin |         // PC13
                             STEP_Z_Pin | DIR_Z_Pin |   // PC0, PC1
                             STEP_Y_Pin | DIR_Y_Pin |   // PC2, PC3
                             STEP_X_Pin | DIR_X_Pin,    // PC4, PC5
                      GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOF, 
                      LED_0_Pin | LED_1_Pin,            // PF9, PF10
                      GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, 
                      CTOUCH_SCL_Pin | FLASH_CS_Pin,    // PB10, PB14
                      GPIO_PIN_SET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(CTOUCH_SDA_GPIO_Port, 
                      CTOUCH_SDA_Pin,                   // PF11
                      GPIO_PIN_SET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, 
                      SPIN_RESET_Pin | COOLANT_ENABLE_Pin, // PB13, PB6
                      GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(STEP_RESET_GPIO_Port, 
                      STEP_RESET_Pin,                   // PG7
                      GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(STEP_ENABLE_GPIO_Port, 
                      STEP_ENABLE_Pin,                  // PC9
                      GPIO_PIN_SET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(SDCARD_CS_GPIO_Port, 
                      SDCARD_CS_Pin,                    // PA15
                      GPIO_PIN_SET);

    /*Configure GPIO pins : Port C */
    GPIO_InitStruct.Pin = CTOUCH_RESET_Pin |            // PC13
                          STEP_Z_Pin | DIR_Z_Pin |      // PC0, PC1
                          STEP_Y_Pin | DIR_Y_Pin |      // PC2, PC3
                          STEP_X_Pin | DIR_X_Pin |      // PC4, PC5
                          STEP_ENABLE_Pin;              // PC9

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pins : Port F */
    GPIO_InitStruct.Pin = BTN_START_Pin |               // PF6
                          BTN_HOLD_Pin |                // PF7
                          BTN_ABORT_Pin;                // PF8
                          
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /*Configure GPIO pins : Port F */
    GPIO_InitStruct.Pin = LED_0_Pin | LED_1_Pin;        // PF9, PF10
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /*Configure GPIO pins : Port B */
    GPIO_InitStruct.Pin = CTOUCH_SCL_Pin | SPIN_RESET_Pin;  // PB0, PB13
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pins : Port B */
    GPIO_InitStruct.Pin = CTOUCH_IRQ_Pin | SPIN_AUX_Pin;    // PB1, PB12
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : Port F */
    GPIO_InitStruct.Pin = CTOUCH_SDA_Pin;                   // PF11
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CTOUCH_SDA_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : Port B */
    GPIO_InitStruct.Pin = FLASH_CS_Pin | COOLANT_ENABLE_Pin;    // PB14, PB6
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : Port G */
    GPIO_InitStruct.Pin = GLOBAL_FAULT_Pin;             // PG6 -> EXTI6
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GLOBAL_FAULT_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : Port G */
    GPIO_InitStruct.Pin = STEP_RESET_Pin;               // PG7
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STEP_RESET_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : Port G */
    GPIO_InitStruct.Pin = PROBE_INPUT_Pin;              // PG8
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(PROBE_INPUT_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : Port C */
    GPIO_InitStruct.Pin = LIM_X_Pin | LIM_Y_Pin | LIM_Z_Pin;  // PC6, PC7, PC8  -> EXTI6, 7, 8
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /*Configure GPIO pin : PtPin */
    GPIO_InitStruct.Pin = SDCARD_CS_Pin;                    // PA15
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SDCARD_CS_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : Port B */
    GPIO_InitStruct.Pin = KEY_1_Pin | KEY_0_Pin;            // PB8, PB9
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // Clear Pending register bits (set during configuration)
    exti_pr = EXTI->PR;
    EXTI->PR = exti_pr;
    
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn); 
}

extern "C" void EXTI9_5_IRQHandler(void)
{
    uint32_t pr_value = EXTI->PR;
    BaseType_t high_prio_woken = pdFALSE;
    
    // Check each bit [EXTI6, EXTI7, EXTI8]
    // EXTI6
    if ((pr_value & (1 << 6)) != 0)
    {
        // EXTI6 can be from PC6 [Limit X] / PG6 [Global Stepper Fault]
        if (HAL_GPIO_ReadPin(GLOBAL_FAULT_GPIO_Port, GLOBAL_FAULT_Pin) != GPIO_PIN_SET)
        {
            // Global Fault = 0. Stepper motors fault condition
            xEventGroupSetBitsFromISR(global_task_info.input_events_handle,  STEPPER_FAULT_EVENT, &high_prio_woken);
        }
        else if (HAL_GPIO_ReadPin(LIM_X_GPIO_Port, LIM_X_Pin) != GPIO_PIN_RESET)
        {
            // Limit X = 1
            xEventGroupSetBitsFromISR(global_task_info.input_events_handle,  LIMIT_X_MIN_EVENT, &high_prio_woken);
        }
    }
    
    // EXTI7
    if ((pr_value & (1 << 7)) != 0)
    {
        // Limit Y
        xEventGroupSetBitsFromISR(global_task_info.input_events_handle,  LIMIT_Y_MIN_EVENT, &high_prio_woken);
    }
    
    // EXTI8
    if ((pr_value & (1 << 8)) != 0)
    {
        // Limit Z
        xEventGroupSetBitsFromISR(global_task_info.input_events_handle,  LIMIT_Z_MIN_EVENT, &high_prio_woken);
    }
    
    // Clear all pending bits
    EXTI->PR = pr_value;
    
    portYIELD_FROM_ISR(high_prio_woken);
}
