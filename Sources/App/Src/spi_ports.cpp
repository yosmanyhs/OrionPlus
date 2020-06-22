#include <stm32f4xx_hal.h>
#include "pins.h"
#include "spi_ports.h"

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi3;

static void W25QXX_Wait_Busy(void);

/* SPI1 init function */
void Init_Flash_SPI1(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // ~5 MHz
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    
    HAL_SPI_Init(&hspi1);
}

/* SPI3 init function */
void Init_SDCard_SPI3(void)
{
    hspi3.Instance = SPI3;
    hspi3.Init.Mode = SPI_MODE_MASTER;
    hspi3.Init.Direction = SPI_DIRECTION_2LINES;
    hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi3.Init.NSS = SPI_NSS_SOFT;
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi3.Init.CRCPolynomial = 10;
    
    HAL_SPI_Init(&hspi3);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
  
    if (spiHandle->Instance == SPI1)
    {
        /* SPI1 clock enable */
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /**SPI1 GPIO Configuration    
        PB3     ------> SPI1_SCK
        PB4     ------> SPI1_MISO
        PB5     ------> SPI1_MOSI 
        */
        GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
        
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    else if (spiHandle->Instance == SPI3)
    {
        /* SPI3 clock enable */
        __HAL_RCC_SPI3_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
    
        /**SPI3 GPIO Configuration    
        PC10     ------> SPI3_SCK
        PC11     ------> SPI3_MISO
        PC12     ------> SPI3_MOSI 
        */
        GPIO_InitStruct.Pin = SDCARD_SCK_Pin | SDCARD_MISO_Pin | SDCARD_MOSI_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;

        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//                              W25Q16 Flash Memory Functions                                    //
///////////////////////////////////////////////////////////////////////////////////////////////////

#define FLASH_TIMEOUT_MAX       10000  // ms

#define W25X_WriteEnable		0x06 
#define W25X_WriteDisable		0x04 
#define W25X_ReadStatusReg		0x05 
#define W25X_WriteStatusReg		0x01 
#define W25X_ReadData			0x03 
#define W25X_FastReadData		0x0B 
#define W25X_FastReadDual		0x3B 
#define W25X_PageProgram		0x02 
#define W25X_BlockErase			0xD8 
#define W25X_SectorErase		0x20 
#define W25X_ChipErase			0xC7 
#define W25X_PowerDown			0xB9 
#define W25X_ReleasePowerDown	0xAB 
#define W25X_DeviceID			0xAB 
#define W25X_ManufactDeviceID	0x90 
#define W25X_JedecDeviceID		0x9F 

uint8_t W25QXX_ReadSR(void)
{
	uint8_t buf[2];
	
    // Select Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    
    buf[0] = W25X_ReadStatusReg;
    buf[1] = 0xFF;
    
    HAL_SPI_TransmitReceive(&hspi1, buf, buf, 2, FLASH_TIMEOUT_MAX);
	
    // Deselect Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
	return buf[1];
}

void W25QXX_Write_SR(uint8_t sr)   
{   
    uint8_t buf[2];
    
    buf[0] = W25X_WriteStatusReg;
    buf[1] = 0xFF;
    
	// Select Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    
	HAL_SPI_TransmitReceive(&hspi1, buf, buf, 2, FLASH_TIMEOUT_MAX);
	
    // Deselect Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
}

uint16_t W25QXX_ReadID(void)
{
    uint8_t buf[6];
	
    // Select Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    
    buf[0] = W25X_ManufactDeviceID;
    buf[1] = 0;
    buf[2] = 0;
	buf[3] = 0x00;
    buf[4] = 0xFF;
    buf[5] = 0xFF;
    
    HAL_SPI_TransmitReceive(&hspi1, buf, buf, 6, FLASH_TIMEOUT_MAX);
	
	// Deselect Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    
	return ((uint16_t)(buf[4])) << 8 | buf[5];
}

void W25QXX_Write_Enable(void)   
{
    uint8_t buf = W25X_WriteEnable;
    
	// Select Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    
    HAL_SPI_Transmit(&hspi1, &buf, 1, FLASH_TIMEOUT_MAX);
	
    // Deselect Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
} 

void W25QXX_Write_Disable(void)   
{  
	uint8_t buf = W25X_WriteDisable;
    
	// Select Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    
    HAL_SPI_Transmit(&hspi1, &buf, 1, FLASH_TIMEOUT_MAX);
	
    // Deselect Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
} 		

void W25QXX_Read(uint8_t * pBuffer,uint32_t ReadAddr, uint16_t NumByteToRead)   
{
    uint8_t buf[4];

    buf[0] = W25X_ReadData;
    buf[1] = (uint8_t)((ReadAddr)>>16);
    buf[2] = (uint8_t)((ReadAddr)>>8);
    buf[3] = (uint8_t)(ReadAddr);

    // Select Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);

    // Send command + address
    HAL_SPI_Transmit(&hspi1, buf, 4, FLASH_TIMEOUT_MAX);
    
    // Then, receive all requested data
    HAL_SPI_Receive(&hspi1, pBuffer, NumByteToRead, FLASH_TIMEOUT_MAX);
    
	// Deselect Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
}

void W25QXX_Write_Page(uint8_t * pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
 	uint8_t buf[4];
    
    buf[0] = W25X_PageProgram;
    buf[1] = (uint8_t)((WriteAddr) >> 16);
    buf[2] = (uint8_t)((WriteAddr) >> 8);
    buf[3] = (uint8_t)(WriteAddr);
    
    W25QXX_Write_Enable();                  //Set WEL bit
	
    // Select Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);

    // Send command
    HAL_SPI_Transmit(&hspi1, buf, 4, FLASH_TIMEOUT_MAX);
    
    // Send data
    HAL_SPI_Transmit(&hspi1, pBuffer, NumByteToWrite, FLASH_TIMEOUT_MAX);
    
    // Deselect Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    
	W25QXX_Wait_Busy();
}

void W25QXX_Erase_Chip(void)   
{                
    uint8_t buf = W25X_ChipErase;
    
    
    W25QXX_Write_Enable();                  //Set WEL bit
    W25QXX_Wait_Busy();
    
  	// Select Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    
    HAL_SPI_Transmit(&hspi1, &buf, 1, FLASH_TIMEOUT_MAX);

    // Deselect Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    
	W25QXX_Wait_Busy();
}

void W25QXX_Erase_Sector(uint32_t Dst_Addr)   
{  
    uint8_t buf[4];
    
 	Dst_Addr *= 4096;
    
    buf[0] = W25X_SectorErase;
    buf[1] = (uint8_t)((Dst_Addr)>>16);
    buf[2] = (uint8_t)((Dst_Addr)>>8);
    buf[3] = (uint8_t)Dst_Addr;
    
    W25QXX_Write_Enable();                  //Set WEL bit
    W25QXX_Wait_Busy();   
  	
    // Select Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    
    // Send command
    HAL_SPI_Transmit(&hspi1, buf, 4, FLASH_TIMEOUT_MAX);
    
    // Deselect Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
    
    W25QXX_Wait_Busy();
}

static void W25QXX_Wait_Busy(void)   
{   
	while((W25QXX_ReadSR() & 0x01) == 0x01)
    {
    }
}

void W25QXX_PowerDown(void)   
{ 
    uint8_t buf = W25X_PowerDown;
        
  	// Select Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    
    HAL_SPI_Transmit(&hspi1, &buf, 1, FLASH_TIMEOUT_MAX);

    // Deselect Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
	
    HAL_Delay(1);
}   

void W25QXX_WakeUp(void)   
{  
  	uint8_t buf = W25X_ReleasePowerDown;
        
  	// Select Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
    
    HAL_SPI_Transmit(&hspi1, &buf, 1, FLASH_TIMEOUT_MAX);

    // Deselect Flash Memory
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
	
    HAL_Delay(1);
}   

