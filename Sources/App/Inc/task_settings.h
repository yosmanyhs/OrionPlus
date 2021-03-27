#ifndef TASK_PRIORITIES_H
#define TASK_PRIORITIES_H

#include "FreeRTOSConfig.h"

#define UI_BOOT_TASK_PRIORITY       (configMAX_PRIORITIES - 3)
#define UI_BOOT_TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE * 3)

#define UI_TASK_PRIORITY            (configMAX_PRIORITIES - 4)
#define UI_TASK_STACK_SIZE          (configMINIMAL_STACK_SIZE * 8)

#define USB_TASK_PRIORITY           (configMAX_PRIORITIES - 2)
#define USB_TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE * 3)

#define GCODE_TASK_PRIORITY         (configMAX_PRIORITIES - 4)
#define GCODE_TASK_STACK_SIZE       (configMINIMAL_STACK_SIZE * 2)

#define LISTENER_TASK_PRIORITY      (configMAX_PRIORITIES - 4)
#define LISTENER_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE * 1)

#define DISK_TASK_PRIORITY          (configMAX_PRIORITIES - 4)
#define DISK_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE * 1)

#define SERIAL_TASK_PRIORITY        (configMAX_PRIORITIES - 4)
#define SERIAL_TASK_STACK_SIZE      (configMINIMAL_STACK_SIZE * 2)

#define SAFETY_TASK_PRIORITY        (configMAX_PRIORITIES - 3)
#define SAFETY_TASK_STACK_SIZE      (configMINIMAL_STACK_SIZE * 1)

#endif
