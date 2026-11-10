#ifndef __ESP8266_H__
#define __ESP8266_H__

#define ESP8266_BAUDRATE           115200
#define ESP8266_DATA_MAX_LEN          256     /*esp消息最大长度*/
#define ESP8266_SEND_DATA_MAX_LEN   ESP8266_DATA_MAX_LEN - 10

#define uint8_t u8
#define uint16_t u16

#define ESP8266_WIFI_READY          0x0001  /*wifi就绪事件*/
#define ESP8266_WIFI_STA_CONNECT    0x0002  /*sta连接事件*/
#define ESP8266_WIFI_STA_DISCONNECT 0x0003  /*sta断连事件*/
#define ESP8266_WIFI_IP_GET         0x0004  /*获取ip事件*/
#define ESP8266_WIFI_AP_CONNECT     0x0008  /*ap 连接事件*/
#define ESP8266_WIFI_AP_DISCONNECT  0x0007  /*ap 断连事件*/
#define ESP8266_SERVER_START        0x0010  /*服务器启动事件*/
#define ESP8266_SERVER_STOP         0x0006  /*服务器停止事件*/
#define ESP8266_SERVER_CONNECT      0x0020  /*server连接事件*/
#define ESP8266_SERVER_DISCONNECT   0x0030  /*server断连事件*/
#define ESP8266_CLIENT_CONNECT      0x0040  /*client连接事件*/
#define ESP8266_CLIENT_DISCONNECT   0x0050  /*client断连事件*/
#define ESP8266_ONENET_CONNECT      0x0080  /*onenet连接事件*/
#define ESP8266_ONENET_DISCONNECT   0x0070  /*onenet断连事件*/


#define ESP8266_WIFI_STA            1  /*Station 模式*/
#define ESP8266_WIFI_AP             2  /*SoftAP  模式*/
#define ESP8266_WIFI_STA_AP         3  /*Soft+Station 模式*/

#define ESP_ONENET_SERVER_IP     "studio-mqtt.heclouds.com" 
#define ESP_ONENET_SERVER_POST   "1883"

#define IP_MAX_LEN              16  /*ip最大长度*/
#define MAC_MAX_LEN             18  /*mac 地址最大长度*/

#define WIFI_NAME_MAX_LEN       32  /*wifi名最大长度*/
#define WIFI_KEY_MAX_LEN        32  /*密码最大长度*/

#define ONENET_ID_MAX_LEN       32  /*id最大长度*/
#define ONENET_NAME_MAX_LEN     32  /*最大设备名长度*/
#define ONENET_PASSWD_MAX_LEN  256  /*最大密码长度*/

#define ONENET_KEY_NAME_MAX_LEN  32 /*key name 最大长度*/
#define ONENET_KEY_VALUE_MAX_LEN 32 /*key value 最大长度*/
#define ONENET_PUSH_MAX_LEN 	 256 /*key value 最大长度*/

//阿里云
#define ALIYUN_USERNAME_MAX_LEN       50  /*username最大长度*/
#define ALIYUN_PASSWD_MAX_LEN         100  /*passwd最大长度*/
#define ALIYUN_CLIENTLD_MAX_LEN       100  /*clientld最大长度*/
#define ALIYUN_MQTTHOSTURL_MAX_LEN    100  /*mqtthosturl最大长度*/
#define ALIYUN_PORT_MAX_LEN           50  /*port最大长度*/
#define ALIYUN_TOPIC_MAX_LEN          100/*topic最大长度*/
///*非最后一条语句使用以下格式*/
#define ONENET_FLOAT	 "\\\"%s\\\":{\\\"value\\\":%f}\\,"
#define ONENET_INT	     "\\\"%s\\\":{\\\"value\\\":%d}\\,"
#define ONENET_STR	     "\\\"%s\\\":{\\\"value\\\":\\\"%s\\\"}\\,"

/*最后一条语句必须使用以下格式*/
#define ONENET_FLOAT_END	 "\\\"%s\\\":{\\\"value\\\":%f}\\,"
#define ONENET_INT_END 	     "\\\"%s\\\":{\\\"value\\\":%d}"
#define ONENET_STR_END	     "\\\"%s\\\":{\\\"value\\\":\\\"%s\\\"}"

#define ESP8266_ALIYUN_TASK_STK     128 


typedef struct __C_ESP8266 c_esp8266;

/*sta 信息*/
typedef struct __WIFI_STA_INFORM
{
    char o_name [WIFI_NAME_MAX_LEN] ; /*sta wifi名*/
    char o_key  [WIFI_KEY_MAX_LEN]  ; /*sta wifi密码*/
}sta_inform;

/*ap 信息*/
typedef struct __WIFI_AP_INFORM
{
    char o_name [WIFI_NAME_MAX_LEN] ; /*ap wifi 名*/
    char o_key  [WIFI_KEY_MAX_LEN]  ; /*ap 密码 */
    char o_ip   [IP_MAX_LEN]        ; /*ap 本机ip*/
    char o_gat  [IP_MAX_LEN]        ; /*ap 网关ip*/
    char o_mask [IP_MAX_LEN]        ; /*ap 掩码*/
}ap_inform;

/*client 连接server信息*/
typedef struct __SERVER_INFORM
{
    char o_ip   [IP_MAX_LEN]  ; /*服务器ip*/
    u16  o_port               ; /*服务器端口*/
    u8   o_reconnect_time     ; /*重连次数*/
    u8   o_reconnect_interval ; /*重连间隔*/
}server_inform;


typedef struct __ONENET_INFORM
{
    char o_product_id  [ONENET_ID_MAX_LEN]     ;  /*产品id*/
    char o_driver_name [ONENET_NAME_MAX_LEN]   ;  /*设备名称*/
    char o_passwd      [ONENET_PASSWD_MAX_LEN] ;  /*密钥  */
}onenet_inform;

typedef struct __ALIYUN_INFORM
{
    char o_username             [ALIYUN_USERNAME_MAX_LEN]   ;  /*阿里云username*/
    char o_passwd               [ALIYUN_PASSWD_MAX_LEN]     ;  /*阿里云passwd*/
	char o_clientld	            [ALIYUN_CLIENTLD_MAX_LEN]   ;  /*阿里云clientld,一定注意：相比云平台上的数据多了两个\\*/
	char o_mqtthosturl          [ALIYUN_MQTTHOSTURL_MAX_LEN];  /*阿里云mqtthosturl*/
	char o_port                 [ALIYUN_PORT_MAX_LEN]       ;  /*阿里云port*/
	char o_topic                [ALIYUN_TOPIC_MAX_LEN]       ;  /*阿里云属性上报topic,对应产品里的topic类属性上班的第一条，注意修改设备名，格式为 /sys/k24kySHOVX1/esp8266DUAN/thing/event/property/post  */
	char o_recv_topic			[ALIYUN_TOPIC_MAX_LEN];	/*app或小程序topic，对应晕产品流转中app或小程序的转发数据中的后半部分*/
	char o_topic_flow                [ALIYUN_TOPIC_MAX_LEN]       ;  /*阿里云topic,对应云产品流转中esp8266的sql语句的后半部分，格式为/k24kySHOVX1/esp8266DUAN/user/ESP8266DUAN */
}aliyun_inform;

typedef struct __ONENET_MSG
{
	char o_data[ONENET_PUSH_MAX_LEN];
//    char o_key[ONENET_KEY_NAME_MAX_LEN + 2];
//    char o_value[ONENET_KEY_VALUE_MAX_LEN + 2];
}onenet_msg;

typedef struct __C_ESP8266
{
    void* this;
    
    /************************************************* 
    * Function: wifi_cfg 
    * Description: 配置wifi
    * Input : <this>   8266对象
    *         <mode>   WiFi模式
    *                  (ESP8266_WIFI_STA)      Station 模式
    *                  (ESP8266_WIFI_AP)       SoftAP  模式
    *                  (ESP8266_WIFI_STA_AP)   Soft+Station 模式
    *         <sta>    sta配置信息
    *         <ap>     ap配置信息
    * Output: 无
    * Return: <E_OK>             配置成功
    *         <E_NULL>           空指针
    *         <E_PARAM>          参数错误
    *         <E_ERROR>          配置失败
    * Others: 无
    * Demo  :
    *         sta_inform sta1={"test","123456789"};	//wifi账号密码
    *         aliyun_inform aliyun={"esp8266&k1h3j6aQls1",                                       //阿里云连接参数
	*			"ae936c4a072c236b177df2bad1fc9751c40886b3ea6d6ec74f6a3b902e7994ce",
	*			"k1h3j6aQls1.esp8266|securemode=2\\,signmethod=hmacsha256\\,timestamp=1718784830449|",
	*			"iot-06z009wuamz2x6c.mqtt.iothub.aliyuncs.com",
	*			"1883",
	*			"/k1h3j6aQls1/esp8266/user/esp8266",
	*			"/sys/k1h3j6aQls1/esp8266/thing/service/property/set"};
	*			ret = esp8266.aliyun_cfg(&esp8266, &aliyun);
    *         if(E_OK != ret)
    *         {
    *             log_error("Wifi cfg failed.");
    *         }
    *************************************************/
int (*wifi_cfg)(const c_esp8266* this,u8 mode,const sta_inform* sta,const ap_inform* ap);//esp8266配置

/************************************************* 
    * Function: aliyun_cfg 
    * Description: 配置阿里云
    * Input : <this>   8266对象
    *         <aliyun>  阿里云配置信息
    * Output: 无
    * Return: <E_OK>             配置成功
    *         <E_NULL>           空指针
    *         <E_PARAM>          参数错误
    *         <E_ERROR>          配置失败
    * Others: 无
    * Demo  :
    *	aliyun_inform aliyun=
	*	{
	*   	"yyTEST&k00vbIG8gNo",
	*		"aa9ec0b1a1c98ed28848eb9217fc8ef6101203be9e07bde17dcc30c3f45b585e",
	*		"k00vbIG8gNo.yyTEST|securemode=2\\,signmethod=hmacsha256\\,timestamp=1696836753977|",         注：  改行的字符串（阿里云的clientld）每个逗号前面要加两个\     
	*   	"iot-06z00bon9tm8tsw.mqtt.iothub.aliyuncs.com",
	*   	"1883",
	*    	"/sys/k00vbIG8gNo/yyTEST/thing/event/property/post"};
    *         
    *    ret = esp8266.aliyun_cfg(&esp8266,aliyun);
    *    if(E_OK != ret)
    *    {
    *             log_error("aliyun connect failed.");
    *    }
    *************************************************/

int (*aliyun_cfg)(const c_esp8266* this,const aliyun_inform* aliyun);//连接阿里云

/************************************************* 
    * Function: aliyun_send 
    * Description: 向阿里云topic发送消息
    * Input : <this>   8266对象
    *         <al_data>  需要向topic发送的消息
    * Output: 无
    * Return: <E_OK>             配置成功
    *         <E_NULL>           空指针
    *         <E_PARAM>          参数错误
    *         <E_ERROR>          配置失败
    * Others: 无
    * Demo  :
    *         
    *         ret = esp8266.aliyun_send(&esp8266,"test_data");
    *         if(E_OK != ret)
    *         {
    *             log_error("aliyun send failed.");
    *         }
    *************************************************/

int (*aliyun_send)(const c_esp8266* this,char* al_data); //向阿里云topic发送消息

/************************************************* 
    * Function: aliyun_set 
    * Description: 设置阿里云上设备参数值
    * Input : <this>   8266对象
    *         <set_sign>  参数的标识符
	*         <set_data>  设置的值
    * Output: 无
    * Return: <E_OK>             配置成功
    *         <E_NULL>           空指针
    *         <E_PARAM>          参数错误
    *         <E_ERROR>          配置失败
    * Others: 无
    * Demo  :
    *         
    *         ret = esp8266.m_aliyun_set(&esp8266,"temp","35");     //注：所有数据字符一律用字符串形式
    *         if(E_OK != ret)
    *         {
    *             log_error("aliyun set failed.");
    *         }
    *************************************************/

int (*aliyun_set)(const c_esp8266* this,char *set_sign,char* set_data);//设置阿里云上参数值

	int (*set_data_process_callback)   (const c_esp8266* this, void* param, int (*callback)(void* param, const u8* data, u16 data_len));
}c_esp8266;

/************************************************* 
* Function: esp8266_create 
* Description: 创建一个esp8266对象
* Input : <uart_id>  esp8266所在的串口编号
*                    (MY_UART_1)  串口1
*                    (MY_UART_2)  串口2
*                    (MY_UART_3)  串口3
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_esp8266 esp8266 = {0};
*         
*	      esp8266 = esp8266_create(MY_UART_1);
*         if(NULL == esp8266.this)
*         {
*             log_error("esp8266 creat failed.");       
*         }
*************************************************/
c_esp8266 esp8266_create(u8 uart_id);




#endif

