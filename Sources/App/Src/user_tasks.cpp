#include <stm32f4xx_hal.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "event_groups.h"

#include "task_settings.h"
#include "user_tasks.h"
#include "settings_manager.h"

#include "ui_task.h"
#include "usb_task.h"
#include "gcode_parsing_task.h"
#include "disk_task.h"
#include "serial_task.h"

#include "pins.h"
#include "gpio.h"


GLOBAL_TASK_INFO_STRUCT_TYPE global_task_info = {0};

static void delayed_startup_callback( TimerHandle_t xTimer );
static void steppers_idle_timeout_callback( TimerHandle_t xTimer );
static void read_user_buttons_callback(TimerHandle_t xTimer);

void Init_UserTasks_and_Objects(void)
{
    xTimerHandle delayed_startup_timer;
    xTimerHandle user_btn_read_timer;
    
    uint32_t stepper_idle_timeout;
    
    global_task_info.gcode_parser_ptr = new GCodeParser();
    global_task_info.planner_ptr = new Planner();
    global_task_info.conveyor_ptr = new Conveyor();
    global_task_info.step_ticker_ptr = new StepTicker();
    
    global_task_info.gcode_str_queue_handle = xQueueCreate(GCODE_STRING_QUEUE_ITEM_COUNT, sizeof(char*));
    global_task_info.gcode_result_queue_handle = xQueueCreate(GCODE_STRING_QUEUE_ITEM_COUNT, sizeof(int32_t));
    global_task_info.input_events_handle = xEventGroupCreate();
    
    stepper_idle_timeout = Settings_Manager::GetIdleLockTime_secs();
    
    global_task_info.stepper_idle_timer = xTimerCreate(NULL, pdMS_TO_TICKS(stepper_idle_timeout * 1000), 
                                                       pdFALSE, NULL, (TimerCallbackFunction_t)steppers_idle_timeout_callback);
    
    // Associate different parts
    global_task_info.step_ticker_ptr->Associate_Conveyor(global_task_info.conveyor_ptr);
    global_task_info.planner_ptr->AssociateConveyor(global_task_info.conveyor_ptr);
    global_task_info.gcode_parser_ptr->AssociatePlanner(global_task_info.planner_ptr);
    
    global_task_info.conveyor_ptr->start();
    global_task_info.step_ticker_ptr->start();
    
    // Reset stepper drivers
    global_task_info.step_ticker_ptr->ResetStepperDrivers(true);
    
    // Create delayed startup timer
    delayed_startup_timer = xTimerCreate(NULL, 1, pdFALSE, NULL, (TimerCallbackFunction_t)delayed_startup_callback);
    xTimerStart(delayed_startup_timer, 0);
    
    user_btn_read_timer = xTimerCreate(NULL, 50, pdTRUE, NULL, (TimerCallbackFunction_t)read_user_buttons_callback);
    xTimerStart(user_btn_read_timer, 0);
    
//  xTaskCreate(UI_BootTask_Entry, "UIBOOT", UI_BOOT_TASK_STACK_SIZE, NULL, UI_BOOT_TASK_PRIORITY, NULL);
//  xTaskCreate(USBTask_Entry, "USBTASK", USB_TASK_STACK_SIZE, NULL, USB_TASK_PRIORITY, &usb_task_handle);
//  xTaskCreate(DiskTask_Entry, "DSKTASK", DISK_TASK_STACK_SIZE, NULL, DISK_TASK_PRIORITY, &disk_task_handle);

    xTaskCreate(GCodeParsingTask_Entry, "GCODE", GCODE_TASK_STACK_SIZE, (void*)&global_task_info, GCODE_TASK_PRIORITY, &gcode_task_handle);
    xTaskCreate(SerialTask_Entry, "SERIAL", SERIAL_TASK_STACK_SIZE, (void*)&global_task_info, SERIAL_TASK_PRIORITY, &serial_task_handle);
    
    vTaskStartScheduler();
}

static void delayed_startup_callback(TimerHandle_t xTimer)
{
    // Release stepper drivers reset
    global_task_info.step_ticker_ptr->ResetStepperDrivers(false);
    
    // Signal the information on this structure (global_task_info) as ready to use
    global_task_info.ready_to_use = true;
    
    // Release calling timer
    xTimerDelete(xTimer, 0);
}

static void steppers_idle_timeout_callback(TimerHandle_t xTimer)
{
    // Disable stepper motors after a while of idling
    global_task_info.step_ticker_ptr->EnableStepperDrivers(false);
}

static void read_user_buttons_callback(TimerHandle_t xTimer)
{
    static uint32_t debounce_cntrs[3];
    
    if (global_task_info.ready_to_use == false)
        return;
    
    // Read start button
    if ((BTN_START_GPIO_Port->IDR & BTN_START_Pin) == 0)
    {
        if (debounce_cntrs[0] < 2) // ~100 ms 
            debounce_cntrs[0]++;
        else
        {
            // Assume the button was pressed
            xEventGroupSetBits(global_task_info.input_events_handle, BTN_START_EVENT);
            debounce_cntrs[0] = 0;
        }
    }
    
    if ((BTN_HOLD_GPIO_Port->IDR & BTN_HOLD_Pin) == 0)
    {
        if (debounce_cntrs[1] < 2) // ~100 ms 
            debounce_cntrs[1]++;
        else
        {
            // Assume the button was pressed
            xEventGroupSetBits(global_task_info.input_events_handle, BTN_HOLD_EVENT);
            debounce_cntrs[1] = 0;
        }
    }
    
    if ((BTN_ABORT_GPIO_Port->IDR & BTN_ABORT_Pin) == 0)
    {
        if (debounce_cntrs[2] < 2) // ~100 ms 
            debounce_cntrs[2]++;
        else
        {
            // Assume the button was pressed
            xEventGroupSetBits(global_task_info.input_events_handle, BTN_ABORT_EVENT);
            debounce_cntrs[2] = 0;
        }
    }
}
