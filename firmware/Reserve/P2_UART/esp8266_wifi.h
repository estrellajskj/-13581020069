#ifndef __ESP8266_WIFI_H__
#define __ESP8266_WIFI_H__


#define ESP8266_BAUDRATE       115200
#define ESP8266_DATA_MAX_LEN   100

#define ESP8266_IP             "192.168.4.1"    /*本机ip*/
#define ESP8266_GAT            "192.168.4.1"    /*网关*/
#define ESP8266_MASK           "255.255.255.0"  /*掩码*/
#define ESP8266_SERVER_PORT              8080   /*服务器端口*/

typedef struct __c_esp8266_wifi c_esp8266_wifi;

typedef struct __c_esp8266_wifi
{
    void* this;

    int (*send)           (const c_esp8266_wifi* this,const u8* data ,u16 data_len);
    int (*set_callback)   (const c_esp8266_wifi* this,void* param,int (*callback)(void* param,const u8* data,u16 data_len));
}c_esp8266_wifi;

c_esp8266_wifi esp8266_wifi_create(u8 uart_id,const char* name,const char* password);


#endif

