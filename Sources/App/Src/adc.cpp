#include <stm32f4xx_hal.h>
#include "pins.h"
#include "adc.h"

ADC_HandleTypeDef hadc1;

/* ADC1 init function */
void Init_ADC(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint32_t index;

    /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion) */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE; // Try to use Timer4 CC4 as ext trigger
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 8;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    
    HAL_ADC_Init(&hadc1);
  
    /** Configure for the selected ADC regular channel its corresponding rank 
        in the sequencer and its sample time. */
    
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  
    for (index = 0; index < 8; index++)
    {
        sConfig.Channel = ADC_CHANNEL_0 + index;
        sConfig.Rank = index + 1;
        
        HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    }
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

//  if (adcHandle->Instance==ADC1) [Only ADC1]
    {
        /* ADC1 clock enable */
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /**ADC1 GPIO Configuration    
        PA0     ------> ADC1_IN0
        PA1     ------> ADC1_IN1
        PA2     ------> ADC1_IN2
        PA3     ------> ADC1_IN3
        PA4     ------> ADC1_IN4
        PA5     ------> ADC1_IN5
        PA6     ------> ADC1_IN6
        PA7     ------> ADC1_IN7 
        */
        GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                              GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}
