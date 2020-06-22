/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.io/license.html
 */

#include "gfx.h"

#if (GFX_USE_GINPUT && GINPUT_NEED_MOUSE)

#define GMOUSE_DRIVER_VMT		GMOUSEVMT_GT9147
#include "ginput_driver_mouse.h"

// Get the hardware interface
#include "gmouse_lld_GT9147_board.h"


static gBool mouse_xyz(GMouse* m, GMouseReading* pdr)
{
	(void)m;

	// No buttons
	pdr->buttons = 0;
    pdr->z = 0;
	
    aquire_bus(m);

	if (getpin_pressed(m)) 
    {		
		read_value(m, 0);
        pdr->z = 1;
	}
    
#if (GDISP_DEFAULT_ORIENTATION == gOrientationLandscape)
    pdr->x = 480 - touch_x;
    pdr->y = 800 - touch_y;
#else
    pdr->x = touch_x;
    pdr->y = touch_y;
#endif

    
    release_bus(m);
	return gTrue;
}

const GMouseVMT const GMOUSE_DRIVER_VMT[1] = {{
	{
		GDRIVER_TYPE_TOUCH,
		GMOUSE_VFLG_TOUCH | /*GMOUSE_VFLG_SELFROTATION |*/ GMOUSE_VFLG_DEFAULTFINGER,
		sizeof(GMouse)+GMOUSE_GT9147_BOARD_DATA_SIZE,
		_gmouseInitDriver,
		_gmousePostInitDriver,
		_gmouseDeInitDriver
	},
	1,				// z_max - (currently?) not supported
	0,				// z_min - (currently?) not supported
	1,				// z_touchon
	0,				// z_touchoff
	{				// pen_jitter
		GMOUSE_GT9147_PEN_CALIBRATE_ERROR,			// calibrate
		GMOUSE_GT9147_PEN_CLICK_ERROR,				// click
		GMOUSE_GT9147_PEN_MOVE_ERROR				// move
	},
	{				// finger_jitter
		GMOUSE_GT9147_FINGER_CALIBRATE_ERROR,		// calibrate
		GMOUSE_GT9147_FINGER_CLICK_ERROR,			// click
		GMOUSE_GT9147_FINGER_MOVE_ERROR			// move
	},
	init_board, 	// init
	0,				// deinit
	mouse_xyz,		// get
	0,				// calsave
	0				// calload
}};

#endif /* GFX_USE_GINPUT && GINPUT_NEED_MOUSE */

