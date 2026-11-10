#ifndef __PIN_CONF_H
#define __PIN_CONF_H

#define DHT11_GRP GPIOA    //dht11 GPIO分组
#define DHT11_PIN GPIO_PIN_1   //dht11引脚

#define MQ135_ADC MY_ADC_4    //空气质量 ADC

#define DS3231_SDA_GRP GPIOB    //DS3231 sda GPIO分组
#define DS3231_SDA_PIN GPIO_PIN_7   //DS3231 sda引脚
#define DS3231_SCL_GRP GPIOB    //DS3231 sda GPIO分组
#define DS3231_SCL_PIN GPIO_PIN_6   //DS3231 sda引脚

#define LED_GRP GPIOB    //led GPIO分组
#define LED_PIN GPIO_PIN_2   //led引脚


#define BUZZER_GRP GPIOC    //蜂鸣器 GPIO分组
#define BUZZER_PIN GPIO_PIN_15   //蜂鸣器引脚


#define GM_ADC MY_ADC_1    //光敏 ADC

#define ESP8266_TCP_UART_ID MY_UART_2    //esp328266(tcp)串口ID

#define WIFI_NAME "1"    //wifi名称
#define WIFI_PASSWORD "123456789"    //wifi密码

#endif

