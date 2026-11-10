#include "driver_conf.h"

#ifdef DRIVER_FRM1213_ENABLED

#define MODULE_NAME       "FRM1213"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

const u8 init1[]  = {0xEF,0xAA,0x30,0x00,0x00,0x30};           /*初始化1*/
const u8 init2[]  = {0xEF,0xAA,0x24,0x00,0x00,0x24};           /*初始化2*/
const u8 sanchu[] = {0xEF,0xAA,0x21,0x00,0x00,0x21};           /*删除所有人脸*/
const u8 jiance[] = {0xEF,0xAA,0x12,0x00,0x02,0x00,0x0A,0x1A}; /*人脸检测*/

/*添加人脸*/
const u8 jia[] = {0xEF,0xAA ,0x26 ,0x00 ,0x28 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00, 
                  0x00,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, 
                  0x00,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, 
                  0x00,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x01 ,0x01, 
                  0x01,0x10 ,0x00 ,0x00 ,0x00 ,0x1E};

                  
                  
typedef struct __M_FRM1213
{
    u8  m_uart_id;
    u8  add;
    u8  det;
    SemaphoreHandle_t m_sem;
}m_frm1213;


static int m_add      (c_frm1213* this);
static int m_del_all  (c_frm1213* this);
static int m_det      (c_frm1213* this);
static int m_callback(void* param,const u8* data,u16 data_len);


c_frm1213 frm1213_creat(u8 uart_id)
{
    int ret = 0;
    c_frm1213  new = {0};
    m_frm1213* m_this = NULL;  

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_frm1213));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_frm1213));
    m_this = new.this;

    /*创建二值信号量*/
    m_this->m_sem = xSemaphoreCreateBinary();
    if(NULL == m_this->m_sem)
    {
        log_error("Semaphore create failed.");
        goto error_handle;
    }
    
    /*保存相关参数*/
    m_this->m_uart_id = uart_id;
    new.add     = m_add     ;
    new.del_all = m_del_all ;
    new.det     = m_det     ;

    vTaskDelay(1000);  /*等待模块 启动*/
    
    /*初始化串口*/
    ret = my_uart.init(uart_id,FRM1213_BAUDRATE,64);
    if(E_OK != ret)
    {
        log_error("Uart init failed.");
        goto error_handle;
    }
    
    /*注册回调*/
    ret = my_uart.set_callback(uart_id,m_this,m_callback);
    if(E_OK != ret)
    {
        log_error("Callback set failed.");
        goto error_handle;
    }
    
	ret = my_uart.send(m_this->m_uart_id,init1,sizeof(init1));
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        goto error_handle;
    }
	
	ret = my_uart.send(m_this->m_uart_id,init2,sizeof(init2));
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        goto error_handle;
    }
	
    return new;

    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int m_add  (c_frm1213* this)
{
    m_frm1213* m_this = NULL;
    int ret = 0;
    BaseType_t os_ret = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*清空一下信号量*/
    xSemaphoreTake(m_this->m_sem,0);
    m_this->add = 1;  /*标记为添加*/
    
    /*发送添加*/
    ret = my_uart.send(m_this->m_uart_id,jia,sizeof(jia));
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        return E_ERROR;
    }
    
    os_ret = xSemaphoreTake(m_this->m_sem,5000);
    if(pdTRUE != os_ret)
    {
        log_error("Receive timeout.");
        return E_ERROR;
    }
    
    /*查询结果*/
    if(2 == m_this->add)
    {
        return E_OK;
    }
    

    return E_ERROR;
}

static int m_del_all  (c_frm1213* this)
{
    m_frm1213* m_this = NULL;
    int ret = 0;
    BaseType_t os_ret = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*发送删除*/
    ret = my_uart.send(m_this->m_uart_id,sanchu,sizeof(sanchu));
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        return E_ERROR;
    }

    return E_OK;
}

static int m_det (c_frm1213* this)
{
    m_frm1213* m_this = NULL;
    int ret = 0;
    BaseType_t os_ret = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*清空一下信号量*/
    xSemaphoreTake(m_this->m_sem,0);
    m_this->det = 1;  /*标记为检测*/
    
    /*发送添加*/
    ret = my_uart.send(m_this->m_uart_id,jiance,sizeof(jiance));
    if(E_OK != ret)
    {
        log_error("Cmd send failed.");
        return E_ERROR;
    }
    
    os_ret = xSemaphoreTake(m_this->m_sem,5000);
    if(pdTRUE != os_ret)
    {
        log_error("Receive timeout.");
        return E_ERROR;
    }
    
    /*查询结果*/
    if(2 == m_this->det)
    {
        return E_OK;
    }
    

    return E_ERROR;
}

static int m_callback(void* param,const u8* data,u16 data_len)
{
    m_frm1213* m_this = NULL;
    BaseType_t os_ret = 0;

    /*检查参数*/
    if(NULL == param || NULL == data || 0 == data_len)
    {
        log_error("Uart data error.");
        return E_ERROR;
    }

    m_this = param;
    
    /*如果正在进行人脸检测*/
    if(m_this->det == 1)
    {
        if(0xEF == data[0] && 0xAA == data[1] && 0x00 == data[2] && 0x00 == data[3] && 
           0x26 == data[4] && 0x12 == data[5] && 0x00 == data[6])
        {
            m_this->det= 2;
            
            os_ret = xSemaphoreGive(m_this->m_sem);
            if(pdTRUE != os_ret)
            {
                log_error("Semaphore give failed.");
                return E_ERROR;
            }
        }
    }
    
    /*正在进行人脸添加*/
    else if(m_this->add == 1)
    {        
        if(0xEF == data[0] && 0xAA == data[1] && 0x00 == data[2] && 0x00 == data[3] && 
           0x05 == data[4] && 0x26 == data[5] && 0x00 == data[6] && 0x00 == data[7] && 0x01 == data[8])
        {
            m_this->add = 2;
            
            os_ret = xSemaphoreGive(m_this->m_sem);
            if(pdTRUE != os_ret)
            {
                log_error("Semaphore give failed.");
                return E_ERROR;
            }
        }
    }
    

    return E_OK; 
}


#endif