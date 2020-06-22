#ifndef CONTROLBAR_H
#define CONTROLBAR_H

#include <stdint.h>
#include "lvgl.h"
#include "driver_screen_NT35510.h"

class ControlBarWindow
{
public:
    void Create(lv_obj_t * parent);
    
    
protected:
    lv_obj_t ** m_control_btn_handle_array;
    lv_obj_t ** m_status_icon_handle_array;
    lv_obj_t ** m_status_label_handle_array;

    uint8_t m_enabled_flag;

    void display_main_menu();
    void control_bar_update_stat_icons(uint8_t stat_flags);


    static const void * m_icon_symbols[9]; 
    static void control_bar_button_click(lv_obj_t * btn, lv_event_t event);
};

#endif
