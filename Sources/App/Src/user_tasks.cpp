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

#include "MachineCore.h"

// System Global Controller class
MachineCore * machine;

void Init_UserTasks_and_Objects(void)
{   
    machine = new MachineCore();
    
    machine->Initialize();   
    
//    xTaskCreate(UI_BootTask_Entry, "UIBOOT", UI_BOOT_TASK_STACK_SIZE, (void*)machine, UI_BOOT_TASK_PRIORITY, NULL);
    xTaskCreate(USBTask_Entry, "USBTASK", USB_TASK_STACK_SIZE, NULL, USB_TASK_PRIORITY, &usb_task_handle);
//  xTaskCreate(DiskTask_Entry, "DSKTASK", DISK_TASK_STACK_SIZE, NULL, DISK_TASK_PRIORITY, &disk_task_handle);

//    xTaskCreate(GCodeParsingTask_Entry, "GCODE", GCODE_TASK_STACK_SIZE, (void*)machine, GCODE_TASK_PRIORITY, &gcode_task_handle);
    xTaskCreate(SerialTask_Entry, "SERIAL", SERIAL_TASK_STACK_SIZE, (void*)machine, SERIAL_TASK_PRIORITY, &serial_task_handle);
    
    vTaskStartScheduler();
}
