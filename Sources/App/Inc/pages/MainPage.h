#ifndef MAINPAGE_H
#define MAINPAGE_H

#include <stdint.h>
#include "lvgl.h"
#include "BasePage.h"


class MainPage : public BasePageWindow
{
public:
    virtual void Create(lv_obj_t * parent);
    virtual void Destroy();
    
protected:
    
    enum MAIN_PAGE_BUTTON_ID_VALUES
    {
        // first row
        MAIN_BTN_CYCLE_START, MAIN_BTN_CYCLE_HOLD, MAIN_BTN_CYCLE_ABORT, MAIN_BTN_POWEROFF,

        // second row
        MAIN_BTN_SAVE_TO_G10, MAIN_BTN_SAVE_TO_G92, MAIN_BTN_NOTIFICATIONS, MAIN_BTN_SETTINGS,

        // third row
        MAIN_BTN_GO_HOME, MAIN_BTN_DO_PROBE, MAIN_BTN_JOG, MAIN_BTN_FILEMGR,

        MAIN_BTN_MAX
    };
    
    enum MAIN_STATUS_LABEL_ID_VALUES
    {
        MAIN_STAT_SYSSTATE_ID,			// Current system status (Homing, Halted, Probing, ...)

        MAIN_STAT_FEEDRATE_LABEL_ID,	// Static text "Feed Rate"
        MAIN_STAT_SPINDLE_LABEL_ID,	// Static text "Spindle"

        MAIN_STAT_CURRENT_FEEDRATE_ID,	// Current feedrate value [units/min] | min-1
        MAIN_STAT_CURRENT_RPM_ID,		// Current rpm of spindle [rpm]

        MAIN_STAT_PLAYED_FILE_ID,	// File name of active SD file being played

        MAIN_STAT_WIDGET_COUNT
    };
    
    void create_coord_table();
    void create_status_tables();

    virtual void page_event_handler(struct _lv_obj_t * obj, lv_event_t evt);
    
    
    lv_obj_t* m_action_bts[MAIN_BTN_MAX];
    lv_obj_t* m_feedrate_gauge;
    lv_obj_t* m_spindle_gauge;
    lv_obj_t* m_task_progress_bar;
    lv_obj_t* m_status_labels[MAIN_STAT_WIDGET_COUNT];
    lv_obj_t* m_coord_table;
    lv_obj_t* m_status_tables[2];
    
};

#endif
