

#ifndef __UART_DMA_H__
#define __UART_DMA_H__

#define UART_MAX_RECV_BUF_SIZE     256  /*接收缓存大小*/
#define UART_MAX_SEND_DATA_LEN     768  /*单条最大发送数据长度*/

#define MY_UART_1   0   /*串口1*/
#define MY_UART_2   1   /*串口2*/
#define MY_UART_3   2   /*串口3*/

typedef struct __C_UART
{
    /************************************************* 
    * Function: init 
    * Description: 初始化一个串口
    * Input : <uart>        初始化哪个uart  
    *         <baud_rate>   串口波特率
    *         <buf_size>    串口接收缓存大小 按使用需求来设计
    *                       注：该缓存为自动覆盖机制 既缓存满后，会自动覆盖老的数据
    * Output: 无
    * Return: <E_OK>             初始化成功
    *         <E_PARAM>          参数错误
    *         <E_OUT_OF_MEMORY>  内存不足
    *         <E_ERROR>          初始化失败
    * Others: 无
    * Demo  :
    *         int ret = 0;
    *         
    *         ret = my_uart.uart_init(MY_UART_1,115200,64);
    *         if(E_OK != ret)
    *         {
    *            log_error("Uart init failed.");
    *         }
    *************************************************/
    int (*init)           (u8 uart,u32 baud_rate,u16 buf_size);

    /************************************************* 
    * Function: send 
    * Description: 通过一个串口 发送串口消息
    * Input : <uart>        使用哪个uart  
    *         <data>        数据地址
    *         <data_len>    数据长度 注：不可超过UART_MAX_SEND_DATA_LEN
    *                       注：该缓存为自动覆盖机制 既缓存满后，会自动覆盖老的数据
    * Output: 无
    * Return: <E_OK>             发送成功
    *         <E_PARAM>          参数错误
    *         <E_OUT_OF_MEMORY>  内存不足
    *         <E_ERROR>          发送失败
    * Others: 该接口的使用，必须建立在初始化成功的基础上，否则调用将失败
    *         本发送为异步的，数据为拷贝传递，不需要保留原来的数据
    * Demo  :
    *         int ret = 0;
    *         
    *         ret = my_uart.send(MY_UART_1,data,data_len);
    *         if(E_OK != ret)
    *         {
    *            log_error("Uart send failed.");
    *         }
    *************************************************/
    int (*send)           (u8 uart,const u8* data     ,u16 data_len);

    /************************************************* 
    * Function: set_callback 
    * Description: 设置串口回调
    * Input : <uart>            设置哪个串口的回调
    *         <param>           回调触发 需要带入的参数
    *         <callback>        回调函数
    *           Input ：<param>       传参
    *                 ：<data>        接收缓存数据首地址
    *                 ：<data_len>    接收缓存数据长度
    * Output: 无
    * Return: <E_OK>             设置成功
    *         <E_PARAM>          参数错误
    *         <E_ERROR>          设置失败
    * Others: 无
    * Demo  :
    *         int uart3_callback(void* param,const u8* data,u16 data_len)
    *         {
    *             ret = my_uart.send(MY_UART_3,NULL,data,data_len);
    *             if(E_OK != ret)
    *             {
    *                log_error("Uart send failed.");
    *             }
    *         }
    *         
    *         my_uart.set_callback(MY_UART_3,NULL,uart3_callback);
    *         if(E_OK != ret)
    *         {
    *            log_error("Callback set failed.");
    *         }
    *************************************************/
    int (*set_callback)   (u8 uart,void* param,int (*callback)(void* param,const u8* data,u16 data_len));
}c_uart;

extern const c_uart my_uart;  /*单例*/


#endif



