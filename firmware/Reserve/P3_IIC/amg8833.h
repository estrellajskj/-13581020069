#ifndef __AMG8833_H__
#define __AMG8833_H__

#define AMG8833_PIXEL_START (0x69 << 1)   // 温度寄存器起始地址
typedef struct __AMG8833 c_amg8833;

typedef struct __AMG8833
{
    void* this;

    /************************************************* 
    * Function: get 
    * Description: 获取温度矩阵
    * Input : <this>  amg8833对象
    * Output: <lux>   温度矩阵
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         float pixels[64] = {0};
    *
    *         ret = amg883.get(&amg883, pixels);
    *         if(E_OK != ret)
    *         {
    *             log_error("amg883 get failed.");
    *         }
    *************************************************/  
    int (*get)(const c_amg8833* this, float* pixels);
}c_amg8833;

/************************************************* 
* Function: amg8833_creat 
* Description: 创建一个amg8833对象
* Input : <iic>  amg8833所在的iic总线
*         <addr> amg8833的器件地址
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_amg8833 amg8833 = {0};
*         
*	      amg8833 = amg8833_creat(&iic);
*         if(NULL == amg8833.this)
*         {
*             log_error("amg8833 creat failed.");
*         }
*************************************************/
c_amg8833 amg8833_creat(const c_my_iic* iic);


/**********************************************************************************************/
#endif

