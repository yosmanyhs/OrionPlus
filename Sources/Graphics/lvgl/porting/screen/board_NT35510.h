/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.io/license.html
 */

#ifndef _GDISP_LLD_BOARD_H
#define _GDISP_LLD_BOARD_H

#include <stm32f4xx_hal.h>
#include "pins.h"
#include "dma.h"
#include "hw_timers.h"

#include "gfx.h" 

typedef union COLOR_MAP_32
{
    uint32_t    AsWord32;
    
    struct 
    {
        // B3=?, B2=b, B1=r, B0=g
        uint8_t green_zeroes : 2;
        uint8_t green : 6;
        
        uint8_t red_zeroes : 3;
        uint8_t red : 5;
        
        uint8_t dont_care;
        
        uint8_t blue_zeroes : 3;
        uint8_t blue : 5;
    };
    
} COLOR_MAP_32;



typedef union LCD_TypeDef
{
    struct {
	__IO uint16_t CMD;
	__IO uint16_t RAM;
    };
    
    struct {
    __IO uint16_t Padding;
    __IO uint32_t RAM32;
    };
    
} LCD_TypeDef;


#define LCD_BASE        ((uint32_t)(0x6C000000 | 0x0000007E))
#define LCD             ((LCD_TypeDef * ) LCD_BASE)


#define LCD_SLEEP_IN_CMD            0x1000
#define LCD_SLEEP_OUT_CMD           0x1100


#define LCD_DISPLAY_OFF_CMD         0x2800
#define LCD_DISPLAY_ON_CMD          0x2900


#define LCD_SET_X_CMD_SC_MSB		0x2A00
#define LCD_SET_X_CMD_SC_LSB        0x2A01
#define LCD_SET_X_CMD_EC_MSB        0x2A02
#define LCD_SET_X_CMD_EC_LSB        0x2A03

#define LCD_SET_Y_CMD_SP_MSB		0x2B00 
#define LCD_SET_Y_CMD_SP_LSB		0x2B01 
#define LCD_SET_Y_CMD_EP_MSB		0x2B02 
#define LCD_SET_Y_CMD_EP_LSB		0x2B03 

#define LCD_WR_RAM_CMD		        0x2C00

#define LCD_RD_RAM_CMD              0x2E00

#define LCD_TEAR_EFFECT_OFF_CMD     0x3400
#define LCD_TEAR_EFFECT_ON_CMD      0x3500


#define LCD_ENTRY_MODE_CMD          0x3600

    #define LCD_ENTRY_L2R_T2B   (0 << 5)    /* Normal Mode = Portrait */
    #define LCD_ENTRY_T2B_L2R   (1 << 5)    /* Landscape Mirror */
    #define LCD_ENTRY_R2L_T2B   (2 << 5)    /* Portrait Mirror */
    #define LCD_ENTRY_T2B_R2L   (3 << 5)    /* Landscape */
    #define LCD_ENTRY_L2R_B2T   (4 << 5)    /* Vertically Inverted Portrait  */
    #define LCD_ENTRY_B2T_L2R   (5 << 5)    /* Vertically Inverted Landscape */
    #define LCD_ENTRY_R2L_B2T   (6 << 5)    /* Vertically Inverted Portrait Mirror */
    #define LCD_ENTRY_B2T_R2L   (7 << 5)    /* Vertically Inverted Landscape Mirror */
    
    #define LCD_ENTRY_PORTRAIT              (0 << 5)
    #define LCD_ENTRY_PORTRAIT_MIRROR       (2 << 5)
    
    #define LCD_ENTRY_PORTRAIT_INV          (4 << 5)
    #define LCD_ENTRY_PORTRAIT_MIRROR_INV   (6 << 5)
    
    #define LCD_ENTRY_LANDSCAPE_MIRROR      (1 << 5)
    #define LCD_ENTRY_LANDSCAPE             (3 << 5)
    
    #define LCD_ENTRY_LANDSCAPE_INV         (5 << 5)
    #define LCD_ENTRY_LANDSCAPE_MIRROR_INV  (7 << 5)
    
#define LCD_INTF_PIXEL_FMT_CMD      0x3A00    

#define LCD_READ_ID_1               0xDA00
#define LCD_READ_ID_2               0xDB00
#define LCD_READ_ID_3               0xDC00

static GFXINLINE void init_board(/*GDisplay *g*/) {
	//(void) g;
}

static GFXINLINE void post_init_board(/*GDisplay *g*/) {
	//(void) g;
    
}

static GFXINLINE void setpin_reset(/*GDisplay *g,*/ gBool state) {
	//(void) g;
	(void) state;
}

static GFXINLINE void set_backlight(/*GDisplay *g,*/ gU8 percent) {
	//(void) g;
    if (percent == 0)
        HAL_TIM_PWM_Stop(&htim12, TIM_CHANNEL_2);
    else
        HAL_TIM_PWM_Start(&htim12, TIM_CHANNEL_2);
    
	__HAL_TIM_SetCompare(&htim12, TIM_CHANNEL_2, percent);
}

static GFXINLINE void acquire_bus(/*GDisplay *g*/) {
	//(void) g;
}

static GFXINLINE void release_bus(/*GDisplay *g*/) {
	//(void) g;
}

static GFXINLINE void write_index(/*GDisplay *g,*/ gU16 index) {
	//(void) g;
	LCD->CMD = index;
}

static GFXINLINE void write_data(/*GDisplay *g, */gU16 data) {
//	(void) g;
	LCD->RAM = data;
}

static GFXINLINE void write_reg_val(gU16 reg, gU16 val) {
	LCD->CMD = reg;
	LCD->RAM = val;
}

static GFXINLINE gU16 read_reg_val(gU16 reg)
{
	LCD->CMD = reg;
	__nop();
	return LCD->RAM;
}

static GFXINLINE void setreadmode(/*GDisplay *g*/) {
	//(void) g;    
    LCD->CMD = LCD_RD_RAM_CMD;
}

static GFXINLINE void setwritemode(/*GDisplay *g*/) {
	//(void) g;
    LCD->CMD = LCD_WR_RAM_CMD;
}

#define BLUE_VALUE(uval32)   ((uval32 & 0xF8000000) >> 27)
#define GREEN_VALUE(uval32)  ((uval32 & 0x000000FC) >> 0)
#define RED_VALUE(uval32)  ((uval32 & 0x0000F800) >> 11)

static volatile uint16_t red, green, blue;

static void dummy_read(/*GDisplay* g*/) {
  volatile gU16 dummy;
  dummy = LCD->RAM;
  (void)dummy;
}

static GFXINLINE gU16 read_data(/*GDisplay *g*/) {
	//(void) g;
    uint16_t r=0,g=0,b=0;	
		
 	r = LCD->RAM;  		  						// Actual coordinate color   	
    b = LCD->RAM; 

    g = r & 0xFF;		
    g <<= 8;
	
    return (((r>>11)<<11)|((g>>10)<<5)|(b>>11));
}

static GFXINLINE gU16 read_data32(/*GDisplay *g*/) {
	//(void) g;
    COLOR_MAP_32 cm;
		
    cm.AsWord32 = LCD->RAM32;   // B3=b, B2=?, B1=r, B0=g
    
 	return (cm.red << 11 | cm.green << 5 | cm.blue);
}

static GFXINLINE void write_data_dma_minc(GDisplay *g, gU16 *data, gU32 nr_bytes) {
	(void) g;
	
	//if memory increment disabled?
    if ((hdma_memtomem_dma2_stream0.Instance->CR & DMA_PINC_ENABLE) == DMA_PINC_DISABLE)
    {
        // First, disable stream to be able to modify settings
        __HAL_DMA_DISABLE(&hdma_memtomem_dma2_stream0);
        
        // Enable source memory addr incr (periph inc)
        hdma_memtomem_dma2_stream0.Instance->CR |= DMA_PINC_ENABLE;
    }
    
    // start dma transfer 
    HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)data, (uint32_t)&LCD->RAM, nr_bytes);
    
    // wait for transfer completion/error
	HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, 1000);
}

static GFXINLINE void write_data_dma_nominc(GDisplay *g, gU16 data, gU32 nr_bytes) {
	(void) g;
	
	// if memory increment enabled?
    if ((hdma_memtomem_dma2_stream0.Instance->CR & DMA_PINC_ENABLE) != DMA_PINC_DISABLE)
    {
        // First, disable stream to be able to modify settings
        __HAL_DMA_DISABLE(&hdma_memtomem_dma2_stream0);
        
        // Disable source memory addr incr (periph inc)
        hdma_memtomem_dma2_stream0.Instance->CR &= ~DMA_PINC_ENABLE; 
    }
    
    // send data in chuncks of 64KB max
	while (nr_bytes > UINT16_MAX){
		nr_bytes -= UINT16_MAX;
		
        // start dma transfer using UINT16_MAX as count
        HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&data, (uint32_t)&LCD->RAM, UINT16_MAX);
    
        // wait for transfer completion/error
        HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, 1000);
	}
    
    // send last chunck of data using nr_bytes
    HAL_DMA_Start(&hdma_memtomem_dma2_stream0, (uint32_t)&data, (uint32_t)&LCD->RAM, nr_bytes);

    // wait for transfer completion/error
    HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, 1000);
}

#endif /* _GDISP_LLD_BOARD_H */
