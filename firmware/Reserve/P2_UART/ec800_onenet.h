

#ifndef __EC800_ALIYUN_H__
#define __EC800_ALIYUN_H__

#define EC800_BAUDRATE           115200

#define CLIENT_ID_MAX_LEN   256  /*id最大长度*/
#define PRO_KEY_MAX_LEN      128  
#define NAME_MAX_LEN         128
#define PASSWD_MAX_LEN      256  /*最大密码长度*/
#define EC800_PUSH_MAX_LEN  300
#define EC800_DATA_MAX_LEN  300

typedef struct __C_EC800 c_ec800;

typedef struct __MQTT_INFORM
{
    char o_passwd[PASSWD_MAX_LEN];	//设备密钥:
    char o_product_key[PRO_KEY_MAX_LEN];	//产品id
    char o_device_name[NAME_MAX_LEN];	//设备id
}mqtt_inform;

typedef struct __C_EC800
{
    void* this;
    
    int (*ec800_start)     (const c_ec800* this, const mqtt_inform* inform);
    int (*ec800_send)      (const c_ec800* this, const char* format, ...);
    int (*ec800_callback)  (const c_ec800* this, void* param,int (*callback)(void* param,const char* msg));
    
    
}c_ec800;

c_ec800 ec800_create(u8 uart_id);

#endif

