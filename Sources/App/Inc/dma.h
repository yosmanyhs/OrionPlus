#ifndef DMA_H
#define DMA_H

#include <stm32f4xx_hal.h>

extern DMA_HandleTypeDef hdma_memtomem_dma2_stream0;

void Init_DMA_Controller(void);

#endif

