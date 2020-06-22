#ifndef SPI_PORTS_H
#define SPI_PORTS_H

#include <stm32f4xx_hal.h>

extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi3;


void Init_Flash_SPI1(void);
void Init_SDCard_SPI3(void);


uint8_t W25QXX_ReadSR(void);
void W25QXX_Write_SR(uint8_t sr);
uint16_t W25QXX_ReadID(void);
void W25QXX_Write_Enable(void);
void W25QXX_Write_Disable(void);
void W25QXX_Read(uint8_t * pBuffer,uint32_t ReadAddr, uint16_t NumByteToRead);
void W25QXX_Write_Page(uint8_t * pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void W25QXX_Erase_Chip(void);
void W25QXX_Erase_Sector(uint32_t Dst_Addr);
void W25QXX_PowerDown(void);
void W25QXX_WakeUp(void);



#endif
