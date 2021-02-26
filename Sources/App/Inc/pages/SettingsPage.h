#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

#include <stdint.h>
#include "lvgl.h"
#include "BasePage.h"

class SettingsPage : public BasePageWindow
{
public:
    virtual void Create(lv_obj_t * parent);
    virtual void Destroy();
    
protected:
    
    enum CFG_PAGE_BUTTON_ID_VALUES
    {
        CFG_BTN_APPLY,
        CFG_BTN_BACK,
        
        CFG_BTN_MAX
    };
    
    virtual void page_event_handler(struct _lv_obj_t * obj, lv_event_t evt);    
    
    
    lv_obj_t* m_applyBtn;
    lv_obj_t* m_scrollPage;
};


#endif
