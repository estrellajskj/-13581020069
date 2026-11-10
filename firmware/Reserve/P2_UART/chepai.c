

#include "driver_conf.h"

#ifdef DRIVER_CHEPAI_ENABLED

#define MODULE_NAME       "chepai"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define CHEPAI_BAUDRATE  9600

static int m_uart_callback(void* param,const u8* data,u16 data_len);
static int m_set_callback(c_chepai* this,void* param,int (*callback)(void* param,u8 type,const char* no));

static char g_hanzhi[][3] = {
"京","津","冀","晋","蒙","辽","吉","黑","沪","苏","浙",
"皖","闽","赣","鲁","豫","鄂","湘","粤","桂","琼","渝",
"川","贵","云","藏","陕","甘","青","宁","新","港","澳",
"台","警","使","WJ","领","学","民","航","应","机","场",
"电"};

typedef struct __M_CHEPAI
{
    u8  m_uart_id;
    char m_no[20]; /*车牌号*/ 
    void* m_param;
    int (*m_callback)(void* param,u8 type,const char* no);
}m_chepai;

c_chepai chepai_create(u8 uart_id)
{
    int ret = 0;
    c_chepai    new = {0};
    m_chepai*   m_this = NULL;  
    
    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_chepai));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_chepai));
    m_this = new.this;
    
    /*保存相关变量*/
    m_this->m_uart_id = uart_id;
    new.set_callback = m_set_callback;
    
    /*初始化串口*/
    ret = my_uart.init(uart_id,CHEPAI_BAUDRATE,15);
    if(E_OK != ret)
    {
        log_error("Uart init failed.");
        goto error_handle;
    }
    
    /*注册串口回调*/
    ret = my_uart.set_callback(m_this->m_uart_id,(void*)m_this,m_uart_callback);
    if(E_OK != ret)
    {
        log_error("Callback set failed.");
        goto error_handle;
    }
    
    return new;
    
    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int m_uart_callback(void* param,const u8* data,u16 data_len)
{
    u8 type;
    m_chepai* m_this = NULL;
    
    if(NULL == param || NULL == data || 0 == data_len)
    {
        log_error("Param error.");
        return E_ERROR;
    }
    m_this = param;
    
    if(0xBB != data[0] || 0x33 != data[data_len - 1])
    {
        log_error("Error page.");
        return E_ERROR;
    }
    
    /*判断识别结果*/
    if(0xAA == data[2])
    {
        ;/*可识别*/
    }
    else 
    {
        log_warning("Unknown license plate.");
        return E_ERROR;
    }
    
    /*准备车牌字符串*/
    memset(m_this->m_no,0,sizeof(m_this->m_no));
    
	 if(data[3]==0)
		 type = LAN_PAI;  /*蓝牌*/
	 else if(data[3]==5)
        type = LV_PAI;  /*绿牌*/ 
	 else if(data[3]==1)
        type = HA_PAI;  /*绿牌*/ 	 
    /*根据数据数据长度 判断车牌类型*/
    if(12 == data_len) //data[3]==0//蓝牌 =1//黄牌=5//绿牌
    {
        if(data[4] >= 46)
        {
            log_error("Error code.");
            return E_ERROR;
        }
        
        /*拿到汉字*/
        memcpy(m_this->m_no,g_hanzhi[data[4]],2);
        
        /*拿到车牌号*/
        memcpy(m_this->m_no + 2,data + 5,6);
    }
    else if(13 == data_len)
    {
        //data[4] -= 128;
        if(data[4] >= 46 + 128)
        {
            log_error("Error code.");
            return E_ERROR;
        }
        
        /*拿到汉字*/
        memcpy(m_this->m_no,g_hanzhi[data[4] - 128],2);
        
        /*拿到车牌号*/
        memcpy(m_this->m_no + 2,data + 5,7);
    }
    else
    {
        log_error("Error type.");
    }
    
    
    if(NULL != m_this->m_callback)
    {
        m_this->m_callback(m_this->m_param,type,m_this->m_no);
    }
    
    return E_OK;
}

static int m_set_callback(c_chepai* this,void* param,int (*callback)(void* param,u8 type,const char* no))
{
    m_chepai* m_this = NULL;

    /*参数检测*/
    if(NULL == this || NULL == this->this || NULL == callback)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    m_this->m_param = param;
    m_this->m_callback = callback;
    
    
    return E_OK;
}


#endif