
#include <stm32f4xx_hal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "task_settings.h"
#include "ui_task.h"
#include "settings_manager.h"
#include "ui_identifiers.h"
//#include "page_manager.h"

#include "user_tasks.h"

#include "driver_screen_NT35510.h"
#include "driver_touch_GT9147.h"
#include "lvgl.h"

#include "ControlBar.h"
#include "PageManager.h"

#include "DataConverter.h"
#include "MachineCore.h"
#include "GCodeParser.h"

//////////////////////////////////////////////////////////////////////
#if 0
void create_control_bar(void);
void control_bar_button_click(lv_obj_t * btn, lv_event_t event);
void display_main_menu(uint8_t enabled_flag);
void control_bar_update_stat_icons(uint8_t stat_flags);
#endif

//////////////////////////////////////////////////////////////////////

static GLOBAL_STATUS_REPORT_DATA ui_status_data;
static char ui_text_fmt_buf[64];

//////////////////////////////////////////////////////////////////////

static bool create_main_page_controls(lv_obj_t* parent);
static void ui_refresh_timer_callback(TimerHandle_t xTimer);

static void format_coords_to_string(const char* prefix, const char* sufix, char* buffer, const float* coords, uint32_t coord_cnt);
//////////////////////////////////////////////////////////////////////

static void UI_MainTask_Entry(void * pvParam);

void UI_BootTask_Entry(void * pvParam)
{
    lv_init();
    lv_port_disp_NT35510_init();
    lv_port_indev_GT9147_init();    
    
    if (pdPASS != xTaskCreate(UI_MainTask_Entry, "UITASK", UI_TASK_STACK_SIZE, NULL, UI_TASK_PRIORITY, NULL))
    {
        /* Notify failure and take action */
        configASSERT(0);
    }
    
    /* Release resources of this initialization task */
    vTaskDelete(NULL);
}

void UI_MainTask_Entry(void * pvParam)
{
    // Initialize UI helper variables
    memset(&ui_status_data, 0, sizeof(ui_status_data));
    memset(&ui_text_fmt_buf, 0, sizeof(ui_text_fmt_buf));    
    
    // Create Main Page User Interface controls
    create_main_page_controls(lv_scr_act());
    
    // Create data refresh timer
    TimerHandle_t timer = xTimerCreate(NULL, 500, pdTRUE, NULL, (TimerCallbackFunction_t)ui_refresh_timer_callback);
    xTimerStart(timer, 0);
    
    for ( ; ; )
    {
        lv_task_handler();
        vTaskDelay(50);
    }
}

///////////////////////////////////////////////////////////////////////////////

enum STATUS_LABEL_ID_VALUES
{
	STAT_COORD_MODE_ID,		// Abs/Inc
	STAT_UNIT_MODE_ID,		// mm/inches
	STAT_PLANE_MODE_ID,		// XY, YZ, ZX
	STAT_CHECKMODE_ID,		// Check mode
	STAT_COORDSYS_ID,		// G54, ... , G59.3

	STAT_SYSSTATE_ID,			// Current system status (Homing, Halted, Probing, ...)

	STAT_ORIG_COORD_ID,		// Coordinates of current origin [G54, ..., values] [0.0000, 0.0000, 0.0000]
	STAT_OFFSET_COORD_ID,	// Coordinate offsets G92 
	STAT_PROBE_POS_ID,		// Probe position P=[0.0000, 0.0000, 0.0000]

	STAT_CURRENT_POS_ID,	// Current steppers position [mm/inch] 
	STAT_NEXT_TARGET_POS_ID,	// Next target position [mm/inch]

	STAT_FEEDRATE_LABEL_ID,	// Static text "Feed Rate"
	STAT_SPINDLE_LABEL_ID,	// Static text "Spindle"
	STAT_TEMPERATURE_LABEL_ID, // Static text "Temperature"

	STAT_SPIN_MODE_ID,		// M3, M4, M5
	STAT_COOL_MODE_ID,		// M7, M8, M9

	STAT_CURRENT_FEEDRATE_ID,	// Current feedrate value [units/min] | min-1
	STAT_CURRENT_RPM_ID,		// Current rpm of spindle [rpm]

	STAT_TOOL_NUM_ID,		// Tool number



	STAT_PLAYED_FILE_ID,	// File name of active SD file being played
	STAT_CONNECTION_STATUS,	// USB connection status

	STAT_WIDGET_COUNT
};

///////////////////////////////////////////////////////////////////////////////

static lv_obj_t* feedrate_gauge;
static lv_obj_t* spindle_gauge;
static lv_obj_t* temperature_gauge;

static lv_obj_t* task_progress_bar;

static lv_obj_t* status_labels[STAT_WIDGET_COUNT];

///////////////////////////////////////////////////////////////////////////////

// User interface constant text values

static const char* UI_EMPTY_TEXT = "";

static const char* UI_DIST_MODE_ABS = "Abs";
static const char* UI_DIST_MODE_INC = "Inc";

static const char* UI_UNIT_MODE_MM = "mm";
static const char* UI_UNIT_MODE_INCH = "Inch";

static const char* UI_PLANE_XY = "XY";
static const char* UI_PLANE_YZ = "YZ";
static const char* UI_PLANE_ZX = "ZX";

static const char* UI_CHECK_MODE = "Check Mode";

static const char* UI_SPIN_CW = "CW";
static const char* UI_SPIN_CCW = "CCW";
static const char* UI_SPIN_STOP = "Stop";

static const char* UI_COOL_DRY = "Coolant: Dry";
static const char* UI_COOL_MIST = "Coolant: Mist";
static const char* UI_COOL_FLOOD = "Coolant: Flood";

static const char* UI_SUFIX_MM_MIN = " mm/min";
static const char* UI_SUFIX_IN_MIN = " ipm";
static const char* UI_SUFIX_RPM = " rpm";

static const char* UI_MACH_READY = "Ready";
static const char* UI_MACH_WORKING = "Working";
static const char* UI_MACH_HALTED = "Halted";
static const char* UI_MACH_PROBING = "Probing";
static const char* UI_MACH_HOMING = "Homing";
static const char* UI_MACH_FEED_HOLD = "Feed Hold";

static const char* UI_HOMING_STATUSES[] =
{
    "Homing: Seek limits",
    "Homing: Release limits",
    "Homing: Confirm limits",
    "Homing: Finishing",
    "Homing: Done",
    "Homing: Error"
};

static const char* UI_PREFIX_ORIGIN = "Origin [";
static const char* UI_PREFIX_OFFSET = "Offset [";
static const char* UI_PREFIX_PROBE = "Probe [";

static const char* UI_PREFIX_POSITION = "Position [";
static const char* UI_PREFIX_TARGET = "Target [";

static const char* UI_SUFIX_COORDS = "]";

///////////////////////////////////////////////////////////////////////////////

static void ui_refresh_timer_callback(TimerHandle_t xTimer)
{
    uint32_t index;
    int16_t percent_value = 0;
    float max_value;
    float helper;
    
    const char* fixedText;
    
    // Read status information
    machine->GetGlobalStatusReport(ui_status_data);  
    
    // Coordinate Mode Abs/Inc
    if (ui_status_data.ModalState.distance_mode == MODAL_DISTANCE_MODE_ABSOLUTE)
        fixedText = UI_DIST_MODE_ABS;
    else
        fixedText = UI_DIST_MODE_INC;
    
    lv_label_set_static_text(status_labels[STAT_COORD_MODE_ID], fixedText);
    
    // Unit Mode mm/Inch
    if (ui_status_data.ModalState.units_mode == MODAL_UNITS_MODE_MILLIMETERS)
        fixedText = UI_UNIT_MODE_MM;
    else
        fixedText = UI_UNIT_MODE_INCH;
    
    lv_label_set_static_text(status_labels[STAT_UNIT_MODE_ID], fixedText);
    
    // Active Plane
    if (ui_status_data.ModalState.plane_select == MODAL_PLANE_SELECT_XY)
        fixedText = UI_PLANE_XY;
    else if (ui_status_data.ModalState.plane_select == MODAL_PLANE_SELECT_YZ)
        fixedText = UI_PLANE_YZ;
    else
        fixedText = UI_PLANE_ZX;
    
    lv_label_set_static_text(status_labels[STAT_PLANE_MODE_ID], fixedText);
    
    // Check Mode
    if (ui_status_data.CheckModeEnabled != false)
        fixedText = UI_CHECK_MODE;
    else
        fixedText = UI_EMPTY_TEXT;
    
    lv_label_set_static_text(status_labels[STAT_CHECKMODE_ID], fixedText);
    
    // Active coordinate system selection
    ui_text_fmt_buf[0] = 'G';
    ui_text_fmt_buf[1] = '5';
    
    // G54 -> 0, 55-1,56-2,57-3,58-4,59-5,59.1-6,59.2-7,59.3-8
    if (ui_status_data.ModalState.work_coord_sys_index > 5)
    {
        // 59.x
        ui_text_fmt_buf[2] = '9';
        ui_text_fmt_buf[3] = '.';
        ui_text_fmt_buf[4] = (ui_status_data.ModalState.work_coord_sys_index - 5) + '0';
        ui_text_fmt_buf[5] = '\0';
    }
    else
    {
        ui_text_fmt_buf[2] = (ui_status_data.ModalState.work_coord_sys_index) + '4';
        ui_text_fmt_buf[3] = '\0';
    }
    
    lv_label_set_text(status_labels[STAT_COORDSYS_ID], ui_text_fmt_buf);
    
    // Spindle Mode
    if (ui_status_data.ModalState.spindle_mode == MODAL_SPINDLE_OFF)
        fixedText = UI_SPIN_STOP;
    else if (ui_status_data.ModalState.spindle_mode == MODAL_SPINDLE_CW)
        fixedText = UI_SPIN_CW;
    else
        fixedText = UI_SPIN_CCW;
    
    lv_label_set_static_text(status_labels[STAT_SPIN_MODE_ID], fixedText);
    
    // Coolant Mode
    if (ui_status_data.ModalState.coolant_mode == MODAL_COOLANT_OFF)
        fixedText = UI_COOL_DRY;
    else if (ui_status_data.ModalState.coolant_mode == MODAL_COOLANT_MIST)
        fixedText = UI_COOL_MIST;
    else
        fixedText = UI_COOL_FLOOD;
    
    lv_label_set_static_text(status_labels[STAT_COOL_MODE_ID], fixedText);
    
    // System status
    if (ui_status_data.SystemHalted == true)
        fixedText = UI_MACH_HALTED;
    else
    {
        if (ui_status_data.FeedHoldActive == true)
            fixedText = UI_MACH_FEED_HOLD;
        if (ui_status_data.HomingState != HOMING_IDLE)
        {
            index = ((uint32_t)ui_status_data.HomingState - 1) % 6;
            fixedText = UI_HOMING_STATUSES[index];
        }
        else if (ui_status_data.ProbingState != PROBING_IDLE)
            fixedText = UI_MACH_PROBING;
        else if (ui_status_data.SteppersEnabled == true)
            fixedText = UI_MACH_WORKING;
        else
            fixedText = UI_MACH_READY;
    }
    
    lv_label_set_static_text(status_labels[STAT_SYSSTATE_ID], fixedText);
    
    // Coordinate Origin
    format_coords_to_string(UI_PREFIX_ORIGIN, UI_SUFIX_COORDS, ui_text_fmt_buf, ui_status_data.OriginCoordinates, 3);
    
    lv_label_set_text(status_labels[STAT_ORIG_COORD_ID], ui_text_fmt_buf);
    
    // Coordinate Offset
    format_coords_to_string(UI_PREFIX_OFFSET, UI_SUFIX_COORDS, ui_text_fmt_buf, ui_status_data.OffsetCoordinates, 3);
    
    lv_label_set_text(status_labels[STAT_OFFSET_COORD_ID], ui_text_fmt_buf);
    
    // Probe Coordinates
    format_coords_to_string(UI_PREFIX_PROBE, UI_SUFIX_COORDS, ui_text_fmt_buf, ui_status_data.ProbingPositions, 3);
    
    lv_label_set_text(status_labels[STAT_PROBE_POS_ID], ui_text_fmt_buf);
    
    // Current Position Coordinates
    format_coords_to_string(UI_PREFIX_POSITION, UI_SUFIX_COORDS, ui_text_fmt_buf, ui_status_data.CurrentPositions, 3);
    
    lv_label_set_text(status_labels[STAT_CURRENT_POS_ID], ui_text_fmt_buf);

    // Next target Position Coordinates
    format_coords_to_string(UI_PREFIX_TARGET, UI_SUFIX_COORDS, ui_text_fmt_buf, ui_status_data.TargetPositions, 3);
    
    lv_label_set_text(status_labels[STAT_NEXT_TARGET_POS_ID], ui_text_fmt_buf);
}

///////////////////////////////////////////////////////////////////////////////

void format_coords_to_string(const char* prefix, const char* sufix, char* buffer, const float* coords, uint32_t coord_cnt)
{
    uint32_t index;
    const char* append_text;
    
    // Create a text in the form "<Prefix>" + <Coord1>,<Coord2>, ... ,<CoordCnt> + "<Sufix>"
    if (prefix != NULL)
        strcpy(buffer, prefix);
    
    for (index = 0; index < coord_cnt; index++)
    {
        // Allow a max of '0.0000' [6 positions]
        append_text = DataConverter::FloatToStringTruncate(coords[index], 6);
        strcat(buffer, append_text);
        
        // add comma to separate values
        if (index < (coord_cnt - 1))
            strcat(buffer, ", ");
    }

    // Finally append sufix
    if (sufix != NULL)
        strcat(buffer, sufix);
}

///////////////////////////////////////////////////////////////////////////////

static void main_page_button_click_handler(struct _lv_obj_t * btn, lv_event_t evt);
static void main_page_label_click_handler(struct _lv_obj_t * lbl, lv_event_t evt);

static void debug_report_coords(lv_obj_t* obj);

///////////////////////////////////////////////////////////////////////////////

typedef enum ACTION_BUTTON_ID_VALUES
{
	BTN_SETTINGS,
	BTN_CYCLE_ABORT,
	BTN_CYCLE_HOLD,
	BTN_CYCLE_START,
	BTN_FILEMGR,

	BTN_SAVE_TO_G92,
	BTN_SAVE_TO_G10,
	BTN_JOG,
	BTN_DO_PROBE,
	BTN_GO_HOME,

	BTN_MAX
}ACTION_BUTTON_ID_VALUES;

static const char* btn_texts[BTN_MAX] =
{
	LV_SYMBOL_SETTINGS "\nSetup",
	LV_SYMBOL_STOP "\nAbort",
	LV_SYMBOL_PAUSE "\nHold",
	LV_SYMBOL_PLAY "\nStart",
	LV_SYMBOL_DIRECTORY "\nFiles",

	LV_SYMBOL_SAVE "\n>G92",
	LV_SYMBOL_SAVE "\n>G10",
	LV_SYMBOL_KEYBOARD "\nJog",
	LV_SYMBOL_DOWNLOAD "\nProbe",
	LV_SYMBOL_HOME "\nHome"
};

///////////////////////////////////////////////////////////////////////////////

const char* initial_status_texts[STAT_WIDGET_COUNT] =
{
	UI_DIST_MODE_ABS,	// Abs, Inc
	UI_UNIT_MODE_MM,	// mm, inch
	UI_PLANE_XY,	// XY, YZ, ZX
	UI_EMPTY_TEXT,	// Check Mode: This will be empty by default
	"G54",	// G54, ... , G59.3

	"Ready",	// The current system status [Ready/Homing/Probing/Halted]

	"Origin  [0.0000, 0.0000, 0.0000]",
	"Offset  [0.0000, 0.0000, 0.0000]",
	"Probe  [0.0000, 0.0000, 0.0000]",

	"Position [0.0000, 0.0000, 0.0000]",
	"Target    [0.0000, 0.0000, 0.0000]",

	"Feed Rate",
	"Spindle",
	"Temperature",

	UI_SPIN_STOP,
	UI_COOL_DRY,

	"0 mm/min",
	"0 rpm",

	"Tool: 0",		// Tool number 0

	"",	// Will be empty by default
	LV_SYMBOL_USB " USB"
};

static lv_area_t lbl_coords[STAT_WIDGET_COUNT] =
{
	{ 8, 8, 50, 20 },		// Abs/Incr
	{ 58, 8, 55, 20 },		// mm/inch
	{ 115, 8, 35, 20 },		// Plane XY, YZ, ZX
	{ 190, 125, 135, 20 },	// Check Mode 
	{ 150, 8, 60, 20 },		// Active Coord system index

	{ 8, 450, 0, 0 },		// Ready/status

	{ 8, 40, 300, 20 },		// Origin
	{ 8, 68, 300, 20 },		// Offset
	{ 8, 96, 300, 20 },		// Probe

	{ 430, 280, 0, 0 },		// Current Position
	{ 430, 370, 0, 0 },		// Next Target

	{ 15, 200, 0, 0 },	// FeedRate label	{ 385, 250, 0, 0 }
	{ 155, 200, 0, 0 },	// Spindle Label	{ 540, 250, 0, 0 }
	{ 265, 200, 0, 0 }, // Temperature Label { 650, 250, 0, 0 }

	{ 180, 395, 0, 0 },	// Spindle mode
	{ 8, 125, 0, 0}, // Coolant mode

	{ 10, 365, 0, 0 },  // Feedrate value
	{ 155, 365, 0, 0 },  // Spindle rpms

	{ 35, 395, 0, 0},		// Tool number

	{ 150, 450, 0, 0},		// File name
	{ 270, 8, 0, 0},	// USB connection status
};




///////////////////////////////////////////////////////////////////////////////


bool create_main_page_controls(lv_obj_t* parent)
{
	lv_obj_t* btn;
	int x, y;
	int index;

	// Create main page buttons
	for (index = BTN_SETTINGS; index < BTN_MAX; index++)
	{
		// Create the button
		btn = lv_btn_create(parent, NULL);

		// Create a child image and assign text/symbols at once
		lv_img_set_src(lv_img_create(btn, NULL), (const void*)btn_texts[index]);

		// Locate and resize buttons
		x = (index < 5) ? (800 - 88 * (index + 1)) : (800 - 88 * (index - 4));
		y = (index < 5) ? 8 : 96;

		lv_obj_set_pos(btn, x, y);
		lv_obj_set_size(btn, 80, 80);
		lv_obj_set_user_data(btn, (lv_obj_user_data_t)index);

		lv_obj_set_event_cb(btn, main_page_button_click_handler);
	}

	// Create feedrate and spindle gauges
	feedrate_gauge = lv_gauge_create(parent, NULL);

	lv_obj_set_size(feedrate_gauge, 120, 120);
	lv_obj_set_pos(feedrate_gauge, 10 + 0, 235);
	//_lv_obj_set_style_local_ptr(widgets[BTN_MAX + 0], LV_GAUGE_PART_MAIN, LV_STYLE_TEXT_FONT, &lv_font_montserrat_14);
	lv_gauge_set_value(feedrate_gauge, 0, 50);

	lv_obj_set_drag(feedrate_gauge, true);
	lv_obj_set_event_cb(feedrate_gauge, main_page_label_click_handler);

	//_lv_obj_set_style_local_int(feedrate_gauge, LV_LABEL_PART_MAIN, LV_STYLE_BORDER_WIDTH | LV_STATE_DEFAULT, 1);

	spindle_gauge = lv_gauge_create(parent, feedrate_gauge);
	lv_obj_set_pos(spindle_gauge, 10 + 130, 235);
	lv_gauge_set_value(spindle_gauge, 0, 75);

	temperature_gauge = lv_gauge_create(parent, feedrate_gauge);
	lv_obj_set_pos(temperature_gauge, 10 + 260, 235);
	lv_gauge_set_value(temperature_gauge, 0, 25);

	
	
	// Create status LEDs
	/*for (index = 0; index < 3; index++)
	{
		lv_obj_t* led = lv_led_create(parent, NULL);

		if (index % 2)
			lv_led_off(led);

		lv_obj_set_pos(led, 16 + index * 20, 150);
		lv_obj_set_size(led, 10, 10);
		lv_obj_set_click(led, true);
		lv_obj_set_drag(led, true);

		lv_obj_set_event_cb(led, main_page_label_click_handler);

		limit_leds[index] = led;
	}*/

	// Create task progress bar
	task_progress_bar = lv_bar_create(parent, NULL);

	lv_obj_set_pos(task_progress_bar, 640, 452);
	lv_obj_set_size(task_progress_bar, 150, 20);
	lv_obj_set_event_cb(task_progress_bar, main_page_label_click_handler);
	lv_obj_set_click(task_progress_bar, true);
	lv_obj_set_drag(task_progress_bar, true);

	// Create all status labels
	for (index = 0; index < STAT_WIDGET_COUNT; index++)
	{
		lv_obj_t* lbl = lv_label_create(parent, NULL);

		//lv_label_set_text_static(lbl, initial_status_texts[index]);
        lv_label_set_text(lbl, initial_status_texts[index]);

		lv_obj_set_drag(lbl, true);
		lv_obj_set_click(lbl, true);

		/*if (i == STAT_SYSSTATE_ID)
		lv_label_set_long_mode(widgets[i], LV_LABEL_LONG_CROP);*/

		lv_obj_set_pos(lbl, lbl_coords[index].x1, lbl_coords[index].y1);
		lv_obj_set_size(lbl, lbl_coords[index].x2, lbl_coords[index].y2);

		//_lv_obj_set_style_local_int(lbl, LV_LABEL_PART_MAIN, LV_STYLE_BORDER_WIDTH | LV_STATE_DEFAULT, 1);
		
		if (index == STAT_CURRENT_POS_ID || index == STAT_NEXT_TARGET_POS_ID)
		{
			//_lv_obj_set_style_local_ptr(lbl, LV_LABEL_PART_MAIN, LV_STYLE_TEXT_FONT, &lv_font_montserrat_24);

			//_lv_obj_set_style_local_color(lbl, LV_OBJ_PART_MAIN, LV_STYLE_TEXT_COLOR, LV_COLOR_INDIGO);
		}

		//if (index == STAT_CONNECTION_STATUS)
		//	_lv_obj_set_style_local_color(lbl, LV_OBJ_PART_MAIN, LV_STYLE_TEXT_COLOR, LV_COLOR_BLUE);


		lv_obj_set_event_cb(lbl, main_page_label_click_handler);
		status_labels[index] = lbl;
	}

	return true;
}


static void main_page_button_click_handler(struct _lv_obj_t * btn, lv_event_t evt)
{
	if (evt == LV_EVENT_RELEASED)
	{
		debug_report_coords(btn);

		switch ((ACTION_BUTTON_ID_VALUES)btn->user_data)
		{
			case BTN_SETTINGS:
				//printf("Settings\n");
				//lv_led_toggle(limit_leds[0]);
				break;

			case BTN_CYCLE_ABORT:
				//printf("Cycle abort\n");
				//lv_led_toggle(limit_leds[1]);
				break;

			case BTN_CYCLE_HOLD:
				//printf("Cycle hold\n");
				//lv_led_toggle(limit_leds[2]);
				break;

			case BTN_CYCLE_START:
				//printf("Cycle start\n");
				lv_gauge_set_value(feedrate_gauge, 0, (rand() % 101));
				break;

			case BTN_FILEMGR:
				//printf("File manager\n");
				lv_bar_set_value(task_progress_bar, (rand() % 101), LV_ANIM_ON);
				break;
				
			case BTN_SAVE_TO_G92:
				//printf("Save current pos as G92 offset");
				break;

			case BTN_SAVE_TO_G10:
				//printf("Save current pos as G10 active system origin\n");
				break;

			case BTN_JOG:
				//printf("Manual control\n");
				break;

			case BTN_DO_PROBE:
				//printf("Start probe cycle\n");
				lv_gauge_set_value(spindle_gauge, 0, (rand() % 101));
				break;

			case BTN_GO_HOME:
				//printf("Start homing cycle\n");
				break;
		}
	}
}

static void main_page_label_click_handler(struct _lv_obj_t * lbl, lv_event_t evt)
{
	if (evt == LV_EVENT_RELEASED)
	{
		debug_report_coords(lbl);
	}
}

// debug
static void debug_report_coords(lv_obj_t* obj)
{
	lv_area_t area;
	int w, h;

	lv_obj_get_coords(obj, &area);

	w = lv_obj_get_width(obj);
	h = lv_obj_get_height(obj);

	//printf("x=%d, y=%d, cx=%d (%d), cy=%d (%d)\n", area.x1, area.y1, area.x2 - area.x1 - 1, w, area.y2 - area.y1 - 1, h);
}


