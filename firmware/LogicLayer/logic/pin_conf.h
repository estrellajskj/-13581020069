#ifndef __PIN_CONF_H
#define __PIN_CONF_H

/* GPIOs follow the supplied Version 1 schematic, not the old demo. */
#define DHT11_GRP GPIOA
#define DHT11_PIN GPIO_PIN_4
#define GM_ADC MY_ADC_0
#define MQ138_ADC MY_ADC_1
#define DS3231_SDA_GRP GPIOB
#define DS3231_SDA_PIN GPIO_PIN_7
#define DS3231_SCL_GRP GPIOB
#define DS3231_SCL_PIN GPIO_PIN_6
#define BUZZER_GRP GPIOB
#define BUZZER_PIN GPIO_PIN_0
#define LED_GRP GPIOB
#define LED_PIN GPIO_PIN_1
#define SCREEN_UART_ID MY_UART_1
#define SCREEN_BAUD 9600U
#define WIFI_UART_ID MY_UART_2
#define WIFI_BAUD 115200U
#define CO2_UART_ID MY_UART_3

/* SY-CO2-28BMI specification A2.0, pages 6 and 10: FF 86, 9-byte checksum. */
#define CO2_PROTOCOL 1 /* 0: receive diagnostics only; 1: SY-CO2-28BMI */
#define CO2_BAUD 9600U
#define CO2_STALE_MS 10000U
#define CO2_ALARM_PPM 1300U
#define CO2_CLEAR_PPM 1250U

/* Fill three measured pairs, sorted by increasing ADC mV, before enabling.
 * Concentrations are ug/m3 (70 = 0.070 mg/m3); either response direction works.
 * Zeros deliberately keep the unmeasured table invalid. */
#define MQ138_CALIBRATED 0
#define MQ138_WARMUP_MS 180000U /* Provisional; follow the seller's manual. */
#define MQ138_CAL_MV_0 0
#define MQ138_CAL_MV_1 0
#define MQ138_CAL_MV_2 0
#define MQ138_CAL_UG_0 0
#define MQ138_CAL_UG_1 0
#define MQ138_CAL_UG_2 0
#define HCHO_ALARM_UG_M3 70
#define HCHO_CLEAR_UG_M3 65

/* User-set ADC threshold: below 500 mV; not an HCHO concentration calibration. */
#define MQ138_RAW_ALARM_ENABLED 1
#define MQ138_RAW_ALARM_ON_MV 500
#define MQ138_RAW_ALARM_OFF_MV 800
#define MQ138_RAW_CONFIRM_SAMPLES 2U

/* User enabled output; the original Q2 base still requires a series resistor. */
#define BUZZER_OUTPUT_ENABLED 1

/* Brightness is relative, not calibrated lux. Swap endpoints if needed. */
#define LIGHT_DARK_MV 2800
#define LIGHT_BRIGHT_MV 200
#define SCREEN_MIN_BRIGHTNESS 10

/* ASCII/URL-encoded city, e.g. beijing. Empty credentials keep Wi-Fi off. */
#define WIFI_NAME ""
#define WIFI_PASSWORD ""
#define WEATHER_CITY "hangzhou"
#define WEATHER_API_KEY ""
#define WEATHER_HOST "api.seniverse.com"
#define WEATHER_PORT 80
#define WEATHER_REFRESH_MS 600000U
#define WEATHER_STALE_MS 1800000U
#define SNTP_TIMEZONE 8
#define SNTP_SERVER "ntp.aliyun.com"

#endif
