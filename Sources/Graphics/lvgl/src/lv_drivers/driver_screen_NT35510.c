#include <stdint.h>
#include <stm32f4xx_hal.h>
#include "driver_screen_NT35510.h"
#include "dma.h"
#include "hw_timers.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"

static void NT35510_Initialize(void);
static void NT35510_SetBacklight(uint8_t percent);
static void NT35510_SetViewPort(uint16_t x, uint16_t y, uint16_t cx, uint16_t cy);
static void NT35510_WriteData_Increment(uint16_t * data, uint32_t nr_bytes);
static void NT35510_WriteData_No_Increment(uint16_t data, uint32_t nr_bytes);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

static const uint16_t nt35510_config_table[] =
{
    0xF000, 0x55, 0xF001, 0xAA, 0xF002, 0x52, 0xF003, 0x08, 0xF004, 0x01,

    //AVDD Set AVDD 5.2V
    0xB000, 0x0D, 0xB001, 0x0D, 0xB002, 0x0D,

    //AVDD ratio
    0xB600, 0x34, 0xB601, 0x34, 0xB602, 0x34,

    //AVEE -5.2V
    0xB100, 0x0D, 0xB101, 0x0D, 0xB102, 0x0D,

    //AVEE ratio
    0xB700, 0x34, 0xB701, 0x34, 0xB702, 0x34,

    //VCL -2.5V
    0xB200, 0x00, 0xB201, 0x00, 0xB202, 0x00,

    //VCL ratio
    0xB800, 0x24, 0xB801, 0x24, 0xB802, 0x24,

    //VGH 15V (Free pump)
    0xBF00, 0x01, 0xB300, 0x0F, 0xB301, 0x0F, 0xB302, 0x0F,

    //VGH ratio
    0xB900, 0x34, 0xB901, 0x34, 0xB902, 0x34,

    //VGL_REG -10V
    0xB500, 0x08, 0xB501, 0x08, 0xB502, 0x08, 0xC200, 0x03,

    //VGLX ratio
    0xBA00, 0x24, 0xBA01, 0x24, 0xBA02, 0x24,

    //VGMP/VGSP 4.5V/0V
    0xBC00, 0x00, 0xBC01, 0x78, 0xBC02, 0x00,

    //VGMN/VGSN -4.5V/0V
    0xBD00, 0x00, 0xBD01, 0x78, 0xBD02, 0x00,

    //VCOM
    0xBE00, 0x00, 0xBE01, 0x64,

    //Gamma Setting
    0xD100, 0x00, 0xD101, 0x33, 0xD102, 0x00, 0xD103, 0x34,
    0xD104, 0x00, 0xD105, 0x3A, 0xD106, 0x00, 0xD107, 0x4A,
    0xD108, 0x00, 0xD109, 0x5C, 0xD10A, 0x00, 0xD10B, 0x81,
    0xD10C, 0x00, 0xD10D, 0xA6, 0xD10E, 0x00, 0xD10F, 0xE5,
    0xD110, 0x01, 0xD111, 0x13, 0xD112, 0x01, 0xD113, 0x54,
    0xD114, 0x01, 0xD115, 0x82, 0xD116, 0x01, 0xD117, 0xCA,
    0xD118, 0x02, 0xD119, 0x00, 0xD11A, 0x02, 0xD11B, 0x01,
    0xD11C, 0x02, 0xD11D, 0x34, 0xD11E, 0x02, 0xD11F, 0x67,
    0xD120, 0x02, 0xD121, 0x84, 0xD122, 0x02, 0xD123, 0xA4,
    0xD124, 0x02, 0xD125, 0xB7, 0xD126, 0x02, 0xD127, 0xCF,
    0xD128, 0x02, 0xD129, 0xDE, 0xD12A, 0x02, 0xD12B, 0xF2,
    0xD12C, 0x02, 0xD12D, 0xFE, 0xD12E, 0x03, 0xD12F, 0x10,
    0xD130, 0x03, 0xD131, 0x33, 0xD132, 0x03, 0xD133, 0x6D,
    0xD200, 0x00, 0xD201, 0x33, 0xD202, 0x00, 0xD203, 0x34,
    0xD204, 0x00, 0xD205, 0x3A, 0xD206, 0x00, 0xD207, 0x4A,
    0xD208, 0x00, 0xD209, 0x5C, 0xD20A, 0x00, 
    
    0xD20B, 0x81, 0xD20C, 0x00, 0xD20D, 0xA6, 0xD20E, 0x00, 
    0xD20F, 0xE5, 0xD210, 0x01, 0xD211, 0x13, 0xD212, 0x01, 
    0xD213, 0x54, 0xD214, 0x01, 0xD215, 0x82, 0xD216, 0x01, 
    0xD217, 0xCA, 0xD218, 0x02, 0xD219, 0x00, 0xD21A, 0x02, 
    0xD21B, 0x01, 0xD21C, 0x02, 0xD21D, 0x34, 0xD21E, 0x02, 
    0xD21F, 0x67, 0xD220, 0x02, 0xD221, 0x84, 0xD222, 0x02, 
    0xD223, 0xA4, 0xD224, 0x02, 0xD225, 0xB7, 0xD226, 0x02, 
    0xD227, 0xCF, 0xD228, 0x02, 0xD229, 0xDE, 0xD22A, 0x02, 
    0xD22B, 0xF2, 0xD22C, 0x02, 0xD22D, 0xFE, 0xD22E, 0x03, 
    0xD22F, 0x10, 0xD230, 0x03, 0xD231, 0x33, 0xD232, 0x03, 
    0xD233, 0x6D, 0xD300, 0x00, 0xD301, 0x33, 0xD302, 0x00, 
    0xD303, 0x34, 0xD304, 0x00, 0xD305, 0x3A, 0xD306, 0x00, 
    0xD307, 0x4A, 0xD308, 0x00, 0xD309, 0x5C, 0xD30A, 0x00,

    0xD30B, 0x81, 0xD30C, 0x00, 0xD30D, 0xA6, 0xD30E, 0x00,
    0xD30F, 0xE5, 0xD310, 0x01, 0xD311, 0x13, 0xD312, 0x01,
    0xD313, 0x54, 0xD314, 0x01, 0xD315, 0x82, 0xD316, 0x01,
    0xD317, 0xCA, 0xD318, 0x02, 0xD319, 0x00, 0xD31A, 0x02,
    0xD31B, 0x01, 0xD31C, 0x02, 0xD31D, 0x34, 0xD31E, 0x02,
    0xD31F, 0x67, 0xD320, 0x02, 0xD321, 0x84, 0xD322, 0x02,
    0xD323, 0xA4, 0xD324, 0x02, 0xD325, 0xB7, 0xD326, 0x02,
    0xD327, 0xCF, 0xD328, 0x02, 0xD329, 0xDE, 0xD32A, 0x02,
    0xD32B, 0xF2, 0xD32C, 0x02, 0xD32D, 0xFE, 0xD32E, 0x03,
    0xD32F, 0x10, 0xD330, 0x03, 0xD331, 0x33, 0xD332, 0x03,
    0xD333, 0x6D, 0xD400, 0x00, 0xD401, 0x33, 0xD402, 0x00,
    0xD403, 0x34, 0xD404, 0x00, 0xD405, 0x3A, 0xD406, 0x00,
    0xD407, 0x4A, 0xD408, 0x00, 0xD409, 0x5C, 0xD40A, 0x00,
    0xD40B, 0x81,

    0xD40C, 0x00, 0xD40D, 0xA6, 0xD40E, 0x00, 0xD40F, 0xE5,
    0xD410, 0x01, 0xD411, 0x13, 0xD412, 0x01, 0xD413, 0x54,
    0xD414, 0x01, 0xD415, 0x82, 0xD416, 0x01, 0xD417, 0xCA,
    0xD418, 0x02, 0xD419, 0x00, 0xD41A, 0x02, 0xD41B, 0x01,
    0xD41C, 0x02, 0xD41D, 0x34, 0xD41E, 0x02, 0xD41F, 0x67,
    0xD420, 0x02, 0xD421, 0x84, 0xD422, 0x02, 0xD423, 0xA4,
    0xD424, 0x02, 0xD425, 0xB7, 0xD426, 0x02, 0xD427, 0xCF,
    0xD428, 0x02, 0xD429, 0xDE, 0xD42A, 0x02, 0xD42B, 0xF2,
    0xD42C, 0x02, 0xD42D, 0xFE, 0xD42E, 0x03, 0xD42F, 0x10,
    0xD430, 0x03, 0xD431, 0x33, 0xD432, 0x03, 0xD433, 0x6D,
    0xD500, 0x00, 0xD501, 0x33, 0xD502, 0x00, 0xD503, 0x34,
    0xD504, 0x00, 0xD505, 0x3A, 0xD506, 0x00, 0xD507, 0x4A,
    0xD508, 0x00, 0xD509, 0x5C, 0xD50A, 0x00, 0xD50B, 0x81,

    0xD50C, 0x00, 0xD50D, 0xA6, 0xD50E, 0x00, 0xD50F, 0xE5,
    0xD510, 0x01, 0xD511, 0x13, 0xD512, 0x01, 0xD513, 0x54,
    0xD514, 0x01, 0xD515, 0x82, 0xD516, 0x01, 0xD517, 0xCA,
    0xD518, 0x02, 0xD519, 0x00, 0xD51A, 0x02, 0xD51B, 0x01,
    0xD51C, 0x02, 0xD51D, 0x34, 0xD51E, 0x02, 0xD51F, 0x67,
    0xD520, 0x02, 0xD521, 0x84, 0xD522, 0x02, 0xD523, 0xA4,
    0xD524, 0x02, 0xD525, 0xB7, 0xD526, 0x02, 0xD527, 0xCF,
    0xD528, 0x02, 0xD529, 0xDE, 0xD52A, 0x02, 0xD52B, 0xF2,
    0xD52C, 0x02, 0xD52D, 0xFE, 0xD52E, 0x03, 0xD52F, 0x10,
    0xD530, 0x03, 0xD531, 0x33, 0xD532, 0x03, 0xD533, 0x6D,
    0xD600, 0x00, 0xD601, 0x33, 0xD602, 0x00, 0xD603, 0x34,
    0xD604, 0x00, 0xD605, 0x3A, 0xD606, 0x00, 0xD607, 0x4A,
    0xD608, 0x00, 0xD609, 0x5C, 0xD60A, 0x00, 0xD60B, 0x81,

    0xD60C, 0x00, 0xD60D, 0xA6, 0xD60E, 0x00, 0xD60F, 0xE5,
    0xD610, 0x01, 0xD611, 0x13, 0xD612, 0x01, 0xD613, 0x54,
    0xD614, 0x01, 0xD615, 0x82, 0xD616, 0x01, 0xD617, 0xCA,
    0xD618, 0x02, 0xD619, 0x00, 0xD61A, 0x02, 0xD61B, 0x01,
    0xD61C, 0x02, 0xD61D, 0x34, 0xD61E, 0x02, 0xD61F, 0x67,
    0xD620, 0x02, 0xD621, 0x84, 0xD622, 0x02, 0xD623, 0xA4,
    0xD624, 0x02, 0xD625, 0xB7, 0xD626, 0x02, 0xD627, 0xCF,
    0xD628, 0x02, 0xD629, 0xDE, 0xD62A, 0x02, 0xD62B, 0xF2,
    0xD62C, 0x02, 0xD62D, 0xFE, 0xD62E, 0x03, 0xD62F, 0x10,
    0xD630, 0x03, 0xD631, 0x33, 0xD632, 0x03, 0xD633, 0x6D,

    //LV2 Page 0 enable
    0xF000, 0x55, 0xF001, 0xAA, 0xF002, 0x52, 0xF003, 0x08, 0xF004, 0x00,

    //Display control
    0xB100,  0xCC, 0xB101,  0x00,

    //Source hold time
    0xB600, 0x05,

    //Gate EQ control
    0xB700, 0x70, 0xB701, 0x70,

    //Source EQ control (Mode 2)
    0xB800, 0x01, 0xB801, 0x03, 0xB802, 0x03, 0xB803, 0x03,

    //Inversion mode (2-dot)
    0xBC00, 0x02, 0xBC01, 0x00, 0xBC02, 0x00,

    //Timing control 4H w/ 4-delay
    0xC900, 0xD0, 0xC901, 0x02, 0xC902, 0x50, 0xC903, 0x50, 0xC904, 0x50,

    LCD_TEAR_EFFECT_ON_CMD,  0x00,
    LCD_INTF_PIXEL_FMT_CMD,  0x55,   //16-bit/pixel
};

void NT35510_SetBacklight(uint8_t percent)
{
    if (percent == 0)
        HAL_TIM_PWM_Stop(&tft_backlight_timer_handle, TIM_CHANNEL_2);
    else
        HAL_TIM_PWM_Start(&tft_backlight_timer_handle, TIM_CHANNEL_2);

	__HAL_TIM_SetCompare(&tft_backlight_timer_handle, TIM_CHANNEL_2, percent);
}

static void NT35510_Initialize(void)
{
    uint32_t index;
    const uint32_t CONFIG_TABLE_VALUE_COUNT = sizeof(nt35510_config_table) / sizeof(nt35510_config_table[0]);

    /* Hardware reset */
	vTaskDelay(120);

    /* Send initialization config table */
    for (index = 0; index < CONFIG_TABLE_VALUE_COUNT-1; index += 2)
    {
        LCD->CMD = nt35510_config_table[index + 0];
        LCD->RAM = nt35510_config_table[index + 1];
        __nop();
    }

    /* Exit from Sleep Mode */
    LCD->CMD = LCD_SLEEP_OUT_CMD;

    /* Give some time */
    vTaskDelay(1);

    /* Turn display ON */
    LCD->CMD = LCD_DISPLAY_ON_CMD;

    /* Set default orientation [horizontal screen] */
	LCD->CMD = LCD_ENTRY_MODE_CMD;
    LCD->RAM = LCD_ENTRY_LANDSCAPE;
    
//    NT35510_SetViewPort(0, 0, 800, 480);
//    NT35510_WriteData_No_Increment(init_color, 800*480);
    
    /* Set initial backlight */
    NT35510_SetBacklight(100);
}

void NT35510_WriteData_Increment(uint16_t * data, uint32_t nr_bytes)
{
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

void NT35510_WriteData_No_Increment(uint16_t data, uint32_t nr_bytes)
{
	// if memory increment enabled?
    if ((hdma_memtomem_dma2_stream0.Instance->CR & DMA_PINC_ENABLE) != DMA_PINC_DISABLE)
    {
        // First, disable stream to be able to modify settings
        __HAL_DMA_DISABLE(&hdma_memtomem_dma2_stream0);

        // Disable source memory addr incr (periph inc)
        hdma_memtomem_dma2_stream0.Instance->CR &= ~DMA_PINC_ENABLE;
    }

    // send data in chuncks of 64KB max
	while (nr_bytes > UINT16_MAX)
    {
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

void NT35510_SetViewPort(uint16_t x, uint16_t y, uint16_t cx, uint16_t cy) 
{        
    /* X */
    LCD->CMD = LCD_SET_X_CMD_SC_MSB; 
    LCD->RAM = ((x >> 8) & 0x00FF);
    
    LCD->CMD = LCD_SET_X_CMD_SC_LSB;
    LCD->RAM = ((x >> 0) & 0x00FF);

    /* CX */
    LCD->CMD = LCD_SET_X_CMD_EC_MSB;
    LCD->RAM = (((x + cx - 1) >> 8) & 0x00FF);
    
    LCD->CMD = LCD_SET_X_CMD_EC_LSB;
    LCD->RAM = (((x + cx - 1) >> 0) & 0x00FF);

    /* Y */
    LCD->CMD = LCD_SET_Y_CMD_SP_MSB;
    LCD->RAM = ((y >> 8) & 0x00FF);
    
    LCD->CMD = LCD_SET_Y_CMD_SP_LSB;
    LCD->RAM = ((y >> 0) & 0x00FF);

    /* CY */
    LCD->CMD = LCD_SET_Y_CMD_EP_MSB;
    LCD->RAM = (((y + cy - 1) >> 8) & 0x00FF);
    
    LCD->CMD = LCD_SET_Y_CMD_EP_LSB;
    LCD->RAM = (((y + cy - 1) >> 0) & 0x00FF);
    
    /* Write RAM */
    LCD->CMD = (LCD_WR_RAM_CMD);
}

////////////////////////////////////////////////////////////////////////////////////

void lv_port_disp_NT35510_init(void)
{
    static lv_disp_buf_t * display_buffer_desc = NULL;
    static lv_color_t * display_buffer_memory = NULL;
    
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    NT35510_Initialize();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /* LittlevGL requires a buffer where it draws the objects. The buffer's has to be greater than 1 display row
     *
     * There are three buffering configurations:
     * 1. Create ONE buffer with some rows: 
     *      LittlevGL will draw the display's content here and writes it to your display
     */
    // Allocate buffer memory
    display_buffer_desc = (lv_disp_buf_t*)pvPortMalloc(sizeof(lv_disp_buf_t));    
    display_buffer_memory = (lv_color_t*)pvPortMalloc(sizeof(lv_color_t) * LV_HOR_RES_MAX * 15);

    // Initialize buffer descriptor
    lv_disp_buf_init(display_buffer_desc, display_buffer_memory, NULL, LV_HOR_RES_MAX * 15);   /*Initialize the display buffer*/

    
    /*-----------------------------------
     * Register the display in LittlevGL
     *----------------------------------*/

    lv_disp_drv_t display_driver_instance;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&display_driver_instance);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    display_driver_instance.hor_res = 800;
    display_driver_instance.ver_res = 480;

    /*Used to copy the buffer's content to the display*/
    display_driver_instance.flush_cb = disp_flush;

    /*Set a display buffer*/
    display_driver_instance.buffer = display_buffer_desc;

#if LV_USE_GPU
    /*Optionally add functions to access the GPU. (Only in buffered mode, LV_VDB_SIZE != 0)*/

    /*Blend two color array using opacity*/
    display_driver_instance.gpu_blend_cb = gpu_blend;

    /*Fill a memory array with a color*/
    display_driver_instance.gpu_fill_cb = gpu_fill;
#endif

    /*Finally register the driver*/
    lv_disp_drv_register(&display_driver_instance);
}

void lv_port_disp_NT35510_set_backlight(uint8_t percent)
{
    NT35510_SetBacklight(percent);
}

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    uint32_t byte_cnt = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
    
    NT35510_SetViewPort(area->x1, area->y1, (area->x2 - area->x1)+1, (area->y2 - area->y1)+1);
    NT35510_WriteData_Increment((uint16_t*)color_p, byte_cnt);
    

    /* IMPORTANT!!!
     * Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);
}

