#ifndef HW_TIMERS_H
#define HW_TIMERS_H

#include <stm32f4xx_hal.h>

extern TIM_HandleTypeDef step_timer_handle;
extern TIM_HandleTypeDef unstep_timer_handle;
extern TIM_HandleTypeDef pwm3_timer_handle;
extern TIM_HandleTypeDef pwm12_timer_handle;
extern TIM_HandleTypeDef tft_backlight_timer_handle;

void Init_Stepper_Timer2(void);
void Init_Unstep_Timer6(void);
void Init_Pwm3_Timer4(void);
void Init_Pwm12_Timer9(void);
void Init_BacklightPwm_Timer12(void);


#endif
