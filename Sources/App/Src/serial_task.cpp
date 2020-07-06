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
    char ch;
    char prev_ch;
    char * line_buffer;
    uint32_t ch_counter = 0;
    int32_t gcode_result;
    bool allow_processing = false;   
    
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
    
    prev_ch = 0;    
    
    for ( ; ; )
    {
        // Try to receive data from serial stream
        if (1 == xStreamBufferReceive(rx_buffer, (void*)&ch, 1, portMAX_DELAY))
        {
            if (ch_counter < (RX_BUFFER_SIZE + 1))
            {
                // We still have space available in the line buffer
                if (ch == '\n')
                {
                    // End of line. Check for previous \r
                    if (prev_ch == '\r')
                        ch_counter--;
                    
                    line_buffer[ch_counter] = '\0';
                    
                    // Reset counter and call to process line 
                    // Not allow the modification of line_buffer during processing
                    ch_counter = 0;
                    prev_ch = 0;    // ???
                    
                    allow_processing = true;                    
                }
                else if (ch == '\b')
                {
                    // Backspace support
                    if (ch_counter > 0)
                        ch_counter--;
                }
                else
                {
                    line_buffer[ch_counter++] = ch;
                    prev_ch = ch;
                }
            }
            else
            {
                // There is no more space available. Send the line 'as is'. 
                line_buffer[RX_BUFFER_SIZE] = '\0';
                allow_processing = true;
            }
        }

        if (allow_processing == true)
        {
            int len;
            
            xQueueSendToBack(((GLOBAL_TASK_INFO_TYPE)pvParam)->gcode_str_queue_handle, (const void*)&line_buffer, portMAX_DELAY);
                    
            // Retrieve response back
            xQueueReceive(((GLOBAL_TASK_INFO_TYPE)pvParam)->gcode_result_queue_handle, (void*)&gcode_result, portMAX_DELAY);
            
            
            const char* msg = GCodeParser::GetErrorText(gcode_result);
            
            // Send back response
            len = strlen(line_buffer);
            
            if (len != 0)
            {            
                xStreamBufferSend(tx_buffer, (const void*)line_buffer, len, portMAX_DELAY);
                xStreamBufferSend(tx_buffer, (const void*)(" >> "), 4, portMAX_DELAY);
            }
            
            xStreamBufferSend(tx_buffer, (const void*)msg, strlen(msg), portMAX_DELAY);
            xStreamBufferSend(tx_buffer, (const void*)("\n"), 1, portMAX_DELAY);
            __HAL_UART_ENABLE_IT(&debug_uart_handle, UART_IT_TXE);
    
            memset(line_buffer, 0, RX_BUFFER_SIZE+1);
            allow_processing = false;
            
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
        
        if (0 == xStreamBufferSendFromISR(rx_buffer, (const void*)&ch, 1, &highPrioWokenRx))
        {
           __nop();
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
