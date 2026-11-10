/*****************************************************************************
* Copyright:
* File name: chassis.c
* Description: 底盘硬件 函数实现
* Author: 许少
* Version: V1.0
* Date: 2021/12/19
*****************************************************************************/

#include "driver_conf.h"
//#include "string.h"
//#include "log/log.h"

#define MODULE_NAME       "chassis"
#define MAX_SPEED         100         /*最大速度*/

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME



TIM_HandleTypeDef 	TIM2_Handler   ;    //定时器句柄 
TIM_OC_InitTypeDef 	TIM2_CHHandler;	//定时器2通道句柄

c_switch AIN1 = {0}, AIN2 = {0}, BIN1 = {0}, BIN2 = {0}, STBY = {0};

static void left(void)
{
	STBY.set(&STBY,SWITCH_HIGHT);	
	BIN1.set(&BIN1,SWITCH_LOW);
	BIN2.set(&BIN2,SWITCH_HIGHT);
	AIN1.set(&AIN1,SWITCH_HIGHT);
	AIN2.set(&AIN2,SWITCH_LOW);
}

static void right(void)
{
	STBY.set(&STBY,SWITCH_HIGHT);	
	BIN1.set(&BIN1,SWITCH_HIGHT);
	BIN2.set(&BIN2,SWITCH_LOW);
	AIN1.set(&AIN1,SWITCH_LOW);
	AIN2.set(&AIN2,SWITCH_HIGHT);
}
static void go(void)
{
	STBY.set(&STBY,SWITCH_HIGHT);	
	BIN1.set(&BIN1,SWITCH_LOW);
	BIN2.set(&BIN2,SWITCH_HIGHT);
	AIN1.set(&AIN1,SWITCH_LOW);
	AIN2.set(&AIN2,SWITCH_HIGHT);

}

static void stop(void)
{
	STBY.set(&STBY,SWITCH_LOW);	
	BIN1.set(&BIN1,SWITCH_LOW);
	BIN2.set(&BIN2,SWITCH_LOW);
	AIN1.set(&AIN1,SWITCH_LOW);
	AIN2.set(&AIN2,SWITCH_LOW);
}
static void back(void)
{
	STBY.set(&STBY,SWITCH_HIGHT);		
	BIN1.set(&BIN1,SWITCH_HIGHT);
	BIN2.set(&BIN2,SWITCH_LOW);
	AIN1.set(&AIN1,SWITCH_HIGHT);
	AIN2.set(&AIN2,SWITCH_LOW);
}

/*************************************************
* Function: 底盘初始化
* Description: chassis_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
int chassis_init(sys_timer pwa_timer, sys_timer_ch pwa_ch, sys_timer pwb_timer, sys_timer_ch pwb_ch, 
	GPIO_TypeDef* ain1_gpio, uint32_t ain1_pin, GPIO_TypeDef* ain2_gpio, uint32_t ain2_pin,
	GPIO_TypeDef* bin1_gpio, uint32_t bin1_pin, GPIO_TypeDef* bin2_gpio, uint32_t bin2_pin, 
	GPIO_TypeDef* stby_gpio, uint32_t stby_pin)
{
	my_pwm.time_init (pwa_timer, 71, 1000);
	my_pwm.ch_init   (pwa_timer, pwa_ch);
	my_pwm.time_init (pwb_timer, 71, 1000);
    my_pwm.ch_init   (pwb_timer, pwb_ch);

	AIN1 = switch_create(ain1_gpio, ain1_pin);
	AIN2 = switch_create(ain2_gpio, ain2_pin);
	BIN1 = switch_create(bin1_gpio, bin1_pin);	
	BIN2 = switch_create(bin2_gpio, bin2_pin);
	STBY = switch_create(stby_gpio, stby_pin);

	my_pwm.ch_set(pwa_timer, pwa_ch, 400);
	my_pwm.ch_set(pwb_timer, pwb_ch, 400);
	
	chassis_work(CHASSIS_STOP,0);  /*默认停车*/
	
	return E_OK;
}

/*************************************************
* Function: 底盘工作
* Description: chassis_work
* Input : <type>  工作类型
*         <speed> 速度  0~100
* Output: 无
* Return:
* Others: 无
*************************************************/
int chassis_work(u8 type, u8 speed)
{
	my_pwm.ch_set(SYS_TIME_2,SYS_TIME_CH2, speed * 5.5);
	my_pwm.ch_set(SYS_TIME_2,SYS_TIME_CH1, speed * 5.5);
	if(speed > MAX_SPEED)
	{
		log_error("Speed too fast.");
		return E_PARAM;
	}
	
	switch(type)
	{
		/*停车*/
		case CHASSIS_STOP:
		{
			stop();
			break;
		}
		
		/*倒车*/
		case CHASSIS_BACK:
		{
			back();
			break;
		}
		
		/*直走*/
		case CHASSIS_GO:
		{
			go();
			break;
		}
		
		/*左转*/
		case CHASSIS_LEFT:
		{
			left();
			break;
		}
		
		/*右转*/
		case CHASSIS_RIGHT:
		{
			right();
			break;
		}
		
		default:
		{
			stop();
			return E_PARAM;
		}
	}
	
	
	return E_OK;
}
