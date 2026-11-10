#ifndef __MY_PWM_H__
#define __MY_PWM_H__

#ifndef ONCHIP_PWM_ENABLED
	# warning "Please enable onchip moudle - <pwm>."
#endif

typedef struct __MY_PWM
{
    int (*time_init) (sys_timer timer,u16 psc,u16 arr);
    int (*ch_init)   (sys_timer timer,sys_timer_ch ch);
    int (*ch_set)    (sys_timer timer,sys_timer_ch ch,u16 duty_cycle);
}c_my_pwm;

extern const c_my_pwm my_pwm;

#endif

