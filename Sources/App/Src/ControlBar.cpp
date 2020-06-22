#include "ControlBar.h"
#include "UI_Identifiers.h"

#include "FreeRTOS.h"
#include "task.h"

const void * ControlBarWindow::m_icon_symbols[9] = 
{ 
    LV_SYMBOL_POWER, LV_SYMBOL_LIST, LV_SYMBOL_STOP, LV_SYMBOL_PAUSE, LV_SYMBOL_PLAY,
    
    LV_SYMBOL_USB, LV_SYMBOL_SD_CARD, LV_SYMBOL_HOME, LV_SYMBOL_WARNING
}; 


void ControlBarWindow::Create(lv_obj_t * parent)
{
    lv_obj_t * icons;
    uint32_t xpos, ypos, index;
    custom_user_data_t myud;
    
    // Set specific buttons enabled [Home, Settings, 0, 0, About]
    m_enabled_flag = 0x13;
    
    lv_obj_t * base_bar = lv_cont_create(lv_scr_act(), NULL);
    
    lv_obj_set_pos(base_bar, 0, 0);
    lv_obj_set_size(base_bar, 800, 64);
    lv_obj_set_style(base_bar, &lv_style_plain_color);

    // Set initial position
    xpos = 800 - 64 + 2;
    ypos =  64 - 60 - 2;
    
    m_control_btn_handle_array = (lv_obj_t**)pvPortMalloc(sizeof(lv_obj_t*) * 5);
    
    // Loop to create buttons
    for (index = 0; index < 5; index++)
    {
        // Create button
        m_control_btn_handle_array[index] = lv_btn_create(base_bar, NULL);
        
        myud.control_id = (UI_Identifier_Values)index;
        myud.control_data = (void*)this;
        
        // Adjust its parameters
        lv_obj_set_pos(m_control_btn_handle_array[index], xpos, ypos);
        lv_obj_set_size(m_control_btn_handle_array[index], 60, 60);
        lv_obj_set_user_data(m_control_btn_handle_array[index], myud);
        
        lv_btn_set_layout(m_control_btn_handle_array[index], LV_LAYOUT_CENTER);
        lv_obj_set_event_cb(m_control_btn_handle_array[index], ControlBarWindow::control_bar_button_click);
        
        // Create the button icon
        icons = lv_img_create(m_control_btn_handle_array[index], NULL);
        lv_img_set_src(icons, ControlBarWindow::m_icon_symbols[index]);
        
        // Adjust parameters for next button
        xpos -= (2 + 64);
    }
    
    // Create four status icons
    xpos = 2;
    ypos = 2;    
    
    // First, allocate memory for handle arrays
    m_status_icon_handle_array = (lv_obj_t**)pvPortMalloc(sizeof(lv_obj_t*) * 4);
    m_status_label_handle_array = (lv_obj_t**)pvPortMalloc(sizeof(lv_obj_t*) * 8);
    
    for (index = 0; index < 4; index++)
    {
        m_status_icon_handle_array[index] = lv_img_create(base_bar, NULL);
        
        lv_obj_set_pos(m_status_icon_handle_array[index], xpos, ypos);
        lv_obj_set_size(m_status_icon_handle_array[index], 30, 30);
        lv_img_set_src(m_status_icon_handle_array[index], m_icon_symbols[index + 5]);
        
        xpos += (2 + 32);
    }
    
    // Create information labels [current modal modes]
    const uint32_t stat_label_widths[] = { 42, 32, 40, 40, 50, 60 };
    const uint32_t stat_label_padding = 12;
    
    
    for (index = 0; index < 6; index++)
    {
        m_status_label_handle_array[index] = lv_label_create(base_bar, NULL);
    
        lv_label_set_long_mode(m_status_label_handle_array[index], LV_LABEL_LONG_CROP);
        lv_label_set_align(m_status_label_handle_array[index], LV_LABEL_ALIGN_CENTER);
        lv_obj_set_pos(m_status_label_handle_array[index], xpos, ypos);
        lv_obj_set_size(m_status_label_handle_array[index], stat_label_widths[index], 32); 
        lv_label_set_text(m_status_label_handle_array[index], "");
        
        xpos += (stat_label_padding + stat_label_widths[index]);
    }
    
    // Create file name label
    m_status_label_handle_array[6] = lv_label_create(base_bar, NULL);
    
    lv_label_set_long_mode(m_status_label_handle_array[6], LV_LABEL_LONG_SROLL_CIRC);
    lv_obj_set_pos(m_status_label_handle_array[6], 8, 32);
    lv_obj_set_size(m_status_label_handle_array[6], 460, 32); 
    
    lv_label_set_text(m_status_label_handle_array[6], "");
}

void ControlBarWindow::display_main_menu()
{
    static uint8_t current_menu_state = 0;
    static lv_obj_t * menu_handle = NULL;
    const char * menu_labels[] = 
    { 
        LV_SYMBOL_HOME " Inicio", 
        LV_SYMBOL_SETTINGS " Ajustes",  
        LV_SYMBOL_DRIVE " Archivos",
        LV_SYMBOL_BELL " Mensajes",
        "Acerca de ..."
    };
    
    const uint32_t MENU_WIDTH = 200;
    const uint32_t MENU_HEIGHT = (5 * 64 + 6 * 10);
    
    if (current_menu_state == 0)
    {
        lv_obj_t * child1, * child2;
        uint32_t ypos = 10, index;
        custom_user_data_t myud;
        
        // Not visible. Create and show
        menu_handle = lv_obj_create(lv_disp_get_layer_top(NULL), NULL);
        
        lv_obj_set_size(menu_handle, MENU_WIDTH, MENU_HEIGHT);
        lv_obj_set_pos(menu_handle, 800 - 200 - 64, 66);
        
        for (index = 0; index < 5; index++)
        {
            // Create first button inside menu
            child1 = lv_btn_create(menu_handle, NULL);
            
            myud.control_id = (UI_Identifier_Values)(index + ControlBar_MenuBtn_Home);
            myud.control_data = (void*)this;
            
            lv_obj_set_size(child1, MENU_WIDTH - 20, 64);
            lv_obj_set_pos(child1, 10, ypos);
            lv_obj_set_user_data(child1, myud);
            lv_obj_set_event_cb(child1, ControlBarWindow::control_bar_button_click);
            lv_btn_set_layout(child1, LV_LAYOUT_OFF);
            
            child2 = lv_label_create(child1, NULL);
            lv_label_set_text(child2, menu_labels[index]);
            lv_obj_align(child2, child1, LV_ALIGN_IN_LEFT_MID, 32, 0);
            
            if ((m_enabled_flag & (1 << index)) == 0)
            {
                lv_btn_set_state(child1, LV_BTN_STATE_INA);
                lv_obj_set_click(child1, false);
            }
            
            ypos += (10 + 64);
        } 
            
        current_menu_state = 1;
    }
    else
    {
        if (menu_handle != NULL)
        {
            lv_obj_del(menu_handle);
        }
        
        current_menu_state = 0;
    }
}

void ControlBarWindow::control_bar_update_stat_icons(uint8_t stat_flags)
{
    uint8_t idx;               
                
    // Update icons
    for (idx = 0; idx < 4; idx++)
    {
        if (stat_flags & (1 << idx))
            lv_img_set_src(m_status_icon_handle_array[idx], ControlBarWindow::m_icon_symbols[idx + 5]);
        else
            lv_img_set_src(m_status_icon_handle_array[idx], LV_SYMBOL_DUMMY);
    }
}

void ControlBarWindow::control_bar_button_click(lv_obj_t * btn, lv_event_t event)
{
    static uint8_t pwr_state = 1;
    ControlBarWindow* self_instance = (ControlBarWindow*)btn->user_data.control_data;

    if (event == LV_EVENT_CLICKED)
    {
        switch ((uint32_t)btn->user_data.control_id)
        {
            case ControlBar_Button_Power: // Power button
            {
                if (pwr_state == 0)
                {
                    lv_port_disp_NT35510_set_backlight(100);
                    pwr_state = 1;
                }
                else
                {
                    lv_port_disp_NT35510_set_backlight(0);
                    pwr_state = 0;
                }
            }
            break;
            
            case ControlBar_Button_Menu: // Main menu
            {
                self_instance->display_main_menu();
            }
            break;
            
            case ControlBar_MenuBtn_Home:
            {
            }
            break;
            
            case ControlBar_Button_Start:
            {
            }
            break;
            
//            case ControlBar_Button_Pause:
//            {
//                const char* txt_mode[] = { "G0", "G1", "G2", "G3", "G80" };
//                const char* txt_plane[] = { "XY", "XZ", "YZ" };
//                const char* txt_absinc[] = { "Abs", "Inc" };
//                const char* txt_mmin[] = { "mm", "in" };
//                const char* txt_spin[] = { "CW", "CCW", "Off" };
//                const char* txt_cool[] = { "Mist", "Flood", "None" }; 
//                
//                lv_label_set_text(status_label_handle_array[0], txt_mode[txt % 5]);
//                lv_label_set_text(status_label_handle_array[1], txt_plane[txt % 3]);
//                lv_label_set_text(status_label_handle_array[2], txt_absinc[txt % 2]);
//                lv_label_set_text(status_label_handle_array[3], txt_mmin[txt % 2]);
//                lv_label_set_text(status_label_handle_array[4], txt_spin[txt % 3]);
//                lv_label_set_text(status_label_handle_array[5], txt_cool[txt % 3]);
//                
//                txt++;
//            }
//            break;
            
            default:
                break;
        }
    }
}
