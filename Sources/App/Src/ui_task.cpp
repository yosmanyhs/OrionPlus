
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

#include "PageManager.h"

#include "DataConverter.h"
#include "MachineCore.h"
#include "GCodeParser.h"

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
	STAT_SYSSTATE_ID,			// Current system status (Homing, Halted, Probing, ...)

	STAT_FEEDRATE_LABEL_ID,	// Static text "Feed Rate"
	STAT_SPINDLE_LABEL_ID,	// Static text "Spindle"

	STAT_CURRENT_FEEDRATE_ID,	// Current feedrate value [units/min] | min-1
	STAT_CURRENT_RPM_ID,		// Current rpm of spindle [rpm]

	STAT_PLAYED_FILE_ID,	// File name of active SD file being played

	STAT_WIDGET_COUNT
};

///////////////////////////////////////////////////////////////////////////////

static lv_obj_t* feedrate_gauge;
static lv_obj_t* spindle_gauge;

static lv_obj_t* task_progress_bar;

static lv_obj_t* status_labels[STAT_WIDGET_COUNT];

static lv_obj_t * coord_table;
static lv_obj_t * status_tables[2];
static lv_obj_t * status_leds[3];

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

static const char* UI_COOL_DRY = "Dry";
static const char* UI_COOL_MIST = "Mist";
static const char* UI_COOL_FLOOD = "Flood";

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

///////////////////////////////////////////////////////////////////////////////

static void ui_refresh_timer_callback(TimerHandle_t xTimer)
{
    uint32_t index;
    const char* fixedText;
    
    // Read status information
    machine->GetGlobalStatusReport(ui_status_data);  
    
    // Coordinate Mode Abs/Inc
    if (ui_status_data.ModalState.distance_mode == MODAL_DISTANCE_MODE_ABSOLUTE)
        fixedText = UI_DIST_MODE_ABS;
    else
        fixedText = UI_DIST_MODE_INC;
    
    //lv_label_set_static_text(status_labels[STAT_COORD_MODE_ID], fixedText);
    
    // Unit Mode mm/Inch
    if (ui_status_data.ModalState.units_mode == MODAL_UNITS_MODE_MILLIMETERS)
        fixedText = UI_UNIT_MODE_MM;
    else
        fixedText = UI_UNIT_MODE_INCH;
    
    //lv_label_set_static_text(status_labels[STAT_UNIT_MODE_ID], fixedText);
    
    // Active Plane
    if (ui_status_data.ModalState.plane_select == MODAL_PLANE_SELECT_XY)
        fixedText = UI_PLANE_XY;
    else if (ui_status_data.ModalState.plane_select == MODAL_PLANE_SELECT_YZ)
        fixedText = UI_PLANE_YZ;
    else
        fixedText = UI_PLANE_ZX;
    
    //lv_label_set_static_text(status_labels[STAT_PLANE_MODE_ID], fixedText);
    
    // Check Mode
    if (ui_status_data.CheckModeEnabled != false)
        fixedText = UI_CHECK_MODE;
    else
        fixedText = UI_EMPTY_TEXT;
    
    //lv_label_set_static_text(status_labels[STAT_CHECKMODE_ID], fixedText);
    
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
    
    //lv_label_set_text(status_labels[STAT_COORDSYS_ID], ui_text_fmt_buf);
    
    // Spindle Mode
    if (ui_status_data.ModalState.spindle_mode == MODAL_SPINDLE_OFF)
        fixedText = UI_SPIN_STOP;
    else if (ui_status_data.ModalState.spindle_mode == MODAL_SPINDLE_CW)
        fixedText = UI_SPIN_CW;
    else
        fixedText = UI_SPIN_CCW;
    
    //lv_label_set_static_text(status_labels[STAT_SPIN_MODE_ID], fixedText);
    
    // Coolant Mode
    if (ui_status_data.ModalState.coolant_mode == MODAL_COOLANT_OFF)
        fixedText = UI_COOL_DRY;
    else if (ui_status_data.ModalState.coolant_mode == MODAL_COOLANT_MIST)
        fixedText = UI_COOL_MIST;
    else
        fixedText = UI_COOL_FLOOD;
    
    //lv_label_set_static_text(status_labels[STAT_COOL_MODE_ID], fixedText);
    
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
    //format_coords_to_string(UI_PREFIX_ORIGIN, UI_SUFIX_COORDS, ui_text_fmt_buf, ui_status_data.OriginCoordinates, 3);
    
    //lv_label_set_text(status_labels[STAT_ORIG_COORD_ID], ui_text_fmt_buf);
    
    // Coordinate Offset
    //format_coords_to_string(UI_PREFIX_OFFSET, UI_SUFIX_COORDS, ui_text_fmt_buf, ui_status_data.OffsetCoordinates, 3);
    
    //lv_label_set_text(status_labels[STAT_OFFSET_COORD_ID], ui_text_fmt_buf);
    
    // Probe Coordinates
    //format_coords_to_string(UI_PREFIX_PROBE, UI_SUFIX_COORDS, ui_text_fmt_buf, ui_status_data.ProbingPositions, 3);
    
    //lv_label_set_text(status_labels[STAT_PROBE_POS_ID], ui_text_fmt_buf);
    
    // Current Position Coordinates
    //format_coords_to_string(UI_PREFIX_POSITION, UI_SUFIX_COORDS, ui_text_fmt_buf, ui_status_data.CurrentPositions, 3);
    
    //lv_label_set_text(status_labels[STAT_CURRENT_POS_ID], ui_text_fmt_buf);

    // Next target Position Coordinates
    //format_coords_to_string(UI_PREFIX_TARGET, UI_SUFIX_COORDS, ui_text_fmt_buf, ui_status_data.TargetPositions, 3);
    
    //lv_label_set_text(status_labels[STAT_NEXT_TARGET_POS_ID], ui_text_fmt_buf);
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

///////////////////////////////////////////////////////////////////////////////

typedef enum ACTION_BUTTON_ID_VALUES
{
	// first row
    BTN_CYCLE_START,
    BTN_CYCLE_HOLD,
    BTN_CYCLE_ABORT,
    BTN_POWEROFF,

    // second row
    BTN_SAVE_TO_G10, 
    BTN_SAVE_TO_G92,
    BTN_NOTIFICATIONS,
    BTN_SETTINGS,

    // third row
    BTN_GO_HOME,
    BTN_DO_PROBE,
    BTN_JOG,
    BTN_FILEMGR,

    BTN_MAX
}ACTION_BUTTON_ID_VALUES;

static const char* btn_texts[BTN_MAX] =
{
	LV_SYMBOL_PLAY "\nStart",
    LV_SYMBOL_PAUSE "\nHold",
    LV_SYMBOL_STOP "\nAbort",
    LV_SYMBOL_POWER "\nSleep",

    LV_SYMBOL_SAVE "\n>G10",
    LV_SYMBOL_SAVE "\n>G92",
    LV_SYMBOL_BELL "\nAlerts",
    LV_SYMBOL_SETTINGS "\nSetup",
    
    LV_SYMBOL_HOME "\nHome",
    LV_SYMBOL_DOWNLOAD "\nProbe",
    LV_SYMBOL_KEYBOARD "\nJog",
    LV_SYMBOL_DIRECTORY "\nFiles",
};

///////////////////////////////////////////////////////////////////////////////

const char* initial_status_texts[STAT_WIDGET_COUNT] =
{
    "Ready",	// The current system status [Ready/Homing/Probing/Halted]

    "Feed Rate",
    "Spindle",

    "0 mm/min",
    "0 rpm",

    ""	// Will be empty by default
};

static lv_point_t lbl_coords[STAT_WIDGET_COUNT] =
{
    { 8, 450 },		// Ready/status

    { 70, 265 },	// FeedRate label	{ 385, 250, 0, 0 }
    { 300, 265 },	// Spindle Label	{ 540, 250, 0, 0 }
    
    { 45, 410 },  // Feedrate value
    { 270, 410 },  // Spindle rpms
    
    { 150, 450 },		// File name
};

///////////////////////////////////////////////////////////////////////////////

static void create_coord_table(lv_obj_t* parent)
{
    coord_table = lv_table_create(parent, NULL);

    int table_width = 800 - 80 * 4 - 8 * 4;
    
    lv_table_set_col_cnt(coord_table, 4);
    lv_table_set_row_cnt(coord_table, 5);

    for (int c = 0; c < 4; c++)
        lv_table_set_col_width(coord_table, c, (table_width - 20) / 4);

    const char * col_headings[] = { LV_SYMBOL_LIST, "X", "Y", "Z" };
    const char * row_headings[] = { "Origin", "Offset", "Probe", "Active" };

    for (size_t i = 0; i < 4; i++)
    {
        lv_table_set_cell_value(coord_table, 0, i, col_headings[i]);
        lv_table_set_cell_align(coord_table, 0, i, LV_LABEL_ALIGN_CENTER);
    }

    for (size_t k = 0; k < 4; k++)
    {
        lv_table_set_cell_value(coord_table, k + 1, 0, row_headings[k]);
        lv_table_set_cell_align(coord_table, k + 1, 0, LV_LABEL_ALIGN_CENTER);
    }

    for (size_t h = 1; h < 4; h++)
    {
        for (size_t w = 1; w < 5; w++)
        {
            lv_table_set_cell_value(coord_table, w, h, "0.0000");
            lv_table_set_cell_align(coord_table, w, h, LV_LABEL_ALIGN_CENTER);
        }
    }
    
    lv_obj_set_style_local_bg_opa(coord_table, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_opa(coord_table, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);

    lv_obj_set_style_local_pad_top(coord_table, LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_bottom(coord_table, LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_left(coord_table, LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_right(coord_table, LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);

    
    lv_obj_set_size(coord_table, table_width, 220);
    lv_obj_set_pos(coord_table, 0, 70);
}

///////////////////////////////////////////////////////////////////////////////

void create_status_tables(lv_obj_t* parent)
{
    status_tables[0] = lv_table_create(parent, NULL);

    int table_width = 800 - 80 * 4 - 8 * 6;

    lv_table_set_col_cnt(status_tables[0], 6);
    lv_table_set_row_cnt(status_tables[0], 1);

    int widths[] = { 60, 65, 45, 80, 80, 100 };

    for (int c = 0; c < 6; c++)
    {
        lv_table_set_col_width(status_tables[0], c, widths[c]);
    }
    
    lv_obj_set_style_local_bg_opa(status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_opa(status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_opa(status_tables[0], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, LV_OPA_TRANSP);

    lv_obj_set_style_local_margin_top(status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_bottom(status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_left(status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_left(status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

    lv_obj_set_style_local_pad_top(status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_bottom(status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_left(status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_right(status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

    lv_obj_set_style_local_pad_top(status_tables[0], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_bottom(status_tables[0], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_left(status_tables[0], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_right(status_tables[0], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);


    // Initialization data
    lv_table_set_cell_value(status_tables[0], 0, 0, "Abs");
    lv_table_set_cell_value(status_tables[0], 0, 1, "mm");
    lv_table_set_cell_value(status_tables[0], 0, 2, "XY");
    lv_table_set_cell_value(status_tables[0], 0, 3, "G54");
    lv_table_set_cell_value(status_tables[0], 0, 4, "");
    lv_table_set_cell_value(status_tables[0], 0, 5, "");
    
    lv_obj_set_size(status_tables[0], table_width, 41);
    lv_obj_set_pos(status_tables[0], 0, 0);

    status_tables[1] = lv_table_create(parent, status_tables[0]);

    lv_table_set_col_cnt(status_tables[1], 4);
    lv_table_set_row_cnt(status_tables[1], 1);

    lv_table_set_col_width(status_tables[1], 0, 80);
    lv_table_set_col_width(status_tables[1], 1, 110);
    lv_table_set_col_width(status_tables[1], 2, 75);
    lv_table_set_col_width(status_tables[1], 3, table_width - 80 - 110 - 75);

    lv_obj_set_style_local_margin_top(status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_bottom(status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_left(status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_left(status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

    lv_obj_set_style_local_pad_top(status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_bottom(status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_left(status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_right(status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

    lv_obj_set_style_local_pad_top(status_tables[1], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_bottom(status_tables[1], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_left(status_tables[1], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_right(status_tables[1], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);

    // Initialization data
    lv_table_set_cell_value(status_tables[1], 0, 0, "Dry");
    lv_table_set_cell_value(status_tables[1], 0, 1, "No Tool");
    lv_table_set_cell_value(status_tables[1], 0, 2, "Stop");
    lv_table_set_cell_value(status_tables[1], 0, 3, "");

    lv_obj_set_size(status_tables[1], table_width, 30);
    lv_obj_set_pos(status_tables[1], 0, 40);

}

///////////////////////////////////////////////////////////////////////////////

void create_status_leds(lv_obj_t* parent)
{
    lv_obj_t* box;
    lv_obj_t* labels;

    box = lv_cont_create(parent, NULL);
    
    lv_obj_set_size(box, 100, 95);
    lv_obj_set_pos(box, 448, 300);
    lv_obj_set_style_local_border_opa(box, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);

    for (size_t i = 0; i < 3; i++)
    {
        int x = i * 32 + 10;
        int y = 40 ;

        status_leds[i] = lv_led_create(box, NULL);

        lv_obj_set_size(status_leds[i], 15, 15);
        lv_obj_set_pos(status_leds[i], x, y);

        lv_led_off(status_leds[i]);
        _lv_obj_set_style_local_opa(status_leds[i], LV_LED_PART_MAIN, LV_STYLE_SHADOW_OPA, LV_OPA_TRANSP);
    }

    labels = lv_label_create(box, NULL);

    lv_label_set_text(labels, "Limits");
    lv_obj_set_pos(labels, 15, 10);

    labels = lv_label_create(box, NULL);

    lv_label_set_text(labels, "X   Y   Z");
    lv_obj_set_pos(labels, 10, 60);
}

///////////////////////////////////////////////////////////////////////////////

bool create_main_page_controls(lv_obj_t* parent)
{
    lv_obj_t* btn;
    int x, y;
    int index;

    // Create main page buttons
    for (index = BTN_CYCLE_START; index < BTN_MAX; index++)
    {
        // Create the button
        btn = lv_btn_create(parent, NULL);

        // Create a child image and assign text/symbols at once
        lv_img_set_src(lv_img_create(btn, NULL), (const void*)btn_texts[index]);

        // Locate and resize buttons
        x = 448 + 88 * (index % 4);
        y = 8 + 88 * (index / 4);

        lv_obj_set_pos(btn, x, y);
        lv_obj_set_size(btn, 80, 80);
        lv_obj_set_user_data(btn, (lv_obj_user_data_t)index);

        lv_obj_set_event_cb(btn, main_page_button_click_handler);
    }
    
    create_coord_table(parent);
    create_status_tables(parent);
    
    // Create feedrate and spindle gauges
    feedrate_gauge = lv_gauge_create(parent, NULL);

    lv_obj_set_size(feedrate_gauge, 120, 120);
    lv_obj_set_pos(feedrate_gauge, 60, 290);
    _lv_obj_set_style_local_ptr(feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STYLE_TEXT_FONT, &lv_font_montserrat_14);
    lv_gauge_set_value(feedrate_gauge, 0, 0);

    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STYLE_PAD_TOP | LV_STATE_DEFAULT, 4);
    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STYLE_PAD_BOTTOM | LV_STATE_DEFAULT, 4);
    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STYLE_PAD_LEFT | LV_STATE_DEFAULT, 4);
    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STYLE_PAD_RIGHT | LV_STATE_DEFAULT, 4);
    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STYLE_PAD_INNER | LV_STATE_DEFAULT, 2);

    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAJOR, LV_STYLE_LINE_WIDTH | LV_STATE_DEFAULT, 1);
    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAJOR, LV_STYLE_SCALE_END_LINE_WIDTH | LV_STATE_DEFAULT, 2);
    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAJOR, LV_STYLE_SCALE_END_BORDER_WIDTH | LV_STATE_DEFAULT, 1);

    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_NEEDLE, LV_STYLE_LINE_WIDTH | LV_STATE_DEFAULT, 3);
    //_lv_obj_set_style_local_color(feedrate_gauge, LV_GAUGE_PART_NEEDLE, LV_STYLE_LINE_COLOR | LV_STATE_DEFAULT, lv_color_hex(0xFF0000));

    spindle_gauge = lv_gauge_create(parent, feedrate_gauge);

    lv_obj_set_pos(spindle_gauge, 270, 290);
    lv_gauge_set_value(spindle_gauge, 0, 0);

    // Create status LEDs
    create_status_leds(parent);

    // Create task progress bar
    task_progress_bar = lv_bar_create(parent, NULL);

    lv_obj_set_pos(task_progress_bar, 640, 452);
    lv_obj_set_size(task_progress_bar, 150, 20);
    
    // Create all status labels
    for (index = 0; index < STAT_WIDGET_COUNT; index++)
    {
        lv_obj_t* lbl = lv_label_create(parent, NULL);

        lv_label_set_text(lbl, initial_status_texts[index]);
        lv_obj_set_pos(lbl, lbl_coords[index].x, lbl_coords[index].y);
        status_labels[index] = lbl;
    }

    lv_obj_set_style_local_text_font(status_labels[STAT_FEEDRATE_LABEL_ID], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
    lv_obj_set_style_local_text_font(status_labels[STAT_CURRENT_FEEDRATE_ID], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
    lv_obj_set_style_local_text_font(status_labels[STAT_SPINDLE_LABEL_ID], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
    lv_obj_set_style_local_text_font(status_labels[STAT_CURRENT_RPM_ID], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);

    lv_label_set_long_mode(status_labels[STAT_CURRENT_FEEDRATE_ID], LV_LABEL_LONG_DOT);
    lv_obj_set_width(status_labels[STAT_CURRENT_FEEDRATE_ID], 150);
    lv_label_set_align(status_labels[STAT_CURRENT_FEEDRATE_ID], LV_LABEL_ALIGN_CENTER);

    // rpms
    lv_label_set_long_mode(status_labels[STAT_CURRENT_RPM_ID], LV_LABEL_LONG_DOT);
    lv_obj_set_width(status_labels[STAT_CURRENT_RPM_ID], 120);
    lv_label_set_align(status_labels[STAT_CURRENT_RPM_ID], LV_LABEL_ALIGN_CENTER);

    return true;
}


static void main_page_button_click_handler(struct _lv_obj_t * btn, lv_event_t evt)
{
    static bool pwr_state = true;
    
	if (evt == LV_EVENT_RELEASED)
	{
		switch ((ACTION_BUTTON_ID_VALUES)btn->user_data)
		{
            case BTN_POWEROFF:
            {
                if (pwr_state == true)
                {
                    pwr_state = false;
                    lv_port_disp_NT35510_set_backlight(0);
                }
                else
                {
                    pwr_state = true;
                    lv_port_disp_NT35510_set_backlight(100);
                }
            }
            break;

            case BTN_SETTINGS:
            {
                static const char* msgs[] =
                {
                    "Abs", "Inc",
                    "mm", "inch",
                    "XY", "YZ", "ZX",
                    "G54", "G55", "G56", "G57", "G58", "G59", "G59.1", "G59.2", "G59.3",
                    "Dry", "Mist", "Flood",
                    "Tool 1", "Tool 2", "Tool 3", "Tool 999", "No Tool",
                    "Stop", "CW", "CCW",
                    "Check Mode", ""
                };

                static int m = 0;

                lv_table_set_cell_value(status_tables[0], 0, 0, msgs[(m % 2) + 0]);
                lv_table_set_cell_value(status_tables[0], 0, 1, msgs[(m % 2) + 2]);
                lv_table_set_cell_value(status_tables[0], 0, 2, msgs[(m % 3) + 4]);
                lv_table_set_cell_value(status_tables[0], 0, 3, msgs[(m % 9) + 7]);

                lv_table_set_cell_value(status_tables[1], 0, 0, msgs[(m % 3) + 16]);
                lv_table_set_cell_value(status_tables[1], 0, 1, msgs[(m % 5) + 19]);
                lv_table_set_cell_value(status_tables[1], 0, 2, msgs[(m % 3) + 24]);
                lv_table_set_cell_value(status_tables[1], 0, 3, msgs[(m % 2) + 27]);
                m++;
            }
            break;

            case BTN_CYCLE_ABORT:
                break;

            case BTN_CYCLE_HOLD:
                break;

            case BTN_CYCLE_START:
                break;

            case BTN_FILEMGR:
                break;

            case BTN_SAVE_TO_G92:
            {
                const char* buf;

                for (int i = 0; i < 3; i++)
                {

                    float f = (float)rand();

                    if (f != 0)
                        f = 1000 / f;

                    buf = DataConverter::FloatToStringTruncate(f, 6);
                    lv_table_set_cell_value(coord_table, 2, i + 1, buf);
                }
            }
            break;

            case BTN_SAVE_TO_G10:
            {
                const char* buf;

                for (int i = 0; i < 3; i++)
                {

                    float f = (float)rand();

                    if (f != 0)
                        f = 1000 / f;

                    buf = DataConverter::FloatToStringTruncate(f, 6);
                    lv_table_set_cell_value(coord_table, 1, i + 1, buf);
                }
            }
            break;

            case BTN_JOG:
                break;

            case BTN_DO_PROBE:
                break;

            case BTN_GO_HOME:
            {
                const char *buf;

                for (size_t i = 0; i < 3; i++)
                {
                    int r = rand() % 101;

                    switch (i)
                    {
                        case 0:
                        {
                            lv_gauge_set_value(feedrate_gauge, 0, r);

                            // feedrate max = 6000 <=> 100
                            buf = DataConverter::IntegerToString(60 * r);
                            lv_label_set_text(status_labels[STAT_CURRENT_FEEDRATE_ID], buf);
                        }   
                        break;

                        case 1:
                        {
                            lv_gauge_set_value(spindle_gauge, 0, r);

                            // spindle max = 10000 <=> 100
                            buf = DataConverter::IntegerToString(100 * r);
                            lv_label_set_text(status_labels[STAT_CURRENT_RPM_ID], buf);
                        }
                        break;
                    }
                }
                
                machine->GoHome(NULL, 0, true);
            }
            break;
		}
	}
}


