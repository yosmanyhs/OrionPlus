#ifndef GPIO_H
#define GPIO_H

#define LIMIT_X_MIN_EVENT       (1 << 0)
#define LIMIT_X_MAX_EVENT       (1 << 1)
#define LIMIT_Y_MIN_EVENT       (1 << 2)
#define LIMIT_Y_MAX_EVENT       (1 << 3)
#define LIMIT_Z_MIN_EVENT       (1 << 4)
#define LIMIT_Z_MAX_EVENT       (1 << 5)

#define PROBE_TOUCH_EVENT       (1 << 6)

#define BTN_START_EVENT         (1 << 7)
#define BTN_HOLD_EVENT          (1 << 8)
#define BTN_ABORT_EVENT         (1 << 9)

#define TOUCH_SCREEN_EVENT      (1 << 10)
#define STEPPER_FAULT_EVENT     (1 << 11)

void Init_GPIO_Pins(void);

#endif

