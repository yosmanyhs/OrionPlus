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

    lv_obj_t ** m_xyz_curr_labels;
    lv_obj_t ** m_xyz_next_labels;    
};

#endif
