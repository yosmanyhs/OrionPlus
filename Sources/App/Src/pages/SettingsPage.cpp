#include "SettingsPage.h"

void SettingsPage::Create(lv_obj_t * parent)
{
    lv_obj_t* ctrl; 
    lv_obj_t* lbl;
    uint32_t x, y, index;
    
    const char* pol_sw_lbl_texts[] = 
	{ 
		"Stp X", "Stp Y", "Stp Z", "Lim X", "Lim Y", "Lim Z", 
		"Dir X", "Dir Y", "Dir Z", "Reset", "St En", "Fault"
	};
    
    /* Create global page container (screen sized) */
    m_page_handle = lv_cont_create(parent, NULL);
    lv_obj_set_pos(m_page_handle, 0, 0);
    lv_obj_set_size(m_page_handle, lv_obj_get_width(parent), lv_obj_get_height(parent));
    
    /* Create back & apply buttons [save only Apply button handle] */
    /** [Back] **/
    ctrl = lv_btn_create(m_page_handle, NULL);
    
    // Create a child image and assign text/symbols at once
    lv_img_set_src(lv_img_create(ctrl, NULL), (const void*)(LV_SYMBOL_LEFT "\nBack"));

    // Locate and resize buttons
    lv_obj_set_pos(ctrl, 8, 8);
    lv_obj_set_size(ctrl, 80, 80);
        
    /* Update user data */
    ctrl->user_data.instance = (void*)this;
    ctrl->user_data.controlId = CFG_BTN_BACK;

    /* Assign default event handler for this page */
    lv_obj_set_event_cb(ctrl, SettingsPage::event_router_proc);
    
    /** [Apply] **/
    m_applyBtn = lv_btn_create(m_page_handle, NULL);

    // Create a child image and assign text/symbols at once
    lv_img_set_src(lv_img_create(m_applyBtn, NULL), (const void*)(LV_SYMBOL_OK "\nSave"));

    // Locate and resize buttons
    lv_obj_set_pos(m_applyBtn, 712, 8);
    lv_obj_set_size(m_applyBtn, 80, 80);
        
    /* Update user data */
    m_applyBtn->user_data.instance = (void*)this;
    m_applyBtn->user_data.controlId = CFG_BTN_APPLY;

    /* Assign default event handler for this page */
    lv_obj_set_event_cb(m_applyBtn, SettingsPage::event_router_proc);
    
    /* Create heading label, centered horizontally on screen */
    ctrl = lv_label_create(m_page_handle, NULL);
    lv_label_set_text(ctrl, LV_SYMBOL_SETTINGS " Machine Settings");
    lv_obj_set_style_local_text_font(ctrl, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_22);
    lv_obj_align(ctrl, m_page_handle, LV_ALIGN_IN_TOP_MID, 0, 40);
    
    /* Create scroll page */
    m_scrollPage = lv_page_create(m_page_handle, NULL);
    
    lv_obj_set_pos(m_scrollPage, 0, 8 + 80 + 8);
    lv_obj_set_size(m_scrollPage, lv_obj_get_width(m_page_handle), lv_obj_get_height(m_page_handle) - (8 + 80 + 8));

    
    /** first group of settings [Stepper signal control] **/
	
	/*** heading label [Polarity inversion] ***/
	ctrl = lv_label_create(m_scrollPage, NULL);
	lv_label_set_text(ctrl, LV_SYMBOL_SHUFFLE  " Stepper motor signal polarity inversion and timing");
	lv_obj_align(ctrl, m_scrollPage, LV_ALIGN_IN_TOP_MID, 0, 8);

	/*** array of switches ***/
	for (index = 0; index < 12; index++)
	{
		x = 3 + 130 * (index % 6) + 3;
		y = 40 + 40 * (index / 6) + 10;

		ctrl = lv_switch_create(m_scrollPage, NULL);
		lv_obj_set_size(ctrl, 50, 25);
		lv_obj_set_pos(ctrl, x, y);

		lbl = lv_label_create(m_scrollPage, NULL);
		lv_label_set_text(lbl , pol_sw_lbl_texts[index]);
		lv_obj_align(lbl, ctrl, LV_ALIGN_OUT_RIGHT_MID, 6, 0);

		//m_switches[index] = ctrl;
	}

	/*** label [Pulse duration us] ***/
	lbl = lv_label_create(m_scrollPage, NULL);
	lv_label_set_text(lbl, "Step pulse duration [us]");
	lv_obj_align(lbl, m_scrollPage, LV_ALIGN_IN_TOP_LEFT, 32, y + 60);

	/*** slider for pulse duration us [10 ... 50us] ***/
	ctrl = lv_slider_create(m_scrollPage, NULL);
	lv_slider_set_range(ctrl, 10, 50);
	//lv_obj_set_event_cb(ctrl, SettingsPage_page_event_handler);

	lv_obj_set_width(ctrl, 220);
	lv_obj_align(ctrl, lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 8, 16);

	lbl = lv_label_create(m_scrollPage, NULL);
	lv_label_set_text(lbl, "10");
	lv_obj_align(lbl, ctrl, LV_ALIGN_OUT_RIGHT_MID, 16, 0);

	//m_stepLenSlider = ctrl;
	//m_stepLenLabel = lbl;

	/*** label [idle lock time] ***/
	lbl = lv_label_create(m_scrollPage, NULL);
	lv_label_set_text(lbl, "Stepper idle lock time [secs]");
	lv_obj_align(lbl, m_scrollPage, LV_ALIGN_IN_TOP_RIGHT, -32, y + 60);

	/*** slider for pulse duration us [0 ... 90secs] ***/
	ctrl = lv_slider_create(m_scrollPage, NULL);
	lv_slider_set_range(ctrl, 0, 90);
	//lv_obj_set_event_cb(ctrl, SettingsPage_page_event_handler);

	lv_obj_set_width(ctrl, 255);
	lv_obj_align(ctrl, lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 8, 16);

	lbl = lv_label_create(m_scrollPage, NULL);
	lv_label_set_text(lbl, "30");
	lv_obj_align(lbl, ctrl, LV_ALIGN_OUT_RIGHT_MID, 16, 0);

	//m_stepIdleSlider = ctrl;
	//m_stepIdleLabel = lbl;
}

void SettingsPage::Destroy()
{
    lv_obj_del(m_page_handle);
}

void SettingsPage::page_event_handler(struct _lv_obj_t * obj, lv_event_t evt)
{    
    if (obj != NULL && evt == LV_EVENT_CLICKED)
    {        
        switch (obj->user_data.controlId)
        {
            default:
                break;
        }
    }
}

