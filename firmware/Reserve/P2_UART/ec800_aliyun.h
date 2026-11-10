

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
    char o_client_id[CLIENT_ID_MAX_LEN];
    char o_passwd[PASSWD_MAX_LEN];
    
    char o_product_key[PRO_KEY_MAX_LEN];
    char o_device_name[NAME_MAX_LEN];
	
	char sub_set[CLIENT_ID_MAX_LEN];	/*订阅设置->小程序/app，对应云产品流转里4g设备sqlite语句后半部分,格式 /k1h3jnBXE0L/4g_device/user/4gDUAN */
	char sub_get[CLIENT_ID_MAX_LEN];	/*订阅小程序->设置，对应晕产品流转中app或小程序的转发数据中的后半部分，格式 /sys/k1h3jnBXE0L/4g_device/thing/service/property/set */
	char send_cmd[CLIENT_ID_MAX_LEN];	/*发送数据,，对应云产品流转里4g设备sqlite语句后半部分,格式 /k1h3jnBXE0L/4g_device/user/4gDUAN */
	char send_cmd_mode[CLIENT_ID_MAX_LEN];	/*发送数据给物模型展示数据,对应产品里的topic类属性上报的第一条，注意修改设备名，格式为 /sys/k24kySHOVX1/esp8266DUAN/thing/event/property/post */
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

