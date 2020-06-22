#include <stm32f4xx_hal.h>
#include "pins.h"

/* FSMC initialization function */
void Init_FSMC_Controller(void)
{
    FSMC_NORSRAM_TimingTypeDef Timing = {0};
    FSMC_NORSRAM_TimingTypeDef ExtTiming = {0};
    
    SRAM_HandleTypeDef hsram3;
    SRAM_HandleTypeDef hsram4;

    /** Perform the SRAM3 memory initialization sequence
    */
    hsram3.Instance = FSMC_NORSRAM_DEVICE;
    hsram3.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
    /* hsram3.Init */
    hsram3.Init.NSBank = FSMC_NORSRAM_BANK3;
    hsram3.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
    hsram3.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
    hsram3.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
    hsram3.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
    hsram3.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
    hsram3.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
    hsram3.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
    hsram3.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
    hsram3.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
    hsram3.Init.ExtendedMode = FSMC_EXTENDED_MODE_ENABLE;
    hsram3.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
    hsram3.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
    hsram3.Init.PageSize = FSMC_PAGE_SIZE_NONE;
  
    /* Timing */
    Timing.AddressSetupTime = 2;
    Timing.AddressHoldTime = 15;
    Timing.DataSetupTime = 10;
    Timing.BusTurnAroundDuration = 15;
    Timing.CLKDivision = 16;
    Timing.DataLatency = 17;
    Timing.AccessMode = FSMC_ACCESS_MODE_A;
  
    /* ExtTiming */
    ExtTiming.AddressSetupTime = 2;
    ExtTiming.AddressHoldTime = 15;
    ExtTiming.DataSetupTime = 10;
    ExtTiming.BusTurnAroundDuration = 15;
    ExtTiming.CLKDivision = 16;
    ExtTiming.DataLatency = 17;
    ExtTiming.AccessMode = FSMC_ACCESS_MODE_A;

    HAL_SRAM_Init(&hsram3, &Timing, &ExtTiming);
    
    /** Perform the SRAM4 memory initialization sequence
    */
    hsram4.Instance = FSMC_NORSRAM_DEVICE;
    hsram4.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
    /* hsram4.Init */
    hsram4.Init.NSBank = FSMC_NORSRAM_BANK4;
    hsram4.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
    hsram4.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
    hsram4.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
    hsram4.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
    hsram4.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
    hsram4.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
    hsram4.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
    hsram4.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
    hsram4.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
    hsram4.Init.ExtendedMode = FSMC_EXTENDED_MODE_ENABLE;
    hsram4.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
    hsram4.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
    hsram4.Init.PageSize = FSMC_PAGE_SIZE_NONE;
  
    /* Timing */
    Timing.AddressSetupTime = 3;
    Timing.AddressHoldTime = 15;
    Timing.DataSetupTime = 27;
    Timing.BusTurnAroundDuration = 15;
    Timing.CLKDivision = 16;
    Timing.DataLatency = 17;
    Timing.AccessMode = FSMC_ACCESS_MODE_A;

    /* ExtTiming */
    ExtTiming.AddressSetupTime = 1;
    ExtTiming.AddressHoldTime = 15;
    ExtTiming.DataSetupTime = 4;
    ExtTiming.BusTurnAroundDuration = 15;
    ExtTiming.CLKDivision = 16;
    ExtTiming.DataLatency = 17;
    ExtTiming.AccessMode = FSMC_ACCESS_MODE_A;

    HAL_SRAM_Init(&hsram4, &Timing, &ExtTiming);
}

static uint32_t FSMC_Initialized = 0;

static void HAL_FSMC_MspInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
 
    if (FSMC_Initialized)
        return;
    
    FSMC_Initialized = 1;
 
    /* Peripheral clock enable */
    __HAL_RCC_FSMC_CLK_ENABLE();

    /** FSMC GPIO Configuration  
    PF0   ------> FSMC_A0
    PF1   ------> FSMC_A1
    PF2   ------> FSMC_A2
    PF3   ------> FSMC_A3
    PF4   ------> FSMC_A4
    PF5   ------> FSMC_A5
    PF12   ------> FSMC_A6
    PF13   ------> FSMC_A7
    PF14   ------> FSMC_A8
    PF15   ------> FSMC_A9
    PG0   ------> FSMC_A10
    PG1   ------> FSMC_A11
    PE7   ------> FSMC_D4
    PE8   ------> FSMC_D5
    PE9   ------> FSMC_D6
    PE10   ------> FSMC_D7
    PE11   ------> FSMC_D8
    PE12   ------> FSMC_D9
    PE13   ------> FSMC_D10
    PE14   ------> FSMC_D11
    PE15   ------> FSMC_D12
    PD8   ------> FSMC_D13
    PD9   ------> FSMC_D14
    PD10   ------> FSMC_D15
    PD11   ------> FSMC_A16
    PD12   ------> FSMC_A17
    PD13   ------> FSMC_A18
    PD14   ------> FSMC_D0
    PD15   ------> FSMC_D1
    PG2   ------> FSMC_A12
    PG3   ------> FSMC_A13
    PG4   ------> FSMC_A14
    PG5   ------> FSMC_A15
    PD0   ------> FSMC_D2
    PD1   ------> FSMC_D3
    PD4   ------> FSMC_NOE
    PD5   ------> FSMC_NWE
    PG10   ------> FSMC_NE3
    PG12   ------> FSMC_NE4
    PE0   ------> FSMC_NBL0
    PE1   ------> FSMC_NBL1
    */
  
    /* GPIO_InitStruct */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                          GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 | 
                          GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;

    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* GPIO_InitStruct */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | 
                          GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_10 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;

    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /* GPIO_InitStruct */
    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | 
                          GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | 
                          GPIO_PIN_15 | GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;

    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* GPIO_InitStruct */
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | 
                          GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15 | 
                          GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;

    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

void HAL_SRAM_MspInit(SRAM_HandleTypeDef* sramHandle)
{
    HAL_FSMC_MspInit();
}

