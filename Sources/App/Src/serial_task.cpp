#include <stm32f4xx_hal.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

#include "task_settings.h"
#include "user_tasks.h"
#include "serial_task.h"
#include "usb_task.h"
#include "settings_manager.h"

#include "uart_ports.h"
#include "pins.h"

#include "GCodeParser.h"
#include "tusb.h"
#include "cdc_device.h"

TaskHandle_t serial_task_handle;

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

//static const char * hello_msg = "\r\nGrbl 1.1h ['$' for help]\r\n";

//static const char * fake_settings = \
//    "$0=10\r\n$1=25\r\n$2=0\r\n$3=0\r\n$4=0\r\n$5=0\r\n$6=0\r\n" \
//    "$10=1\r\n$11=0.010\r\n$12=0.002\r\n$13=0\r\n" \
//    "$20=0\r\n$21=0\r\n$22=1\r\n$23=0\r\n$24=25.0\r\n$25=500\r\n$26=250\r\n$27=1\r\n" \
//    "$30=1000\r\n$31=0\r\n$32=0\r\n" \
//    "$100=200\r\n$101=200\r\n$102=200\r\n" \
//    "$110=500\r\n$111=500\r\n$112=500\r\n" \
//    "$120=10\r\n$121=10\r\n$122=10\r\n" \
//    "$130=360\r\n$131=360\r\n$132=200\r\n";

void SerialTask_Entry(void * pvParam)
{   
    char * line;    
    
    char ch = 0;
    char prev_ch = 0;
    
    uint32_t ch_counter = 0;
    uint32_t avail;
    bool allow_processing = false;   
    
    /* Reserve memory for line copy/processing */
    line = new char[RX_BUFFER_SIZE + 1];
    
    for ( ; ; )
    {
        // Try to receive data from CDC FIFO buffer
        avail = tud_cdc_n_available(0);

        if (avail != 0)
        {
            ch = (char)tud_cdc_n_read_char(0);
        
            if (ch_counter < (RX_BUFFER_SIZE + 1))
            {
                // We still have space available in the line buffer
                if (ch == '\n')
                {
                    // End of line. Check for previous \r
                    if (prev_ch == '\r')
                        ch_counter--;
                    
                    line[ch_counter] = '\0';
                    
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
                    line[ch_counter++] = ch;
                    prev_ch = ch;
                }
            }
            else
            {
                // There is no more space available. Send the line 'as is'. 
                line[RX_BUFFER_SIZE] = '\0';
                allow_processing = true;
            }
        }

        if (allow_processing == true)
        {
            const char* msg = machine->GetGCodeErrorText(machine->ParseGCodeLine(line));
            
            // Send back response            
            if (*line != '\0')
            {
                tud_cdc_n_write_str(0, line);
                tud_cdc_n_write_str(0, " >> ");
            }
            
            tud_cdc_n_write_str(0, msg);
            tud_cdc_n_write_str(0, "\r\n");
            tud_cdc_n_write_flush(0);

            allow_processing = false;            
        }
        else if (avail == 0)
        {
            // If there is nothing to process nor data available take a rest
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" void USART1_IRQHandler(void)
{
    char ch;
    
    // Check if rx'd something
    if (__HAL_UART_GET_FLAG(&debug_uart_handle, UART_FLAG_RXNE))
    {
        // Rx Interrupt
        ch = (char)debug_uart_handle.Instance->DR;
    }
    
    if ( (__HAL_UART_GET_FLAG(&debug_uart_handle, UART_FLAG_TXE)) && 
         ((debug_uart_handle.Instance->CR1 & USART_CR1_TXEIE) == USART_CR1_TXEIE))
    {
        // Tx Interrupt
        /* The interrupt was caused by the THR becoming empty.  Are there any
		more characters to transmit? */
        // Disable if no more data
        __HAL_UART_DISABLE_IT(&debug_uart_handle, UART_IT_TXE);
    }
}
