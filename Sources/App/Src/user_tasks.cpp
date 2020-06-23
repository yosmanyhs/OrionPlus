#include <stm32f4xx_hal.h>

#include "FreeRTOS.h"
#include "task.h"

#include "task_settings.h"
#include "user_tasks.h"
#include "settings_manager.h"

#include "ui_task.h"
#include "usb_task.h"
#include "gcode_parsing_task.h"
#include "disk_task.h"
#include "serial_task.h"


GLOBAL_TASK_INFO_STRUCT_TYPE global_task_info = {0};

void Init_UserTasks_and_Objects(void)
{   
    global_task_info.gcode_parser_ptr = new GCodeParser();
    global_task_info.planner_ptr = new Planner();
    global_task_info.conveyor_ptr = new Conveyor();
    global_task_info.step_ticker_ptr = new StepTicker();
    
    global_task_info.gcode_str_queue_handle = xQueueCreate(GCODE_STRING_QUEUE_ITEM_COUNT, sizeof(char*));
    global_task_info.gcode_result_queue_handle = xQueueCreate(GCODE_STRING_QUEUE_ITEM_COUNT, sizeof(int32_t));
    global_task_info.input_events_handle = xEventGroupCreate();
    
    // Associate different parts
    global_task_info.step_ticker_ptr->Associate_Conveyor(global_task_info.conveyor_ptr);
    global_task_info.planner_ptr->AssociateConveyor(global_task_info.conveyor_ptr);
    global_task_info.gcode_parser_ptr->AssociatePlanner(global_task_info.planner_ptr);
    
    global_task_info.conveyor_ptr->start();
    global_task_info.step_ticker_ptr->start();
    
    global_task_info.ready_to_use = true;
    
//  xTaskCreate(UI_BootTask_Entry, "UIBOOT", UI_BOOT_TASK_STACK_SIZE, NULL, UI_BOOT_TASK_PRIORITY, NULL);
//  xTaskCreate(USBTask_Entry, "USBTASK", USB_TASK_STACK_SIZE, NULL, USB_TASK_PRIORITY, &usb_task_handle);
//  xTaskCreate(DiskTask_Entry, "DSKTASK", DISK_TASK_STACK_SIZE, NULL, DISK_TASK_PRIORITY, &disk_task_handle);

    xTaskCreate(GCodeParsingTask_Entry, "GCODE", GCODE_TASK_STACK_SIZE, (void*)&global_task_info, GCODE_TASK_PRIORITY, &gcode_task_handle);
    xTaskCreate(SerialTask_Entry, "SERIAL", SERIAL_TASK_STACK_SIZE, (void*)&global_task_info, SERIAL_TASK_PRIORITY, &serial_task_handle);
    
    vTaskStartScheduler();
}

