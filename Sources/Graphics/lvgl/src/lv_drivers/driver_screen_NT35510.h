#ifndef DRIVER_SCREEN_NT35510_H
#define DRIVER_SCREEN_NT35510_H

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include <stm32f4xx_hal.h>

#pragma anon_unions
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

//void NT35510_Initialize(void);
//void NT35510_SetBacklight(uint8_t percent);
//void NT35510_SetViewPort(uint16_t x, uint16_t y, uint16_t cx, uint16_t cy);
//void NT35510_WriteData_Increment(uint16_t * data, uint32_t nr_bytes);
//void NT35510_WriteData_No_Increment(uint16_t data, uint32_t nr_bytes);

void lv_port_disp_NT35510_init(void);
void lv_port_disp_NT35510_set_backlight(uint8_t percent);

#ifdef __cplusplus
 }
#endif

#endif
