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
};

#endif
