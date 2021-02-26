
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


#include "user_tasks.h"

#include "driver_screen_NT35510.h"
#include "driver_touch_GT9147.h"
#include "lvgl.h"

#include "PageManager.h"
#include "MainPage.h"
#include "SettingsPage.h"

#include "DataConverter.h"
#include "MachineCore.h"
#include "GCodeParser.h"

//////////////////////////////////////////////////////////////////////

static GLOBAL_STATUS_REPORT_DATA ui_status_data;
static char ui_text_fmt_buf[64];

//////////////////////////////////////////////////////////////////////

#ifdef SAVED_FOR_LATER
static void ui_refresh_timer_callback(TimerHandle_t xTimer);

static void format_coords_to_string(const char* prefix, const char* sufix, char* buffer, const float* coords, uint32_t coord_cnt);
#endif

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
    SettingsPage mp;
    
    mp.Create(lv_scr_act());
    
    // Create data refresh timer
    //TimerHandle_t timer = xTimerCreate(NULL, 500, pdTRUE, NULL, (TimerCallbackFunction_t)ui_refresh_timer_callback);
    //xTimerStart(timer, 0);
    
    for ( ; ; )
    {
        lv_task_handler();
        vTaskDelay(50);
    }
}

///////////////////////////////////////////////////////////////////////////////
#ifdef SAVED_FOR_LATER

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
    
    //lv_label_set_static_text(status_labels[STAT_SYSSTATE_ID], fixedText);
    
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

#endif // SAVED_FOR_LATER
///////////////////////////////////////////////////////////////////////////////
