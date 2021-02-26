#include "MainPage.h"


void MainPage::Create(lv_obj_t * parent)
{
    lv_obj_t* btn;
    int x, y;
    int index;
    
    /* Initialize array with button names */
    const char* btn_texts[MAIN_BTN_MAX] = { LV_SYMBOL_PLAY "\nStart", LV_SYMBOL_PAUSE "\nHold", 
                                            LV_SYMBOL_STOP "\nAbort", LV_SYMBOL_POWER "\nSleep",

                                            LV_SYMBOL_SAVE "\n>G10", LV_SYMBOL_SAVE "\n>G92",
                                            LV_SYMBOL_BELL "\nAlerts", LV_SYMBOL_SETTINGS "\nSetup",
                                            
                                            LV_SYMBOL_HOME "\nHome", LV_SYMBOL_DOWNLOAD "\nProbe",
                                            LV_SYMBOL_KEYBOARD "\nJog", LV_SYMBOL_DIRECTORY "\nFiles" };
    
    const uint8_t btn_enable_states[MAIN_BTN_MAX] = { 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0 };
    
    /* Initialize array with status labels initial texts */
    const char* initial_status_texts[MAIN_STAT_WIDGET_COUNT] = 
    {
        "Ready",	// The current system status [Ready/Homing/Probing/Halted]
        "Feed Rate",
        "Spindle",
        "0 mm/min",
        "0 rpm",
        ""	// Will be empty by default
    };
    
    /* Initialize array with status label coordinates */
    const lv_point_t lbl_coords[MAIN_STAT_WIDGET_COUNT] =
    {
        { 8, 450 },		// Ready/status
        { 70, 265 },	// FeedRate label	{ 385, 250, 0, 0 }
        { 300, 265 },	// Spindle Label	{ 540, 250, 0, 0 }
        { 45, 410 },    // Feedrate value
        { 270, 410 },   // Spindle rpms
        { 150, 450 },	// File name
    };
    
    /* Create global page container (screen sized) */
    m_page_handle = lv_cont_create(parent, NULL);
    lv_obj_set_pos(m_page_handle, 0, 0);
    lv_obj_set_size(m_page_handle, lv_obj_get_width(parent), lv_obj_get_height(parent));

    // Create main page buttons
    for (index = MAIN_BTN_CYCLE_START; index < MAIN_BTN_MAX; index++)
    {
        // Create the button
        btn = lv_btn_create(m_page_handle, NULL);

        // Create a child image and assign text/symbols at once
        lv_img_set_src(lv_img_create(btn, NULL), (const void*)btn_texts[index]);

        // Locate and resize buttons
        x = 448 + 88 * (index % 4);
        y = 8 + 88 * (index / 4);

        lv_obj_set_pos(btn, x, y);
        lv_obj_set_size(btn, 80, 80);
        
        /* Update user data */
        btn->user_data.instance = (void*)this;
        btn->user_data.controlId = index;

        /* Assign default event handler for this page */
        lv_obj_set_event_cb(btn, MainPage::event_router_proc);
        
        /* Default disabled buttons */
        if (btn_enable_states[index] == 0)
           lv_obj_set_state(btn, LV_STATE_DISABLED);
        
        /* Save object reference */
        m_action_bts[index] = btn;
    }
    
    create_coord_table();
    create_status_tables();
    
    // Create feedrate and spindle gauges
    m_feedrate_gauge = lv_gauge_create(m_page_handle, NULL);

    lv_obj_set_size(m_feedrate_gauge, 120, 120);
    lv_obj_set_pos(m_feedrate_gauge, 60, 290);
    
    lv_obj_set_style_local_text_font(m_feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_14);
    
    //_lv_obj_set_style_local_ptr(m_feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STYLE_TEXT_FONT, &lv_font_montserrat_14);
    lv_gauge_set_value(m_feedrate_gauge, 0, 0);

    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STYLE_PAD_TOP | LV_STATE_DEFAULT, 4);
    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STYLE_PAD_BOTTOM | LV_STATE_DEFAULT, 4);
    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STYLE_PAD_LEFT | LV_STATE_DEFAULT, 4);
    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STYLE_PAD_RIGHT | LV_STATE_DEFAULT, 4);
    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAIN, LV_STYLE_PAD_INNER | LV_STATE_DEFAULT, 2);

    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAJOR, LV_STYLE_LINE_WIDTH | LV_STATE_DEFAULT, 1);
    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAJOR, LV_STYLE_SCALE_END_LINE_WIDTH | LV_STATE_DEFAULT, 2);
    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_MAJOR, LV_STYLE_SCALE_END_BORDER_WIDTH | LV_STATE_DEFAULT, 1);

    //_lv_obj_set_style_local_int(feedrate_gauge, LV_GAUGE_PART_NEEDLE, LV_STYLE_LINE_WIDTH | LV_STATE_DEFAULT, 3);
    //_lv_obj_set_style_local_color(feedrate_gauge, LV_GAUGE_PART_NEEDLE, LV_STYLE_LINE_COLOR | LV_STATE_DEFAULT, lv_color_hex(0xFF0000));

    m_spindle_gauge = lv_gauge_create(m_page_handle, m_feedrate_gauge);

    lv_obj_set_pos(m_spindle_gauge, 270, 290);
    lv_gauge_set_value(m_spindle_gauge, 0, 0);

    // Create status LEDs
    //create_status_leds(parent);

    // Create task progress bar
    m_task_progress_bar = lv_bar_create(m_page_handle, NULL);

    lv_obj_set_pos(m_task_progress_bar, 640, 452);
    lv_obj_set_size(m_task_progress_bar, 150, 20);
    lv_obj_set_hidden(m_task_progress_bar, true);
    
    // Create all status labels
    for (index = 0; index < MAIN_STAT_WIDGET_COUNT; index++)
    {
        lv_obj_t* lbl = lv_label_create(m_page_handle, NULL);

        lv_label_set_text(lbl, initial_status_texts[index]);
        lv_obj_set_pos(lbl, lbl_coords[index].x, lbl_coords[index].y);
        m_status_labels[index] = lbl;
    }

    lv_obj_set_style_local_text_font(m_status_labels[MAIN_STAT_FEEDRATE_LABEL_ID], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
    lv_obj_set_style_local_text_font(m_status_labels[MAIN_STAT_CURRENT_FEEDRATE_ID], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
    lv_obj_set_style_local_text_font(m_status_labels[MAIN_STAT_SPINDLE_LABEL_ID], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
    lv_obj_set_style_local_text_font(m_status_labels[MAIN_STAT_CURRENT_RPM_ID], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);

    lv_label_set_long_mode(m_status_labels[MAIN_STAT_CURRENT_FEEDRATE_ID], LV_LABEL_LONG_DOT);
    lv_obj_set_width(m_status_labels[MAIN_STAT_CURRENT_FEEDRATE_ID], 150);
    lv_label_set_align(m_status_labels[MAIN_STAT_CURRENT_FEEDRATE_ID], LV_LABEL_ALIGN_CENTER);

    // rpms
    lv_label_set_long_mode(m_status_labels[MAIN_STAT_CURRENT_RPM_ID], LV_LABEL_LONG_DOT);
    lv_obj_set_width(m_status_labels[MAIN_STAT_CURRENT_RPM_ID], 120);
    lv_label_set_align(m_status_labels[MAIN_STAT_CURRENT_RPM_ID], LV_LABEL_ALIGN_CENTER);
}

void MainPage::Destroy()
{
    lv_obj_del(m_page_handle);
}


void MainPage::create_coord_table()
{
    const int table_width = 800 - 80 * 4 - 8 * 4;
    
    const char * col_headings[] = { LV_SYMBOL_LIST, "X", "Y", "Z" };
    const char * row_headings[] = { "Origin", "Offset", "Probe", "Active" };
    
    m_coord_table = lv_table_create(m_page_handle, NULL);
    
    lv_table_set_col_cnt(m_coord_table, 4);
    lv_table_set_row_cnt(m_coord_table, 5);        

    for (uint32_t i = 0; i < 4; i++)
    {
        lv_table_set_col_width(m_coord_table, i, (table_width - 20) / 4);
        
        lv_table_set_cell_value(m_coord_table, 0, i, col_headings[i]);
        lv_table_set_cell_align(m_coord_table, 0, i, LV_LABEL_ALIGN_CENTER);
    }

    for (uint32_t k = 0; k < 4; k++)
    {
        lv_table_set_cell_value(m_coord_table, k + 1, 0, row_headings[k]);
        lv_table_set_cell_align(m_coord_table, k + 1, 0, LV_LABEL_ALIGN_CENTER);
    }

    for (uint32_t h = 1; h < 4; h++)
    {
        for (uint32_t w = 1; w < 5; w++)
        {
            lv_table_set_cell_value(m_coord_table, w, h, "0.0000");
            lv_table_set_cell_align(m_coord_table, w, h, LV_LABEL_ALIGN_CENTER);
        }
    }
    
    lv_obj_set_style_local_bg_opa(m_coord_table, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_opa(m_coord_table, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);

    lv_obj_set_style_local_pad_top(m_coord_table, LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_bottom(m_coord_table, LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_left(m_coord_table, LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_right(m_coord_table, LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);

    
    lv_obj_set_size(m_coord_table, table_width, 220);
    lv_obj_set_pos(m_coord_table, 0, 70);
}


///////////////////////////////////////////////////////////////////////////////

void MainPage::create_status_tables()
{
    const int table_width = 800 - 80 * 4 - 8 * 6;
    
    m_status_tables[0] = lv_table_create(m_page_handle, NULL);

    lv_table_set_col_cnt(m_status_tables[0], 6);
    lv_table_set_row_cnt(m_status_tables[0], 1);

    const int widths[] = { 60, 65, 45, 80, 80, 100 };

    for (int c = 0; c < 6; c++)
    {
        lv_table_set_col_width(m_status_tables[0], c, widths[c]);
    }
    
    lv_obj_set_style_local_bg_opa(m_status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_opa(m_status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_opa(m_status_tables[0], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, LV_OPA_TRANSP);

    lv_obj_set_style_local_margin_top(m_status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_bottom(m_status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_left(m_status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_left(m_status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

    lv_obj_set_style_local_pad_top(m_status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_bottom(m_status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_left(m_status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_right(m_status_tables[0], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

    lv_obj_set_style_local_pad_top(m_status_tables[0], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_bottom(m_status_tables[0], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_left(m_status_tables[0], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_right(m_status_tables[0], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);


    // Initialization data
    lv_table_set_cell_value(m_status_tables[0], 0, 0, "Abs");
    lv_table_set_cell_value(m_status_tables[0], 0, 1, "mm");
    lv_table_set_cell_value(m_status_tables[0], 0, 2, "XY");
    lv_table_set_cell_value(m_status_tables[0], 0, 3, "G54");
    lv_table_set_cell_value(m_status_tables[0], 0, 4, "");
    lv_table_set_cell_value(m_status_tables[0], 0, 5, "");
    
    lv_obj_set_size(m_status_tables[0], table_width, 41);
    lv_obj_set_pos(m_status_tables[0], 0, 0);

    m_status_tables[1] = lv_table_create(m_page_handle, m_status_tables[0]);

    lv_table_set_col_cnt(m_status_tables[1], 4);
    lv_table_set_row_cnt(m_status_tables[1], 1);

    lv_table_set_col_width(m_status_tables[1], 0, 80);
    lv_table_set_col_width(m_status_tables[1], 1, 110);
    lv_table_set_col_width(m_status_tables[1], 2, 75);
    lv_table_set_col_width(m_status_tables[1], 3, table_width - 80 - 110 - 75);

    lv_obj_set_style_local_margin_top(m_status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_bottom(m_status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_margin_left(m_status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_left(m_status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

    lv_obj_set_style_local_pad_top(m_status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_bottom(m_status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_left(m_status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_right(m_status_tables[1], LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

    lv_obj_set_style_local_pad_top(m_status_tables[1], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_bottom(m_status_tables[1], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_left(m_status_tables[1], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);
    lv_obj_set_style_local_pad_right(m_status_tables[1], LV_TABLE_PART_CELL1, LV_STATE_DEFAULT, 6);

    // Initialization data
    lv_table_set_cell_value(m_status_tables[1], 0, 0, "Dry");
    lv_table_set_cell_value(m_status_tables[1], 0, 1, "No Tool");
    lv_table_set_cell_value(m_status_tables[1], 0, 2, "Stop");
    lv_table_set_cell_value(m_status_tables[1], 0, 3, "");

    lv_obj_set_size(m_status_tables[1], table_width, 30);
    lv_obj_set_pos(m_status_tables[1], 0, 40);
}

void MainPage::page_event_handler(struct _lv_obj_t * obj, lv_event_t evt)
{    
    if (obj != NULL && evt == LV_EVENT_CLICKED)
    {        
        switch (obj->user_data.controlId)
        {
            case MAIN_BTN_CYCLE_START:
                break;
            
            case MAIN_BTN_CYCLE_HOLD:
                break;
            
            case MAIN_BTN_CYCLE_ABORT:
                break;
            
            case MAIN_BTN_POWEROFF:
                break;
            
            case MAIN_BTN_SAVE_TO_G10:
                break;
            
            case MAIN_BTN_SAVE_TO_G92:
                break;
            
            case MAIN_BTN_NOTIFICATIONS:
                break;
            
            case MAIN_BTN_SETTINGS:
                break;
            
            case MAIN_BTN_GO_HOME:
                break;
            
            case MAIN_BTN_DO_PROBE:
                break;
            
            case MAIN_BTN_JOG:
                break;
            
            case MAIN_BTN_FILEMGR:
                break;
            
            default:
                break;
        }
    }
}
