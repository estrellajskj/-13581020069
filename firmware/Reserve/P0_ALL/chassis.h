/*****************************************************************************
* Copyright:
* File name: chassis.c
* Description: 底盘硬件 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2021/12/19
*****************************************************************************/
#ifndef __CHASSIS_H__
#define __CHASSIS_H__


//#include "sys.h"

#define LEFT_WHEEL_REVERSE  0  /*左轮反向*/
#define RIGHT_WHEEL_RIVERSE 1  /*右轮反向*/
#define PWM_REVIERSE        0  /*PWM波反向*/

#define CHASSIS_STOP        0  
#define CHASSIS_GO          1
#define CHASSIS_BACK        2
#define CHASSIS_LEFT        3
#define CHASSIS_RIGHT       4

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
	GPIO_TypeDef* stby_gpio, uint32_t stby_pin);

/*************************************************
* Function: 底盘工作
* Description: chassis_work
* Input : <type>  工作类型
*         <speed> 速度  0~100
* Output: 无
* Return:
* Others: 无
*************************************************/
int chassis_work(u8 type,u8 speed);

#endif


