#ifndef __NBIOT_ONENET_H__
#define __NBIOT_ONENET_H__

#define NBIOT_BAUDRATE           9600
#define NBIOT_DATA_MAX_LEN          256                       /*esp消息最大长度*/

#define WIFI_NAME_MAX_LEN        32  /*wifi名最大长度*/
#define WIFI_KEY_MAX_LEN         32  /*密码最大长度*/

#define ONENET_ID_MAX_LEN       32  /*id最大长度*/
#define ONENET_NAME_MAX_LEN     32  /*最大设备名长度*/
#define ONENET_PASSWD_MAX_LEN  256  /*最大密码长度*/

#define ONENET_KEY_NAME_MAX_LEN  32 /*key name 最大长度*/
#define ONENET_KEY_VALUE_MAX_LEN 32 /*key value 最大长度*/
#define ONENET_PUSH_MAX_LEN 	 256 /*key value 最大长度*/

/*非最后一条语句使用以下格式*/
#define ONENET_FLOAT	 "\"%s\":{\"value\":%.1f},"
#define ONENET_INT	     "\"%s\":{\"value\":%d},"
#define ONENET_STR	     "\"%s\":{\"value\":\"%s\"},"

/*最后一条语句必须使用以下格式*/
#define ONENET_FLOAT_END	  "\"%s\":{\"value\":%.1f}"
#define ONENET_INT_END 	      "\"%s\":{\"value\":%d}"
#define ONENET_STR_END	     "\"%s\":{\"value\":\"%s\"}"

typedef struct __C_BC260Y c_bc260y;

typedef struct __ONENET_INFORM
{
    char o_product_id  [ONENET_ID_MAX_LEN]     ;  /*产品id*/
    char o_driver_name [ONENET_NAME_MAX_LEN]   ;  /*设备名称*/
    char o_passwd      [ONENET_PASSWD_MAX_LEN] ;  /*密钥  */
}onenet_inform;

typedef struct __ONENET_MSG
{
	char o_data[ONENET_PUSH_MAX_LEN];
}onenet_msg;

typedef struct __C_BC260Y
{
    void* this;
    
    int (*onenet_start)     (const c_bc260y* this, const onenet_inform* onenet);
    int (*onenet_send)      (const c_bc260y* this, const char* format, ...);
    int (*onenet_callback)  (const c_bc260y* this, void* param,int (*callback)(void* param,const char* msg));
    
    
}c_bc260y;

c_bc260y bc260y_create(u8 uart_id);

#endif
