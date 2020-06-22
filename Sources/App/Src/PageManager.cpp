#include "PageManager.h"

#include "MainPage.h"
#include "FileManagerPage.h"

lv_obj_t* PageManager::m_page_manager_surface;

void PageManager::Initialize(lv_obj_t * parent)
{
    m_page_manager_surface = lv_cont_create(parent, NULL);
    
    lv_obj_set_pos(m_page_manager_surface, 0, 64);  // Position below ControlBar
    lv_obj_set_size(m_page_manager_surface, 800, 480 - 64); // Cover remaining screen
    lv_obj_set_style(m_page_manager_surface, &lv_style_plain_color);
}

void PageManager::SwitchToPage(uint32_t page_index)
{
    
}

