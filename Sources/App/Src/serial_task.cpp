#include <stm32f4xx_hal.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

#include "task_settings.h"
#include "user_tasks.h"
#include "serial_task.h"
#include "settings_manager.h"

#include "uart_ports.h"
#include "pins.h"

#include "GCodeParser.h"


const uint32_t  RX_LINE_NOTIFY_VALUE        (1 << 0);

TaskHandle_t serial_task_handle;

static StreamBufferHandle_t     rx_buffer;
static StreamBufferHandle_t     tx_buffer; 

//const char* lines[] = 
//{ 
//    "G17 G90 G21 G54",
//    "G10 L2 P2 X10 Y10 Z10 A10 B10 C10",
//    "T1 M6",
//    "G0 X100 Y100",
//    "S5000 M3 M8 F20",
//    "G1 Z-0.5",
//    "G1 X0 Y0",
//    "G0 Z50",
//    "M9 M5",
//    "M30"
//};

void SerialTask_Entry(void * pvParam)
{
    char * line_buffer;
    uint32_t avail_count = 0;
    uint32_t task_notify_value = 0;
    uint32_t led_counter = 0;    
    int32_t gcode_result;
    
    // Create Tx & Rx stream buffers
    rx_buffer = xStreamBufferCreate(RX_BUFFER_SIZE, 1);
    tx_buffer = xStreamBufferCreate(TX_BUFFER_SIZE, 1);
    line_buffer = (char*)pvPortMalloc(RX_BUFFER_SIZE + 1);
    
    if (rx_buffer == NULL || tx_buffer == NULL || line_buffer == NULL)
    {
        configASSERT(0);
    }
    
    /* USART1 interrupt Init */
    __HAL_UART_ENABLE_IT(&debug_uart_handle, UART_IT_RXNE);
    
    for ( ; ; )
    {        
        if (pdPASS == xTaskNotifyWait(ULONG_MAX, ULONG_MAX, &task_notify_value, portMAX_DELAY))
        {
            if ((RX_LINE_NOTIFY_VALUE & task_notify_value) != 0)
            {
                // Copy received line to line_buffer and process
                avail_count = xStreamBufferBytesAvailable(rx_buffer);
                
                avail_count = xStreamBufferReceive(rx_buffer, (void*)line_buffer, avail_count, 0);
                line_buffer[avail_count] = '\0';
                
                if (++led_counter == 2)
                {
                    led_counter = 0;
                    HAL_GPIO_TogglePin(LED_0_GPIO_Port, LED_0_Pin);
                }
    
                // Process received data
                xQueueSendToBack(((GLOBAL_TASK_INFO_TYPE)pvParam)->gcode_str_queue_handle, (const void*)&line_buffer, portMAX_DELAY);
                
                // Retrieve response back
                xQueueReceive(((GLOBAL_TASK_INFO_TYPE)pvParam)->gcode_result_queue_handle, (void*)&gcode_result, portMAX_DELAY);
                
                const char* msg = GCodeParser::GetErrorText(gcode_result);
                
                // Send back response
                xStreamBufferSend(tx_buffer, (const void*)msg, strlen(msg), portMAX_DELAY);
                xStreamBufferSend(tx_buffer, (const void*)("\r\n"), 2, portMAX_DELAY);
                __HAL_UART_ENABLE_IT(&debug_uart_handle, UART_IT_TXE);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" void USART1_IRQHandler(void)
{
    char ch;
    BaseType_t highPrioWokenRx = pdFALSE;
    BaseType_t highPrioWokenTx = pdFALSE;
    BaseType_t highPrioWokenLine = pdFALSE;
    
    // Check if rx'd something
    if (__HAL_UART_GET_FLAG(&debug_uart_handle, UART_FLAG_RXNE))
    {
        // Rx Interrupt
        ch = (char)debug_uart_handle.Instance->DR;
        
        // Ignore \r and notify task when receive \n
        if (ch != '\r')
        {
            if (ch == '\n') // Don't store \n
                xTaskNotifyFromISR(serial_task_handle, RX_LINE_NOTIFY_VALUE, eSetBits, &highPrioWokenLine);
            else
                xStreamBufferSendFromISR(rx_buffer, (const void*)&ch, 1, &highPrioWokenRx);
        }
    }
    
    if ( (__HAL_UART_GET_FLAG(&debug_uart_handle, UART_FLAG_TXE)) && 
         ((debug_uart_handle.Instance->CR1 & USART_CR1_TXEIE) == USART_CR1_TXEIE))
    {
        // Tx Interrupt
        /* The interrupt was caused by the THR becoming empty.  Are there any
		more characters to transmit? */
        if (xStreamBufferReceiveFromISR( tx_buffer, (void*) &ch, 1, &highPrioWokenTx) != pdFALSE)
		{
			/* A character was retrieved from the queue so can be sent to the
			THR now. */
            debug_uart_handle.Instance->DR = ch;
		}
		else
		{
            // Disable if no more data
            __HAL_UART_DISABLE_IT(&debug_uart_handle, UART_IT_TXE);
		}
    }

    portYIELD_FROM_ISR(highPrioWokenRx || highPrioWokenTx || highPrioWokenLine);   
}
