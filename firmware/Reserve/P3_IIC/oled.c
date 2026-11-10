
#include "driver_conf.h"
#include "oledfont.h"

#ifdef DRIVER_OLED_ENABLED

#define MODULE_NAME       "oled"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define OLED_IIC_SPEED            3

#define OLED_CMD               0x00	//写命令
#define OLED_DATA              0x40	//写数据
#define OLED_DEFAULT_IIC_ADDR  0x78 /*显示屏 地址*/
#define OLED_DISPLAY_ON        0xAF /*开显示*/
#define OLED_DISPLAY_OFF       0xAE /*关显示*/

#define FONT_SIZE    16
#define X_WIDTH 	128  /*宽度*/
#define Y_WIDTH 	 64	 /*高度*/

#define IIC_Start()      SMBus_StartBit()
#define IIC_Stop()       SMBus_StopBit()
#define IIC_Wait_Ack()   
#define Write_IIC_Byte(dat) SMBus_SendByte(dat)

typedef struct __M_OLED
{
    const c_my_iic*    m_iic          ;  /*iic对象*/
    u8                 m_iic_addr     ;  /*iic地址*/
    u8                 m_type         ;  /*屏幕类型*/
    u8                 m_font_width   ;  /*字宽*/
    SemaphoreHandle_t  m_semaphore    ;  /*互斥信号量 */
}m_oled;

static int m_display_clear (const c_oled* this);  
static int m_display_str   (const c_oled* this,u8 x,u8 y, char* format,...);
static int m_display_char  (const c_oled* this,u8 x,u8 y, u8 cha);
static int m_display_chinese(const c_oled* this,u8 x,u8 y,char* str);
static int m_display_chinese_str(const c_oled* this,u8 x,u8 y,char* str);
static int m_display_on    (const c_oled* this);
static int m_display_off   (const c_oled* this);
static int m_set_pos       (const c_oled* this,u8 x,u8 y);

static int m_oled_write    (const c_oled* this,const u8* dat,u8 len,u8 type);

static const u8 g_cmd_dis_clear  [X_WIDTH] ;
const u8 g_init_cmd[] = {0xAE,0x02,0x10,0x40,0xB0,0x81,0xFF,0xA1,0xA6,
                         0xA8,0x3F,0xC8,0xD3,0x00,0xD5,0x80,0xD8,0x05,
                         0xD9,0xF1,0xDA,0x12,0xDB,0x30,0x8D,0x14,0xAF};

c_oled oled_create(c_my_iic* iic,u8 type)
{
    c_oled new = {0};
    m_oled* m_this = NULL;
    int    ret = 0;
    
    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }

    if(OLED_TYPE_96 != type && OLED_TYPE_91 != type)
    {
        log_error("Param error.");
        return new;
    }

    /*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(m_oled));
	if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
    memset(new.this,0,sizeof(m_oled));
	m_this = new.this;

	/*为新对象 创建互斥信号量*/
	m_this->m_semaphore = xSemaphoreCreateMutex();
	if(NULL == m_this->m_semaphore)
	{
		log_error("Mutex semaphore creat failed.");
		goto error_handle;
	}

     /*保存相关变量*/
    new.display_str   = m_display_str   ;
    new.display_clear = m_display_clear ;
    new.display_on    = m_display_on    ;
    new.display_off   = m_display_off   ;
    new.display_chinese_str = m_display_chinese_str;
    m_this->m_iic        = iic;
    m_this->m_type       = type;
    m_this->m_font_width = 8;
    m_this->m_iic_addr   = OLED_DEFAULT_IIC_ADDR;
    
    /*oled 初始化命令*/
	m_oled_write(&new,g_init_cmd,1,OLED_CMD);
	vTaskDelay(100);
	m_oled_write(&new,g_init_cmd,1,OLED_CMD);	
	vTaskDelay(100);
    ret = m_oled_write(&new,g_init_cmd,sizeof(g_init_cmd)/sizeof(u8),OLED_CMD);
    if(E_OK != ret)
    {
        log_error("OLED cmd init failed.");
        goto error_handle;
    }
    return new;
    
    error_handle:
    vPortFree(m_this);
    new.this = NULL;
    
    return new;
}



//OLED的显存
//存放格式如下.
//[0]0 1 2 3 ... 127    
//[1]0 1 2 3 ... 127    
//[2]0 1 2 3 ... 127    
//[3]0 1 2 3 ... 127    
//[4]0 1 2 3 ... 127    
//[5]0 1 2 3 ... 127    
//[6]0 1 2 3 ... 127    
//[7]0 1 2 3 ... 127               

/**********************************************
// IIC Write Command
**********************************************/
static int m_oled_write(const c_oled* this,const u8* dat,u8 len,u8 type)
{
    iic_cmd_handle_t  cmd = NULL;
    int  ret = 0;
    const m_oled* m_this = this->this;
    u8   iic_addr = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

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
    iic_addr = m_this->m_iic_addr | I2C_MASTER_WRITE;  /*写地址*/
    ret = iic_write_bytes(cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*写入 数据类型*/
    ret = iic_write_bytes(cmd,&type,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*写入 数据*/
    ret = iic_write_bytes(cmd,dat,len);
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,OLED_IIC_SPEED,1000);
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

////画点 
////x:0~127
////y:0~63
////t:1 填充 0,清空	
//void OLED_DrawPoint(u8 x,u8 y,u8 t)
//{
//	u8 i,m,n;
//	i=y/8;
//	m=y%8;
//	n=1<<m;
//	if(t){OLED_GRAM[x][i]|=n;}
//	else
//	{
//		OLED_GRAM[x][i]=~OLED_GRAM[x][i];
//		OLED_GRAM[x][i]|=n;
//		OLED_GRAM[x][i]=~OLED_GRAM[x][i];
//	}
//}

int m_display_chinese(const c_oled* this,u8 x,u8 y,char* str)
{
    int ret = 0;
    
    
    
    /*查找对应的汉字*/
    for(u16 i = 0;i < sizeof(chinese_font_16)/sizeof(typFNT_GB16);++i)
    {
        /*找到对应汉字的位置*/
        if((chinese_font_16[i].Index[0]==*(str))&&(chinese_font_16[i].Index[1]==*(str+1)))
		{ 	
            /*设置上半部分显示坐标*/
            ret = m_set_pos(this,x,y);
            if(E_OK != ret)
            {
                log_error("Set pos failed.");
                return E_ERROR;
            }
            
            /*写入 上半部分*/
            ret = m_oled_write(this,(u8*)&chinese_font_16[i].Msk,16,OLED_DATA);
            if(E_OK != ret)
            {
                log_error("Write failed.");
                return E_ERROR;
            } 
            
            /*写入  下半部分*/
            ret = m_set_pos(this,x,y + 1);
            if(E_OK != ret)
            {
                log_error("Set pos failed.");
                return E_ERROR;
            }
            
            ret = m_oled_write(this,(u8*)&chinese_font_16[i].Msk + 16,16,OLED_DATA);
            if(E_OK != ret)
            {
                log_error("Write failed.");
                return E_ERROR;
            } 
        }
    }
    

    return E_OK;
}

static int m_display_char   (const c_oled* this,u8 x,u8 y, u8 cha)
{       
    u8 c = 0;  
    int ret = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    
    c = cha-' ';//得到偏移后的值

    /*设置显示坐标*/
    ret = m_set_pos(this,x,y);
    if(E_OK != ret)
    {
        log_error("Set pos failed.");
        return E_ERROR;
    }

    /*字体大小 为16*/
    if(FONT_SIZE ==16)
    { 
        /*写入*/
        ret = m_oled_write(this,(u8*)&F8X16[c*16],8,OLED_DATA);
        if(E_OK != ret)
        {
            log_error("Write failed.");
            return E_ERROR;
        }    
        
        ret = m_set_pos(this,x,y+1);
        if(E_OK != ret)
        {
            log_error("Set pos failed.");
            return E_ERROR;
        }
        
        ret = m_oled_write(this,(u8*)&F8X16[c*16 + 8],8,OLED_DATA);
        if(E_OK != ret)
        {
            log_error("Write failed.");
            return E_ERROR;
        }
    }
    else 
    {
        ret = m_oled_write(this,(u8*)&F6x8[c][0],8,OLED_DATA);
        if(E_OK != ret)
        {
            log_error("Write failed.");
            return E_ERROR;
        }
    }
    
    return E_OK;
}

static int m_display_chinese_str(const c_oled* this,u8 x,u8 y,char* str)
{
    const m_oled*  m_this = NULL;
    
    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    for(u8 str_index = 0;'\0' != str[str_index];str_index += 2)
    {
        m_display_chinese(this,x + str_index * 8,y,str + str_index);
    }
}

static int m_display_str   (const c_oled* this,u8 x,u8 y, char* format,...)
{
    u8 str_index = 0;
    const m_oled*  m_this = NULL;
    va_list   argptr;
    char str_buf[20] = {0};
    
    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    /*检查坐标*/
    if(x >= X_WIDTH || y >= Y_WIDTH / FONT_SIZE)
    {
        log_error("Param error");
        return E_ERROR;
    }
    y *= FONT_SIZE / 8;
    
    va_start( argptr, format );  // 初始化argptr	
	memset(str_buf,0,20);
	vsnprintf(str_buf,20,format,argptr);
	va_end(argptr);
    
    
    for(str_index = 0;'\0' != str_buf[str_index];++str_index)
    {
        m_display_char(this,x + str_index * m_this->m_font_width,y,str_buf[str_index]);
    }
    
    
    return E_OK;
}

static int m_set_pos(const c_oled* this,u8 x,u8 y)
{
    int ret = 0;
    u8 dat[3] = {0};

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
  
    dat[0] = 0xb0+y;                 /*设置y坐标*/
    dat[1] = ((x&0xf0)>>4)|0x10; /*设置x坐标*/
    dat[2] = (x&0x0f);
    ret = m_oled_write(this,dat,3,OLED_CMD);
    if(E_OK != ret)
    {
        log_error("Set pos failed.");
        return E_ERROR;
    }
   
    return E_OK;
}

static int m_display_clear(const c_oled* this)
{
    u16 i = 0;         
    int ret = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    for(i = 0;i<8;i++)
    {
        ret = m_set_pos(this,0,i);
        if(E_OK != ret)
        {
            log_error("Write failed.");
            return E_ERROR;
        }
        ret = m_oled_write(this,g_cmd_dis_clear,X_WIDTH,OLED_DATA);
        if(E_OK != ret)
        {
            log_error("Write failed.");
            return E_ERROR;
        }
    }

    return E_OK;
}

static int m_display_on   (const c_oled* this)
{
    u8 data = OLED_DISPLAY_ON;
    int ret = 0;
    
    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    /*发送开显示命令*/
    ret = m_oled_write(this,&data,1,OLED_CMD);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    
    return E_OK;
}

static int m_display_off   (const c_oled* this)
{
    u8 data = OLED_DISPLAY_OFF;
    int ret = 0;
    
    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    /*发送关显示命令*/
    ret = m_oled_write(this,&data,1,OLED_CMD);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    
    return E_OK;
}
#endif

/*
SSD1306显存：

		SEG0  SEG1 ... SEG127
PAGE0	
PAGE1
PAGE2
PAGE3
PAGE4
PAGE5
PAGE6
PAGE7

8 PAGE， 每页 128个 SEG，每个SEG为一个字节(8bits)， 显存总大小为 128 * 8 bytes
每个bit标志一个像素点的亮或者灭，一个SEG可以表示8个像素点如下：
SEG0
bit0
bit1
bit2
bit3
bit4
bit5
bit6
bit7
所以X向共128个像素，Y向共 8 * 8 = 64个像素 


SSD1306寻址模式：页地址模式，水平地址模式，竖直地址模式
页地址模式：
页面寻址模式下，读写显示RAM后，列地址指针自动增加1。如果列地址指针到达列结束地址(0 - 127)，
则将列地址指针重置为列起始地址，而不对页面地址指针进行更改（PAGE不会改变）。
用户必须设置新的页和列地址，才能访问下一页的RAM内容。


dat[0] = 0xb0+y;              		//设置页地址     （y: 0 - 7）  
dat[1] = ((x&0xf0)>>4)|0x10; 	    //设置列地址高位
dat[2] = (x&0x0f);					//设置列地址低位  (x: 0 - 8)

*/



























