/**
 * @file lv_port_indev_GT9147.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include <stm32f4xx_hal.h>

#include "FreeRTOS.h"
#include "task.h"

#include "driver_touch_GT9147.h"
#include "stm32f4xx_hal_gpio.h"
#include "pins.h"


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void touchpad_init(void);
static bool touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static bool touchpad_is_pressed(void);
static void touchpad_get_xy(lv_coord_t * x, lv_coord_t * y);

static void clear_status_reg_GT9147(void);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_GT9147_init(void)
{
    lv_indev_drv_t input_device_driver;

    /*------------------
     * Touchpad
     * -----------------*/

    /*Initialize your touchpad if you have*/
    touchpad_init();

    /*Register a touchpad input device*/
    lv_indev_drv_init(&input_device_driver);
    input_device_driver.type = LV_INDEV_TYPE_POINTER;
    input_device_driver.read_cb = touchpad_read;

    lv_indev_t * mydev = lv_indev_drv_register(&input_device_driver);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

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


static uint8_t soft_i2c_send_byte(uint8_t tx);
static uint8_t soft_i2c_recv_byte(uint8_t final_byte);

static uint8_t soft_i2c_start(uint8_t dev_addr);
static uint8_t soft_i2c_restart(uint8_t dev_addr);
static void soft_i2c_stop(void);
static void soft_delay(volatile uint32_t us);

/*------------------
 * Touchpad
 * -----------------*/

/*Initialize your touchpad*/
static void touchpad_init(void)
{
    /* Reset */
    CLR_RST();

    vTaskDelay(30);

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
}

/* Will be called by the library to read the touchpad */
static bool touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;

    /*Save the pressed coordinates and the state*/
    if(touchpad_is_pressed())
    {
        touchpad_get_xy(&last_y, &last_x);
        data->state = LV_INDEV_STATE_PR;
        
        last_y = 480 - last_y;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
    
    // Always clear status register
    clear_status_reg_GT9147();

    /* Set the last pressed coordinates */
    data->point.x = last_x;
    data->point.y = last_y;

    /*Return `false` because we are not buffering and no more data to read*/
    return false;
}

/*Return true is the touchpad is pressed*/
static bool touchpad_is_pressed(void)
{
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
            return true;
        }
    }

    return false;
}

/*Get the x and y coordinates if the touchpad is pressed*/
static void touchpad_get_xy(lv_coord_t * px, lv_coord_t * py)
{
    uint8_t data_buf[4];
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
            data_buf[index] = soft_i2c_recv_byte((index == (sizeof(data_buf)-1)) ? 1 : 0);

        /* Send stop bit */
        soft_i2c_stop();

        /* Update data fields of mouse parameter */
        *px = (data_buf[0] | (data_buf[1] << 8));
        *py = (data_buf[2] | (data_buf[3] << 8));
    }
}

static void clear_status_reg_GT9147(void)
{
    /* Clear status reg */
    soft_i2c_start(GT9147_I2C_WRITE_ADDR);

    /* Send register address (0x814E) */
    soft_i2c_send_byte(GT9147_STATUS_REG_HIGH);
    soft_i2c_send_byte(GT9147_STATUS_REG_LOW);
    soft_i2c_send_byte(0);

    /* Send a stop */
    soft_i2c_stop();
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
