#include <stm32f4xx_hal.h>
#include "pins.h"
#include "uart_ports.h"

UART_HandleTypeDef debug_uart_handle;
UART_HandleTypeDef spindle_uart_handle;

/* USART1 init function [Debug Information Interface]*/
void Init_Debug_UART1(void)
{
    debug_uart_handle.Instance = USART1;
    debug_uart_handle.Init.BaudRate = 115200;
    debug_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
    debug_uart_handle.Init.StopBits = UART_STOPBITS_1;
    debug_uart_handle.Init.Parity = UART_PARITY_NONE;
    debug_uart_handle.Init.Mode = UART_MODE_TX_RX;
    debug_uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    debug_uart_handle.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&debug_uart_handle);
}

/* USART3 init function [Spindle Control Interface]*/
void Init_Spindle_UART3(void)
{
    spindle_uart_handle.Instance = USART3;
    spindle_uart_handle.Init.BaudRate = 115200;
    spindle_uart_handle.Init.WordLength = UART_WORDLENGTH_9B;
    spindle_uart_handle.Init.StopBits = UART_STOPBITS_1;
    spindle_uart_handle.Init.Parity = UART_PARITY_EVEN;
    spindle_uart_handle.Init.Mode = UART_MODE_TX_RX;
    spindle_uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    spindle_uart_handle.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&spindle_uart_handle);
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
 
    if (uartHandle->Instance == USART1)
    {
        /* USART1 clock enable */
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /**USART1 GPIO Configuration    
        PA9     ------> USART1_TX
        PA10     ------> USART1_RX 
        */  
        GPIO_InitStruct.Pin = DEBUG_TX_O_Pin | DEBUG_RX_I_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        
        HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
    else if (uartHandle->Instance == USART3)
    {
        /* USART3 clock enable */
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /**USART3 GPIO Configuration    
        PB10     ------> USART3_TX
        PB11     ------> USART3_RX 
        */
        GPIO_InitStruct.Pin = SPIN_TX_O_Pin | SPIN_RX_I_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;

        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* USART3 interrupt Init */
        HAL_NVIC_SetPriority(USART3_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(USART3_IRQn);
    }
}
