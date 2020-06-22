/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.io/license.html
 */

/**
 * @file    gmouse_lld_GT9147_board.h
 * @brief   GINPUT Touch low level driver source for the GT9147
 *
 * @note	This file contains a mix of hardware specific and operating system specific
 *			code. You will need to change it for your CPU and/or operating system.
 */

#ifndef _GINPUT_LLD_MOUSE_BOARD_H
#define _GINPUT_LLD_MOUSE_BOARD_H

#include <stm32f4xx_hal.h>

#include "stm32f4xx_hal_gpio.h"
#include "pins.h"

#include "gfx.h"

// set/clr scl (PB0) 1 << 0
#define SET_SCL()		do { CTOUCH_SCL_GPIO_Port->BSRR = CTOUCH_SCL_Pin; } while (0)
#define CLR_SCL()		do { CTOUCH_SCL_GPIO_Port->BSRR = (CTOUCH_SCL_Pin << 16); } while (0)


// set/clr sda (PF11) 1 << 11
#define SET_SDA()		do { CTOUCH_SDA_GPIO_Port->BSRR = CTOUCH_SDA_Pin; } while (0)
#define CLR_SDA()		do { CTOUCH_SDA_GPIO_Port->BSRR = (CTOUCH_SDA_Pin << 16); } while (0)

#define READ_SDA()      ( CTOUCH_SDA_GPIO_Port->IDR & CTOUCH_SDA_Pin )

	// set/clr reset (PC13)	1 << 13
#define SET_RST()		do { CTOUCH_RESET_GPIO_Port->BSRR = CTOUCH_RESET_Pin; } while(0)
#define CLR_RST()		do { CTOUCH_RESET_GPIO_Port->BSRR = (CTOUCH_RESET_Pin >> 16); } while(0)

// Resolution and Accuracy Settings
#define GMOUSE_GT9147_PEN_CALIBRATE_ERROR		0//8
#define GMOUSE_GT9147_PEN_CLICK_ERROR			0//6
#define GMOUSE_GT9147_PEN_MOVE_ERROR			0//4
#define GMOUSE_GT9147_FINGER_CALIBRATE_ERROR	0//14
#define GMOUSE_GT9147_FINGER_CLICK_ERROR		0//9 //18
#define GMOUSE_GT9147_FINGER_MOVE_ERROR		0//7 //14

// How much extra data to allocate at the end of the GMouse structure for the board's use
#define GMOUSE_GT9147_BOARD_DATA_SIZE			0


/* Soft delay values */
#define DELAY_SCL_HIGH      1
#define DELAY_SCL_LOW       1

#define DELAY_SDA_HIGH      1
#define DELAY_SDA_LOW       1

#define DELAY_SDA_INPUT     1


#define DELAY_START_CONDITION       1
#define DELAY_STOP_CONDITION        1

#define DELAY_DATA_SETUP            1
#define DELAY_DATA_HOLD             1

#define DELAY_READ_ACK              1
#define DELAY_IO_TURN           1

#define DELAY_SEND_ACK_1         10
#define DELAY_SEND_ACK_2         10
#define DELAY_SEND_ACK_3         10

#define GT9147_I2C_READ_ADDR        0xBB
#define GT9147_I2C_WRITE_ADDR       0xBA

#define GT9147_STATUS_REG_LOW       0x4E
#define GT9147_STATUS_REG_HIGH      0x81

#define GT9147_CONFIG_REG_LOW       0x47
#define GT9147_CONFIG_REG_HIGH      0x80

#define GT9147_CRC_REG_HIGH         0x80
#define GT9147_CRC_REG_LOW          0xFF


#define GT9147_COMMAND_REG_LOW      0x40
#define GT9147_COMMAND_REG_HIGH     0x80

#define GT9147_STATUS_BUF_VALID_MASK    0x80
#define GT9147_STATUS_TP_COUNT_MASK     0x0F


///* GT9147 Configuration Table Values */
#define GT9147_CONFIG_VERSION       0x32


static const uint8_t GT9147_Config_Table[] = 
{
    GT9147_CONFIG_VERSION, 
    0xE0, 0x01, 0x20, 0x03, // X max = 0x01E0 (480); Y max = 0x0320 (800)
    0x05,                   // Max Touch Number
    0x37, 0x00,             // Module Switch 1 & 2 [INT HIGH LEVEL]
    0x02, 0x08, 0x1E,       // Shake_Count, Filter, Large_Touch
    0x08, 0x50, 0x3C, 0x0F, // Noise Reduction, Screen_Touch_Level, Screen_Leave_Level, Low_Power_Control
    0x05,                   // Refresh Rate (5 + 0x05) ms
    0x00, 0x00,             // x_threshold, y_threshold
    0xFF, 0x67,             // Gesture_Switch1, Gesture_Switch2
    0x50, 0x00,             // WSpace @ 0x805B:0x805C [T(x32) = 5, B(x32) = 0, L(x32) = 0, R(x32) = 0]
    0x00,                   // Mini_Filter
    0x18, 0x1A, 0x1E, 0x14, // Stretch_R0, R1, R2, RM
    0x89, 0x28,             // Drv_GroupA_Num, Drv_GroupB_Num
    0x0A,                   // Sensor_Num @ 0x8064
    0x30, 0x2E,             // FreqA_factor, FreqB_factor @ 0x8065:0x8066     
    0xBB, 0x0A,             // Pannel_BitFreqL, Pannel_BitFreqH @ 0x8067:0x8068
    0x03,                   // MODULE_SWITCH4 @ 0x8069 [Approch_En, HotKnot_En]
    0x00,                   // Gesture_Long_Press_Time @ 0x806A
    0x00,                   // Pannel_Tx_Gain @ 0x806B [Pannel_Drv_output_R:4, Pannel_DAC_Gain {0 max, 7 min}]
    0x02,                   // Pannel_Rx_Gain @ 0x806C [Pannel_PGA_Gain = 2]
    0x33,                   // Pannel_Dump_Shift @ 0x806D [??]
    0x1D,                   // Drv_Frame_Control @ 0x806E []
    0x00,                   // @ 0x806F
    0x00,                   // Module_Switch3 @ 0x8070
    0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	0x2A, 0x1C, 0x5A, 0x94, 0xC5, 0x02, 0x07, 0x00,   
    0x00, 0x00, 0xB5, 0x1F, 0x00, 0x90, 0x28, 0x00,
    0x77, 0x32, 0x00, 0x62, 0x3F, 0x00, 0x52, 0x50,
    0x00, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0F, 0x0F, 0x03, 0x06, 0x10, 0x42, 0xF8, 
    0x0F, 0x14, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x18,
    0x16, 0x14, 0x12, 0x10, 0x0E, 0x0C, 0x0A, 0x08, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x29, 0x28, 0x24, 0x22,
    0x20, 0x1F, 0x1E, 0x1D, 0x0E, 0x0C, 0x0A, 0x08,
    0x06, 0x05, 0x04, 0x02, 0x00, 0xFF, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,    
    
    // 80F2 ... 80FE
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF 
};

/* Function prototypes */
static uint8_t soft_i2c_send_byte(uint8_t tx);
static uint8_t soft_i2c_recv_byte(uint8_t final_byte);

static uint8_t soft_i2c_start(uint8_t dev_addr);
static uint8_t soft_i2c_restart(uint8_t dev_addr);
static void soft_i2c_stop(void);
static void soft_delay(volatile uint32_t us);

static uint16_t touch_x, touch_y, touch_z;


static gBool init_board(GMouse* m, unsigned driverinstance) {
    (void)m;

    // Only one touch interface on this board
    if (driverinstance)
		return gFalse;
    
    /* Reset */
    CLR_RST();
    
    soft_delay(30);
    
    SET_RST();
    
    // write 0x02 -> 0x8040 (Ctrl Reg) [Perform soft reset]
    if (soft_i2c_start(GT9147_I2C_WRITE_ADDR) != 0)
    {
        /* Send register address (0x8040) */
        soft_i2c_send_byte(GT9147_COMMAND_REG_HIGH);
        soft_i2c_send_byte(GT9147_COMMAND_REG_LOW);
        soft_i2c_send_byte(0x02);
        
        /* Send a stop */
        soft_i2c_stop();
    }
    
#if 0
    // send configuration data (if not updated)
    /* Try to read Reg_814E */
    if (soft_i2c_start(GT9147_I2C_WRITE_ADDR) != 0)
    {
        /* Send register address (0x814E) */
        soft_i2c_send_byte(GT9147_CONFIG_REG_HIGH);
        soft_i2c_send_byte(GT9147_CONFIG_REG_LOW);
        
        /* Send read address after a restart */
        soft_i2c_restart(GT9147_I2C_READ_ADDR);
        
        /* Read register value */
        helper = soft_i2c_recv_byte(1);
        
        /* Send a stop */
        soft_i2c_stop();
    }
    
    if (helper != GT9147_CONFIG_VERSION)
    {
        // Configuration is different, update data
        if (soft_i2c_start(GT9147_I2C_WRITE_ADDR) != 0)
        {
            /* Send register address (0x8047) */
            soft_i2c_send_byte(GT9147_CONFIG_REG_HIGH);
            soft_i2c_send_byte(GT9147_CONFIG_REG_LOW);
            
            helper = 0x00;
            
            /* Send the whole config table, the CRC and finally the update flag */
            for (index = 0; index < sizeof(GT9147_Config_Table); index++)
            {
                helper += GT9147_Config_Table[index];
                soft_i2c_send_byte(GT9147_Config_Table[index]);
            }
            
            /* Send CRC and update */
            helper = (~helper) + 1;
            
            /* Send CRC value */
            soft_i2c_send_byte(helper);
            
            /* Send Update Flag */
            soft_i2c_send_byte(0x01);
            
            /* Send a stop */
            soft_i2c_stop();
        }
    }
#endif

    return gTrue;
}

static GFXINLINE gBool getpin_pressed(GMouse* m) {

    uint8_t status_reg = 0;
    
    /* Try to read Reg_814E */
    if (soft_i2c_start(GT9147_I2C_WRITE_ADDR) != 0)
    {
        /* Send register address (0x814E) */
        soft_i2c_send_byte(GT9147_STATUS_REG_HIGH);
        soft_i2c_send_byte(GT9147_STATUS_REG_LOW);
        
        /* Send read address after a restart */
        soft_i2c_restart(GT9147_I2C_READ_ADDR);
        
        /* Read register value */
        status_reg = soft_i2c_recv_byte(1);
        
        /* Send a stop */
        soft_i2c_stop();
               
        if ((status_reg > 0x80) && (status_reg < 0x86))
        {
            /* update mouse data */
            return gTrue;
        }
    }
    
	return (gFalse);
}

static GFXINLINE void aquire_bus(GMouse* m) {
	(void)		m;    
}

static GFXINLINE void release_bus(GMouse* m) {
	    
    /* Clear status reg */
    soft_i2c_start(GT9147_I2C_WRITE_ADDR);
    
    /* Send register address (0x814E) */
    soft_i2c_send_byte(GT9147_STATUS_REG_HIGH);
    soft_i2c_send_byte(GT9147_STATUS_REG_LOW);
    soft_i2c_send_byte(0);
    
    /* Send a stop */
    soft_i2c_stop();
}

static GFXINLINE gU16 read_value(GMouse* m, gU16 port) {
	
    uint8_t data_buf[6];
    uint8_t index;
    
    /* 0x8150 .. 0x8155 [x1_low, x1_high, y1_l, y1_h, pt1_size_l, pt1_size_h] */
    if (soft_i2c_start(GT9147_I2C_WRITE_ADDR) != 0)
    {
        /* Send register addr */
        soft_i2c_send_byte(0x81); // High addr
        soft_i2c_send_byte(0x50); // Low addr
        
        /* Send a restart for a read operation */
        soft_i2c_restart(GT9147_I2C_READ_ADDR);
        
        /* and read upto 6 bytes */
        for (index = 0; index < sizeof(data_buf); index++)
        {
            data_buf[index] = soft_i2c_recv_byte((index == (sizeof(data_buf)-1)) ? 1 : 0);
        }
        
        /* Send stop bit */
        soft_i2c_stop();
        
        /* Update data fields of mouse parameter */
        touch_x = (data_buf[0] | (data_buf[1] << 8));
        touch_y = (data_buf[2] | (data_buf[3] << 8));
        touch_z = (data_buf[4] | (data_buf[5] << 8));
    }
	
	return 0;
}


/* Software I2C bit-banging */

/*

    CTOUCH_SCL_Pin PB0
    CTOUCH_SDA_Pin PF11
    CTOUCH_RST_Pin PC13
    CTOUCH_IRQ_Pin PB1

*/

static void soft_delay(volatile uint32_t us)
{
    us *= 50;
    while (us != 0)
    {
        us--;
    }
}

static uint8_t soft_i2c_start(uint8_t dev_addr)
{    
    CLR_SDA(); /* SDA -\_ Start condition */
    
    soft_delay(DELAY_START_CONDITION);
    
    CLR_SCL(); /* SCL -\_ */
    
    return soft_i2c_send_byte(dev_addr);
}

static uint8_t soft_i2c_restart(uint8_t dev_addr)
{    
    SET_SDA();
    SET_SCL();
    
    soft_delay(DELAY_START_CONDITION);
    
    return soft_i2c_start(dev_addr);
}

static void soft_i2c_stop(void)
{   
    CLR_SDA();
    soft_delay(DELAY_STOP_CONDITION);
    
    SET_SCL();
    
    soft_delay(DELAY_STOP_CONDITION);
    
    SET_SDA(); /* _/- SDA Stop condition */
    soft_delay(DELAY_STOP_CONDITION);
}

static uint8_t soft_i2c_send_byte(uint8_t tx)
{
    uint8_t bit;
    
	for (bit = 0x80; bit != 0; bit >>= 1)
    {
        if ((bit & tx) != 0)
            SET_SDA();
        else
            CLR_SDA();
        
        soft_delay(DELAY_DATA_SETUP);
        
        SET_SCL();
        
        soft_delay(DELAY_SCL_HIGH);
        
        CLR_SCL();
  }  
    
  // get Ack or Nak
  SET_SDA();    // Be ready to read from pin
  
  soft_delay(DELAY_DATA_HOLD);
  
  SET_SCL();
  
  soft_delay(DELAY_READ_ACK);
  
  bit = READ_SDA();
  
  CLR_SCL();
  
  soft_delay(DELAY_READ_ACK);  
  //CLR_SDA();
  
  return (bit == 0);
}


static uint8_t soft_i2c_recv_byte(uint8_t final_byte)
{
    uint8_t rx = 0;
    uint8_t bit_index;
    
    SET_SDA();  // be ready to read the pin
    
    for (bit_index = 0; bit_index < 8; bit_index++)
    {
        rx <<= 1;
        soft_delay(DELAY_IO_TURN);
        
        SET_SCL();
        
        soft_delay(DELAY_SCL_HIGH);
        
        if (READ_SDA() != 0)
            rx |= 1;
        
        CLR_SCL();
    }
    
    if (final_byte != 0)
        SET_SDA();  // NAK
    else
        CLR_SDA();  // ACK
    
    soft_delay(DELAY_DATA_SETUP);
    
    SET_SCL();
    
    soft_delay(DELAY_SCL_HIGH);
    
    CLR_SCL();
    
    CLR_SDA();
    
    return rx;
}

#endif /* _GINPUT_LLD_MOUSE_BOARD_H */
