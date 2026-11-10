
#include "driver_conf.h"

#ifdef DRIVER_HC_SR04_IIC_ENABLED

#define MODULE_NAME       "hc_sr04_iic"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define SR04_IIC_SPEED             200
#define SR04_ADDR                  (0x57 << 1)

static int m_get(const c_hc_sr04* this,float* dis_mm);

typedef struct _M_HC_SR04
{
    const c_my_iic*  m_iic ;   /*所在的IIC总线*/
}m_hc_sr04;

c_hc_sr04 hc_sr04_creat(const c_my_iic* iic)
{
    c_hc_sr04 new = {0};
    m_hc_sr04* m_this = NULL;

    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_hc_sr04));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_hc_sr04));
    m_this = new.this;

    /*保存相关变量*/
    m_this->m_iic   = iic      ;
    new.get         = m_get    ;

    /*返回新建的对象*/
    return new;    
}

int m_get(const c_hc_sr04* this,float* dis_mm)
{
    const m_hc_sr04* m_this = NULL;
    iic_cmd_handle_t  cmd = NULL;
    u8   iic_data[3] = {0};
    u8   iic_write   = 0;
    int  ret  = 0;
    u8   iic_addr = 0;
    u32 dis = 0;
    
    /*参数检查*/
    if(NULL == this || NULL == dis_mm || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*创建新的命令链接*/
    cmd = iic_cmd_link_create();
    if(NULL == cmd)
    {
        log_error("IIC cmd link create failed.");
        return E_ERROR;
    }
    
    /*放入启动命令*/
    ret = iic_master_start(cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }
    
    /*写入地址*/
    iic_addr = SR04_ADDR | I2C_MASTER_WRITE;  /*写地址*/
    ret = iic_write_bytes(cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    /*写入数据*/
    iic_write = 0x01;
    ret = iic_write_bytes(cmd,&iic_write,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    /*停止命令*/
    ret = iic_master_stop(cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }
    
    /*启动命令序列*/
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,SR04_IIC_SPEED,1000);
    if(E_OK != ret)
    {
        //log_error("Add iic begin failed.");
        //goto error_handle;
    }
    
    /*释放命令*/
    ret = iic_cmd_link_delete(cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }
    
    vTaskDelay(120); /*等待超声返回*/
      
    /*创建新的命令链接*/
    cmd = iic_cmd_link_create();
    if(NULL == cmd)
    {
        log_error("IIC cmd link create failed.");
        return E_ERROR;
    }

    /*放入启动命令*/
    ret = iic_master_start(cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }
    
    /*写入地址*/
    iic_addr = SR04_ADDR | I2C_MASTER_READ;  /*读地址*/
    ret = iic_write_bytes(cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    /*读取距离数据*/
    ret = iic_read_bytes(cmd,iic_data,3);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    /*停止命令*/
    ret = iic_master_stop(cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }
    
    /*启动命令序列*/
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,SR04_IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("Add iic begin failed.");
        goto error_handle;
    }
    
    /*释放命令*/
    ret = iic_cmd_link_delete(cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }
    
    /*换算距离*/
    dis = ((iic_data[0] << 16) + (iic_data[1] << 8) + iic_data[2]);
    *dis_mm = (float)dis / (1000.0f * 10.0f);	//单位cm
    
    vTaskDelay(100);  /*手册要求 两次测量间隔不要小于200ms*/
    
    return E_OK;
    
    error_handle:
    
    /*删除链接*/
    ret = iic_cmd_link_delete(cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
    }
    return E_ERROR; 
}

#endif


