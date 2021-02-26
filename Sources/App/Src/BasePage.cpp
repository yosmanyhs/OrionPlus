#include "BasePage.h"

void BasePageWindow::event_router_proc(struct _lv_obj_t * obj, lv_event_t evt)
{
    BasePageWindow* win = (BasePageWindow*)obj->user_data.instance;
    
    if (NULL != win)
    {
        win->page_event_handler(obj, evt);
    }
}

