#include "onchip_conf.h"

#ifdef ONCHIP_UART_DMA_ENABLED
#define UART_QUEUE_LENGTH 3
#define UART_BUFFER_SIZE UART_MAX_RECV_BUF_SIZE

typedef struct { u8 *data; u16 length; } uart_message;
typedef struct {
    UART_HandleTypeDef uart;
    DMA_HandleTypeDef tx, rx;
    QueueHandle_t queue;
    u8 buffer[UART_BUFFER_SIZE];
    unsigned position;
    int ready;
    void *arg;
    int (*callback)(void *,const u8 *,u16);
} uart_port;
static uart_port ports[3];
static TaskHandle_t tx_task, rx_task;
static int uart_init(u8 id,u32 baud,u16 size);
static int uart_send(u8 id,const u8 *data,u16 length);
static int uart_callback(u8 id,void *arg,int (*callback)(void *,const u8 *,u16));
const c_uart my_uart = {uart_init,uart_send,uart_callback};

static void stop_tx(uart_port *p)
{
    CLEAR_BIT(p->uart.Instance->CR3,USART_CR3_DMAT);
    HAL_DMA_Abort(&p->tx);
    p->uart.State = p->uart.State == HAL_UART_STATE_BUSY_TX_RX ? HAL_UART_STATE_BUSY_RX : HAL_UART_STATE_READY;
}
static int tx_interrupt(u32 channel,void *arg)
{
    uart_port *p = arg;
    BaseType_t wake = pdFALSE;
    (void)channel;
    if (__HAL_DMA_GET_FLAG(&p->tx,__HAL_DMA_GET_TC_FLAG_INDEX(&p->tx)) != RESET) {
        stop_tx(p);
        if (tx_task) vTaskNotifyGiveFromISR(tx_task,&wake);
    }
    HAL_DMA_IRQHandler(&p->tx);
    portYIELD_FROM_ISR(wake);
    return E_OK;
}
static int rx_interrupt(u32 channel,void *arg)
{
    uart_port *p = arg;
    (void)channel;
    HAL_DMA_IRQHandler(&p->rx);
    return E_OK;
}
static void transmit_task(void *arg)
{
    unsigned id;
    uart_message message;
    (void)arg;
    for (;;) {
        for (id = 0; id < 3; ++id) {
            uart_port *p = &ports[id];
            if (!p->ready || xQueueReceive(p->queue,&message,0) != pdTRUE) continue;
            ulTaskNotifyTake(pdTRUE,0);
            if (HAL_UART_Transmit_DMA(&p->uart,message.data,message.length) == HAL_OK) {
                if (!ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(1500))) stop_tx(p);
            }
            vPortFree(message.data);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
static void receive_task(void *arg)
{
    unsigned id, end, part;
    (void)arg;
    for (;;) {
        for (id = 0; id < 3; ++id) {
            uart_port *p = &ports[id];
            if (!p->ready) continue;
            end = (UART_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&p->rx)) % UART_BUFFER_SIZE;
            if (end == p->position) continue;
            if (end < p->position) {
                part = UART_BUFFER_SIZE - p->position;
                if (p->callback) p->callback(p->arg,p->buffer + p->position,(u16)part);
                p->position = 0;
            }
            if (end > p->position && p->callback)
                p->callback(p->arg,p->buffer + p->position,(u16)(end - p->position));
            p->position = end;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
static int uart_init(u8 id,u32 baud,u16 size)
{
    static USART_TypeDef * const instances[] = {USART1,USART2,USART3};
    static DMA_Channel_TypeDef * const tx_channels[] = {DMA1_Channel4,DMA1_Channel7,DMA1_Channel2};
    static DMA_Channel_TypeDef * const rx_channels[] = {DMA1_Channel5,DMA1_Channel6,DMA1_Channel3};
    static const u32 tx_irqs[] = {DMA1_CHA_4,DMA1_CHA_7,DMA1_CHA_2};
    static const u32 rx_irqs[] = {DMA1_CHA_5,DMA1_CHA_6,DMA1_CHA_3};
    uart_port *p;
    if (id >= 3 || size > UART_BUFFER_SIZE || !baud) return E_PARAM;
    p = &ports[id];
    if (p->ready) return E_OK;
    p->uart.Instance = instances[id];
    p->uart.Init.BaudRate = baud;
    p->uart.Init.WordLength = UART_WORDLENGTH_8B;
    p->uart.Init.StopBits = UART_STOPBITS_1;
    p->uart.Init.Parity = UART_PARITY_NONE;
    p->uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    p->uart.Init.Mode = UART_MODE_TX_RX;
    if (HAL_UART_Init(&p->uart) != HAL_OK) return E_ERROR;
    p->tx.Instance = tx_channels[id];
    p->tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    p->tx.Init.PeriphInc = DMA_PINC_DISABLE;
    p->tx.Init.MemInc = DMA_MINC_ENABLE;
    p->tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    p->tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    p->tx.Init.Mode = DMA_NORMAL;
    p->tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    p->rx.Instance = rx_channels[id];
    p->rx.Init = p->tx.Init;
    p->rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    p->rx.Init.Mode = DMA_CIRCULAR;
    if (HAL_DMA_Init(&p->tx) != HAL_OK || HAL_DMA_Init(&p->rx) != HAL_OK) return E_ERROR;
    __HAL_LINKDMA(&p->uart,hdmatx,p->tx);
    __HAL_LINKDMA(&p->uart,hdmarx,p->rx);
    if (int_manage.login_dma(tx_irqs[id],tx_interrupt,p) != E_OK ||
        int_manage.login_dma(rx_irqs[id],rx_interrupt,p) != E_OK) return E_ERROR;
    p->queue = xQueueCreate(UART_QUEUE_LENGTH,sizeof(uart_message));
    if (!p->queue) return E_OUT_OF_MEMORY;
    if (HAL_UART_Receive_DMA(&p->uart,p->buffer,UART_BUFFER_SIZE) != HAL_OK) return E_ERROR;
    if (!tx_task && xTaskCreate(transmit_task,"uart tx",192,NULL,UART_1_SEND_TASK_PRO,&tx_task) != pdPASS) return E_OUT_OF_MEMORY;
    if (!rx_task && xTaskCreate(receive_task,"uart rx",192,NULL,UART_1_RECV_TASK_PRO,&rx_task) != pdPASS) return E_OUT_OF_MEMORY;
    __DMB(); p->ready = 1;
    return E_OK;
}
static int uart_send(u8 id,const u8 *data,u16 length)
{
    uart_message message;
    if (id >= 3 || !ports[id].ready || !data || !length || length > UART_MAX_SEND_DATA_LEN) return E_PARAM;
    message.data = pvPortMalloc(length);
    if (!message.data) return E_OUT_OF_MEMORY;
    memcpy(message.data,data,length); message.length = length;
    if (xQueueSend(ports[id].queue,&message,0) != pdTRUE) {
        vPortFree(message.data); return E_ERROR;
    }
    return E_OK;
}
static int uart_callback(u8 id,void *arg,int (*callback)(void *,const u8 *,u16))
{
    if (id >= 3 || !ports[id].ready) return E_PARAM;
    ports[id].arg = arg; __DMB(); ports[id].callback = callback;
    return E_OK;
}
void HAL_UART_MspInit(UART_HandleTypeDef *handle)
{
    GPIO_InitTypeDef gpio = {0};
    GPIO_TypeDef *group;
    u32 tx,rx;
    if (handle->Instance == USART1) { __HAL_RCC_USART1_CLK_ENABLE(); group=GPIOA; tx=GPIO_PIN_9; rx=GPIO_PIN_10; }
    else if (handle->Instance == USART2) { __HAL_RCC_USART2_CLK_ENABLE(); group=GPIOA; tx=GPIO_PIN_2; rx=GPIO_PIN_3; }
    else if (handle->Instance == USART3) { __HAL_RCC_USART3_CLK_ENABLE(); group=GPIOB; tx=GPIO_PIN_10; rx=GPIO_PIN_11; }
    else return;
    gpio.Pin=tx; gpio.Mode=GPIO_MODE_AF_PP; gpio.Pull=GPIO_PULLUP; gpio.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(group,&gpio);
    gpio.Pin=rx; gpio.Mode=GPIO_MODE_AF_INPUT;
    HAL_GPIO_Init(group,&gpio);
}
#endif
