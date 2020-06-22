
#include <stm32f4xx_hal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

//#include "gfx.h"

#include "task_settings.h"
#include "ui_task.h"
#include "settings_manager.h"
#include "ui_identifiers.h"
//#include "page_manager.h"

#include "driver_screen_NT35510.h"
#include "driver_touch_GT9147.h"
#include "lvgl.h"

#include "ControlBar.h"
#include "PageManager.h"

#include "DataConverter.h"

//////////////////////////////////////////////////////////////////////
#if 0
void create_control_bar(void);
void control_bar_button_click(lv_obj_t * btn, lv_event_t event);
void display_main_menu(uint8_t enabled_flag);
void control_bar_update_stat_icons(uint8_t stat_flags);
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
    ControlBarWindow * control_bar;
    PageManager * page_manager;
    
    control_bar = (ControlBarWindow*)pvPortMalloc(sizeof(ControlBarWindow));
    page_manager = (PageManager*)pvPortMalloc(sizeof(PageManager));
    
    control_bar->Create(lv_scr_act());
    page_manager->Initialize(lv_scr_act());
        
    for ( ; ; )
    {
        lv_task_handler();
        vTaskDelay(50);
    }
}

///////////////////////////////////////////////////////////////////////////////
