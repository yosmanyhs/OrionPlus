/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.io/license.html
 */

#include "gfx.h"

#if GFX_USE_GDISP

#if defined(GDISP_SCREEN_HEIGHT) || defined(GDISP_SCREEN_HEIGHT)
	#if GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_DIRECT
		#warning "GDISP: This low level driver does not support setting a screen size. It is being ignored."
	#elif GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_MACRO
		COMPILER_WARNING("GDISP: This low level driver does not support setting a screen size. It is being ignored.")
	#endif
	#undef GDISP_SCREEN_WIDTH
	#undef GDISP_SCREEN_HEIGHT
#endif

#define GDISP_DRIVER_VMT			GDISPVMT_NT35510
#include "gdisp_lld_config.h"
#include "gdisp_driver.h"

#include "board_NT35510.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#ifndef GDISP_SCREEN_HEIGHT
	#define GDISP_SCREEN_HEIGHT		800
#endif
#ifndef GDISP_SCREEN_WIDTH
	#define GDISP_SCREEN_WIDTH		480
#endif
#ifndef GDISP_INITIAL_CONTRAST
	#define GDISP_INITIAL_CONTRAST	50
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
	#define GDISP_INITIAL_BACKLIGHT	50
#endif

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/


static void set_viewport(GDisplay* g) {    
    
    /* X */
    write_reg_val(LCD_SET_X_CMD_SC_MSB, ((g->p.x >> 8) & 0x00FF));
    write_reg_val(LCD_SET_X_CMD_SC_LSB, ((g->p.x >> 0) & 0x00FF));

    /* CX */
    write_reg_val(LCD_SET_X_CMD_EC_MSB, (((g->p.x + g->p.cx - 1) >> 8) & 0x00FF));
    write_reg_val(LCD_SET_X_CMD_EC_LSB, (((g->p.x + g->p.cx - 1) >> 0) & 0x00FF));

    /* Y */
    write_reg_val(LCD_SET_Y_CMD_SP_MSB, ((g->p.y >> 8) & 0x00FF));
    write_reg_val(LCD_SET_Y_CMD_SP_LSB, ((g->p.y >> 0) & 0x00FF));

    /* CY */
    write_reg_val(LCD_SET_Y_CMD_EP_MSB, (((g->p.y + g->p.cy - 1) >> 8) & 0x00FF));
    write_reg_val(LCD_SET_Y_CMD_EP_LSB, (((g->p.y + g->p.cy - 1) >> 0) & 0x00FF));
    
    /* Write RAM */
    write_index(LCD_WR_RAM_CMD);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

LLDSPEC gBool gdisp_lld_init(GDisplay *g) {
	
	// No private area for this controller
	g->priv = 0;

	// Initialise the board interface
	//init_board(g);

	/* Hardware reset */
	gfxSleepMilliseconds(120);

	/* Get the bus for the following initialisation commands */
	//acquire_bus(g);

	/* Init sequence start */
	/*write_index(LCD_READ_ID_1);
	
	id = read_data();
	
	write_index(LCD_READ_ID_2);
	
	id = read_data();
	
	id <<= 8;
	
	write_index(LCD_READ_ID_3);
	
	id |= read_data();
	
	if (id == 0x8000)
		id = 0x5510;
	
	if (id == 0x5510)
	{*/
		write_reg_val(0xF000,0x55);
		write_reg_val(0xF001,0xAA);
		write_reg_val(0xF002,0x52);
		write_reg_val(0xF003,0x08);
		write_reg_val(0xF004,0x01);
        
		//AVDD Set AVDD 5.2V
		write_reg_val(0xB000,0x0D);
		write_reg_val(0xB001,0x0D);
		write_reg_val(0xB002,0x0D);
		
        //AVDD ratio
		write_reg_val(0xB600,0x34);
		write_reg_val(0xB601,0x34);
		write_reg_val(0xB602,0x34);
		
        //AVEE -5.2V
		write_reg_val(0xB100,0x0D);
		write_reg_val(0xB101,0x0D);
		write_reg_val(0xB102,0x0D);
		
        //AVEE ratio
		write_reg_val(0xB700,0x34);
		write_reg_val(0xB701,0x34);
		write_reg_val(0xB702,0x34);
		
        //VCL -2.5V
		write_reg_val(0xB200,0x00);
		write_reg_val(0xB201,0x00);
		write_reg_val(0xB202,0x00);
		
        //VCL ratio
		write_reg_val(0xB800,0x24);
		write_reg_val(0xB801,0x24);
		write_reg_val(0xB802,0x24);
		
        //VGH 15V (Free pump)
		write_reg_val(0xBF00,0x01);
		write_reg_val(0xB300,0x0F);
		write_reg_val(0xB301,0x0F);
		write_reg_val(0xB302,0x0F);
		
        //VGH ratio
		write_reg_val(0xB900,0x34);
		write_reg_val(0xB901,0x34);
		write_reg_val(0xB902,0x34);
		
        //VGL_REG -10V
		write_reg_val(0xB500,0x08);
		write_reg_val(0xB501,0x08);
		write_reg_val(0xB502,0x08);
		write_reg_val(0xC200,0x03);
		
        //VGLX ratio
		write_reg_val(0xBA00,0x24);
		write_reg_val(0xBA01,0x24);
		write_reg_val(0xBA02,0x24);
		
        //VGMP/VGSP 4.5V/0V
		write_reg_val(0xBC00,0x00);
		write_reg_val(0xBC01,0x78);
		write_reg_val(0xBC02,0x00);
		
        //VGMN/VGSN -4.5V/0V
		write_reg_val(0xBD00,0x00);
		write_reg_val(0xBD01,0x78);
		write_reg_val(0xBD02,0x00);
		
        //VCOM
		write_reg_val(0xBE00,0x00);
		write_reg_val(0xBE01,0x64);
		
        //Gamma Setting
		write_reg_val(0xD100,0x00);
		write_reg_val(0xD101,0x33);
		write_reg_val(0xD102,0x00);
		write_reg_val(0xD103,0x34);
		write_reg_val(0xD104,0x00);
		write_reg_val(0xD105,0x3A);
		write_reg_val(0xD106,0x00);
		write_reg_val(0xD107,0x4A);
		write_reg_val(0xD108,0x00);
		write_reg_val(0xD109,0x5C);
		write_reg_val(0xD10A,0x00);
		write_reg_val(0xD10B,0x81);
		write_reg_val(0xD10C,0x00);
		write_reg_val(0xD10D,0xA6);
		write_reg_val(0xD10E,0x00);
		write_reg_val(0xD10F,0xE5);
		write_reg_val(0xD110,0x01);
		write_reg_val(0xD111,0x13);
		write_reg_val(0xD112,0x01);
		write_reg_val(0xD113,0x54);
		write_reg_val(0xD114,0x01);
		write_reg_val(0xD115,0x82);
		write_reg_val(0xD116,0x01);
		write_reg_val(0xD117,0xCA);
		write_reg_val(0xD118,0x02);
		write_reg_val(0xD119,0x00);
		write_reg_val(0xD11A,0x02);
		write_reg_val(0xD11B,0x01);
		write_reg_val(0xD11C,0x02);
		write_reg_val(0xD11D,0x34);
		write_reg_val(0xD11E,0x02);
		write_reg_val(0xD11F,0x67);
		write_reg_val(0xD120,0x02);
		write_reg_val(0xD121,0x84);
		write_reg_val(0xD122,0x02);
		write_reg_val(0xD123,0xA4);
		write_reg_val(0xD124,0x02);
		write_reg_val(0xD125,0xB7);
		write_reg_val(0xD126,0x02);
		write_reg_val(0xD127,0xCF);
		write_reg_val(0xD128,0x02);
		write_reg_val(0xD129,0xDE);
		write_reg_val(0xD12A,0x02);
		write_reg_val(0xD12B,0xF2);
		write_reg_val(0xD12C,0x02);
		write_reg_val(0xD12D,0xFE);
		write_reg_val(0xD12E,0x03);
		write_reg_val(0xD12F,0x10);
		write_reg_val(0xD130,0x03);
		write_reg_val(0xD131,0x33);
		write_reg_val(0xD132,0x03);
		write_reg_val(0xD133,0x6D);
		write_reg_val(0xD200,0x00);
		write_reg_val(0xD201,0x33);
		write_reg_val(0xD202,0x00);
		write_reg_val(0xD203,0x34);
		write_reg_val(0xD204,0x00);
		write_reg_val(0xD205,0x3A);
		write_reg_val(0xD206,0x00);
		write_reg_val(0xD207,0x4A);
		write_reg_val(0xD208,0x00);
		write_reg_val(0xD209,0x5C);
		write_reg_val(0xD20A,0x00);

		write_reg_val(0xD20B,0x81);
		write_reg_val(0xD20C,0x00);
		write_reg_val(0xD20D,0xA6);
		write_reg_val(0xD20E,0x00);
		write_reg_val(0xD20F,0xE5);
		write_reg_val(0xD210,0x01);
		write_reg_val(0xD211,0x13);
		write_reg_val(0xD212,0x01);
		write_reg_val(0xD213,0x54);
		write_reg_val(0xD214,0x01);
		write_reg_val(0xD215,0x82);
		write_reg_val(0xD216,0x01);
		write_reg_val(0xD217,0xCA);
		write_reg_val(0xD218,0x02);
		write_reg_val(0xD219,0x00);
		write_reg_val(0xD21A,0x02);
		write_reg_val(0xD21B,0x01);
		write_reg_val(0xD21C,0x02);
		write_reg_val(0xD21D,0x34);
		write_reg_val(0xD21E,0x02);
		write_reg_val(0xD21F,0x67);
		write_reg_val(0xD220,0x02);
		write_reg_val(0xD221,0x84);
		write_reg_val(0xD222,0x02);
		write_reg_val(0xD223,0xA4);
		write_reg_val(0xD224,0x02);
		write_reg_val(0xD225,0xB7);
		write_reg_val(0xD226,0x02);
		write_reg_val(0xD227,0xCF);
		write_reg_val(0xD228,0x02);
		write_reg_val(0xD229,0xDE);
		write_reg_val(0xD22A,0x02);
		write_reg_val(0xD22B,0xF2);
		write_reg_val(0xD22C,0x02);
		write_reg_val(0xD22D,0xFE);
		write_reg_val(0xD22E,0x03);
		write_reg_val(0xD22F,0x10);
		write_reg_val(0xD230,0x03);
		write_reg_val(0xD231,0x33);
		write_reg_val(0xD232,0x03);
		write_reg_val(0xD233,0x6D);
		write_reg_val(0xD300,0x00);
		write_reg_val(0xD301,0x33);
		write_reg_val(0xD302,0x00);
		write_reg_val(0xD303,0x34);
		write_reg_val(0xD304,0x00);
		write_reg_val(0xD305,0x3A);
		write_reg_val(0xD306,0x00);
		write_reg_val(0xD307,0x4A);
		write_reg_val(0xD308,0x00);
		write_reg_val(0xD309,0x5C);
		write_reg_val(0xD30A,0x00);

		write_reg_val(0xD30B,0x81);
		write_reg_val(0xD30C,0x00);
		write_reg_val(0xD30D,0xA6);
		write_reg_val(0xD30E,0x00);
		write_reg_val(0xD30F,0xE5);
		write_reg_val(0xD310,0x01);
		write_reg_val(0xD311,0x13);
		write_reg_val(0xD312,0x01);
		write_reg_val(0xD313,0x54);
		write_reg_val(0xD314,0x01);
		write_reg_val(0xD315,0x82);
		write_reg_val(0xD316,0x01);
		write_reg_val(0xD317,0xCA);
		write_reg_val(0xD318,0x02);
		write_reg_val(0xD319,0x00);
		write_reg_val(0xD31A,0x02);
		write_reg_val(0xD31B,0x01);
		write_reg_val(0xD31C,0x02);
		write_reg_val(0xD31D,0x34);
		write_reg_val(0xD31E,0x02);
		write_reg_val(0xD31F,0x67);
		write_reg_val(0xD320,0x02);
		write_reg_val(0xD321,0x84);
		write_reg_val(0xD322,0x02);
		write_reg_val(0xD323,0xA4);
		write_reg_val(0xD324,0x02);
		write_reg_val(0xD325,0xB7);
		write_reg_val(0xD326,0x02);
		write_reg_val(0xD327,0xCF);
		write_reg_val(0xD328,0x02);
		write_reg_val(0xD329,0xDE);
		write_reg_val(0xD32A,0x02);
		write_reg_val(0xD32B,0xF2);
		write_reg_val(0xD32C,0x02);
		write_reg_val(0xD32D,0xFE);
		write_reg_val(0xD32E,0x03);
		write_reg_val(0xD32F,0x10);
		write_reg_val(0xD330,0x03);
		write_reg_val(0xD331,0x33);
		write_reg_val(0xD332,0x03);
		write_reg_val(0xD333,0x6D);
		write_reg_val(0xD400,0x00);
		write_reg_val(0xD401,0x33);
		write_reg_val(0xD402,0x00);
		write_reg_val(0xD403,0x34);
		write_reg_val(0xD404,0x00);
		write_reg_val(0xD405,0x3A);
		write_reg_val(0xD406,0x00);
		write_reg_val(0xD407,0x4A);
		write_reg_val(0xD408,0x00);
		write_reg_val(0xD409,0x5C);
		write_reg_val(0xD40A,0x00);
		write_reg_val(0xD40B,0x81);

		write_reg_val(0xD40C,0x00);
		write_reg_val(0xD40D,0xA6);
		write_reg_val(0xD40E,0x00);
		write_reg_val(0xD40F,0xE5);
		write_reg_val(0xD410,0x01);
		write_reg_val(0xD411,0x13);
		write_reg_val(0xD412,0x01);
		write_reg_val(0xD413,0x54);
		write_reg_val(0xD414,0x01);
		write_reg_val(0xD415,0x82);
		write_reg_val(0xD416,0x01);
		write_reg_val(0xD417,0xCA);
		write_reg_val(0xD418,0x02);
		write_reg_val(0xD419,0x00);
		write_reg_val(0xD41A,0x02);
		write_reg_val(0xD41B,0x01);
		write_reg_val(0xD41C,0x02);
		write_reg_val(0xD41D,0x34);
		write_reg_val(0xD41E,0x02);
		write_reg_val(0xD41F,0x67);
		write_reg_val(0xD420,0x02);
		write_reg_val(0xD421,0x84);
		write_reg_val(0xD422,0x02);
		write_reg_val(0xD423,0xA4);
		write_reg_val(0xD424,0x02);
		write_reg_val(0xD425,0xB7);
		write_reg_val(0xD426,0x02);
		write_reg_val(0xD427,0xCF);
		write_reg_val(0xD428,0x02);
		write_reg_val(0xD429,0xDE);
		write_reg_val(0xD42A,0x02);
		write_reg_val(0xD42B,0xF2);
		write_reg_val(0xD42C,0x02);
		write_reg_val(0xD42D,0xFE);
		write_reg_val(0xD42E,0x03);
		write_reg_val(0xD42F,0x10);
		write_reg_val(0xD430,0x03);
		write_reg_val(0xD431,0x33);
		write_reg_val(0xD432,0x03);
		write_reg_val(0xD433,0x6D);
		write_reg_val(0xD500,0x00);
		write_reg_val(0xD501,0x33);
		write_reg_val(0xD502,0x00);
		write_reg_val(0xD503,0x34);
		write_reg_val(0xD504,0x00);
		write_reg_val(0xD505,0x3A);
		write_reg_val(0xD506,0x00);
		write_reg_val(0xD507,0x4A);
		write_reg_val(0xD508,0x00);
		write_reg_val(0xD509,0x5C);
		write_reg_val(0xD50A,0x00);
		write_reg_val(0xD50B,0x81);

		write_reg_val(0xD50C,0x00);
		write_reg_val(0xD50D,0xA6);
		write_reg_val(0xD50E,0x00);
		write_reg_val(0xD50F,0xE5);
		write_reg_val(0xD510,0x01);
		write_reg_val(0xD511,0x13);
		write_reg_val(0xD512,0x01);
		write_reg_val(0xD513,0x54);
		write_reg_val(0xD514,0x01);
		write_reg_val(0xD515,0x82);
		write_reg_val(0xD516,0x01);
		write_reg_val(0xD517,0xCA);
		write_reg_val(0xD518,0x02);
		write_reg_val(0xD519,0x00);
		write_reg_val(0xD51A,0x02);
		write_reg_val(0xD51B,0x01);
		write_reg_val(0xD51C,0x02);
		write_reg_val(0xD51D,0x34);
		write_reg_val(0xD51E,0x02);
		write_reg_val(0xD51F,0x67);
		write_reg_val(0xD520,0x02);
		write_reg_val(0xD521,0x84);
		write_reg_val(0xD522,0x02);
		write_reg_val(0xD523,0xA4);
		write_reg_val(0xD524,0x02);
		write_reg_val(0xD525,0xB7);
		write_reg_val(0xD526,0x02);
		write_reg_val(0xD527,0xCF);
		write_reg_val(0xD528,0x02);
		write_reg_val(0xD529,0xDE);
		write_reg_val(0xD52A,0x02);
		write_reg_val(0xD52B,0xF2);
		write_reg_val(0xD52C,0x02);
		write_reg_val(0xD52D,0xFE);
		write_reg_val(0xD52E,0x03);
		write_reg_val(0xD52F,0x10);
		write_reg_val(0xD530,0x03);
		write_reg_val(0xD531,0x33);
		write_reg_val(0xD532,0x03);
		write_reg_val(0xD533,0x6D);
		write_reg_val(0xD600,0x00);
		write_reg_val(0xD601,0x33);
		write_reg_val(0xD602,0x00);
		write_reg_val(0xD603,0x34);
		write_reg_val(0xD604,0x00);
		write_reg_val(0xD605,0x3A);
		write_reg_val(0xD606,0x00);
		write_reg_val(0xD607,0x4A);
		write_reg_val(0xD608,0x00);
		write_reg_val(0xD609,0x5C);
		write_reg_val(0xD60A,0x00);
		write_reg_val(0xD60B,0x81);

		write_reg_val(0xD60C,0x00);
		write_reg_val(0xD60D,0xA6);
		write_reg_val(0xD60E,0x00);
		write_reg_val(0xD60F,0xE5);
		write_reg_val(0xD610,0x01);
		write_reg_val(0xD611,0x13);
		write_reg_val(0xD612,0x01);
		write_reg_val(0xD613,0x54);
		write_reg_val(0xD614,0x01);
		write_reg_val(0xD615,0x82);
		write_reg_val(0xD616,0x01);
		write_reg_val(0xD617,0xCA);
		write_reg_val(0xD618,0x02);
		write_reg_val(0xD619,0x00);
		write_reg_val(0xD61A,0x02);
		write_reg_val(0xD61B,0x01);
		write_reg_val(0xD61C,0x02);
		write_reg_val(0xD61D,0x34);
		write_reg_val(0xD61E,0x02);
		write_reg_val(0xD61F,0x67);
		write_reg_val(0xD620,0x02);
		write_reg_val(0xD621,0x84);
		write_reg_val(0xD622,0x02);
		write_reg_val(0xD623,0xA4);
		write_reg_val(0xD624,0x02);
		write_reg_val(0xD625,0xB7);
		write_reg_val(0xD626,0x02);
		write_reg_val(0xD627,0xCF);
		write_reg_val(0xD628,0x02);
		write_reg_val(0xD629,0xDE);
		write_reg_val(0xD62A,0x02);
		write_reg_val(0xD62B,0xF2);
		write_reg_val(0xD62C,0x02);
		write_reg_val(0xD62D,0xFE);
		write_reg_val(0xD62E,0x03);
		write_reg_val(0xD62F,0x10);
		write_reg_val(0xD630,0x03);
		write_reg_val(0xD631,0x33);
		write_reg_val(0xD632,0x03);
		write_reg_val(0xD633,0x6D);
		
        //LV2 Page 0 enable
		write_reg_val(0xF000,0x55);
		write_reg_val(0xF001,0xAA);
		write_reg_val(0xF002,0x52);
		write_reg_val(0xF003,0x08);
		write_reg_val(0xF004,0x00);
		
        //Display control
		write_reg_val(0xB100, 0xCC);
		write_reg_val(0xB101, 0x00);
		
        //Source hold time
		write_reg_val(0xB600,0x05);
		
        //Gate EQ control
		write_reg_val(0xB700,0x70);
		write_reg_val(0xB701,0x70);
		
        //Source EQ control (Mode 2)
		write_reg_val(0xB800,0x01);
		write_reg_val(0xB801,0x03);
		write_reg_val(0xB802,0x03);
		write_reg_val(0xB803,0x03);
		
        //Inversion mode (2-dot)
		write_reg_val(0xBC00,0x02);
		write_reg_val(0xBC01,0x00);
		write_reg_val(0xBC02,0x00);
		
        //Timing control 4H w/ 4-delay
		write_reg_val(0xC900,0xD0);
		write_reg_val(0xC901,0x02);
		write_reg_val(0xC902,0x50);
		write_reg_val(0xC903,0x50);
		write_reg_val(0xC904,0x50);
        
		write_reg_val(LCD_TEAR_EFFECT_ON_CMD, 0x00);
		write_reg_val(LCD_INTF_PIXEL_FMT_CMD, 0x55);  //16-bit/pixel
		
		write_index(LCD_SLEEP_OUT_CMD);
		gfxSleepMilliseconds(1);
		write_index(LCD_DISPLAY_ON_CMD);
	/*}	*/
	
	/* Set default orientation [vertical screen] */
	write_reg_val(LCD_ENTRY_MODE_CMD, LCD_ENTRY_PORTRAIT);
    
	/* Init sequence end */

	// Finish Init
	post_init_board(/*g*/);

 	// Release the bus
	//release_bus(/*g*/);

	// Turn on the back-light
	set_backlight(/*g,*/ GDISP_INITIAL_BACKLIGHT);

	// Initialise the GDISP structure
    
    g->g.Width = GDISP_SCREEN_WIDTH;
	g->g.Height = GDISP_SCREEN_HEIGHT;
    g->g.Orientation = gOrientation0;
	g->g.Powermode = gPowerOn;
	g->g.Backlight = GDISP_INITIAL_BACKLIGHT;
	g->g.Contrast = GDISP_INITIAL_CONTRAST;

	return gTrue;
}

#if GDISP_HARDWARE_STREAM_WRITE
	LLDSPEC	void gdisp_lld_write_start(GDisplay *g) {
		acquire_bus(/*g*/);
		set_viewport(g);
	}

	LLDSPEC	void gdisp_lld_write_color(GDisplay *g) {
		write_data(/*g,*/ gdispColor2Native(g->p.color));
	}

	LLDSPEC	void gdisp_lld_write_stop(GDisplay *g) {
		release_bus(/*g*/);
	}
#endif

#if GDISP_HARDWARE_STREAM_READ
	LLDSPEC	void gdisp_lld_read_start(GDisplay *g) {
		acquire_bus(/*g*/);
		set_viewport(g);
		setreadmode(/*g*/);
		dummy_read(/*g*/);
	}

	LLDSPEC	gColor gdisp_lld_read_color(GDisplay *g) {
		gU16	data;

		data = read_data32(/*g*/);
		return gdispNative2Color(data);
	}

	LLDSPEC	void gdisp_lld_read_stop(GDisplay *g) {
		setwritemode(/*g*/);
		release_bus(/*g*/);
	}
#endif

#if GDISP_NEED_CONTROL && GDISP_HARDWARE_CONTROL
	LLDSPEC void gdisp_lld_control(GDisplay *g) {
		switch(g->p.x) {
		case GDISP_CONTROL_POWER:
			if (g->g.Powermode == (gPowermode)g->p.ptr)
				return;

			switch((gPowermode)g->p.ptr) {
			case gPowerOff:
				acquire_bus(/*g*/);
				write_index(/*g,*/ LCD_DISPLAY_OFF_CMD);
				gfxSleepMilliseconds(10);
				write_index(/*g,*/ LCD_SLEEP_IN_CMD);
				release_bus(/*g*/);
				break;

			case gPowerOn:
				acquire_bus(/*g*/);
				write_index(/*g,*/ LCD_SLEEP_OUT_CMD);
				gfxSleepMilliseconds(120);
				write_index(/*g,*/ LCD_DISPLAY_ON_CMD);
				release_bus(/*g*/);
				if (g->g.Powermode != gPowerSleep)
					gdisp_lld_init(g);
				break;

			case gPowerSleep:
				acquire_bus(/*g*/);
        write_index(/*g,*/ LCD_DISPLAY_OFF_CMD);
        gfxSleepMilliseconds(10);
        write_index(/*g,*/ LCD_SLEEP_IN_CMD);
				release_bus(/*g*/);
				break;

			default:
				return;
			}

			g->g.Powermode = (gPowermode)g->p.ptr;
			return;

		case GDISP_CONTROL_ORIENTATION:
			if (g->g.Orientation == (gOrientation)g->p.ptr)
				return;

			switch((gOrientation)g->p.ptr) {
			case gOrientation0:
				acquire_bus(/*g*/);

                write_reg_val(LCD_ENTRY_MODE_CMD, LCD_ENTRY_PORTRAIT);

				release_bus(/*g*/);
				g->g.Height = GDISP_SCREEN_HEIGHT;
				g->g.Width = GDISP_SCREEN_WIDTH;
				break;

			case gOrientation90:
				acquire_bus(/*g*/);

                write_reg_val(LCD_ENTRY_MODE_CMD, LCD_ENTRY_LANDSCAPE);

				release_bus(/*g*/);
				g->g.Height = GDISP_SCREEN_WIDTH;
				g->g.Width = GDISP_SCREEN_HEIGHT;
				break;

			case gOrientation180:
				acquire_bus(/*g*/);

                write_reg_val(LCD_ENTRY_MODE_CMD, LCD_ENTRY_PORTRAIT_INV);

				release_bus(/*g*/);
				g->g.Height = GDISP_SCREEN_HEIGHT;
				g->g.Width = GDISP_SCREEN_WIDTH;
				break;

			case gOrientation270:
				acquire_bus(/*g*/);

                write_reg_val(LCD_ENTRY_MODE_CMD, LCD_ENTRY_LANDSCAPE_INV);

				release_bus(/*g*/);
				g->g.Height = GDISP_SCREEN_WIDTH;
				g->g.Width = GDISP_SCREEN_HEIGHT;
				break;

			default:
				return;
			}

			g->g.Orientation = (gOrientation)g->p.ptr;
			return;

        case GDISP_CONTROL_BACKLIGHT:
        {
            set_backlight((gU8)g->p.ptr);
        }
        break;
            
      default:
        return;
		}
	}
#endif

#if GDISP_HARDWARE_FILLS
	LLDSPEC void gdisp_lld_fill_area(GDisplay *g){
		acquire_bus(/*g*/);
		set_viewport(g);
		/* Write RAM */
        write_index(LCD_WR_RAM_CMD);
  		write_data_dma_nominc(g, (gU16)gdispColor2Native(g->p.color), g->p.cx * g->p.cy);
		release_bus(/*g*/);
	}
#endif

#if GDISP_HARDWARE_BITFILLS
	LLDSPEC void gdisp_lld_blit_area(GDisplay *g){
		acquire_bus(/*g*/);
		set_viewport(g);
		/* Write RAM */
        write_index(LCD_WR_RAM_CMD);		
		write_data_dma_minc(g, (gU16*)g->p.ptr, g->p.cx * g->p.cy);
		release_bus(/*g*/);	
	}
#endif    
    
#endif /* GFX_USE_GDISP */
