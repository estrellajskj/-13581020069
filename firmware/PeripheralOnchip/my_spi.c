
#include "onchip_conf.h"

#ifdef ONCHIP_SPI_ENABLED

#define MODULE_NAME       "spi"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

typedef struct __SPI_CFG
{   
    SPI_TypeDef*                 m_spi_addr         ;   /*spi地址*/
    DMA_Channel_TypeDef*         m_send_dma_channel ;   /*发送DMA通道*/
    DMA_Channel_TypeDef*         m_recv_dma_channel ;   /*接收DMA通道*/
    u32                       m_send_dma_cha_int ;   /*发送dma中断通道*/
    u32                       m_recv_dma_cha_int ;   /*接收dma中断通道*/
    GPIO_TypeDef *            m_spi_gpio         ;   /*spi gpio 分组*/
    u32                       m_pin_sck          ;   /*spi 时钟线*/
    u32                       m_pin_miso         ;
    u32                       m_pin_mosi         ;
}spi_cfg;

typedef struct _M_SPI
{
    u8                m_id            ;
    u8                m_incremental   ;    
    u32               m_send_len      ;
    u32               m_need_len      ;
    const u8*         m_data_addr     ;
    
    bool              m_init_state    ;
    SPI_HandleTypeDef     m_spi_handle    ;
    DMA_HandleTypeDef     m_tx_dma_handle ;
    DMA_HandleTypeDef     m_rx_dma_handle ;
    SemaphoreHandle_t m_mutex         ;  /*互斥锁 */
    TaskHandle_t      m_spi_task      ;  /*正在使用spi的 任务句柄*/
}m_spi;

static int m_transmission		(u8 spi,u8 datasize,const u8* send,u8* recv,u32 len,bool incremental,TickType_t time);
static int m_set_datasize		(u8 spi,u8 datasize);
static int m_init  				(u8 spi);
//static int m_dinit  		    (u8 spi);
static int m_send_int_handle    (u32 dma_cha,void* param);
static int m_recv_int_handle(u32 dma_cha,void* param);
static int m_set_speed          (u8 spi,u8 speed);
const c_spi my_spi = {m_init,m_set_datasize,m_set_speed,m_transmission};

/*spi配置*/
static const spi_cfg g_cfg[2] = {{SPI1,DMA1_Channel3,DMA1_Channel2,DMA1_CHA_3,DMA1_CHA_2,
									GPIOA,GPIO_PIN_5,GPIO_PIN_6,GPIO_PIN_7},
							     {SPI2,DMA1_Channel5,DMA1_Channel4,DMA1_CHA_5,DMA1_CHA_4,
									GPIOB,GPIO_PIN_13,GPIO_PIN_14,GPIO_PIN_15},
                                };
static m_spi g_my_spi[2];
                                
static int m_init  (u8 spi)
{
     GPIO_InitTypeDef gpio = {0};
    int ret = 0;

	if(g_my_spi[spi].m_init_state != false)  return E_OK;
//    __HAL_RCC_SPI1_CLK_ENABLE();  //开启SPI 时钟后会影响 TIME3 后TIME4的使用
    __HAL_RCC_SPI2_CLK_ENABLE();
    
    /*初始化IO*/
    gpio.Pin   = g_cfg[spi].m_pin_sck|g_cfg[spi].m_pin_miso|g_cfg[spi].m_pin_mosi;
    gpio.Mode  = GPIO_MODE_AF_PP;             //复用推挽输出
    gpio.Pull  = GPIO_PULLUP    ;             //上拉
    gpio.Speed = GPIO_SPEED_HIGH;             //快速            
    HAL_GPIO_Init(g_cfg[spi].m_spi_gpio,&gpio);

    /*初始化spi*/
    g_my_spi[spi].m_spi_handle.Instance              = g_cfg[spi].m_spi_addr      ; //spi寄存器基地址
    g_my_spi[spi].m_spi_handle.Init.Mode             = SPI_MODE_MASTER            ; //设置SPI工作模式，设置为主模式
    g_my_spi[spi].m_spi_handle.Init.Direction        = SPI_DIRECTION_2LINES       ; //设置SPI单向或者双向的数据模式:SPI设置为双线模式
    g_my_spi[spi].m_spi_handle.Init.DataSize         = SPI_DATASIZE_8BIT          ; //设置SPI的数据大小:SPI发送接收8位帧结构
    g_my_spi[spi].m_spi_handle.Init.CLKPolarity      = SPI_POLARITY_HIGH          ; //串行同步时钟的空闲状态为高电平
    g_my_spi[spi].m_spi_handle.Init.CLKPhase         = SPI_PHASE_2EDGE            ; //串行同步时钟的第二个跳变沿（上升或下降）数据被采样
    g_my_spi[spi].m_spi_handle.Init.NSS              = SPI_NSS_SOFT               ; //NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
    g_my_spi[spi].m_spi_handle.Init.BaudRatePrescaler= SPI_BAUDRATEPRESCALER_2    ; //定义波特率预分频的值:波特率预分频值为2
    g_my_spi[spi].m_spi_handle.Init.FirstBit         = SPI_FIRSTBIT_MSB           ; //指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
    g_my_spi[spi].m_spi_handle.Init.TIMode           = SPI_TIMODE_DISABLE         ; //关闭TI模式
    g_my_spi[spi].m_spi_handle.Init.CRCCalculation   = SPI_CRCCALCULATION_DISABLE ; //关闭硬件CRC校验
    g_my_spi[spi].m_spi_handle.Init.CRCPolynomial    = 7                          ; //CRC值计算的多项式
    HAL_SPI_Init(&g_my_spi[spi].m_spi_handle);//初始化
    
    __HAL_SPI_ENABLE(&g_my_spi[spi].m_spi_handle);                    //使能SPI
    
    /*配置发送DMA*/
    __HAL_LINKDMA(&g_my_spi[spi].m_spi_handle,hdmatx,g_my_spi[spi].m_tx_dma_handle);
    g_my_spi[spi].m_tx_dma_handle.Instance                 = g_cfg[spi].m_send_dma_channel ;   //通道选择
    g_my_spi[spi].m_tx_dma_handle.Init.Direction           = DMA_MEMORY_TO_PERIPH          ;   //存储器到外设
    g_my_spi[spi].m_tx_dma_handle.Init.PeriphInc           = DMA_PINC_DISABLE              ;   //外设非增量模式
    g_my_spi[spi].m_tx_dma_handle.Init.MemInc              = DMA_MINC_ENABLE               ;   //存储器增量模式
    g_my_spi[spi].m_tx_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE           ;   //外设数据长度:8位
    g_my_spi[spi].m_tx_dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE           ;   //存储器数据长度:8位
    g_my_spi[spi].m_tx_dma_handle.Init.Mode                = DMA_NORMAL                    ;   //外设普通模式
    g_my_spi[spi].m_tx_dma_handle.Init.Priority            = DMA_PRIORITY_MEDIUM           ;   //中等优先级
    HAL_DMA_DeInit(&g_my_spi[spi].m_tx_dma_handle);   
    HAL_DMA_Init  (&g_my_spi[spi].m_tx_dma_handle);  /*初始化DMA*/
    
    /*注册DMA发送回调函数*/
    ret = int_manage.login_dma(g_cfg[spi].m_send_dma_cha_int,m_send_int_handle,&g_my_spi[spi]);
	if(E_OK != ret)
	{
		log_error("Int func login failed.");
		return E_ERROR;
	}    
    //HAL_NVIC_SetPriority(g_cfg[spi].m_send_dma_irqn, 9, 0);
    //HAL_NVIC_EnableIRQ(g_cfg[spi].m_send_dma_irqn);  
    
    /*配置接收DMA*/
    __HAL_LINKDMA(&g_my_spi[spi].m_spi_handle,hdmarx,g_my_spi[spi].m_rx_dma_handle);
    g_my_spi[spi].m_rx_dma_handle.Instance                 = g_cfg[spi].m_recv_dma_channel ;   //通道选择
    g_my_spi[spi].m_rx_dma_handle.Init.Direction           = DMA_PERIPH_TO_MEMORY          ;   //外设到存储器
    g_my_spi[spi].m_rx_dma_handle.Init.PeriphInc           = DMA_PINC_DISABLE              ;   //外设非增量模式
    g_my_spi[spi].m_rx_dma_handle.Init.MemInc              = DMA_MINC_ENABLE               ;   //存储器增量模式
    g_my_spi[spi].m_rx_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE           ;   //外设数据长度:8位
    g_my_spi[spi].m_rx_dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE           ;   //存储器数据长度:8位
    g_my_spi[spi].m_rx_dma_handle.Init.Mode                = DMA_NORMAL                    ;   //外设普通模式
    g_my_spi[spi].m_rx_dma_handle.Init.Priority            = DMA_PRIORITY_MEDIUM           ;   //中等优先级
    HAL_DMA_DeInit(&g_my_spi[spi].m_rx_dma_handle);   
    HAL_DMA_Init  (&g_my_spi[spi].m_rx_dma_handle);  /*初始化DMA*/
    //HAL_NVIC_SetPriority(g_cfg[spi].m_recv_dma_irqn, 8, 0);
    //HAL_NVIC_EnableIRQ(g_cfg[spi].m_recv_dma_irqn);  
   	
	    /*注册DMA接收回调函数*/
    ret = int_manage.login_dma(g_cfg[spi].m_recv_dma_cha_int,m_recv_int_handle,&g_my_spi[spi]);
	if(E_OK != ret)
	{
		log_error("Int func login failed.");
		return E_ERROR;
	} 
    
    /*创建互斥锁*/
    g_my_spi[spi].m_mutex = xSemaphoreCreateMutex();
    if(NULL == g_my_spi[spi].m_mutex)
    {
        log_error("Mutex creat failed.");
        return E_ERROR;
    }
    
    return E_OK;
}

static int m_transmission(u8 spi,u8 datasize,const u8* send,u8* recv,u32 len,bool incremental,TickType_t time)
{
    BaseType_t os_ret = 0;

    /*参数检查*/
    if(MY_SPI_1 != spi && MY_SPI_2 != spi)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    if(0 == len || (NULL == send && NULL == recv))
    {
        log_error("Param error.");
        return E_PARAM;
    }
    g_my_spi[spi].m_spi_task = xTaskGetCurrentTaskHandle();
    if(NULL == g_my_spi[spi].m_spi_task)
    {
        log_error("Task handle get failed.");
        return E_ERROR;
    }

    /*获取互斥锁*/
    os_ret = xSemaphoreTake(g_my_spi[spi].m_mutex,time);
    if(pdTRUE != os_ret)
    {
        log_warning("Mutex take failed.");
        return E_ERROR;
    }
    
    /*确认spi状态*/
    if(g_my_spi[spi].m_spi_handle.State != HAL_SPI_STATE_READY)
    {
        log_error("Spi is busy.");
        return E_ERROR;
    }

    /*确认DMA状态*/
    if(g_my_spi[spi].m_tx_dma_handle.State != HAL_DMA_STATE_READY ||
       g_my_spi[spi].m_rx_dma_handle.State != HAL_DMA_STATE_READY)
    {
        log_error("Spi dma is busy.");
        return E_ERROR;
    }

    /*必须先关闭DMA 才可以修改DMA配置 和SPI一样*/
    __HAL_DMA_DISABLE(&g_my_spi[spi].m_tx_dma_handle);
	HAL_SPI_DMAStop(&g_my_spi[spi].m_spi_handle);
   
    
    /*确认增量模式*/
    if(incremental)
    { 
        g_cfg[spi].m_send_dma_channel->CCR |= DMA_CCR_MINC_Msk;  /*存储器增量模式*/      
    }
    else
    {
        g_cfg[spi].m_send_dma_channel->CCR &= ~DMA_CCR_MINC_Msk;  /*不执行存储器增量模式*/
    }
	  
	__HAL_DMA_ENABLE(&g_my_spi[spi].m_tx_dma_handle);

    
        /*清空任务通知*/
        os_ret = xTaskNotifyStateClear(g_my_spi[spi].m_spi_task);

        /*根据参数类型 启动相应的spi收发模式*/
        
        /*启动发送*/
        if(NULL != send)
        {
            g_my_spi[spi].m_incremental = incremental ;  /*保存增量模式*/
            g_my_spi[spi].m_data_addr   = send        ;  /*保存数据地址*/
            if(len > 0xFFFF)
            {
                g_my_spi[spi].m_send_len = 0xFFFF;
                g_my_spi[spi].m_need_len = len - 0xFFFF;
            }
            else
            {
                g_my_spi[spi].m_send_len = len;
                g_my_spi[spi].m_need_len = 0;
            } 
            
			HAL_SPI_Transmit_DMA(&g_my_spi[spi].m_spi_handle,(void*)send, len);

        }
        
       
        /*等待传输完成*/
        os_ret = ulTaskNotifyTake(pdTRUE,time);
        if(pdTRUE != os_ret)
        {
            log_error("Get task notify timout.");
            goto error_handle;
        } 


    /*清空数据*/
    g_my_spi[spi].m_send_len = 0;
    g_my_spi[spi].m_need_len = 0;
    g_my_spi[spi].m_data_addr = NULL;
    g_my_spi[spi].m_incremental = false;
        
    /*关闭spi*/
	__HAL_SPI_DISABLE(&g_my_spi[spi].m_spi_handle);
    //spi_enable(g_cfg[spi].m_spi_addr,false);
	__HAL_DMA_DISABLE(&g_my_spi[spi].m_tx_dma_handle);
	__HAL_DMA_DISABLE(&g_my_spi[spi].m_rx_dma_handle);
//    dma_channel_enable(g_cfg[spi].m_send_dma_channel,false); 
//    dma_channel_enable(g_cfg[spi].m_recv_dma_channel,false); 
    g_my_spi[spi].m_spi_task = NULL;
    
    os_ret = xSemaphoreGive(g_my_spi[spi].m_mutex);
    if(pdTRUE != os_ret)
    {
        log_error("Mutex give failed.");
    }
    
    return E_OK;

error_handle:
   
    /*关闭spi*/
	__HAL_SPI_DISABLE(&g_my_spi[spi].m_spi_handle);
    //spi_enable(g_cfg[spi].m_spi_addr,false);
	__HAL_DMA_DISABLE(&g_my_spi[spi].m_tx_dma_handle);
	__HAL_DMA_DISABLE(&g_my_spi[spi].m_rx_dma_handle);
//    dma_channel_enable(g_cfg[spi].m_send_dma_channel,false); 
//    dma_channel_enable(g_cfg[spi].m_recv_dma_channel,false); 
    g_my_spi[spi].m_spi_task = NULL;
    
    os_ret = xSemaphoreGive(g_my_spi[spi].m_mutex);
    if(pdTRUE != os_ret)
    {
        log_error("Mutex give failed.");
    }
    
    return E_ERROR;
}

static int m_set_datasize(u8 spi,u8 datasize)
{
    /*参数检查*/
    if(MY_SPI_1 != spi && MY_SPI_2 != spi)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    if(8 != datasize && 16 != datasize)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*确认spi状态*/
    if(g_my_spi[spi].m_spi_handle.State != HAL_SPI_STATE_READY)
    {
        log_error("Spi is busy.");
        return E_ERROR;
    }

    /*确认DMA状态*/
    if(g_my_spi[spi].m_tx_dma_handle.State != HAL_DMA_STATE_READY ||
       g_my_spi[spi].m_rx_dma_handle.State != HAL_DMA_STATE_READY)
    {
        log_error("Spi dma is busy.");
        return E_ERROR;
    }

    /*必须先关闭SPI DMA使能 才能更改配置*/
	__HAL_SPI_DISABLE(&g_my_spi[spi].m_spi_handle);
	__HAL_DMA_DISABLE(&g_my_spi[spi].m_tx_dma_handle);
	__HAL_DMA_DISABLE(&g_my_spi[spi].m_rx_dma_handle);

    if(8 == datasize)
    {
        g_my_spi[spi].m_spi_handle.Init.DataSize = SPI_DATASIZE_8BIT; 
        
        /*DFF 清0 位8位宽*/
        g_cfg[spi].m_spi_addr->CR1 &= ~SPI_CR1_DFF;

        /*发送 修改为8位宽*/
        g_cfg[spi].m_send_dma_channel->CCR &= ~DMA_CCR_PSIZE_Msk;
        g_cfg[spi].m_send_dma_channel->CCR |= DMA_PDATAALIGN_BYTE;
        g_cfg[spi].m_send_dma_channel->CCR &= ~DMA_CCR_MSIZE_Msk;
        g_cfg[spi].m_send_dma_channel->CCR |= DMA_MDATAALIGN_BYTE;

        /*接收 修改位8位宽*/
        g_cfg[spi].m_recv_dma_channel->CCR &= ~DMA_CCR_PSIZE_Msk;
        g_cfg[spi].m_recv_dma_channel->CCR |= DMA_PDATAALIGN_BYTE;
        g_cfg[spi].m_recv_dma_channel->CCR &= ~DMA_CCR_MSIZE_Msk;
        g_cfg[spi].m_recv_dma_channel->CCR |= DMA_MDATAALIGN_BYTE;
    }
    else
    {
        g_my_spi[spi].m_spi_handle.Init.DataSize = SPI_DATASIZE_16BIT;
        
        /*DFF 置1 位16位宽*/
        g_cfg[spi].m_spi_addr->CR1 |= SPI_CR1_DFF;

        /*发送 修改位16位宽*/
        g_cfg[spi].m_send_dma_channel->CCR &= ~DMA_CCR_PSIZE_Msk;
        g_cfg[spi].m_send_dma_channel->CCR |= DMA_PDATAALIGN_HALFWORD;
        g_cfg[spi].m_send_dma_channel->CCR &= ~DMA_CCR_MSIZE_Msk;
        g_cfg[spi].m_send_dma_channel->CCR |= DMA_MDATAALIGN_HALFWORD;

        /*接收 修改位16位宽*/
        g_cfg[spi].m_recv_dma_channel->CCR &= ~DMA_CCR_PSIZE_Msk;
        g_cfg[spi].m_recv_dma_channel->CCR |= DMA_PDATAALIGN_HALFWORD;
        g_cfg[spi].m_recv_dma_channel->CCR &= ~DMA_CCR_MSIZE_Msk;
        g_cfg[spi].m_recv_dma_channel->CCR |= DMA_MDATAALIGN_HALFWORD;
    }

    __HAL_SPI_ENABLE(&g_my_spi[spi].m_spi_handle);
    __HAL_DMA_ENABLE(&g_my_spi[spi].m_tx_dma_handle);
    __HAL_DMA_ENABLE(&g_my_spi[spi].m_rx_dma_handle);

    return E_OK;
}


static int m_send_int_handle(u32 dma_cha,void* param)
{
    m_spi* m_this = NULL;
    BaseType_t ret = 0;
    
    /*检查参数*/
    if(NULL == param)
    {
        log_error("NULL pointer.");
        return E_NULL;
    }
	m_this = param;
    
    /*如果触发接收完成DMA中断*/
    
    /*通道3 spi1 发送完成中断*/
	if(__HAL_DMA_GET_FLAG(&m_this->m_tx_dma_handle, __HAL_DMA_GET_TC_FLAG_INDEX(&m_this->m_tx_dma_handle)) != RESET)
    //if(RESET != dma_flag_get(DMA1_FDT3_FLAG))
    {
        //dma_flag_clear(DMA1_FDT3_FLAG);                                 /*清除标志*/
		//__HAL_DMA_DISABLE(&g_my_spi[m_this->m_id].m_tx_dma_handle);
        //dma_channel_enable(g_cfg[MY_SPI_1].m_send_dma_channel,FALSE);   /*关闭spi1 发送dma*/
        
        /*是否发送完毕*/
        if(0 != m_this->m_need_len)
        {
            /*检查需要发送的长度*/
            if(m_this->m_need_len > 0xFFFF)
            {
                m_this->m_send_len  = 0xFFFF;
                m_this->m_need_len -= 0xFFFF;
            }
            else
            {
                m_this->m_send_len = m_this->m_need_len;
                m_this->m_need_len = 0;
            }
            
            /*地址偏移*/
            if(m_this->m_incremental)
            {
                m_this->m_data_addr += m_this->m_send_len;
            }
            
            
            /*再次启动发送*/
			HAL_SPI_Transmit_DMA(&g_my_spi[m_this->m_id].m_spi_handle,(void*)m_this->m_data_addr,m_this->m_send_len);

//            m_this->m_tx_dma_handle.buffer_size       = m_this->m_send_len       ;       /*要传输的数据长度 */
//            m_this->m_tx_dma_handle.memory_base_addr  = (u32)m_this->m_data_addr ;       /*内存地址         */
//            
//            dma_init(g_cfg[MY_SPI_1].m_send_dma_channel,&g_my_spi[MY_SPI_1].m_tx_dma_handle);  /*配置发送dma*/
//			
//			__HAL_DMA_DISABLE(&g_my_spi[spi].m_tx_dma_handle);
            //dma_channel_enable(g_cfg[MY_SPI_1].m_send_dma_channel,TRUE);                  /*启动发送dma*/  
        }
        
        /*发送完毕*/
        else
        {
            
            /*发送任务通知 */
            vTaskNotifyGiveFromISR(m_this->m_spi_task,&ret); 
        }            
    }
	
	HAL_DMA_IRQHandler(&m_this->m_tx_dma_handle);

     /*手动进行一次任务切换*/
	if(pdTRUE == ret)
	{
		taskYIELD();
	}
//    /*通道2 spi1 接收完成中断*/
//    if(RESET != dma_flag_get(DMA1_FDT2_FLAG))
//    {
//        dma_flag_clear(DMA1_FDT2_FLAG);                                 /*清除标志*/
//		__HAL_DMA_DISABLE(&g_my_spi[m_this->m_id].m_rx_dma_handle);
//        //dma_channel_enable(g_cfg[MY_SPI_1].m_recv_dma_channel,FALSE);   /*关闭spi1 接收dma*/
//        
//        /*发送信号量 */
//        vTaskNotifyGiveFromISR(m_this->m_spi_task,&ret);
//    }
//    else
//    {
//        dma_flag_clear(DMA1_GL2_FLAG);
//    }
//    
//    /*通道5 spi2 发送完成中断*/
//    if(RESET != dma_flag_get(DMA1_FDT5_FLAG))
//    {
//        dma_flag_clear(DMA1_FDT5_FLAG);                                 /*清除标志*/
//        //dma_channel_enable(g_cfg[MY_SPI_2].m_send_dma_channel,FALSE);   /*关闭spi2 发送dma*/
//		__HAL_DMA_DISABLE(&g_my_spi[m_this->m_id].m_tx_dma_handle);
//        
//        /*是否发送完毕*/
//        if(0 != m_this->m_need_len)
//        {
//            /*检查需要发送的长度*/
//            if(m_this->m_need_len > 0xFFFF)
//            {
//                m_this->m_send_len  = 0xFFFF;
//                m_this->m_need_len -= 0xFFFF;
//            }
//            else
//            {
//                m_this->m_send_len = m_this->m_need_len;
//                m_this->m_need_len = 0;
//            }
//            
//            /*地址偏移*/
//            if(m_this->m_incremental)
//            {
//                m_this->m_data_addr += m_this->m_send_len;
//            }
//            
//            
//            /*再次启动发送*/
//            m_this->m_tx_dma_handle.buffer_size       = m_this->m_send_len       ;       /*要传输的数据长度 */
//            m_this->m_tx_dma_handle.memory_base_addr  = (u32)m_this->m_data_addr ;       /*内存地址         */
//            
//            dma_init(g_cfg[MY_SPI_2].m_send_dma_channel,&g_my_spi[MY_SPI_2].m_tx_dma_handle);  /*配置发送dma*/
//            //dma_channel_enable(g_cfg[MY_SPI_2].m_send_dma_channel,TRUE);                  /*启动发送dma*/  
//			__HAL_DMA_DISABLE(&g_my_spi[m_this->m_id].m_tx_dma_handle);
//        }
//        
//        
//        /*发送完毕*/
//        else
//        {
//            
//            /*发送任务通知 */
//            vTaskNotifyGiveFromISR(m_this->m_spi_task,&ret); 
//        }   
//    }
//    else
//    {
//        dma_flag_clear(DMA1_GL5_FLAG);
//    }
//    
//    /*通道4 spi2 接收完成中断*/
//    if(RESET != dma_flag_get(DMA1_FDT4_FLAG))
//    {
//        dma_flag_clear(DMA1_FDT4_FLAG);                                 /*清除标志*/
//        //dma_channel_enable(g_cfg[MY_SPI_2].m_recv_dma_channel,FALSE);   /*关闭spi2 接收dma*/
//		__HAL_DMA_DISABLE(&g_my_spi[m_this->m_id].m_rx_dma_handle);
//		
//        /*发送信号量 */
//        vTaskNotifyGiveFromISR(m_this->m_spi_task,&ret);
//    }
//    else
//    {
//        dma_flag_clear(DMA1_GL4_FLAG);
//    }
       
    
   
    
    return E_OK;
}

static int m_recv_int_handle(u32 dma_cha,void* param)
{
    m_spi* m_this = NULL;
    BaseType_t ret = 0;
    
    /*检查参数*/
    if(NULL == param)
    {
        log_error("NULL pointer.");
        return E_NULL;
    }
		m_this = param;
    
    /*如果触发接收完成DMA中断*/
    if(__HAL_DMA_GET_FLAG(&m_this->m_rx_dma_handle, __HAL_DMA_GET_TC_FLAG_INDEX(&m_this->m_rx_dma_handle)) != RESET)
    {
        /*这里接收完成不能马上关DMA有可能发送还没完*/
//        HAL_SPI_DMAStop(&m_this->m_spi_handle);  /*停止SPI*/

        /*发送信号量 */
//        xSemaphoreGiveFromISR(m_this->m_semaphore,&ret);
    }
    
    HAL_DMA_IRQHandler(&m_this->m_rx_dma_handle);
    
    /*手动进行一次任务切换*/
	if(pdTRUE == ret)
	{
		taskYIELD();
	}
    
    return E_OK;
}

static int m_set_speed(u8 spi,u8 speed)
{
			/*参数检查*/
    if(MY_SPI_1 != spi && MY_SPI_2 != spi)
    {
        log_error("Param error.");
        return E_PARAM;
    }
		
//		__HAL_SPI_DISABLE(&g_my_spi[spi].m_spi_handle);            //关闭SPI
//    g_my_spi[spi].m_spi_handle.Instance->CR1&=0XFFC7;          //位3-5清零，用来设置波特率
//    g_my_spi[spi].m_spi_handle.Instance->CR1|=speed;//设置SPI速度
//    __HAL_SPI_ENABLE(&g_my_spi[spi].m_spi_handle);             //使能SPI
		
	return E_OK;
}

//static int m_dinit(u8 spi)
//{
//    /*参数检查*/
//    if(MY_SPI_1 != spi && MY_SPI_2 != spi)
//    {
//        log_error("Param error.");
//        return E_PARAM;
//    }
//    
//    /*析构 IO口*/
//    crm_periph_clock_enable(g_cfg[spi].m_gpio_clk, FALSE);
//    gpio_bits_reset(g_cfg[spi].m_spi_gpio,g_cfg[spi].m_pin_sck);
//    gpio_bits_reset(g_cfg[spi].m_spi_gpio,g_cfg[spi].m_pin_miso);
//    gpio_bits_reset(g_cfg[spi].m_spi_gpio,g_cfg[spi].m_pin_mosi);
//    
//    /*析构SPI*/
//    crm_periph_clock_enable(g_cfg[spi].m_spi_clk, FALSE);
//    spi_enable(g_cfg[spi].m_spi_addr,FALSE);
//    spi_i2s_reset(g_cfg[spi].m_spi_addr);
//    
//    /*析构DMA*/
//    crm_periph_clock_enable(SPI_DMA_CLK, FALSE)                 ;
//    dma_channel_enable(g_cfg[spi].m_send_dma_channel,FALSE)     ; 
//    dma_channel_enable(g_cfg[spi].m_recv_dma_channel,FALSE)     ;     
//    spi_i2s_dma_transmitter_enable(g_cfg[spi].m_spi_addr,FALSE) ;   
//    dma_reset(g_cfg[spi].m_send_dma_channel);
//    dma_reset(g_cfg[spi].m_recv_dma_channel);
//    
//    return E_OK;
//}

#endif

