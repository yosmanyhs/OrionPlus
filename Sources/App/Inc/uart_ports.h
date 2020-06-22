#ifndef UART_PORTS_H
#define UART_PORTS_H

#include <stm32f4xx_hal.h>


extern UART_HandleTypeDef debug_uart_handle;
extern UART_HandleTypeDef spindle_uart_handle;

void Init_Debug_UART1(void);
void Init_Spindle_UART3(void);

#endif
