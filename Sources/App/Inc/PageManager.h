#ifndef PAGEMANAGER_H
#define PAGEMANAGER_H

#include <stdint.h>
#include "lvgl.h"

#include "BasePage.h"

class PageManager
{
public:
    static void Initialize(lv_obj_t * parent);    
    static void SwitchToPage(uint32_t page_index);
    
    
protected:
    static lv_obj_t * m_page_manager_surface;
};

#endif
