#include "driver_conf.h"

#ifdef DRIVER_AMG8833_ENABLED

#define MODULE_NAME       "amg8833"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define AMG8833_IIC_SPEED             I2C_SPEED_DEFAULT

/*=========================================================================*/

int m_get(const c_amg8833* this, float* pixels);

typedef struct _M_AMG8833
{
    const c_my_iic*  m_iic ;   /*所在的IIC总线*/
    u8               m_addr;   /*器件地址*/
}m_amg8833;

c_amg8833 amg8833_creat(const c_my_iic* iic)
{
    c_amg8833 new = {0};
    m_amg8833* m_this = NULL;

    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_amg8833));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_amg8833));
    m_this = new.this;

    /*保存相关变量*/
    m_this->m_iic   = iic      ;
    m_this->m_addr  = AMG8833_PIXEL_START     ;
    new.get        = m_get    ;

    /*返回新建的对象*/
    return new;
}

int m_get(const c_amg8833* this, float* pixels)
{
    iic_cmd_handle_t  cmd = NULL;
    u8   iic_addr = 0;
    u8   iic_read_data[128] = {0};
    int ret  = 0;
    const m_amg8833* m_this = NULL;
	int zero_cnt = 0;

    /*参数检查*/
    if(NULL == this || NULL == pixels)
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
    iic_addr = m_this->m_addr | I2C_MASTER_READ;  /*读地址*/
	//iic_addr = (0x69 << 1) | I2C_MASTER_READ;  /*读地址*/
	ret = iic_write_bytes(cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*读取数据*/
    ret = iic_read_bytes(cmd, iic_read_data, 128);
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,AMG8833_IIC_SPEED,1000);
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
	
	//数据校验
	for(int i=0;i<64;i++)
    {
        int16_t temp = iic_read_data[2*i] | (iic_read_data[2*i+1]<<8);
		if (temp == 0)
			zero_cnt++;
		pixels[i] = temp * 0.25;
    }
	
	if (zero_cnt >= 20)
		return E_ERROR;

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
