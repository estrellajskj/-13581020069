#ifndef __DHT11_H__
#define __DHT11_H__

#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx_hal.h"  // 根据实际使用的STM32系列调整头文件

/* 错误码定义 */
#define E_OK     0
//#define E_NULL   1
//#define E_ERROR  2

typedef struct __DHT11 c_dht11;

typedef struct __DHT11
{
    void* this;
    
    /************************************************* 
    * Function: get 
    * Description: 获取温湿度
    * Input : <this>  dht11对象
    * Output: <temp>   当前温度 单位度（带一位小数）
    *         <humi>   当前湿度 百分比（带一位小数）
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         float temp = 0.0f, humi = 0.0f;
    *
    *         ret = dht11.get(&dht11, &temp, &humi);
    *         if(E_OK != ret)
    *         {
    *             log_error("dht11 get failed.");
    *         }
    *************************************************/  
    int (*get)(const c_dht11* this, float* temp, float* humi);
} c_dht11;

/************************************************* 
* Function: dht11_create 
* Description: 创建一个dht11对象
* Input : <gpio>  数据线的gpio分组
*         <pin>   数据线的pin分组
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_dht11  dht11  = {0};
*         
*         dht11 = dht11_create(GPIOA, GPIO_PIN_11);
*         if(NULL == dht11.this)
*         {
*             log_error("dht11 creat failed.");
*         }
*************************************************/
c_dht11 dht11_create(GPIO_TypeDef* gpio, uint32_t pin);

#endif



