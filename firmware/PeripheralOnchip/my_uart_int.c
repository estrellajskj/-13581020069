#include "onchip_conf.h"

#ifdef ONCHIP_UART_INT_ENABLED


#define MODULE_NAME       "uart"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME
 

typedef struct __UART_CFG
{  
    USART_TypeDef*       m_uart_addr       ;   /*串口地址*/
    UBaseType_t  m_recv_task_pro;              /*接收任务优先级*/
    uint16_t     m_recv_task_stk;              /*接收任务堆栈大小*/
    const char*  m_recv_task_name;             /*接收任务名字*/
}uart_cfg;

/*发送消息*/
typedef struct _UART_SEND_MSG
{
    void*  m_data     ;  /*数据地址*/
    u16    m_data_len ;  /*数据长度*/
}uart_send_msg;

typedef struct _M_UART
{
    u8                 m_my_uart_id                                    ;   /*串口id*/
    bool               m_init_state                                    ;  /*初始化状态*/
    volatile uint16_t  m_recv_cnt;                                     ;  /*串口接收计数*/
    u8                 m_recv_buf[UART_MAX_RECV_BUF_SIZE]              ;  /*接收缓存*/
    u8                 m_callback_buf[UART_MAX_RECV_BUF_SIZE]          ;  /*回调函数用的缓存*/
    void*              m_callback_param                                ;  /*回调函数参数*/
    int                (*m_callback)(void* param,const u8* data,const u16 data_len);  /*接收回调函数*/
    UART_HandleTypeDef m_uart_handle                                   ;  /*串口句柄        */
    TaskHandle_t       m_recv_task_handle                              ;  /*接收任务句柄*/
    TimerHandle_t      m_timer    ;  /*软件定时器*/
}m_uart;

static int m_uart_init           (u8  uart,u32 baud_rate,u16 buf_size);
static int m_uart_send           (u8  uart,const u8* data     ,u16 data_len);
static int m_uart_set_callback   (u8     uart,void* param,int (*callback)(void* param,const u8* data,u16 data_len));
static void m_recv_task(void* pdata);
static int m_recv_int_handle(u32 dma_cha,void* param);
static void  timer_call_back( TimerHandle_t xTimer );

const c_uart my_uart = {m_uart_init,m_uart_send,m_uart_set_callback};

/*串口配置*/
const uart_cfg  g_my_uart_cfg[3] = {{ USART1 , UART_1_RECV_TASK_PRO , UART_1_RECV_TASK_STK , "uart 1 recv"} ,
                                    { USART2 , UART_1_RECV_TASK_PRO , UART_2_RECV_TASK_STK , "uart 2 recv"} ,
                                    { USART3 , UART_1_RECV_TASK_PRO , UART_3_RECV_TASK_STK , "uart 3 recv"}};

/*串口 对象列表*/
m_uart g_my_uart[3];



static int m_uart_init           (u8 uart, u32 baud_rate, u16 buf_size)
{
    HAL_StatusTypeDef hal_ret  = HAL_ERROR ;
    BaseType_t        os_ret   = 0 ;
    int                   ret  = 0 ;

    /*检查参数*/
    if((MY_UART_1 != uart && MY_UART_2 != uart && MY_UART_3 != uart) || buf_size > UART_MAX_RECV_BUF_SIZE)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*检查初始化状态*/
    if(g_my_uart[uart].m_init_state)
    {
        return E_OK;
    }
    
    g_my_uart[uart].m_my_uart_id    = uart;
    g_my_uart[uart].m_recv_cnt      = 0;

     /*初始化串口*/
    g_my_uart[uart].m_uart_handle.Instance        = g_my_uart_cfg[uart].m_uart_addr  ;
    g_my_uart[uart].m_uart_handle.Init.BaudRate   = baud_rate                        ; //波特率
    g_my_uart[uart].m_uart_handle.Init.WordLength = UART_WORDLENGTH_8B               ; //字长为8位数据格式
    g_my_uart[uart].m_uart_handle.Init.StopBits   = UART_STOPBITS_1                  ; //一个停止位
    g_my_uart[uart].m_uart_handle.Init.Parity     = UART_PARITY_NONE                 ; //无奇偶校验位
    g_my_uart[uart].m_uart_handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE              ; //无硬件流控
    g_my_uart[uart].m_uart_handle.Init.Mode       = UART_MODE_TX_RX                  ; //收发模式
    hal_ret = HAL_UART_Init(&g_my_uart[uart].m_uart_handle)             ; //HAL_UART_Init()会使能UART1
    if(HAL_OK != hal_ret)
    {
        log_error("HAL init uart failed.");
        return E_ERROR;
    }
    
    //开启中断接收
    __HAL_UART_ENABLE_IT(&g_my_uart[uart].m_uart_handle, UART_IT_RXNE); 
   
    /*创建软件定时器*/
    g_my_uart[uart].m_timer = xTimerCreate("uart timer",      /*定时器名称*/
                         UART_TIME_INTERVAL_MS         ,      /*默认一秒*/
                         pdFALSE                       ,      /*单次定时器 */
                         &g_my_uart[uart]              ,      /*把串口传进去*/
                         timer_call_back);
    /*创建情况*/
    if(NULL == g_my_uart[uart].m_timer)
    {
        log_error("Timer creat failed.");
        return E_ERROR;
    }
    
    
    /*创建接收任务*/
    if(NULL == g_my_uart[uart].m_recv_task_handle)
    {
        os_ret = xTaskCreate((TaskFunction_t )m_recv_task    ,
                             (const char*    )g_my_uart_cfg[uart].m_recv_task_name ,
                             (uint16_t       )g_my_uart_cfg[uart].m_recv_task_stk  ,
                             (void*          )&g_my_uart[uart]                     ,
                             (UBaseType_t    )g_my_uart_cfg[uart].m_recv_task_pro  ,
                             (TaskHandle_t*  )&g_my_uart[uart].m_recv_task_handle);
                             
        if(pdPASS != os_ret)
        {
            log_error("UART send task creat filed,ret=%d",(int)os_ret);
            return E_ERROR;
        }
    } 

    g_my_uart[uart].m_init_state = true;

    return E_OK;
}

static void  timer_call_back( TimerHandle_t xTimer )
{
    m_uart* uart_handle = NULL;

    /*获取定时器ID*/
    uart_handle = pvTimerGetTimerID(xTimer);
    if(NULL == uart_handle)
    {
        log_error("Timer id get failed.");
        return;
    }
    
    /*发送任务通知*/
    xTaskNotifyGive(uart_handle->m_recv_task_handle);
    
    return;
}

static void m_recv_task(void* pdata)
{
    m_uart* uart_handle = NULL;
    int ret = 0;
    u32 data_len = 0;

    /*参数检查*/
    if(NULL == pdata)
    {
        log_error("Param error.");
        vTaskDelete(NULL);   /*删除本任务*/
    }

    /*打开接收*/
    uart_handle = pdata;
	
    while(1)
    {
        /*等待任务通知*/
        ret = ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        if(0 == ret)
        {

        }

        /*把数据拷贝过来*/
        if(0 != uart_handle->m_recv_cnt)
        {
            memcpy(uart_handle->m_callback_buf,uart_handle->m_recv_buf,uart_handle->m_recv_cnt);  
            data_len = uart_handle->m_recv_cnt;
            memset(uart_handle->m_recv_buf,0,uart_handle->m_recv_cnt);
            uart_handle->m_recv_cnt = 0;
        }
        
        /*调用回调函数*/
        if(NULL != uart_handle->m_callback)
        {
            ret = uart_handle->m_callback(uart_handle->m_callback_param,uart_handle->m_callback_buf,data_len);
            memset(uart_handle->m_callback_buf,0,data_len == UART_MAX_RECV_BUF_SIZE ? data_len : data_len + 1);
            data_len = 0;
        }
        
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_Initure;
    
    if(huart->Instance==USART1)//如果是串口1，进行串口1 MSP初始化
    {
        __HAL_RCC_USART1_CLK_ENABLE();          //使能USART1时钟
        __HAL_RCC_AFIO_CLK_ENABLE();
    
        GPIO_Initure.Pin    = GPIO_PIN_9           ; //PA9 TX
        GPIO_Initure.Mode   = GPIO_MODE_AF_PP      ; //复用推挽输出
        GPIO_Initure.Pull   = GPIO_PULLUP          ; //上拉
        GPIO_Initure.Speed  = GPIO_SPEED_FREQ_HIGH ; //高速
        HAL_GPIO_Init(GPIOA,&GPIO_Initure);          //初始化PA9

        GPIO_Initure.Pin    = GPIO_PIN_10;           //PA10 RX
        GPIO_Initure.Mode   = GPIO_MODE_AF_INPUT;    //模式要设置为复用输入模式！   
        HAL_GPIO_Init(GPIOA,&GPIO_Initure);          //初始化PA10      
		

		HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);                                
        HAL_NVIC_EnableIRQ(USART1_IRQn);   
    }
    else if(huart->Instance==USART2)//如果是串口2，进行串口2 MSP初始化
    {
        __HAL_RCC_USART2_CLK_ENABLE();          //使能USART1时钟
        __HAL_RCC_AFIO_CLK_ENABLE();
    
        GPIO_Initure.Pin    = GPIO_PIN_2           ; //PA2 TX
        GPIO_Initure.Mode   = GPIO_MODE_AF_PP      ; //复用推挽输出
        GPIO_Initure.Pull   = GPIO_PULLUP          ; //上拉
        GPIO_Initure.Speed  = GPIO_SPEED_FREQ_HIGH ; //高速
        HAL_GPIO_Init(GPIOA,&GPIO_Initure);          //初始化PA9

        GPIO_Initure.Pin    = GPIO_PIN_3;           //PA3 RX
        GPIO_Initure.Mode   = GPIO_MODE_AF_INPUT;    //模式要设置为复用输入模式！   
        HAL_GPIO_Init(GPIOA,&GPIO_Initure);          //初始化PA10      

		HAL_NVIC_SetPriority(USART2_IRQn, 5, 1);                                
		HAL_NVIC_EnableIRQ(USART2_IRQn);   		
    }
    else if(huart->Instance==USART3)//如果是串口3，进行串口3 MSP初始化
    {
        __HAL_RCC_USART3_CLK_ENABLE();          //使能USART1时钟
        __HAL_RCC_AFIO_CLK_ENABLE();
    
        GPIO_Initure.Pin    = GPIO_PIN_10           ; //PB10 TX
        GPIO_Initure.Mode   = GPIO_MODE_AF_PP      ; //复用推挽输出
        GPIO_Initure.Pull   = GPIO_PULLUP          ; //上拉
        GPIO_Initure.Speed  = GPIO_SPEED_FREQ_HIGH ; //高速
        HAL_GPIO_Init(GPIOB,&GPIO_Initure);          //初始化PA9

        GPIO_Initure.Pin    = GPIO_PIN_11;           //PB11 RX
        GPIO_Initure.Mode   = GPIO_MODE_AF_INPUT;    //模式要设置为复用输入模式！   
        HAL_GPIO_Init(GPIOB,&GPIO_Initure);          //初始化PA10      

		HAL_NVIC_SetPriority(USART3_IRQn, 5, 2);                                
		HAL_NVIC_EnableIRQ(USART3_IRQn);    		
    }
}

static int m_uart_send(u8 uart,const u8* data     ,u16 data_len)
{
    BaseType_t     os_ret  = 0;
    uint32_t           cnt = 0;

    /*检查参数*/
    if(MY_UART_1 != uart && MY_UART_2 != uart && MY_UART_3 != uart)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*参数检查*/
    if(NULL == data || 0 == data_len || UART_MAX_SEND_DATA_LEN < data_len)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*检查初始化状态*/
    if(!g_my_uart[uart].m_init_state)
    {
        log_error("Need init first.");
        return E_PARAM;
    }

    for(cnt = 0;cnt < data_len;++cnt)
    {
        while((g_my_uart[uart].m_uart_handle.Instance->SR & 0x40) == 0);	
        g_my_uart[uart].m_uart_handle.Instance->DR = data[cnt];
        while((g_my_uart[uart].m_uart_handle.Instance->SR & 0x40) == 0);
    }        

    return E_OK;
}

static int m_uart_set_callback   (u8 uart,void* param,int (*callback)(void* param,const u8* data,u16 data_len))
{
    /*检查参数*/
    if((MY_UART_1 != uart && MY_UART_2 != uart && MY_UART_3 != uart))
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*检查初始化状态*/
    if(!g_my_uart[uart].m_init_state)
    {
        log_error("Need init first.");
        return E_PARAM;
    }
    
    /*保存参数*/
    g_my_uart[uart].m_callback = callback;
    g_my_uart[uart].m_callback_param = param;
    
    return E_OK;
}

void USART1_IRQHandler(void)
{
    uint8_t nRec  = 0;
    BaseType_t ret = pdFAIL;
    BaseType_t tret = pdFAIL;
    
    if(__HAL_UART_GET_FLAG(&g_my_uart[MY_UART_1].m_uart_handle, UART_FLAG_RXNE) != RESET)
    {	      
        /*从寄存器 取到串口数据*/
        nRec = (uint16_t)READ_REG(g_my_uart[MY_UART_1].m_uart_handle.Instance->DR);        
        
        /*数据放到缓存中*/
        g_my_uart[MY_UART_1].m_recv_buf[g_my_uart[MY_UART_1].m_recv_cnt] = nRec;
        
        ++g_my_uart[MY_UART_1].m_recv_cnt;
        
        /*如果满了 立即通知*/
        if(g_my_uart[MY_UART_1].m_recv_cnt >= UART_MAX_RECV_BUF_SIZE)
        {
            xTimerStopFromISR(g_my_uart[MY_UART_1].m_timer,&tret); 
            vTaskNotifyGiveFromISR(g_my_uart[MY_UART_1].m_recv_task_handle,&ret);
        }
        
        /*如果收到了 \r\n 触发 发送接收完成 任务通知*/
        else if(g_my_uart[MY_UART_1].m_recv_cnt >= 2 && g_my_uart[MY_UART_1].m_recv_buf[g_my_uart[MY_UART_1].m_recv_cnt - 1] == '\n' && g_my_uart[MY_UART_1].m_recv_buf[g_my_uart[MY_UART_1].m_recv_cnt - 2] == '\r')
        {
            xTimerStopFromISR(g_my_uart[MY_UART_1].m_timer,&tret); 
            vTaskNotifyGiveFromISR(g_my_uart[MY_UART_1].m_recv_task_handle,&ret);
        } 

        /*收一个数据 刷一下定时器*/
        else
        {
            xTimerResetFromISR(g_my_uart[MY_UART_1].m_timer,&tret);    
        }

        /*手动进行一次任务切换*/
        if(pdTRUE == ret || pdTRUE == tret)
        {
            taskYIELD();
        }
    
    }
}   

void USART2_IRQHandler(void)
{
    uint8_t nRec  = 0;
    BaseType_t ret = pdFAIL;
    BaseType_t tret = pdFAIL;
    
    if(__HAL_UART_GET_FLAG(&g_my_uart[MY_UART_2].m_uart_handle, UART_FLAG_RXNE) != RESET)
    {	      
        /*从寄存器 取到串口数据*/
        nRec = (uint16_t)READ_REG(g_my_uart[MY_UART_2].m_uart_handle.Instance->DR);        
        
        /*数据放到缓存中*/
        g_my_uart[MY_UART_2].m_recv_buf[g_my_uart[MY_UART_2].m_recv_cnt] = nRec;
        
        ++g_my_uart[MY_UART_2].m_recv_cnt;
        
        /*如果满了 立即通知*/
        if(g_my_uart[MY_UART_2].m_recv_cnt >= UART_MAX_RECV_BUF_SIZE)
        {
            xTimerStopFromISR(g_my_uart[MY_UART_2].m_timer,&tret); 
            vTaskNotifyGiveFromISR(g_my_uart[MY_UART_2].m_recv_task_handle,&ret);
        }
        
        /*如果收到了 \r\n 触发 发送接收完成 任务通知*/
        else if(g_my_uart[MY_UART_2].m_recv_cnt >= 2 && g_my_uart[MY_UART_2].m_recv_buf[g_my_uart[MY_UART_2].m_recv_cnt - 1] == '\n' && g_my_uart[MY_UART_2].m_recv_buf[g_my_uart[MY_UART_2].m_recv_cnt - 2] == '\r')
        {
            xTimerStopFromISR(g_my_uart[MY_UART_2].m_timer,&tret); 
            vTaskNotifyGiveFromISR(g_my_uart[MY_UART_2].m_recv_task_handle,&ret);
        } 

        /*收一个数据 刷一下定时器*/
        else
        {
            xTimerResetFromISR(g_my_uart[MY_UART_2].m_timer,&tret);    
        }

        /*手动进行一次任务切换*/
        if(pdTRUE == ret || pdTRUE == tret)
        {
            taskYIELD();
        }
    
    }
}

void USART3_IRQHandler(void)
{
    uint8_t nRec  = 0;
    BaseType_t ret = pdFAIL;
    BaseType_t tret = pdFAIL;
    
    if(__HAL_UART_GET_FLAG(&g_my_uart[MY_UART_3].m_uart_handle, UART_FLAG_RXNE) != RESET)
    {	      
        /*从寄存器 取到串口数据*/
        nRec = (uint16_t)READ_REG(g_my_uart[MY_UART_3].m_uart_handle.Instance->DR);        
        
        /*数据放到缓存中*/
        g_my_uart[MY_UART_3].m_recv_buf[g_my_uart[MY_UART_3].m_recv_cnt] = nRec;
        
        ++g_my_uart[MY_UART_3].m_recv_cnt;
        
        /*如果满了 立即通知*/
        if(g_my_uart[MY_UART_3].m_recv_cnt >= UART_MAX_RECV_BUF_SIZE)
        {
            xTimerStopFromISR(g_my_uart[MY_UART_3].m_timer,&tret); 
            vTaskNotifyGiveFromISR(g_my_uart[MY_UART_3].m_recv_task_handle,&ret);
        }
        
        /*如果收到了 \r\n 触发 发送接收完成 任务通知*/
        else if(g_my_uart[MY_UART_3].m_recv_cnt >= 2 && g_my_uart[MY_UART_3].m_recv_buf[g_my_uart[MY_UART_3].m_recv_cnt - 1] == '\n' && g_my_uart[MY_UART_3].m_recv_buf[g_my_uart[MY_UART_3].m_recv_cnt - 2] == '\r')
        {
            xTimerStopFromISR(g_my_uart[MY_UART_3].m_timer,&tret); 
            vTaskNotifyGiveFromISR(g_my_uart[MY_UART_3].m_recv_task_handle,&ret);
        } 

        /*收一个数据 刷一下定时器*/
        else
        {
            xTimerResetFromISR(g_my_uart[MY_UART_3].m_timer,&tret);    
        }

        /*手动进行一次任务切换*/
        if(pdTRUE == ret || pdTRUE == tret)
        {
            taskYIELD();
        }
    
    }
}
#endif 
