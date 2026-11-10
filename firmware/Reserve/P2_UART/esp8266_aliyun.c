#include "driver_conf.h"
#ifdef DRIVER_ESP8266_ALIYUN_ENABLED

static int m_wifi_cfg(const c_esp8266* this,u8 mode,const sta_inform* sta,const ap_inform* ap);
static int m_aliyun_cfg(const c_esp8266* this,const aliyun_inform* aliyun);
static int m_aliyun_send(const c_esp8266* this,char* al_data);
static int m_aliyun_set(const c_esp8266* this,char *set_sign,char* set_data);
static int data_intercepet(char* data,char* sign);
static int m_set_data_process_callback(const c_esp8266* this, void* param, int (*callback)(void* param, const u8* data, u16 data_len));
static void m_aliyun_task(void* pdata);

static const char* g_sys_restore   = "AT+RESTORE\r\n"                        ;  /*恢复出厂设置*/
static const char* g_sys_rst       = "AT+RST\r\n"                            ;  /*重启模块*/
static const char* g_sys_start     = "AT\r\n"                                ;  /*启动at*/

static const char* g_wifi_mode_sta    =     "AT+CWMODE=1\r\n"              ;  /*sta模式*/
static const char* g_wifi_mode_ap     =     "AT+CWMODE=2\r\n"              ;  /*ap模式*/
static const char* g_wifi_mode_ap_sta =     "AT+CWMODE=3\r\n"              ;  /*sta+ap模式*/

//阿里云
static const char* g_aliyun_sntp          ="AT+CIPSNTPCFG=1,8,\"ntpl.aliyun.com\"\r\n"  ;
	
typedef struct ___M_ESP8266   //成员结构体
{
	u8*  m_ret_data;	//回调验证
	u8   m_uart_id;	    //串口号
	u8   m_wifi_mode;   //wifi模式
	sta_inform  m_wifi_sta;
	ap_inform   m_wifi_ap;	
	char       aliyun_topic[100];
	char       aliyun_flow_topic[100];
	char  *aliyun_recv;
	int (*m_callback)(void* param,const u8* data,const u16 data_len);  /*接收回调函数*/
}m_esp8266;

static int m_aliyun_return(m_esp8266* this,char* return_data) ;

const u8* all_data; //指针
char ready[]="ready";
char OK[]="OK";

static int aliyun_callback(void* param,const u8* data,u16 data_len) //阿里云串口回调设置
{
    m_esp8266* this   = NULL    ;
	/*参数检查*/
    if(NULL == param)
	{      
		log_error("ali ret error.");
		return E_ERROR;
	}
    this = param;
	char *ret;
	char a[50];
	log_inform("recv data: %s", data);

	this->aliyun_recv = (char*)data;
	m_aliyun_return(this, (char*)data);
}

static int det_data(const u8* all_data,char *str,int time)  //等待返回值并比较 参数:字符串1地址，字符串2地址，等待时间，
{
	int a=0;
	char* b=NULL;
	while(all_data==NULL || a<=time){
		delay_ms(1);
		a++;
    }
    b=strstr((char*)all_data, str);
    if(b==NULL){
		all_data=NULL;
		return E_ERROR;
	}
    else{
		all_data=NULL;	
		return E_OK;
	}
}

c_esp8266 esp8266_create(u8 uart_id)                   //创建函数
{
	int ret = 0; 
	c_esp8266  new  ={0};
	m_esp8266* m_this =NULL; 

	log_inform("adv");

    new.this = pvPortMalloc(sizeof(m_esp8266));
    if(NULL == new.this)
    {
        log_inform("123");
		return new;
    }

	new.this = pvPortMalloc(sizeof(m_esp8266));

	if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_esp8266));
    m_this = new.this; 
	
	m_this->m_uart_id = uart_id   ;
	new.wifi_cfg    = m_wifi_cfg  ; //函数
    new.aliyun_cfg  = m_aliyun_cfg;
	new.aliyun_send =m_aliyun_send;
	new.aliyun_set  =m_aliyun_set;
	new.set_data_process_callback = m_set_data_process_callback;
	
	ret = my_uart.init(uart_id,115200,64);//初始化
	
	if(E_OK != ret)
	{
		log_error("init failed.");
		goto ERROR;
	}
	
	ret = my_uart.send(uart_id,(const u8*)g_sys_restore,strlen(g_sys_restore));//恢复出厂设置
	if(E_OK != ret)
	{
		log_error("restore failed.");
		goto ERROR;
	}
    
	vTaskDelay(1000);
	
	ret = my_uart.send(uart_id,(const u8*)g_sys_rst,strlen(g_sys_rst));//重启
	if(E_OK != ret)
	{
		log_error("reset failed.");
		goto ERROR;
	}

    vTaskDelay(1000);
	
	ret = my_uart.send(uart_id,(const u8*)g_sys_start,strlen(g_sys_start));//at验证
	if(E_OK != ret)
	{
		log_error("send at failed.");
		goto ERROR;
	}
   
	vTaskDelay(1000);
	
	log_inform("created success");
	return new;
	
 ERROR:
	log_inform("create error");
	vPortFree(new.this);
    new.this = NULL;
    return new;

}

u8 str_process(char* src, char** dst, int len) //字符串操作，逗号和双引号前面加上反斜杠
{
	if (NULL == src || NULL == dst)
	{
		log_error("param error");
		return E_ERROR;
	}
	
	for (int i = 0, j = 0; i < strlen(src); i++, j++)
	{
		if (src[i] == '\"')
		{
			(*dst)[j] = '\\';
			j++;
		}
		else if(src[i]==','){
			(*dst)[j]='\\';
			j++;
		}
		(*dst)[j] = src[i];
	}
	
	return E_OK;
}

static int m_wifi_cfg(const c_esp8266* this,u8 mode,const sta_inform* sta,const ap_inform* ap)     //esp8266配置函数
{
	m_esp8266* m_this =NULL; 
    int ret;
	if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

	    /*检查模式*/
    if(ESP8266_WIFI_STA    != mode && ESP8266_WIFI_AP != mode && ESP8266_WIFI_STA_AP != mode)
    {
        log_error("Param.");
        return E_ERROR;
    }

    if((ESP8266_WIFI_STA == mode ||ESP8266_WIFI_STA_AP == mode) && NULL == sta)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if((ESP8266_WIFI_AP == mode ||ESP8266_WIFI_STA_AP == mode) && NULL == ap)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    if(NULL != sta && (WIFI_KEY_MAX_LEN < strlen(sta->o_key) || WIFI_NAME_MAX_LEN < strlen(sta->o_name)))
    {
        log_error("Param error.");
        return E_ERROR;
    }
    if(NULL != ap && (WIFI_KEY_MAX_LEN < strlen(ap->o_key) || WIFI_NAME_MAX_LEN < strlen(ap->o_name) || IP_MAX_LEN < strlen(ap->o_ip) || IP_MAX_LEN < strlen(ap->o_gat) || IP_MAX_LEN < strlen(ap->o_mask)))
    {
        log_error("Param error.");
        return E_ERROR;
    }
    //设置模式
	switch(mode){
		case ESP8266_WIFI_STA:    ret=my_uart.send(m_this->m_uart_id,(const u8*)      g_wifi_mode_sta   ,strlen(g_wifi_mode_sta));break;    //sta模式
		case ESP8266_WIFI_AP :    ret=my_uart.send(m_this->m_uart_id,(const u8 *)     g_wifi_mode_ap    ,strlen(g_wifi_mode_ap));break;     //ap模式
		case ESP8266_WIFI_STA_AP :ret=my_uart.send(m_this->m_uart_id,(const u8*)      g_wifi_mode_ap_sta,strlen(g_wifi_mode_ap_sta));break; //sta+ap模式
	}
	if(ret!=E_OK){
		log_error("senderror");
		return E_ERROR;
	}
  	
    vTaskDelay(2000);
    ret = my_uart.send(m_this->m_uart_id,(const u8*)g_sys_rst,strlen(g_sys_rst));                                                  //重启
	if(ret!=E_OK)
	{
		log_error("cqshibai");
		return E_ERROR;
	}
    vTaskDelay(1000);
		 
	char wifi[100];
    snprintf(wifi,sizeof(wifi),"AT+CWJAP=\"%s\",\"%s\"\r\n" ,sta->o_name,sta->o_key);
	
	if(mode == ESP8266_WIFI_STA||mode==ESP8266_WIFI_STA_AP){
		ret=my_uart.send(m_this->m_uart_id,(const u8*)wifi,strlen(wifi));
		if(ret!=E_OK)
		{
			log_error("cqshibai");
			return E_ERROR;
		}
		vTaskDelay(4000);
	}
	
	return E_OK;
}

static int m_aliyun_cfg(const c_esp8266* this,const aliyun_inform* aliyun)         //阿里云函数
{
    BaseType_t   os_ret = 0 ;
	m_esp8266* m_this =NULL; 
    int ret;
	if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
	
    strcpy(m_this->aliyun_topic,aliyun->o_topic);
	strcpy(m_this->aliyun_flow_topic,aliyun->o_topic_flow);
	ret = my_uart.send(m_this->m_uart_id,(const u8*)g_aliyun_sntp,strlen(g_aliyun_sntp)); //使能sntp服务器
	if(ret!=E_OK)
	{
		log_error("SNTP");
		return E_ERROR;
	}
    vTaskDelay(1000);
 
	char aliyun_data[500];
	snprintf(aliyun_data,sizeof(aliyun_data),"AT+MQTTUSERCFG=0,1,\"NULL\",\"%s\",\"%s\",0,0,\"\"\r\n" ,aliyun->o_username,aliyun->o_passwd);
	//snprintf(aliyun_data,sizeof(aliyun_data),"AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n" ,aliyun->o_clientld,aliyun->o_username,aliyun->o_passwd);
	ret = my_uart.send(m_this->m_uart_id,(const u8*)aliyun_data,strlen(aliyun_data)); //阿里云登录
	if(ret!=E_OK)
	{
		log_error("aliyun1");
		return E_ERROR;
	}
	vTaskDelay(1000);
	
	//clientid
	snprintf(aliyun_data,sizeof(aliyun_data),"AT+MQTTCLIENTID=0,\"%s\"\r\n" ,aliyun->o_clientld);
	ret = my_uart.send(m_this->m_uart_id,(const u8*)aliyun_data,strlen(aliyun_data)); //阿里云登录
	if(ret!=E_OK)
	{
		log_error("aliyun1");
		return E_ERROR;
	}
	vTaskDelay(1000);
    
	snprintf(aliyun_data,sizeof(aliyun_data), "AT+MQTTCONN=0,\"%s\",%s,1\r\n",aliyun->o_mqtthosturl,aliyun->o_port);
	ret = my_uart.send(m_this->m_uart_id,(const u8*)aliyun_data,strlen(aliyun_data)); //阿里云连接
	if(ret!=E_OK)
	{
		log_error("aliyuncnn");
		return E_ERROR;
	}
	vTaskDelay(1000);
     
	//测试订阅
	snprintf(aliyun_data,sizeof(aliyun_data), "AT+MQTTSUB=0,\"%s\",1\r\n", aliyun->o_recv_topic);//topic订阅
	ret = my_uart.send(m_this->m_uart_id,(const u8*)aliyun_data,strlen(aliyun_data)); //阿里云订阅
	if(ret!=E_OK)
	{
		log_error("aliyundingyun");
		return E_ERROR;
	}
	
	vTaskDelay(1000);
	log_error("aliyun_success");
	

	ret= my_uart.set_callback(m_this->m_uart_id,m_this,aliyun_callback);
	return E_OK;
}

static int m_aliyun_send(const c_esp8266* this,char* al_data)           //向topic发送消息
{
	m_esp8266* m_this =NULL; 
    int ret;
	if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
	
	char aliyun_data[300];
	snprintf(aliyun_data,sizeof(aliyun_data),"AT+MQTTPUBRAW=0,\"%s\",%d,1,0\r\n", m_this->aliyun_topic, strlen(al_data));    
	ret = my_uart.send(m_this->m_uart_id,(const u8*)aliyun_data,strlen(aliyun_data)); //阿里云连接
	if(ret!=E_OK)
	{
		log_error("cnn_error");
		return E_ERROR;
	}
	vTaskDelay(500);
	ret = my_uart.send(m_this->m_uart_id,(const u8*)al_data,strlen(al_data)); //阿里云连接
	if(ret!=E_OK)
	{
		log_error("cnn_error");
		return E_ERROR;
	}
	vTaskDelay(500);
	
	memset(aliyun_data, 0, sizeof(aliyun_data));
	snprintf(aliyun_data,sizeof(aliyun_data),"AT+MQTTPUBRAW=0,\"%s\",%d,1,0\r\n", m_this->aliyun_flow_topic, strlen(al_data));    
	ret = my_uart.send(m_this->m_uart_id,(const u8*)aliyun_data,strlen(aliyun_data)); //阿里云连接
	if(ret!=E_OK)
	{
		log_error("cnn_error");
		return E_ERROR;
	}
	vTaskDelay(500);
	ret = my_uart.send(m_this->m_uart_id,(const u8*)al_data,strlen(al_data)); //阿里云连接
	if(ret!=E_OK)
	{
		log_error("cnn_error");
		return E_ERROR;
	}
	vTaskDelay(500);
	
	return E_OK;
}

static int m_aliyun_set(const c_esp8266* this,char *set_sign,char* set_data)                //设置参数
{
	m_esp8266* m_this =NULL; 
    int ret;
	if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
	
	char aliyun_data[100];
	
    snprintf(aliyun_data, sizeof(aliyun_data), "{\"params\":{\"%s\":%s}}", set_sign, set_data);
	ret=m_aliyun_send(this,aliyun_data);
	if(ret!=E_OK){
	log_inform("setsuc");
		return E_ERROR;
	}
	return E_OK;
}
	
int m_set_data_process_callback(const c_esp8266* this, void* param, int (*callback)(void* param, const u8* data, u16 data_len))
{
	m_esp8266* m_this =NULL; 
    int ret;
	if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
	
	m_this->m_callback = callback;
	return E_OK;
}

static int m_aliyun_return(m_esp8266* this,char* return_data)           //返回消息
	{
	m_esp8266* m_this =NULL; 
    int ret;
	if(NULL == this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this;
	int len=strlen(return_data)+20;
	char *aliyun_data2=(char*)malloc(len);
	str_process(return_data,&aliyun_data2,len);

	if (strstr(return_data, "user/get") || strstr(return_data, "property/set"))
		m_this->m_callback(NULL, (const u8*) return_data, strlen(return_data));
	return E_OK;
}

#endif

