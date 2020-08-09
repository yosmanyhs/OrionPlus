#include <stm32f4xx_hal.h>

#include "gpio.h"
#include "dma.h"
#include "fsmc.h"
#include "hw_timers.h"
#include "spi_ports.h"
#include "uart_ports.h"
#include "adc.h"
#include "watchdog.h"

#include "settings_manager.h"
#include "user_tasks.h"


static void SystemClock_Config(void);
static void Error_Handler(void);

static void CheckResetCause(void);


int main(void)
{
    CheckResetCause();
    
    HAL_Init();
    SystemClock_Config();
    
    //Init_WatchDog();
    
    Init_GPIO_Pins();
    Init_DMA_Controller();
    Init_FSMC_Controller();
    Init_Stepper_Timer2();
    Init_Unstep_Timer6();
    Init_Flash_SPI1();
    Init_Debug_UART1();
    Init_ADC();
    Init_SDCard_SPI3();
    Init_BacklightPwm_Timer12();
    Init_Spindle_UART3();
    Init_Pwm3_Timer4();
    Init_Pwm12_Timer9();
    
    Settings_Manager::Initialize();
    
    Init_UserTasks_and_Objects();
    
    while (1)
    {
        NVIC_SystemReset();
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    /** Initializes the CPU, AHB and APB busses clocks */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 4;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
  
    /** Initializes the CPU, AHB and APB busses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                 |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
    
    /** Enables the Clock Security System */
    HAL_RCC_EnableCSS();
}

static void CheckResetCause(void)
{
    uint32_t flags = RCC->CSR;
    
    if ((flags & RCC_CSR_LPWRRSTF) != 0)
    {
        // Low power management reset
    }
    else if ((flags & RCC_CSR_WWDGRSTF) != 0)
    {
        // Window watchdog reset
    }
    else if ((flags & RCC_CSR_IWDGRSTF) != 0)
    {
        // Independent watchdog reset
    }
    else if ((flags & RCC_CSR_SFTRSTF) != 0)
    {
        // software reset
    }
    else if ((flags & RCC_CSR_PORRSTF) != 0)
    {
        // power on reset
    }
    else if ((flags & RCC_CSR_BORRSTF) != 0)
    {
        // brown out reset
    }
    else if ((flags & RCC_CSR_PINRSTF) != 0)
    {
        // NRST pin reset
    }   

    // Clear reset cause
    __HAL_RCC_CLEAR_RESET_FLAGS();
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    while (1)
    {
    }
}



#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */

