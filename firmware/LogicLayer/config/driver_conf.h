#ifndef __DRIVER_conf_H
#define __DRIVER_conf_H
#ifdef __cplusplus
	extern "C" {
#endif

#include "help_conf.h"
#include "onchip_conf.h"

#define DRIVER_DHT11_ENABLED
#include "dht11.h"

#define DRIVER_DS3231_ENABLED
#include "ds3231.h"

#define DRIVER_SWITCH_ENABLED
#include "switch.h"

#define DRIVER_ESP8266_WIFI_ENABLED
#include "esp8266_wifi.h"


#ifdef __cplusplus
}
#endif

#endif

