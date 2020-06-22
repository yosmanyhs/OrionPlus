#include "MainPage.h"


void MainPage::Create(lv_obj_t * parent)
{
    uint16_t xsize, ysize;
    
    m_page_handle = lv_cont_create(parent, NULL);
    
    xsize = lv_obj_get_width(parent);
    ysize = lv_obj_get_height(parent);
    
    lv_obj_set_size(m_page_handle, xsize, ysize);
}

void MainPage::Destroy()
{
    lv_obj_del(m_page_handle);
}
