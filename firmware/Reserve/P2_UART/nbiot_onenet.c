#include "driver_conf.h"

#ifdef DRIVER_NBIOT_ONENET_ENABLED

#define MODULE_NAME       "bc260y"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define QUEUE_LEN            5
#define CMD_ANSWER_OK    "OK\r\n"

typedef struct __M_BC260Y m_bc260y;

static int m_onenet_start     (const c_bc260y* this, const onenet_inform* onenet);
static int m_onenet_send      (const c_bc260y* this, const char* format, ...);
static int m_onenet_callback  (const c_bc260y* this, void* param,int (*callback)(void* param,const char* msg));

static void m_task(void* pdata);
static void m_recv(void* pdata);
static int  m_callback(void* param,const u8* data,u16 data_len);
static int  m_cmd_send(m_bc260y* this,u8 count,TickType_t time,const char* answer,const char* data,...);
static int  m_reset   (m_bc260y* this,TickType_t time);

typedef struct __M_BC260Y
{
    u8                 m_uart_id ;
    u8                 m_wait_ask;    /*等待命令应答*/
    u8                 m_recv_buf[NBIOT_DATA_MAX_LEN + 1];
    u8                 m_recv_len;
    u8                 m_send_buf[NBIOT_DATA_MAX_LEN + 1];
    
    char            m_onenet_product_id  [ONENET_ID_MAX_LEN + 1]     ;  /*onenet产品id*/
    char            m_onenet_device_name [ONENET_NAME_MAX_LEN  +1]   ;  /*onenet设备名称*/
    char            m_onenet_passwd      [ONENET_PASSWD_MAX_LEN + 1] ;  /*onenet密钥  */
    
    void*           m_onenet_recv_callback_param;
    int           (*m_onenet_recv_callback)(void* param,const char* msg);  /*接收回调函数*/
    
    TaskHandle_t    m_task      ;  /*主任务  */
    TaskHandle_t    m_recv_task ;  /*接收任务*/
    
    QueueHandle_t   m_send_msg  ;  /*发送消息*/
    QueueHandle_t   m_recv_msg  ;  /*接收消息*/
}m_bc260y;

static const char* g_sys_start     = "AT\r\n"                                ;  /*启动at*/
static const char* g_disable_sleep     = "AT+QSCLK=0\r\n"                                ;  /*禁用休眠*/
static const char* g_cgatt     = "AT+CGATT=1\r\n"                                ;  /*PS 域附着或去附着*/
static const char* g_csq     = "AT+CSQ\r\n"                                ;  /*上报信号质量*/
static const char* g_mqtt_version     = "AT+QMTCFG=\"version\",0,1\r\n"                                ;  /* 设置MQTT协议版本*/
static const char* g_onenet_open     = "AT+QMTOPEN=0,\"mqtts.heclouds.com\",1883\r\n"                                ;  /*打开mqtt服务器*/
static const char* g_onenet_conn     = "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n"                                ;  /*连接mqtt服务器*/
/*上发数据*/
static const char* g_onenet_post_pub         = "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/post\",%d\r\n";
static const char* g_onenet_payload = "{\"id\":\"%s\",\"version\":\"1.0\",\"params\":{%s}}\r\n";

static const char* g_onenet_discon           = "AT+QMTDISC=0\r\n";	/*断开连接*/
static const char* g_ans_onenet_con       = "+QMTCONN:"                ;  /*onenet 连接*/


static const char* g_sys_rst       = "AT+QRST=1\r\n"                            ;  /*重启模块*/

/*订阅主动上报*/
static const char* g_onenet_post_sub_reply   = "AT+QMTSUB=0,1,\"$sys/%s/%s/thing/property/post/reply\",0\r\n";

/*订阅下发*/
static const char* g_onenet_post_sub_set     = "AT+QMTSUB=0,2,\"$sys/%s/%s/thing/property/set\",1\r\n";

/*接收应答*/
static const char* g_onenet_post_pub_set_ans = "AT+QMTPUB=0,\"$sys/%s/%s/thing/property/set_reply\",\"{\\\"id\\\":\\\"%u\\\"\\,\\\"code\\\":200\\,\\\"msg\\\":\\\"success\\\"}\",0,0\r\n";

static const char* g_ans_onenet_set       = "thing/property/set"             ;  /*onenet 设置*/

c_bc260y bc260y_create(u8 uart_id)
{
    int ret = 0;
    c_bc260y    new    = {0}  ;
    m_bc260y*   m_this = NULL ;
    BaseType_t   os_ret = 0    ;
    data_cut_cfg cfg = {0};

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_bc260y));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_bc260y));
    m_this = new.this; 

    /*保存相关变量*/
    m_this->m_uart_id = uart_id;
                                             
    new.onenet_start     = m_onenet_start ;
    new.onenet_send      = m_onenet_send  ;
    new.onenet_callback  = m_onenet_callback;
                                             
    /*初始化串口*/
    ret = my_uart.init(uart_id,NBIOT_BAUDRATE,64);
    if(E_OK != ret)
    {
        log_error("Uart init failed.");
        goto error_handle;
    }
    
    /*注册串口回调*/
    ret = my_uart.set_callback(m_this->m_uart_id,(void*)m_this,m_callback);
    if(E_OK != ret)
    {
        log_error("Callback set failed.");
        goto error_handle;
    }
    

    /*创建发送队列*/
    m_this->m_send_msg = xQueueCreate(QUEUE_LEN,sizeof(onenet_msg));
    if(NULL == m_this->m_send_msg)
    {
        log_error("Queue creat filed.");
        goto error_handle;
    }
    
    /*创建接收队列*/
    m_this->m_recv_msg = xQueueCreate(QUEUE_LEN,sizeof(u8*));
    if(NULL == m_this->m_recv_msg)
    {
        log_error("Queue creat filed.");
        goto error_handle;
    }
        
    return new;

    error_handle:

    new.this = NULL;
    return new;
}

static int m_onenet_start     (const c_bc260y* this, const onenet_inform* onenet)
{
    m_bc260y* m_this   = NULL ;
    BaseType_t   os_ret = 0    ;
    
    /*参数检测*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_PARAM;
    }
    m_this = this->this;
    
    
    strcpy(m_this->m_onenet_product_id,onenet->o_product_id);
    strcpy(m_this->m_onenet_device_name,onenet->o_driver_name);
    strcpy(m_this->m_onenet_passwd,onenet->o_passwd);
    
    /*创建bc260y任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_task            ,
                         (const char*    )"bc260y task"            ,
                         (uint16_t       )ESP8266_ONENET_CON_TASK_STK  ,
                         (void*          )m_this                    ,
                         (UBaseType_t    )ESP8266_ONENET_CON_TASK_PRO       ,
                         (TaskHandle_t*  )&m_this->m_task);
                             
    if(pdPASS != os_ret)
    {
        log_error("Task creat failed,ret=%d",(int)os_ret);
        return E_ERROR;
    }
    
    /*创建bc260y接收任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_recv            ,
                         (const char*    )"bc260y recv task"           ,
                         (uint16_t       )ESP8266_ONENET_RECV_TASK_STK  ,
                         (void*          )m_this                    ,
                         (UBaseType_t    )ESP8266_ONENET_RECV_TASK_PRO  ,
                         (TaskHandle_t*  )&m_this->m_recv_task);
                             
    if(pdPASS != os_ret)
    {
        log_error("Task creat failed,ret=%d",(int)os_ret);
        return E_ERROR;
    }
    
    return E_OK;
}


static int m_onenet_send      (const c_bc260y* this, const char* format, ...)
{
    onenet_msg  new_msg = {0};
    m_bc260y*  m_this = NULL;
    BaseType_t  os_ret  = 0;
    u32         event = 0;
    va_list   argptr;

    /*检查参数*/
    if(NULL == this|| NULL == this->this || NULL == format)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    va_start(argptr, format);  // 初始化argptr 
    vsnprintf(new_msg.o_data, ONENET_PUSH_MAX_LEN, format, argptr);
    va_end(argptr);
    
    if(ONENET_PUSH_MAX_LEN < strlen(new_msg.o_data))
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*新消息 压到队列*/
    os_ret = xQueueSend(m_this->m_send_msg,&new_msg,0);
    if(pdTRUE != os_ret)
    {
        log_error("Send message filed.");
        return E_ERROR;
    }  

    return E_OK;
}
static int m_onenet_callback  (const c_bc260y* this, void* param,int (*callback)(void* param,const char* msg))
{
    m_bc260y*        m_this = NULL;

    /*检查参数*/
    if(NULL == this|| NULL == this->this)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    m_this->m_onenet_recv_callback       = callback;
    m_this->m_onenet_recv_callback_param = param;

    return E_OK;
}

static void m_task(void* pdata)
{
    m_bc260y* this   = NULL ;
    int        ret      = 0    ;
    BaseType_t   os_ret = 0    ;
    EventBits_t  event_bit = 0;
    u32        event    = 0;
    onenet_msg   new_msg  = {0};
	char payload[256] = {0};

    /*参数检测*/
    if(NULL == pdata)
    {
        log_error("Null pointer.");
        vTaskDelete(NULL);   /*删除本任务*/
    }
    this = pdata;
    
    vTaskDelay(1000);
    
    /*进入wifi 的连接逻辑*/
    do
    {
        /*复位*/
        ret = m_reset(this,1000);
        if(E_OK != ret)
        {
            log_error("Reset failed.");
            continue;
        }  
        
        break;
    }while(1);
    
    vTaskDelay(3000);
    
    /*进入onenet的连接逻辑*/
    do
    {
		ret = m_cmd_send(this,3,1000,NULL,g_disable_sleep);
		vTaskDelay(200);

		ret = m_cmd_send(this,3,1000,NULL,g_cgatt);
		vTaskDelay(200);
		
		ret = m_cmd_send(this,3,1000,NULL,g_csq);
		vTaskDelay(200);
		
		ret = m_cmd_send(this,3,1000,NULL,g_mqtt_version);
		vTaskDelay(200);
		
		ret = m_cmd_send(this,3,1000,NULL,g_onenet_open);
		vTaskDelay(2000);
		ret = m_cmd_send(this,3,1000,NULL,g_onenet_conn, this->m_onenet_device_name, this->m_onenet_product_id, this->m_onenet_passwd);
                
        break;
    }while(1);
    
	vTaskDelay(500);
    /*订阅主动上报*/
    ret = m_cmd_send(this,1,1000,CMD_ANSWER_OK,g_onenet_post_sub_reply,this->m_onenet_product_id,this->m_onenet_device_name);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
    }

	vTaskDelay(500);
    /*订阅下发*/
    ret = m_cmd_send(this,1,1000,CMD_ANSWER_OK,g_onenet_post_sub_set,this->m_onenet_product_id,this->m_onenet_device_name);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
    }
	vTaskDelay(200);
    
    /*进入onenet 上报流程*/
    do
    {
        /*获取要上报的onenet消息*/
        memset(&new_msg,0,sizeof(new_msg));
        os_ret = xQueueReceive(this->m_send_msg,&new_msg,portMAX_DELAY);
        if(pdTRUE != os_ret)
        {
            continue;
        }
        
		memset(payload, 0, 256);
		snprintf(payload, 256, g_onenet_payload,"123",new_msg.o_data);
        /*发送数据*/
		//ret = m_cmd_send(this,2,1000,NULL,g_onenet_post_pub, this->m_onenet_product_id,this->m_onenet_device_name, 92);
		ret = m_cmd_send(this,2,1000,NULL,g_onenet_post_pub, this->m_onenet_product_id,this->m_onenet_device_name, strlen(payload));
		vTaskDelay(200);
		
//		ret = m_cmd_send(this,2,1000,NULL, "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{\"humidity\":{\"value\":16},\"temperature\":{\"value\":17}}}\r\n");
        ret = m_cmd_send(this,2,1000,NULL,payload);
        if(E_OK != ret)
        {
            log_error("Cmd send failed.");
            continue;
        }
    
        vTaskDelay(1500);
        
    }while(1);
    
}
static void m_recv(void* param)
{
	m_bc260y* this   = NULL ;
    u8* new_msg  = NULL;
	int ret = 0;
    char* str_param = NULL;
    BaseType_t   os_ret = 0    ;
    
    /*参数检测*/
    if(NULL == param)
    {
        log_error("Null pointer.");
        vTaskDelete(NULL);   /*删除本任务*/
    }
    this = param;
    
    while(1)
    {
        str_param = NULL;
        if(NULL != new_msg)
        {
            vPortFree(new_msg);
            new_msg = NULL;
        }
        
        /*获取要上报的onenet消息*/
        memset(&new_msg,0,sizeof(new_msg));
        os_ret = xQueueReceive(this->m_recv_msg,&new_msg,portMAX_DELAY);
        if(pdTRUE != os_ret)
        {
            continue;
        }
        
		/*应答接收成功*/
        ret = m_cmd_send(this, 1, 1000, NULL, g_onenet_post_pub_set_ans, this->m_onenet_product_id, this->m_onenet_device_name, 123);
        if(E_OK != ret)
        {
            log_error("Set ans failed.");
            continue;
        }
		
        str_param = strstr((const char*)new_msg,"params");
        /*调用回调函数*/
        if(NULL != str_param && NULL != this->m_onenet_recv_callback)
        {
            this->m_onenet_recv_callback(this->m_onenet_recv_callback_param,str_param);
        }
    }
}

static int  m_callback(void* param,const u8* data,u16 data_len)
{
    m_bc260y* this   = param ;
    int        ret      = 0    ;
    BaseType_t   os_ret = 0    ;
    EventBits_t  event_bit = 0;  
    u8*        recv_data = NULL;  
	static u8 data_set = 0;
	static u8 temp_data[200] = {0};
    
    /*如果是 空换行 不要  无效命令*/
    if('\r' == data[0] && '\n' == data[1] && 2 == data_len)
    {
        return E_OK;
    }
    
//    log_inform("%s",data);
    
    /*过滤到 发送js的回包*/
    if('{' == data[0] || '>' == data[0])
    {
        return E_OK;
    }
    
    /*参数检测*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_ERROR;  
    }
    this = param;   
    
	if (NULL != strstr((char*)data, "set\",1"))
		return E_ERROR;
	
    /*如果是接收 下发到接收*/
	if((NULL != strstr((char*)data, g_ans_onenet_set) && NULL == strstr((char*)data, "thing/property/set_reply")) || data_set)
    {
		//没找到结尾，说明set命令被截断
		if (NULL == strstr((char*)data, "}}"))
			data_set = 1;
		
		//如果当前命令是被截断的set指令
		if (1 == data_set)
		{
			memcpy(temp_data + strlen((char*)temp_data), data, data_len);
			if (NULL != strstr((char*)data, "}}"))
			{
				data_set = 0;
				recv_data = pvPortMalloc(strlen((char*)temp_data) + 1);
				memset(recv_data, 0, strlen((char*)temp_data) + 1);
				memcpy(recv_data, temp_data, strlen((char*)temp_data));
        
				log_inform("%s",recv_data);
				os_ret = xQueueSend(this->m_recv_msg,&recv_data,0);
				if(pdTRUE != os_ret)
				{
					log_error("Send failed.");
					vPortFree(recv_data);
					memset(temp_data,0,sizeof(temp_data));
					return E_ERROR;
				}
				memset(temp_data,0,sizeof(temp_data));
			}
		}
		else
		{
			log_inform("%s",data);
			recv_data = pvPortMalloc(data_len + 1);
			if(NULL == recv_data)
			{
				log_error("Out of memory.");
				return E_ERROR;
			}
			memset(recv_data,0,data_len + 1);
			memcpy(recv_data,data,data_len);
        
			os_ret = xQueueSend(this->m_recv_msg,&recv_data,0);
			if(pdTRUE != os_ret)
			{
				log_error("Send failed.");
				vPortFree(recv_data);
				return E_ERROR;
			}
		}		
    }
    
    else if(1 == this->m_wait_ask)
    {   
		data_set = 0;
        if('\0' != this->m_recv_buf[0])
        {
            /*和接收目标做对比*/
            if(NULL != strstr((char*)data,(char*)this->m_recv_buf))
            {
                this->m_wait_ask = 2; /*对比成功*/
            }
            else
            {
                this->m_wait_ask = 3; /*对比失败*/
            }                
        }
        
        /*拷贝出去 自己看*/
        else
        {
            memcpy(this->m_recv_buf,data,data_len);
        }
        
        /*通知 */
        os_ret = xTaskNotifyGive(this->m_task);
        if(pdTRUE != os_ret)
        {
            log_error("Semaphore give failed.");
            return E_ERROR;
        }
    }
    
    else
    {
		data_set = 0;
        ;/*抛弃 无效命令*/
    }

    return E_OK;
}

static int m_cmd_send(m_bc260y* this,u8 count,TickType_t time,const char* answer,const char* data,...)
{
    int ret = 0;
    va_list   argptr;
    BaseType_t os_ret = pdFALSE;
    

    /*参数检测*/
    if(NULL == this || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
       
    
    /*一个命令重复做coun次*/
    for(u8 i = 0;i < count;++i)
    {       
        /*标记为等待应答*/
        if(0 != time)
        {
            this->m_wait_ask = 1;
        }
        
        /*拼接命令*/
        memset((void*)this->m_send_buf,0,NBIOT_DATA_MAX_LEN);
        va_start( argptr, data );  // 初始化argptr 
        vsnprintf((void*)this->m_send_buf,NBIOT_DATA_MAX_LEN,data,argptr);
        va_end(argptr);
        
        /*发送命令*/
        vTaskDelay(100);
        ret = my_uart.send(this->m_uart_id,this->m_send_buf,strlen((const char*)this->m_send_buf));
        if(E_OK != ret)
        {
            log_error("Send failed.");
            goto error_handle;
        }
        
        /*如果需要应答 等待应答命令*/
        if(0 != time)
        {
            ret = ulTaskNotifyTake(pdTRUE,time);
            if(0 != os_ret)
            {
                log_error("At ack timer out.");
                continue;
            }
                                  
            this->m_wait_ask = 0;
            return E_OK;
        }
        
        /*不需要应答 直接退出*/
        else
        {
            this->m_wait_ask = 0;
            return E_OK;
        }
    }
    
    this->m_wait_ask = 0;
    log_error("AT no ack.");
    return E_ERROR;
    
    error_handle:
    
    this->m_wait_ask = 0;
    return E_ERROR;
}

static int  m_reset   (m_bc260y* this,TickType_t time)
{
    int ret = 0;

    /*参数检测*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    /*复位命令*/
    ret = my_uart.send(this->m_uart_id,(const u8*)g_sys_rst,strlen(g_sys_rst));
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }
    
    vTaskDelay(2000);
    
    /*启动at命令*/
    ret = m_cmd_send(this,8,time,CMD_ANSWER_OK,g_sys_start);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        return E_ERROR;
    }
    
    return E_OK;
}

#endif