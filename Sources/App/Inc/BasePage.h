#ifndef BASEPAGE_H
#define BASEPAGE_H

#include <stdint.h>
#include "lvgl.h"

class BasePageWindow
{
public:
    virtual void Create(lv_obj_t * parent) = 0;
    virtual void Destroy() = 0;

protected:
    lv_obj_t * m_page_handle;

    static void event_router_proc(struct _lv_obj_t * obj, lv_event_t evt);
    virtual void page_event_handler(struct _lv_obj_t * obj, lv_event_t evt) = 0;
};

#endif
