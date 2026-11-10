#ifndef __SYS_CFG_H__
#define __SYS_CFG_H__

#include "FreeRTOSConfig.h"

/*任务优先级*/
/*
freertos v9.0.0 优先级值越大 优先级越高
相同优先级采用时间片轮转原则
高优先级任务必须进 vTaskDelay() 才会执行低优先级任务
*/
typedef enum __TASK_PRO
{
    START_TASK_PRO = 5           ,    /*启动任务*/
                                 
	MY_APP_1_PRO                 ,
	MY_APP_2_PRO                 ,
	MY_APP_3_PRO                 ,
                                 
    TRACKING_CAR_PRO             ,
	INTERFACE_TASK_PRO           ,    /*界面任务        */
	DIS_DETECTION_PRO            ,                      
                                                        
    KEY_BOARD_TASK_PRO           ,    /*键盘任务        */
	MATRIX_KEY_BOARD_TASK_PRO    ,	  /*矩阵键盘任务    */  
                                                        
    MAX30102_TASK_PRO            ,    /*心率任务        */
    MPU6050_TASK_PRO             ,    /*mpu6050任务     */
    RC522_TASK_PRO               ,    /*rc522 任务      */
                                 
    GPS_TASK_PRO                 ,    /*gps任务         */
    ESP8266_WIFI_TASK_PRO        ,    /*wifi   任务     */
    ESP8266_ONENET_CON_TASK_PRO  ,    /*onenet 连接任务 */
    ESP8266_ONENET_RECV_TASK_PRO ,    /*onenet 接收任务 */
	EC800_ALIYUN_SEND_TASK_PRO		,	/*ec800发送*/
	EC800_ALIYUN_RECV_TASK_PRO		,	/*ec800接收*/
	EC800_ONENET_SEND_TASK_PRO		,	/*ec800发送*/
	EC800_ONENET_RECV_TASK_PRO		,	/*ec800接收*/
    SIM800_TASK_PRO              ,    /*sim800任务      */
    CUT_TASK_PRO                 ,    /*数据裁剪 任务   */
                                                        
    UART_1_RECV_TASK_PRO         ,    /*串口1接收任务   */
	UART_1_SEND_TASK_PRO         ,    /*串口1发送任务   */
    UART_2_RECV_TASK_PRO         ,    /*串口2接收任务   */
    UART_2_SEND_TASK_PRO         ,    /*串口2发送任务   */
    UART_3_RECV_TASK_PRO         ,    /*串口3接收任务   */
    UART_3_SEND_TASK_PRO         ,    /*串口3发送任务   */
}task_pro;

/*中断优先级*/
typedef enum __INTERRUPT_PRO
{
    HIGHEST_PRO = 5      ,
    EXTI_0_PRO      = 5  ,
    EXTI_1_PRO      = 5  ,
    EXTI_2_PRO      = 5  ,
    EXTI_3_PRO      = 5  ,
    EXTI_4_PRO      = 5  ,
    EXTI_5_9_PRO    = 5  ,
    EXTI_10_15_PRO  = 5  ,
    DMA_1_PRO       = 6  ,
    DMA_2_PRO       = 6  ,
    DMA_3_PRO       = 6  ,
    DMA_4_PRO       = 6  ,
    DMA_5_PRO       = 6  ,
    DMA_6_PRO       = 6  ,
    DMA_7_PRO       = 6  ,
    TIM_1_UP_PRO    = 7  ,
    TIM_2_UP_PRO    = 7  ,
    TIM_3_UP_PRO    = 7  ,
    TIM_4_UP_PRO    = 7  ,
    UART_1_RECV_PRO      ,
    UART_2_RECV_PRO      ,
    UART_3_RECV_PRO      ,
    UART_1_SEND_DMA_PRO  ,
    UART_2_SEND_DMA_PRO  ,
    UART_3_SEND_DMA_PRO  ,
    
    TIME_1_UP_PRO        ,
    
    LOWEST_PRO = 15      ,
}interrupt_pro;

#define UART_1_SEND_TASK_STK            256    /*串口1发送任务栈*/
#define UART_1_RECV_TASK_STK            256    /*串口1接收任务栈*/
#define UART_2_SEND_TASK_STK            256    /*串口2发送任务栈*/
#define UART_2_RECV_TASK_STK            256    /*串口2接收任务栈*/
#define UART_3_SEND_TASK_STK            256    /*串口3发送任务栈*/
#define UART_3_RECV_TASK_STK            256    /*串口3接收任务栈*/
#define START_TASK_STK                  64     /*启动任务栈*/
#define TRACKING_CAR_STK                128    /*循迹车*/
#define INTERFACE_TASK_STK              128    /*界面任务栈*/
#define DIS_DETECTION_STK               64
#define MY_APP_1_STK                    1024
#define MY_APP_2_STK                    1024
#define MY_APP_3_STK                    1024
#define KEY_BOARD_TASK_STK              128   /*键盘任务*/
#define MATRIX_KEY_BOARD_TASK_STK		128	  /*矩阵键盘任务*/

#define GPS_TASK_STK                    128   /*gps任务堆栈*/
#define MPU6050_TASK_STK                256   /*mpu6050任务*/
#define CUT_TASK_STK                    300   /*数据裁剪任务堆栈*/
#define MAX30102_TASK_STK               512   /*心率任务堆栈*/
#define RC522_TASK_STK                  128   /*rc522 任务堆栈*/
#define SIM800_TASK_STK                 256
#define ESP8266_WIFI_TASK_STK           256 /*wifi   任务*/
#define ESP8266_ONENET_CON_TASK_STK     512 /*onenet 连接任务*/
#define ESP8266_ONENET_RECV_TASK_STK    256 /*onenet 接收任务*/

#define	EC800_ONENET_SEND_TASK_STK 1024
#define EC800_ONENET_RECV_TASK_STK 512

#define	EC800_ALIYUN_SEND_TASK_STK 1024
#define EC800_ALIYUN_RECV_TASK_STK 512

#endif



