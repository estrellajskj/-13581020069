#include "driver_conf.h"

#ifdef DRIVER_EC800M_ALIYUN_ENABLED

#define QUEUE_LEN 10

typedef struct __M_EC800 m_ec800;

static const char* g_reset       = "AT+CFUN=1,1\r\n"                    ;  /*强制复位*/
static const char* g_close_ate   = "ATE0\r\n"                           ;  /*关闭回显*/
static const char* g_csq         = "AT+CSQ\r\n"                         ;  /*检查信号强度*/
static const char* g_test        = "AT\r\n"                             ;  /*测试命令*/
static const char* g_clost_sleep = "AT+QSCLK=0\r\n"                     ;  /*关闭休眠模式*/
static const char* g_check_card  = "AT+CPIN?\r\n"                       ;  /*确认卡状态*/
static const char* g_cfg_apn     = "AT+CGDCONT=1,\"IP\",\"CMNET\"\r\n"  ;  /*配置vpn 为移动*/
static const char* g_en_pdp      = "AT+CGACT=1,1\r\n"                   ;  /*激活上下文*/
#define TOPIC_LEN 128
static char g_sub_set[TOPIC_LEN]     = "AT+QMTSUB=0,1,\"/k20sfijc5OC/stm32/user/stm32_duan\",0\r\n"                 ; /*订阅设置->小程序*/
static char g_sub_get[TOPIC_LEN]     = "AT+QMTSUB=0,1,\"/sys/k20sfijc5OC/stm32/thing/service/property/set\",0\r\n" ; /*订阅小程序->设置*/
static const char* g_mqtt_open   = "AT+QMTOPEN=0,\"iot-as-mqtt.cn-shanghai.aliyuncs.com\",1883\r\n"           ; /*打开mqtt*/
static const char* g_mqtt_con    = "AT+QMTCONN=0,\"%s\",\"%s&%s\",\"%s\"\r\n"     ; /*连接到mqtt*/
static char g_send_cmd[TOPIC_LEN]    = "AT+QMTPUBEX=0,0,0,0,\"/%s/%s/user/4gDUAN\",%d\r\n"                     ; /*发送数据*/
static char g_send_cmd_mode[TOPIC_LEN]    = "AT+QMTPUBEX=0,0,0,0,\"/sys/%s/%s/thing/event/property/post\",%d\r\n"                     ; /*发送数据*/

static const char* g_ans_aliyun_open = "+QMTOPEN: 0,0\r\n"  ;
static const char* g_ans_aliyun_con  = "+QMTCONN: 0,0,0\r\n";
static const char* g_ans_aliyun_recv = "+QMTRECV:"     ;  /*阿里云接收 */
static const char* g_ans_card        = "+CPIN: READY\r\n"   ;  /*卡状态     */
static const char* g_ans_csq         = "+CSQ: %d,%d\r\n"    ;  /*信号强度*/
static const char* g_ans_ok          = "OK\r\n"             ;

static int m_ec800_start     (const c_ec800* this, const mqtt_inform* inform);
static int m_ec800_send      (const c_ec800* this, const char* format, ...);
static int m_ec800_callback  (const c_ec800* this, void* param,int (*callback)(void* param,const char* msg));

static int m_callback        (void* param,const u8* data,u16 data_len);
static int m_cmd_send        (m_ec800* this,u8 count,TickType_t time,const char* answer,const char* data,...);

static void m_send_task(void* param);
static void m_recv_task(void* param);

/*发送消息 类型*/
typedef struct __ALIYUN_MSG
{
	char o_data[EC800_PUSH_MAX_LEN];
}aliyun_msg;


typedef struct __M_EC800
{
    u8                 m_uart_id ;
    u8                 m_wait_ask;    /*等待命令应答*/
    u8                 m_recv_buf[EC800_DATA_MAX_LEN + 1];
    u8                 m_recv_len;
    u8                 m_send_buf[EC800_DATA_MAX_LEN + 1];
    u8                 m_aliyun_con                      ;
    
    char               m_client_id  [CLIENT_ID_MAX_LEN];
    char               m_passwd     [PASSWD_MAX_LEN];                    
    char               m_product_key[PRO_KEY_MAX_LEN];
    char               m_device_name[NAME_MAX_LEN];
    
    void*               m_aliyun_recv_callback_param;
    int               (*m_aliyun_recv_callback)(void* param,const char* msg);  /*接收回调函数*/
    
    TaskHandle_t    m_task      ;  /*主任务  */
    TaskHandle_t    m_recv_task ;  /*接收任务*/
    
    QueueHandle_t   m_send_msg  ;  /*发送消息*/
    QueueHandle_t   m_recv_msg  ;  /*接收消息*/
    
}m_ec800;

c_ec800 ec800_create(u8 uart_id)
{
    int           ret = 0;
    c_ec800       new    = {0}  ;
    m_ec800*      m_this = NULL ;
    BaseType_t    os_ret = 0  ;
    data_cut_cfg  cfg = {0};
    
    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_ec800));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_ec800));
    m_this = new.this; 
    
    m_this->m_uart_id    = uart_id;
    new.ec800_start     = m_ec800_start ;
    new.ec800_send      = m_ec800_send  ;
    new.ec800_callback  = m_ec800_callback;
    
    /*初始化串口*/
    ret = my_uart.init(uart_id,EC800_BAUDRATE,64);
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
    m_this->m_send_msg = xQueueCreate(QUEUE_LEN,sizeof(aliyun_msg));
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

static int m_ec800_start     (const c_ec800* this, const mqtt_inform* inform)
{
    m_ec800* m_this   = NULL ;
    BaseType_t   os_ret = 0    ;
    
    /*参数检测*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_PARAM;
    }
    m_this = this->this;
    
    /*拷贝用户信息*/
    strcpy(m_this->m_client_id   ,inform->o_client_id   );
    strcpy(m_this->m_passwd      ,inform->o_passwd      );
    strcpy(m_this->m_product_key ,inform->o_product_key );
    strcpy(m_this->m_device_name ,inform->o_device_name );
	strcpy(g_sub_set ,inform->sub_set );
	strcpy(g_sub_get,inform->sub_get );
	strcpy(g_send_cmd ,inform->send_cmd );
	strcpy(g_send_cmd_mode ,inform->send_cmd_mode );
    log_inform("m_this->m_client_id: %s", m_this->m_client_id);
	log_inform("m_this->m_passwd: %s", m_this->m_passwd);
	log_inform("m_this->m_product_key: %s", m_this->m_product_key);
	log_inform("m_this->m_device_name: %s", m_this->m_device_name);
	
    /*创建ec800发送任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_send_task            ,
                         (const char*    )"ec800 send task"      ,
                         (uint16_t       )EC800_ALIYUN_SEND_TASK_STK  ,
                         (void*          )m_this                    ,
                         (UBaseType_t    )EC800_ALIYUN_SEND_TASK_PRO       ,
                         (TaskHandle_t*  )&m_this->m_task);
                             
    if(pdPASS != os_ret)
    {
        log_error("Task creat failed,ret=%d",(int)os_ret);
        return E_ERROR;
    }
    
    /*创建ec800接收任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_recv_task            ,
                         (const char*    )"ec800 recv task"           ,
                         (uint16_t       )EC800_ALIYUN_RECV_TASK_STK  ,
                         (void*          )m_this                    ,
                         (UBaseType_t    )EC800_ALIYUN_RECV_TASK_PRO  ,
                         (TaskHandle_t*  )&m_this->m_recv_task);
                             
    if(pdPASS != os_ret)
    {
        log_error("Task creat failed,ret=%d",(int)os_ret);
        return E_ERROR;
    }
    
    return E_OK;
}

static int m_ec800_send      (const c_ec800* this, const char* format, ...)
{
    aliyun_msg  new_msg = {0};
    m_ec800*  m_this = NULL;
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
    
    /*阿里云是否已连接*/
    if(0 == m_this->m_aliyun_con)
    {
        return E_OK;
    }

    va_start(argptr, format);  // 初始化argptr 
    vsnprintf(new_msg.o_data, EC800_PUSH_MAX_LEN, format, argptr);
    va_end(argptr);
    
    if(EC800_PUSH_MAX_LEN < strlen(new_msg.o_data))
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*确认onenet状态*/

    /*新消息 压到队列*/
    os_ret = xQueueSend(m_this->m_send_msg,&new_msg,0);
    if(pdTRUE != os_ret)
    {
        log_error("Send message filed.");
        return E_ERROR;
    }  
    
    return E_OK;
}

static int m_ec800_callback  (const c_ec800* this, void* param,int (*callback)(void* param,const char* msg))
{
    m_ec800*        m_this = NULL;

    /*检查参数*/
    if(NULL == this|| NULL == this->this)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    m_this->m_aliyun_recv_callback       = callback;
    m_this->m_aliyun_recv_callback_param = param;
    
    

    return E_OK;
}

static int m_callback        (void* param,const u8* data,u16 data_len)
{
    m_ec800* this   = param ;
    int        ret      = 0    ;
    BaseType_t   os_ret = 0    ;
    EventBits_t  event_bit = 0;  
    u8*        recv_data = NULL;    
    
    /*如果是 空换行 不要  无效命令*/
    if('\r' == data[0] && '\n' == data[1] && 2 == data_len)
    {
        return E_OK;
    }
    
    log_inform("%s",data);
    
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
    
    /*如果是接收 下发到接收*/
    if(NULL != strstr((char*)data,g_ans_aliyun_recv))
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
        ;/*抛弃 无效命令*/
    }

    
    
    return E_OK;
}

static void m_send_task(void* param)
{
    m_ec800* this   = NULL ;
    int          ret      = 0    ;
    BaseType_t   os_ret = 0    ;
    EventBits_t  event_bit = 0;
    u32          event    = 0;
    aliyun_msg   new_msg  = {0};

    /*参数检测*/
    if(NULL == param)
    {
        log_error("Null pointer.");
        vTaskDelete(NULL);   /*删除本任务*/
    }
    this = param;
    
    vTaskDelay(1000);
    
    /*进入初始化 入网检测*/
    do
    {     
        /*复位 模块*/
        ret = m_cmd_send(this,1,0,NULL,g_reset);
        if(E_OK != ret)
        {
            log_error("Reset failed.");
            continue;
        }
        
        vTaskDelay(10000);  
        
        /*关闭回显*/
        ret = m_cmd_send(this,1,0,NULL,g_close_ate);
        if(E_OK != ret)
        {
            log_error("No card.");
            continue;
        }
        
        vTaskDelay(1000);
        
        /*关闭休眠模式*/
        ret = m_cmd_send(this,1,1000,g_ans_ok,g_clost_sleep);
        if(E_OK != ret)
        {
            log_error("Cmd failed.");
            continue;
        }
        
				vTaskDelay(500);
				
        /*检查卡状态*/
        ret = m_cmd_send(this,1,1000,g_ans_card,g_check_card);
        if(E_OK != ret)
        {
            log_error("No card.");
            continue;
        }
				
//				vTaskDelay(500);
//        
//        /*确认信号强度*/
//        memset(this->m_recv_buf,0,EC800_DATA_MAX_LEN);
//        this->m_wait_ask = 1;
//        ret = my_uart.send(this->m_uart_id,(const u8*)g_csq,strlen(g_csq));
//        if(E_OK != ret)
//        {
//            log_error("Send failed.");
//            continue;
//        }
//				
//        ret = ulTaskNotifyTake(pdTRUE,1000);
//        if(0 != os_ret)
//        {
//            log_error("At ack timer out.");
//            continue;
//        }
//        
//        u32 p1 = 0;
//        u32 p2 = 0;
//        ret = sscanf((const char*)this->m_recv_buf,g_ans_csq,&p1,&p2);
//        if(ret < 2)
//        {
//            log_error("Formate failed.");
//            continue;
//        }
//        
//        /*检查信号强度*/
//        if(p1 < 2 || p1 > 31 || p2 > 7)
//        {
//            log_error("Poor signal quality");
//            continue;
//        }
        
				vTaskDelay(500);
				
        /*设置运营商为 移动*/
        ret = m_cmd_send(this,1,1000,g_ans_ok,g_cfg_apn);
        if(E_OK != ret)
        {
            log_error("Cmd failde.");
            continue;
        }
				
				vTaskDelay(500);
        
        /*激活上下文 */
        ret = m_cmd_send(this,1,1000,g_ans_ok,g_en_pdp);
        if(E_OK != ret)
        {
            log_error("Cmd failde.");
            continue;
        }
        
				vTaskDelay(500);
				
        /*打开 mqtt*/
        ret = m_cmd_send(this,3,1000,g_ans_ok,g_mqtt_open);
        if(E_OK != ret)
        {
            log_error("Cmd failde.");
            continue;
        }
        
        vTaskDelay(500);
        
        /*连接 mqtt*/
        ret = m_cmd_send(this,3,1000,g_ans_ok,g_mqtt_con,this->m_client_id,this->m_device_name,this->m_product_key,this->m_passwd);
        if(E_OK != ret)
        {
            log_error("Cmd failde.");
            continue;
        }
        
        vTaskDelay(500);
        
        /*订阅 接收*/
        ret = m_cmd_send(this,3,1000,g_ans_ok,g_sub_get);
        if(E_OK != ret)
        {
            log_error("Cmd failde.");
            continue;
        }
        
        vTaskDelay(500);
        
        /*订阅 发送*/
        ret = m_cmd_send(this,3,1000,g_ans_ok,g_sub_set);
        if(E_OK != ret)
        {
            log_error("Cmd failde.");
            continue;
        }
        
        break;
    }while(1);
    
    this->m_aliyun_con = 1;
    u32 send_len = 0;
    
    while(1)
    {
        /*获取要上报的onenet消息*/
        memset(&new_msg,0,sizeof(new_msg));
        os_ret = xQueueReceive(this->m_send_msg,&new_msg,portMAX_DELAY);
        if(pdTRUE != os_ret)
        {
            continue;
        }
        
        /*数据有效性*/
        if(0 == strlen(new_msg.o_data))
        {
            continue;
        }
        
        memset((void*)this->m_send_buf,0,EC800_DATA_MAX_LEN);
        snprintf((char*)this->m_send_buf,EC800_DATA_MAX_LEN,"{%s}\r\n",new_msg.o_data);
        send_len = strlen((char*)this->m_send_buf);
        if(0 == send_len )
        {
            send_len = 0;
            memset((void*)this->m_send_buf,0,EC800_DATA_MAX_LEN);
            continue;
        }
        
        /*请求发送数据*/
        ret = m_cmd_send(this,1,0,NULL,g_send_cmd,this->m_product_key,this->m_device_name,send_len);
        if(E_OK != ret)
        {
            log_error("Cmd send failed.");
            continue;
        }
        
        /*发送数据*/
        ret = m_cmd_send(this,1,1000,g_ans_ok,"{%s}\r\n",new_msg.o_data);
        if(E_OK != ret)
        {
            log_error("Cmd send failed.");
            continue;
        }
		
		/*请求发送数据*/
        ret = m_cmd_send(this,1,0,NULL,g_send_cmd_mode,this->m_product_key,this->m_device_name,send_len);
        if(E_OK != ret)
        {
            log_error("Cmd send failed.");
            continue;
        }
        
        /*发送数据*/
        ret = m_cmd_send(this,1,1000,g_ans_ok,"{%s}\r\n",new_msg.o_data);
        if(E_OK != ret)
        {
            log_error("Cmd send failed.");
            continue;
        }
        
        //vTaskDelay(100);
    }
    
}

static void m_recv_task(void* param)
{
    m_ec800* this   = NULL ;
    u8*   new_msg  = NULL;
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
        
        /*找到key的位置*/
        str_param = strstr((const char*)new_msg,"{\"data\":");
        if(NULL == str_param)
        {
            log_error("Error msg.");
            continue;
        }
        
        /*调用回调函数*/
        if(NULL != this->m_aliyun_recv_callback)
        {
            this->m_aliyun_recv_callback(this->m_aliyun_recv_callback_param,str_param);
        }
    }
}

static int m_cmd_send(m_ec800* this,u8 count,TickType_t time,const char* answer,const char* data,...)
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
       
    /*拷贝应答字符串 等待比对*/
    memset(this->m_recv_buf,0,EC800_DATA_MAX_LEN);
    
    if(NULL != answer)
    {
        strcpy((char*)this->m_recv_buf,answer);
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
        memset((void*)this->m_send_buf,0,EC800_DATA_MAX_LEN);
        va_start( argptr, data );  // 初始化argptr 
        vsnprintf((void*)this->m_send_buf,EC800_DATA_MAX_LEN,data,argptr);
        va_end(argptr);
        
        /*发送命令*/
        vTaskDelay(100);
        log_inform("%s",this->m_send_buf);
        ret = my_uart.send(this->m_uart_id,this->m_send_buf,strlen((const char*)this->m_send_buf));
        if(E_OK != ret)
        {
            log_error("Send failed.");
            goto error_handle;
        }
        
        /*如果需要应答 等待应答命令*/
        if(0 != time && NULL != answer)
        {
            ret = ulTaskNotifyTake(pdTRUE,time);
            if(0 != os_ret)
            {
                log_error("At ack timer out.");
                continue;
            }
            
            /*成功*/
            if(2 == this->m_wait_ask)
            {
                this->m_wait_ask = 0;
                return E_OK;
            }
            
            /*失败*/
            else
            {
                this->m_wait_ask = 0;
                return E_ERROR;
            }                   
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

#endif

