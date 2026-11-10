#include "driver_conf.h"

#ifdef DRIVER_ESP8266_ONENET_ENABLED

#define MODULE_NAME       "esp8266"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define QUEUE_LEN            5
#define CMD_SEND_TIME     25000
#define CMD_ANSWER_OK    "OK\r\n"
#define CMD_ANSWER_ERROR "ERROR\r\n"

#define ESP8266_WIFI_STA            1  /*Station 模式*/
#define ESP8266_WIFI_AP             2  /*SoftAP  模式*/
#define ESP8266_WIFI_STA_AP         3  /*Soft+Station 模式*/

#define ESP8266_WIFI_STA_CONNECT    0x0002  /*sta连接事件*/
#define ESP8266_ONENET_CONNECT      0x0080  /*onenet连接事件*/

typedef struct __M_ESP8266 m_esp8266;

static int m_onenet_start     (const c_esp8266* this, const sta_inform* sta,const onenet_inform* onenet);
static int m_onenet_send      (const c_esp8266* this, const char* format, ...);
static int m_onenet_callback  (const c_esp8266* this, void* param,int (*callback)(void* param,const char* msg));

static void m_task(void* pdata);
static void m_recv(void* pdata);
static int  m_callback(void* param,const u8* data,u16 data_len);
static int  m_cmd_send(m_esp8266* this,u8 count,TickType_t time,const char* answer,const char* data,...);
static int  m_reset   (m_esp8266* this,TickType_t time);

/*发送消息 类型*/
typedef struct _ESP8266_MSG
{
    void*  o_data     ;  /*数据地址*/
    u16    o_data_len ;  /*数据长度*/
}esp8266_msg;

/*onenet消息 类型*/
typedef struct _ESP8266_ONENET_MSG
{
    char* o_key_name;
    char* o_key_value;
}esp8266_onenet_msg;

typedef struct __M_ESP8266
{
    u8                 m_uart_id ;
    u8                 m_wait_ask;    /*等待命令应答*/
    u8                 m_recv_buf[ESP8266_DATA_MAX_LEN + 1];
    u8                 m_recv_len;
    u8                 m_send_buf[ESP8266_DATA_MAX_LEN + 1];
    
    char            m_name [WIFI_NAME_MAX_LEN] ; /*sta wifi名*/
    char            m_key  [WIFI_KEY_MAX_LEN]  ; /*sta wifi密码*/
    
    char            m_onenet_product_id  [ONENET_ID_MAX_LEN + 1]     ;  /*onenet产品id*/
    char            m_onenet_driver_name [ONENET_NAME_MAX_LEN  +1]   ;  /*onenet设备名称*/
    char            m_onenet_passwd      [ONENET_PASSWD_MAX_LEN + 1] ;  /*onenet密钥  */
    
    void*           m_onenet_recv_callback_param;
    int           (*m_onenet_recv_callback)(void* param,const char* msg);  /*接收回调函数*/
    
    TaskHandle_t    m_task      ;  /*主任务  */
    TaskHandle_t    m_recv_task ;  /*接收任务*/
    
    QueueHandle_t   m_send_msg  ;  /*发送消息*/
    QueueHandle_t   m_recv_msg  ;  /*接收消息*/
    
    EventGroupHandle_t m_event  ;  /*事件组*/
}m_esp8266;

static const char* g_sys_rst       = "AT+RST\r\n"                            ;  /*重启模块*/
static const char* g_sys_start     = "AT\r\n"                                ;  /*启动at*/
static const char* g_wifi_mode     = "AT+CWMODE=%d\r\n"                  ;  /*设置wifi模式*/
static const char* g_wifi_sta_con  = "AT+CWJAP=\"%s\",\"%s\"\r\n"        ;  /*连接到到ap */

static const char* g_onenet_cfg              = "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n";
static const char* g_onenet_con              = "AT+MQTTCONN=0,\"%s\",%s,1\r\n";   
static const char* g_onenet_discon           = "AT+MQTTCLEAN=0\r\n";

/*订阅主动上报*/
static const char* g_onenet_post_sub_reply   = "AT+MQTTSUB=0,\"$sys/%s/%s/thing/property/post/reply\",1\r\n";

/*订阅下发*/
static const char* g_onenet_post_sub_set     = "AT+MQTTSUB=0,\"$sys/%s/%s/thing/property/set\",1\r\n";

/*接收应答*/
static const char* g_onenet_post_pub_set_ans = "AT+MQTTPUB=0,\"$sys/%s/%s/thing/property/set_reply\",\"{\\\"id\\\":\\\"%u\\\"\\,\\\"code\\\":200\\,\\\"msg\\\":\\\"success\\\"}\",0,0\r\n";

/*上发数据*/
static const char* g_onenet_post_pub         = "AT+MQTTPUB=0,\"$sys/%s/%s/thing/property/post\",\"{\\\"id\\\":\\\"%s\\\"\\,\\\"version\\\":\\\"1.0\\\"\\,\\\"params\\\":{%s}}\",0,0\r\n";

static const char* g_ans_wifi_sta_con     = "WIFI CONNECTED\r\n"             ;  /*wifi sta 连接*/

static const char* g_ans_onenet_discon    = "+MQTTDISCONNECTED:"             ;  /*onenet 断连*/
static const char* g_ans_onenet_con       = "+MQTTCONNECTED:"                ;  /*onenet 连接*/
static const char* g_ans_onenet_recv      = "+MQTTSUBRECV:"                  ;  /*onenet 接收数据*/
static const char* g_ans_onenet_set       = "thing/property/set"             ;  /*onenet 设置*/

c_esp8266 esp8266_create(u8 uart_id)
{
    int ret = 0;
    c_esp8266    new    = {0}  ;
    m_esp8266*   m_this = NULL ;
    BaseType_t   os_ret = 0    ;
    data_cut_cfg cfg = {0};

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_esp8266));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_esp8266));
    m_this = new.this; 

    /*保存相关变量*/
    m_this->m_uart_id = uart_id;
                                             
    new.onenet_start     = m_onenet_start ;
    new.onenet_send      = m_onenet_send  ;
    new.onenet_callback  = m_onenet_callback;
                                             
    /*初始化串口*/
    ret = my_uart.init(uart_id,ESP8266_BAUDRATE,64);
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
    
    /*创建事件组*/
    m_this->m_event = xEventGroupCreate();
    if(NULL == m_this->m_event)
    {
        log_error("Event group create failed.");
        goto error_handle;
    }

    
    return new;

    error_handle:

    new.this = NULL;
    return new;
}

static int m_onenet_start     (const c_esp8266* this, const sta_inform* sta,const onenet_inform* onenet)
{
    m_esp8266* m_this   = NULL ;
    BaseType_t   os_ret = 0    ;
    
    /*参数检测*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_PARAM;
    }
    m_this = this->this;
    
    /*拷贝wifi账号密码*/
    strcpy(m_this->m_name,sta->o_name);
    strcpy(m_this->m_key,sta->o_key);
    
    strcpy(m_this->m_onenet_product_id,onenet->o_product_id);
    strcpy(m_this->m_onenet_driver_name,onenet->o_driver_name);
    strcpy(m_this->m_onenet_passwd,onenet->o_passwd);
    
    /*创建esp8266任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_task            ,
                         (const char*    )"esp8266 task"            ,
                         (uint16_t       )ESP8266_ONENET_CON_TASK_STK  ,
                         (void*          )m_this                    ,
                         (UBaseType_t    )ESP8266_ONENET_CON_TASK_PRO       ,
                         (TaskHandle_t*  )&m_this->m_task);
                             
    if(pdPASS != os_ret)
    {
        log_error("Task creat failed,ret=%d",(int)os_ret);
        return E_ERROR;
    }
    
    /*创建esp8266接收任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_recv            ,
                         (const char*    )"esp8266 recv task"           ,
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


static int m_onenet_send      (const c_esp8266* this, const char* format, ...)
{
    onenet_msg  new_msg = {0};
    m_esp8266*  m_this = NULL;
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

    /*确认onenet状态*/
    event = xEventGroupGetBits(m_this->m_event);
    if(0 == (ESP8266_ONENET_CONNECT & event))
    {
        log_error("Onenet not connected.");
        return E_ERROR;
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
static int m_onenet_callback  (const c_esp8266* this, void* param,int (*callback)(void* param,const char* msg))
{
    m_esp8266*        m_this = NULL;

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
    m_esp8266* this   = NULL ;
    int        ret      = 0    ;
    BaseType_t   os_ret = 0    ;
    EventBits_t  event_bit = 0;
    u32        event    = 0;
    onenet_msg   new_msg  = {0};

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

        /*设置工作模式*/
        ret = m_cmd_send(this,3,1000,CMD_ANSWER_OK,g_wifi_mode,ESP8266_WIFI_STA);
        if(E_OK != ret)
        {
            log_error("Mode set failed.");
            continue;
        }      
    
        /*清除连接事件*/
        xEventGroupClearBits(this->m_event,ESP8266_WIFI_STA_CONNECT);
        
        /*sta wifi 的账号密码*/
        ret = m_cmd_send(this,3,1000,NULL,g_wifi_sta_con,this->m_name,this->m_key);
        if(E_OK != ret)
        {
            log_error("Mode set failed.");
            continue;
        }   
        
        /*等待最长 30s wifi连接*/
        event_bit = xEventGroupWaitBits(this->m_event,ESP8266_WIFI_STA_CONNECT,pdFALSE,pdFALSE,30000);
        if(0 == (event_bit & ESP8266_WIFI_STA_CONNECT))
        {
            log_error("Event wait timeout.");
            continue;
        }
        
        break;
    }while(1);
    
    vTaskDelay(5000);
    
    /*进入onenet的连接逻辑*/
    do
    {
        /*关闭onenet连接*/
        ret = m_cmd_send(this,3,1000,NULL,g_onenet_discon);
        if(E_OK != ret)
        {
            log_error("Cmd send failed.");
            continue;
        }
        
        vTaskDelay(100);

        /*配置onenet*/
        ret = m_cmd_send(this,3,1000,CMD_ANSWER_OK,g_onenet_cfg,this->m_onenet_driver_name,this->m_onenet_product_id,this->m_onenet_passwd);
        if(E_OK != ret)
        {
            log_error("Cmd send failed.");
            continue;
        }

        /*启动onenet连接*/
        ret = m_cmd_send(this,3,5000,CMD_ANSWER_OK,g_onenet_con,ESP_ONENET_SERVER_IP,ESP_ONENET_SERVER_POST);
        if(E_OK != ret)
        {
            log_error("Cmd send failed.");
            continue;
        }
        
        /*等待最长 30s onenet连接*/
        event_bit = xEventGroupWaitBits(this->m_event,ESP8266_ONENET_CONNECT,pdFALSE,pdFALSE,30000);
        if(0 == (event_bit & ESP8266_ONENET_CONNECT))
        {
            log_error("Event wait timeout.");
            continue;
        }
        
        break;
    }while(1);
    
    /*订阅主动上报*/
    ret = m_cmd_send(this,1,1000,CMD_ANSWER_OK,g_onenet_post_sub_reply,this->m_onenet_product_id,this->m_onenet_driver_name);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
    }

    /*订阅下发*/
    ret = m_cmd_send(this,1,1000,CMD_ANSWER_OK,g_onenet_post_sub_set,this->m_onenet_product_id,this->m_onenet_driver_name);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
    }
    
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
        
        /*发送数据*/
        ret = m_cmd_send(this,2,1000,NULL,g_onenet_post_pub,this->m_onenet_product_id,this->m_onenet_driver_name,"123",new_msg.o_data);
        if(E_OK != ret)
        {
            log_error("Cmd send failed.");
            continue;
        }
    
        vTaskDelay(1500);
        
    }while(1);
    
}
static void m_recv(void* pdata)
{
    m_esp8266* this   = NULL ;
    int        ret      = 0    ;
    BaseType_t   os_ret = 0    ;
    EventBits_t  event_bit = 0;
    u32          event    = 0;
    u8*   new_msg  = NULL;
    
    char*  recv_msg_head = NULL;
    char*  recv_msg_end = NULL;
    char*  call_back_msg = NULL;
    u32    id = 0;
    
    /*参数检测*/
    if(NULL == pdata)
    {
        log_error("Null pointer.");
        vTaskDelete(NULL);   /*删除本任务*/
    }
    this = pdata;
    
    event_bit = xEventGroupWaitBits(this->m_event,ESP8266_ONENET_CONNECT,pdFALSE,pdFALSE,portMAX_DELAY);
    if(0 == (event_bit & ESP8266_ONENET_CONNECT))
    {
        log_error("Event wait error.");
        vTaskDelete(NULL);   /*删除本任务*/
    }
    
    while(1)
    {
        if(NULL != new_msg)
        {
            vPortFree(new_msg);
            new_msg = NULL;
        }
        recv_msg_head = NULL;
        recv_msg_end = NULL;
        
        /*获取要上报的onenet消息*/
        memset(&new_msg,0,sizeof(new_msg));
        os_ret = xQueueReceive(this->m_recv_msg,&new_msg,portMAX_DELAY);
        if(pdTRUE != os_ret)
        {
            continue;
        }
     
        /*取到ID*/
        recv_msg_head = strstr((char*)new_msg,"\"id\"");
        if(NULL == recv_msg_head)
        {
            log_error("Error msg.");
            continue;
        }
        ret = sscanf(recv_msg_head,"\"id\":\"%u\"",&id);
        if(ret < 1)
        {
            log_error("Error msg.");
            continue;
        }

        /*应答接收成功*/
        ret = m_cmd_send(this,1,1000,CMD_ANSWER_OK,g_onenet_post_pub_set_ans,this->m_onenet_product_id,this->m_onenet_driver_name,id);
        if(E_OK != ret)
        {
            log_error("Set ans failed.");
            continue;
        }

        /*找到参数部分*/
        recv_msg_head = strstr((char*)new_msg,"\"params\":{");
        if(NULL == new_msg)
        {
            log_error("Error msg.");
            continue;
        }
        recv_msg_head = strstr((char*)recv_msg_head,"{");
        if(NULL == new_msg)
        {
            log_error("Error msg.");
            continue;
        }
        recv_msg_end = strstr((char*)recv_msg_head,"}");
        if(NULL == new_msg)
        {
            log_error("Error msg.");
            continue;
        }

        /*调用回调*/
        if(NULL != this->m_onenet_recv_callback)
        {
            call_back_msg = pvPortMalloc(recv_msg_end - recv_msg_head);
            if(NULL == call_back_msg)
            {
                log_error("Out of memory");
                continue;
            }
            memset(call_back_msg,0,recv_msg_end - recv_msg_head);
            memcpy(call_back_msg,recv_msg_head + 1,recv_msg_end - recv_msg_head - 1);

            ret = this->m_onenet_recv_callback(this->m_onenet_recv_callback_param,call_back_msg);

            vPortFree(call_back_msg);
            call_back_msg = NULL;
        }
    };
}

static int  m_callback(void* param,const u8* data,u16 data_len)
{
    m_esp8266* this   = param ;
    int        ret      = 0    ;
    BaseType_t   os_ret = 0    ;
    EventBits_t  event_bit = 0;  
    u8*        recv_data = NULL;    
    
    log_inform("%s",data);
    
    /*参数检测*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_ERROR;   /*删除本任务*/
    }
    this = param;
    
    /*连接事件*/
    
    /*wifi连接事件*/
    if(NULL != strstr((char*)data,g_ans_wifi_sta_con))
    {
        xEventGroupSetBits(this->m_event,ESP8266_WIFI_STA_CONNECT);
    }
    
    /*onenet连接事件*/
    else if(NULL != strstr((char*)data,g_ans_onenet_con))
    {
        xEventGroupSetBits(this->m_event,ESP8266_ONENET_CONNECT);
    }
    
    /*onenet 下发事件*/
    else if(NULL != strstr((char*)data,g_ans_onenet_recv) && NULL != strstr((char*)data,g_ans_onenet_set))
    {
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
    
    
    else if(1 == this->m_wait_ask)
    {
        /*数据拷贝出去 做比对*/
        //memcpy(this->m_recv_buf,data,data_len);
        if(NULL != strstr((char*)this->m_recv_buf,CMD_ANSWER_OK))
        {
            /*通知 比对成功*/
            //os_ret = xSemaphoreGive(this->m_cmd_signal);
            os_ret = xTaskNotifyGive(this->m_task);
            if(pdTRUE != os_ret)
            {
                log_error("Semaphore give failed.");
                return E_ERROR;
            }
        }  
    }
    
    return E_OK;
}

static int m_cmd_send(m_esp8266* this,u8 count,TickType_t time,const char* answer,const char* data,...)
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
        memset((void*)this->m_send_buf,0,ESP8266_DATA_MAX_LEN);
        va_start( argptr, data );  // 初始化argptr 
        vsnprintf((void*)this->m_send_buf,ESP8266_DATA_MAX_LEN,data,argptr);
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

static int  m_reset   (m_esp8266* this,TickType_t time)
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