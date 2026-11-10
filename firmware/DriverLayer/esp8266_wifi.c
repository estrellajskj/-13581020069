#include "driver_conf.h"

#ifdef DRIVER_ESP8266_WIFI_ENABLED

#define MODULE_NAME       "esp8266"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define SEND_QUEUE_LEN          10
#define AT_RECV_MAX_LEN        (ESP8266_DATA_MAX_LEN + 20)
#define NAME_PASSWARD_MAX_LEN   20

/*发送消息*/
typedef struct _ESP8266_SEND_MSG
{
    void*  o_data     ;  /*数据地址*/
    u16    o_data_len ;  /*数据长度*/
}esp8266_send_msg;

typedef struct __M_ESP8266
{
    u8              m_uart_id;
    u8              m_name[NAME_PASSWARD_MAX_LEN];
    u8              m_password[NAME_PASSWARD_MAX_LEN];
    u8              m_recv_data[AT_RECV_MAX_LEN];
    TaskHandle_t    m_task  ;
    QueueHandle_t   m_queue ;  /*短信消息队列*/
    c_data_cut      m_cut   ;  /*数据裁剪*/
    int           (*m_callback)(void* param,const u8* data,const u16 data_len);  /*接收回调函数*/
    void*           m_callback_param;
}m_esp8266;

static int m_send(const c_esp8266_wifi* this,const u8* data ,u16 data_len);
static int m_set_callback(const c_esp8266_wifi* this,void* param,int (*callback)(void* param,const u8* data,u16 data_len));
static int m_callback(void* param,const u8* data,u16 data_len);
static void m_task(void* pdata);
static int m_cmd_send(const m_esp8266* this,const u8* data,u16 data_len,TickType_t time);
static int m_data_send(m_esp8266* this,const u8* data,u16 data_len,TickType_t time);
static int m_recv_find(m_esp8266* this,const char* str,TickType_t time);
static int m_recv_check(const m_esp8266* this);
static int m_cfg(const m_esp8266* this);

static const char* g_cmd_rst    = "AT+RST\r\n"                            ;  /*重启模块*/
static const char* g_cmd_start  = "AT\r\n"                                ;  /*启动at*/
static const char* g_cmd_wifi   = "AT+CWSAP=\"%s\",\"%s\",1,0,1,0\r\n";  /*设置wifi账号密码*/
static const char* g_cmd_ip     = "AT+CIPAP=\"%s\",\"%s\",\"%s\"\r\n" ;  /*设置ip*/
static const char* g_cmd_mode   = "AT+CWMODE=2\r\n";                         /*设置ap模式*/
static const char* g_cmd_ip_mod = "AT+CIPMODE=0\r\n";
static const char* g_cmd_ip_mux = "AT+CIPMUX=1\r\n";
static const char* g_cmd_server = "AT+CIPSERVER=1,%u\r\n";  /*启动服务器*/
static const char* g_cmd_send   = "AT+CIPSEND=0,%d\r\n";    /*启动发送*/

c_esp8266_wifi esp8266_wifi_create(u8 uart_id,const char* name,const char* password)
{
    int ret = 0;
    c_esp8266_wifi    new    = {0}  ;
    m_esp8266*   m_this = NULL ;
    BaseType_t   os_ret = 0    ;
    data_cut_cfg cfg = {0};
    
    /*参数检查*/
    if(NULL == name || NULL == password)
    {
        log_error("Null pointer.");
        return new;
    }
    if(strlen(name) > NAME_PASSWARD_MAX_LEN || strlen(password) > NAME_PASSWARD_MAX_LEN)
    {
        log_error("Param error.");
        return new;
    }

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
    strcpy((char*)m_this->m_name,name);
    strcpy((char*)m_this->m_password,password);
    new.send = m_send;
    new.set_callback = m_set_callback;

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
    
    /*初始化数据裁剪器*/
    cfg.o_end     = (u8*)"\r\n"     ;
    cfg.o_end_len = 2               ;
    cfg.o_max_len = AT_RECV_MAX_LEN ;
    m_this->m_cut = data_cut_creat(CUT_TYPE_B,&cfg);
    if(NULL == m_this->m_cut.this)
    {
        log_error("Cut creat filed.");
        goto error_handle;
    }
    
    m_this->m_queue = xQueueCreate(SEND_QUEUE_LEN,sizeof(esp8266_send_msg));
    if(NULL == m_this->m_queue)
    {
        log_error("Queue creat filed.");
        goto error_handle;
    }

    /*创建任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_task        ,
                     (const char*    )"esp8266 task"    ,
                     (uint16_t       )ESP8266_WIFI_TASK_STK  ,
                     (void*          )m_this            ,
                     (UBaseType_t    )ESP8266_WIFI_TASK_PRO  ,
                     (TaskHandle_t*  )&m_this->m_task)  ;
    if(pdPASS != os_ret)
    {
        log_error("Esp8266 task creat filed,ret=%d",(int)os_ret);
        goto error_handle;
    }

    return new;

    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static void m_task(void* pdata)
{
    int ret = 0;
    m_esp8266* this = NULL;
    BaseType_t  os_ret = 0 ;    
    u8* new_msg = 0;
    u16 recv_len = 0;
    esp8266_send_msg  send_msg = {0};

    /*参数检查*/
    if(NULL == pdata)
    {
        log_error("Param error.");
        goto error_handle;
    }
    this = pdata;

    /*配置esp*/
    while(1)
    {
        vTaskDelay(1500);
        
        ret = m_cfg(this);
        if(E_OK != ret)
        {
            log_warning("Config failed.");
            continue;
        }
        
        break;
    }

    WATI_CONNECT:
    
    /*等待连接*/
    while(1)
    {
        
        recv_len = AT_RECV_MAX_LEN;
        ret = this->m_cut.get_static(&this->m_cut,this->m_recv_data,&recv_len,portMAX_DELAY);
        if(E_OK != ret)
        {
            log_error("Get failed.");
            continue;
        }  
        if(NULL != strstr((char*)this->m_recv_data,"0,CONNECT"))
        {
            log_inform("Clinet connect.");
            break;
        }
    }
    
    /*收发数据*/
    while(1)
    {
           
        /*收数据*/
        recv_len = AT_RECV_MAX_LEN;
        ret = this->m_cut.get_static(&this->m_cut,this->m_recv_data,&recv_len,10);
        if(E_OK == ret)
        {
            if(NULL != strstr((char*)this->m_recv_data,"0,CLOSED"))
            {
                log_inform("Clinet offline.");
                goto WATI_CONNECT;
            }
            
            /*接收消息*/
            else if(NULL != strstr((char*)this->m_recv_data,"+IPD,0,"))
            {
                ret = m_recv_check(this);
                if(E_OK != ret)
                {
                    log_error("Recv failed.");                   
                }
            }
        }  
        
        /*发数据 有消息就会立即发 没有就跑去取消息*/
        os_ret = xQueueReceive(this->m_queue,&send_msg,0);
        if(pdTRUE != os_ret)
        {
            //log_error("Failed to accept message ");
            continue;
        }
        
        /*发送数据*/
        ret = m_data_send(this,send_msg.o_data,send_msg.o_data_len,1000);
        if(E_OK != ret)
        {
            log_error("Send failed.");
        }
        if(NULL != send_msg.o_data)
        {
            vPortFree(send_msg.o_data);
            send_msg.o_data = NULL;
        }
        send_msg.o_data_len = 0;
    }
    
    error_handle:
    
    vTaskDelete(NULL);   /*删除本任务*/
}


static int m_cmd_send(const m_esp8266* this,const u8* data,u16 data_len,TickType_t time)
{
    int ret = 0;
    u8* new_msg = 0;
    u16 recv_len = 0;
    u8  cmd_recv[AT_RECV_MAX_LEN] = {0};

    /*参数检测*/
    if(NULL == this || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(0 == data_len)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*清除一下buf*/
    ret = this->m_cut.clear(&this->m_cut);
    if(E_OK != ret)
    {
        log_error("Clear failed.");
        return E_ERROR;
    }

    /*发送命令*/
    ret = my_uart.send(this->m_uart_id,data,data_len);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }
    
    /*收命令 会收到一条和发送的一样的命令*/
    memset(cmd_recv,0,AT_RECV_MAX_LEN);
    memcpy(cmd_recv,data,strlen((char*)data) - 2);
    ret = m_recv_find((m_esp8266*)this,(char*)cmd_recv,time);
    if(E_OK != ret)
    {
        log_error("Find failed.");
        return E_ERROR;
    }
    
    /*等待ok*/
    ret = m_recv_find((m_esp8266*)this,"OK\r\n",time);
    if(E_OK != ret)
    {
        log_error("Find failed.");
        return E_ERROR;
    }   

    return E_OK;
}

static int m_data_send(m_esp8266* this,const u8* data,u16 data_len,TickType_t time)
{
    int ret = 0;
    u8* new_msg = 0;
    u16 recv_len = 0;

    /*参数检测*/
    if(NULL == this || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(0 == data_len || ESP8266_DATA_MAX_LEN < data_len)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*清除一下buf*/
    ret = this->m_cut.clear(&this->m_cut);
    if(E_OK != ret)
    {
        log_error("Clear failed.");
        return E_ERROR;
    }

    /*拼接启动发送*/
    memset(this->m_recv_data,0,AT_RECV_MAX_LEN);
    ret = snprintf((char*)this->m_recv_data,AT_RECV_MAX_LEN,g_cmd_send,data_len);
    if(ret < 0)
    {
        log_error("Format cmd error.");
        return E_ERROR;
    }
    ret = my_uart.send(this->m_uart_id,this->m_recv_data,strlen((char*)this->m_recv_data));
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }
    
    /*收命令 会收到一条和发送的一样的命令*/
    this->m_recv_data[strlen((char*)this->m_recv_data) - 1] = '\0';
    this->m_recv_data[strlen((char*)this->m_recv_data) - 2] = '\0';
    ret = m_recv_find(this,(char*)this->m_recv_data,time);
    if(E_OK != ret)
    {
        log_error("Find failed.");
        return E_ERROR;
    }
    
    /*发送数据*/
    ret = my_uart.send(this->m_uart_id,data,data_len);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }
    
    /*等待ok*/
    ret = m_recv_find(this,"SEND OK\r\n",time);
    if(E_OK != ret)
    {
        log_error("Find failed.");
        return E_ERROR;
    }   

    return E_OK;
    
    error_handle:
    
    return E_ERROR;
}

static int m_callback(void* param,const u8* data,u16 data_len)
{
    int ret = 0;
    m_esp8266*  m_this = NULL;

    /*参数检查*/
    if(NULL == param || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(0 == data_len)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = param;

    /*数据压入给数据裁剪对象*/
    ret = m_this->m_cut.add(&m_this->m_cut,data,data_len);
    if(E_OK != ret)
    {
        log_error("Cut add failed.");
        return E_ERROR;
    }

    return E_OK;
}

static int m_send(const c_esp8266_wifi* this,const u8* data ,u16 data_len)
{
    esp8266_send_msg  new_msg = {0};
    m_esp8266*        m_this = NULL;
    BaseType_t       os_ret  = 0;

    /*检查参数*/
    if(NULL == this|| NULL == this->this || NULL == data)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    /*参数检查*/
    if(0 == data_len || ESP8266_DATA_MAX_LEN < data_len)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    
    /*检查客户端状态*/

    /*为数据申请新内存*/
    new_msg.o_data = pvPortMalloc(data_len);
    if(NULL == new_msg.o_data)
    {
        log_error("Out of memory.");
        return E_OUT_OF_MEMORY;
    }
    memset(new_msg.o_data,0,sizeof(data_len));

    /*拷贝数据*/
    memcpy(new_msg.o_data,data,data_len);
    new_msg.o_data_len = data_len;

    /*新消息 压到队列*/
    os_ret = xQueueSend(m_this->m_queue,&new_msg,0);
    if(pdTRUE != os_ret)
    {
        //log_error("Send message filed.");
        vPortFree(new_msg.o_data);
        new_msg.o_data = NULL;
        return E_ERROR;
    }   
    
    return E_OK;
}

static int m_recv_find(m_esp8266* this,const char* str,TickType_t time)
{
    int ret = 0;
    BaseType_t  os_ret = 0 ;
    u16 recv_len = 0;   
    char* param = NULL;

    /*参数检测*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
            
            
        if(E_OK == ret)
        {
            
            
            
        } 

    do
    {
        /*等待回应*/
        recv_len = AT_RECV_MAX_LEN;
        ret = this->m_cut.get_static(&this->m_cut,this->m_recv_data,&recv_len,time);
        if(E_OK == ret)
        {
            /*对比信息*/
            if(NULL != strstr((char*)this->m_recv_data,str))
            {
                return E_OK;
            }
            
            /*接收消息*/
            else if(NULL != strstr((char*)this->m_recv_data,"+IPD,0,"))
            {
                ret = m_recv_check(this);
                if(E_OK != ret)
                {
                    log_error("Recv failed.");                   
                }
            }
            
            else if(NULL != strstr((char*)this->m_recv_data,"0,CLOSED"))
            {
                log_inform("Clinet offline.");
            }
        }
        
        
    }while(E_OK == ret);

    return E_ERROR;
}

static int m_cfg(const m_esp8266* this)
{
    int ret = 0;
    u8 str[AT_RECV_MAX_LEN + 1] = {0};
    
    /*参数检测*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    /*启动at命令*/
    ret = m_cmd_send(this,(const u8*)g_cmd_start,strlen(g_cmd_start),1000);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        return E_ERROR;
    }
    
    /*复位命令*/
    //ret = m_cmd_send(this,(const u8*)g_cmd_rst,strlen(g_cmd_rst),1000);
    ret = my_uart.send(this->m_uart_id,(const u8*)g_cmd_rst,strlen(g_cmd_rst));
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }
    
    
    /*关闭裁剪 esp8266*/
    ret = this->m_cut.close(&this->m_cut);
    if(E_OK != ret)
    {
        log_error("Cut close failed.");
        return E_ERROR;
    }
    
    vTaskDelay(2000);
    
    /*打开裁剪 esp8266*/
    ret = this->m_cut.open(&this->m_cut);
    if(E_OK != ret)
    {
        log_error("Cut open failed.");
        return E_ERROR;
    }
    
    /*启动at命令*/
    ret = m_cmd_send(this,(const u8*)g_cmd_start,strlen(g_cmd_start),1000);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        return E_ERROR;
    }
    
    /*启动AP*/
    ret = m_cmd_send(this,(const u8*)g_cmd_mode,strlen(g_cmd_mode),1000);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        return E_ERROR;
    }
    
    /*设置wifi 账号密码*/
    memset(str,0,AT_RECV_MAX_LEN);
    ret = snprintf((char*)str,AT_RECV_MAX_LEN,g_cmd_wifi,this->m_name,this->m_password);
    if(ret < 0)
    {
        log_error("Format failed.");
        return E_ERROR;
    }
    ret = m_cmd_send(this,(const u8*)str,strlen((char*)str),1000);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        return E_ERROR;
    }
    
    /*设置IP*/
    memset(str,0,AT_RECV_MAX_LEN);
    ret = snprintf((char*)str,AT_RECV_MAX_LEN,g_cmd_ip,ESP8266_IP,ESP8266_GAT,ESP8266_MASK);
    if(ret < 0)
    {
        log_error("Format failed.");
        return E_ERROR;
    }
    ret = m_cmd_send(this,(const u8*)str,strlen((char*)str),1000);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        return E_ERROR;
    } 
    
    /*普通传输模式*/
    ret = m_cmd_send(this,(const u8*)g_cmd_ip_mod,strlen(g_cmd_ip_mod),1000);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        return E_ERROR;
    }
    
    /*多传输模式*/
    ret = m_cmd_send(this,(const u8*)g_cmd_ip_mux,strlen(g_cmd_ip_mux),1000);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        return E_ERROR;
    }
    
    /*启动服务器*/
    memset(str,0,AT_RECV_MAX_LEN);
    ret = snprintf((char*)str,AT_RECV_MAX_LEN,g_cmd_server,ESP8266_SERVER_PORT);
    if(ret < 0)
    {
        log_error("Format failed.");
        return E_ERROR;
    }
    ret = m_cmd_send(this,(const u8*)str,strlen((char*)str),1000);
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        return E_ERROR;
    }
    
    return E_OK;
}

static int m_recv_check(const m_esp8266* this)
{
    int ret = 0;
    u8* new_msg = NULL;
    char* param = NULL;
    u16 recv_len = 0;
    
    /*参数检测*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    /*如果设置了回调函数 才启用*/
    if(NULL != this->m_callback)
    {                
        ret = sscanf((const char*)this->m_recv_data,"+IPD,0,%hu:",&recv_len);
        if(ret < 1)
        {
            log_warning("Recv error data.");
            return E_ERROR;
        }
        
        /*为新消息 开辟内存*/
        new_msg = pvPortMalloc(recv_len);
        if(NULL == new_msg)
        {
            log_error("Out of memory.");
            new_msg = NULL;
            return E_ERROR;
        }
        
        /*拷贝数据*/
        param = strstr((const char*)this->m_recv_data,":");
        if(NULL != param)
        {
            memcpy(new_msg,param + 1,recv_len);
            this->m_callback(this->m_callback_param,new_msg,recv_len);
        }
                                                                    
        /*释放内存*/
        vPortFree(new_msg);
        new_msg = NULL;
        recv_len = 0;
    }
    
    return E_OK;
}

static int m_set_callback(const c_esp8266_wifi* this,void* param,int (*callback)(void* param,const u8* data,u16 data_len))
{
    esp8266_send_msg  new_msg = {0};
    m_esp8266*        m_this = NULL;
    BaseType_t       os_ret  = 0;

    /*检查参数*/
    if(NULL == this|| NULL == this->this)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;
    
    /*保存参数*/
    m_this->m_callback = callback;
    m_this->m_callback_param = param;
    
    return E_OK;    
}


#endif