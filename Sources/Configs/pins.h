#ifndef PINS_H
#define PINS_H

#define PWM_1_Pin GPIO_PIN_5
#define PWM_1_GPIO_Port GPIOE               //  PE5

#define PWM_2_Pin GPIO_PIN_6
#define PWM_2_GPIO_Port GPIOE               //  PE6

#define CTOUCH_RESET_Pin GPIO_PIN_13
#define CTOUCH_RESET_GPIO_Port GPIOC        //  PC13

#define BTN_START_Pin GPIO_PIN_6
#define BTN_START_GPIO_Port GPIOF           //  PF6

#define BTN_HOLD_Pin GPIO_PIN_7
#define BTN_HOLD_GPIO_Port GPIOF            //  PF7

#define BTN_ABORT_Pin GPIO_PIN_8
#define BTN_ABORT_GPIO_Port GPIOF           //  PF8

#define LED_0_Pin GPIO_PIN_9
#define LED_0_GPIO_Port GPIOF               //  PF9     [Onboard LED 0 { Active Low }]

#define LED_1_Pin GPIO_PIN_10
#define LED_1_GPIO_Port GPIOF               //  PF10    [Onboard LED 1 { Active Low }]

#define STEP_PINS_GPIO_PORT     GPIOC

#define STEP_Z_Pin GPIO_PIN_0
#define STEP_Z_GPIO_Port GPIOC              //  PC0

#define DIR_Z_Pin GPIO_PIN_1
#define DIR_Z_GPIO_Port GPIOC               //  PC1

#define STEP_Y_Pin GPIO_PIN_2
#define STEP_Y_GPIO_Port GPIOC              //  PC2

#define DIR_Y_Pin GPIO_PIN_3
#define DIR_Y_GPIO_Port GPIOC               //  PC3

#define STEP_X_Pin GPIO_PIN_4
#define STEP_X_GPIO_Port GPIOC              //  PC4

#define DIR_X_Pin GPIO_PIN_5
#define DIR_X_GPIO_Port GPIOC               //  PC5

#define CTOUCH_SCL_Pin GPIO_PIN_0
#define CTOUCH_SCL_GPIO_Port GPIOB          //  PB0     [Pullup on LCD TFT Touch controller I2C side]

#define CTOUCH_IRQ_Pin GPIO_PIN_1
#define CTOUCH_IRQ_GPIO_Port GPIOB          //  PB1

#define CTOUCH_SDA_Pin GPIO_PIN_11
#define CTOUCH_SDA_GPIO_Port GPIOF          //  PF11    [Pullup on LCD TFT Touch controller I2C side]

#define SPIN_TX_O_Pin GPIO_PIN_10
#define SPIN_TX_O_GPIO_Port GPIOB           //  PB10

#define SPIN_RX_I_Pin GPIO_PIN_11
#define SPIN_RX_I_GPIO_Port GPIOB           //  PB11

#define SPIN_AUX_Pin GPIO_PIN_12
#define SPIN_AUX_GPIO_Port GPIOB            //  PB12

#define SPIN_RESET_Pin GPIO_PIN_13
#define SPIN_RESET_GPIO_Port GPIOB          //  PB13

#define FLASH_CS_Pin GPIO_PIN_14
#define FLASH_CS_GPIO_Port GPIOB            //  PB14

#define TFT_BACKLIGHT_Pin GPIO_PIN_15
#define TFT_BACKLIGHT_GPIO_Port GPIOB       //  PB15

#define GLOBAL_FAULT_Pin GPIO_PIN_6
#define GLOBAL_FAULT_GPIO_Port GPIOG        //  PG6

#define STEP_RESET_Pin GPIO_PIN_7
#define STEP_RESET_GPIO_Port GPIOG          //  PG7

#define PROBE_INPUT_Pin GPIO_PIN_8
#define PROBE_INPUT_GPIO_Port GPIOG         //  PG8

#define LIM_X_Pin GPIO_PIN_6
#define LIM_X_GPIO_Port GPIOC               //  PC6

#define LIM_Y_Pin GPIO_PIN_7
#define LIM_Y_GPIO_Port GPIOC               //  PC7

#define LIM_Z_Pin GPIO_PIN_8
#define LIM_Z_GPIO_Port GPIOC               //  PC8     [Onboard pullup PR3]

#define LIMITS_GPIO_Port    GPIOC           // Limit pins are located at PC6, PC7, PC8
#define LIMITS_Mask (LIM_X_Pin | LIM_Y_Pin | LIM_Z_Pin)
#define LIMITS_Offset   6

#define STEP_ENABLE_Pin GPIO_PIN_9
#define STEP_ENABLE_GPIO_Port GPIOC         //  PC9     [Onboard pullup PR3]

#define DEBUG_TX_O_Pin GPIO_PIN_9
#define DEBUG_TX_O_GPIO_Port GPIOA          //  PA9

#define DEBUG_RX_I_Pin GPIO_PIN_10
#define DEBUG_RX_I_GPIO_Port GPIOA          //  PA10

#define USB_D_MINUS_Pin GPIO_PIN_11
#define USB_D_MINUS_GPIO_Port GPIOA         //  PA11

#define USB_D_PLUS_Pin GPIO_PIN_12
#define USB_D_PLUS_GPIO_Port GPIOA          //  PA12    [Onboard pullup R8 {removed}]

#define SDCARD_CS_Pin GPIO_PIN_15
#define SDCARD_CS_GPIO_Port GPIOA           //  PA15    [Onboard pullup PR1]

#define SDCARD_SCK_Pin GPIO_PIN_10
#define SDCARD_SCK_GPIO_Port GPIOC          //  PC10    [Onboard pullup PR2]

#define SDCARD_MISO_Pin GPIO_PIN_11
#define SDCARD_MISO_GPIO_Port GPIOC         //  PC11    [Onboard pullup PR2]

#define SDCARD_MOSI_Pin GPIO_PIN_12
#define SDCARD_MOSI_GPIO_Port GPIOC         //  PC12    [Onboard pullup PR2]

#define COOLANT_ENABLE_Pin GPIO_PIN_6
#define COOLANT_ENABLE_GPIO_Port GPIOB      //  PB6

#define PWM_3_Pin GPIO_PIN_7
#define PWM_3_GPIO_Port GPIOB               //  PB7

#define KEY_1_Pin GPIO_PIN_8
#define KEY_1_GPIO_Port GPIOB               //  PB8     [Onboard Key 1 button to ground]

#define KEY_0_Pin GPIO_PIN_9
#define KEY_0_GPIO_Port GPIOB               //  PB9     [Onboard Key 0 button to ground]

#define USB_CONN_DETECT_Pin     GPIO_PIN_3
#define USB_CONN_DETECT_GPIO_Port   GPIOD   //  PD3

#endif

