#ifndef FILEMANAGERPAGE_H
#define FILEMANAGERPAGE_H

#include <stdint.h>
#include "lvgl.h"
#include "BasePage.h"


class FileManagerPage : public BasePageWindow
{
public:
    virtual void Create(lv_obj_t * parent);
    virtual void Destroy();
    
protected:
    lv_obj_t * m_file_list_handle;
};

#endif
