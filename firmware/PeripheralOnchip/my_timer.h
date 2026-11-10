#ifndef __MY_TIMER_H__
#define __MY_TIMER_H__



#define MY_TIMER_1  0
#define MY_TIMER_2  1
#define MY_TIMER_3  2
#define MY_TIMER_4  3

#define MY_TIMER_CH_1  TIM_CHANNEL_1
#define MY_TIMER_CH_2  TIM_CHANNEL_2
#define MY_TIMER_CH_3  TIM_CHANNEL_3
#define MY_TIMER_CH_4  TIM_CHANNEL_4

typedef struct __C_TIMER
{
    int (*init)   (u8 timer,u16 arr,u16 psc);
    int (*init_ch)(u8 timer,u32 ch);
}c_timer;

extern const c_timer my_timer;

#endif

