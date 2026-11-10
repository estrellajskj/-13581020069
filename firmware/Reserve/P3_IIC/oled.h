#ifndef __OLED_H
#define __OLED_H

					    					
#define OLED_TYPE_96  0 /*0.96寸oled*/
#define OLED_TYPE_91  1 /*0.91寸oled*/
 		     
typedef struct __C_OLED c_oled;

/*OLED对象*/
typedef struct __C_OLED
{
    void*   this;

    /************************************************* 
    * Function: display_clear 
    * Description: 清空屏幕
    * Input : <this>  OLED对象
    * Output: 在屏幕首次初始化时，或者需要清楚全屏时 使用
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         oled.display_clear(&oled);
    *         if(E_OK != ret)
    *         {
    *             log_error("Oled clear failed.");
    *         }
    *************************************************/ 
    int (*display_clear)(const c_oled* this); 

    /************************************************* 
    * Function: display_str 
    * Description: 显示字符串
    * Input : <this>  OLED对象
    *         <x>     字符串的x坐标  0~127
    *         <y>     字符串的y坐标0~3
    *         <format>格式化字符串
    *         <...>   可变参
    * Output: 注单次输入的字符数量不可超过16个 否则会多出的字符串会覆盖本行显示
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         float a = 10.0f;
    *
    *         oled.display_str(&oled,0,1,"dis2:%.2fcm ",a);
    *         if(E_OK != ret)
    *         {
    *             log_error("Oled display failed.");
    *         }
    *************************************************/ 
    int (*display_str)  (const c_oled* this,u8 x,u8 y, char* format,...);
    
    int (*display_chinese_str)  (const c_oled* this,u8 x,u8 y, char* str);

    /************************************************* 
    * Function: display_on 
    * Description: 打开屏幕显示
    * Input : <this>  OLED对象
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         oled.display_on(&oled);
    *         if(E_OK != ret)
    *         {
    *             log_error("Oled on failed.");
    *         }
    *************************************************/ 
    int (*display_on)   (const c_oled* this); 

    /************************************************* 
    * Function: display_off 
    * Description: 关闭屏幕显示
    * Input : <this>  OLED对象
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         oled.display_off(&oled);
    *         if(E_OK != ret)
    *         {
    *             log_error("Oled off failed.");
    *         }
    *************************************************/ 
    int (*display_off)  (const c_oled* this); 
}c_oled;

/************************************************* 
* Function: oled_create 
* Description: 创建一个oled对象
* Input : <iic>  oled所在的iic总线
*         <type> oled屏幕类型
*                (OLED_TYPE_96)   0.96寸oled
*                (OLED_TYPE_91)   0.91寸oled
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_bh1750 bh1750 = {0};
*         
*	      bh1750 = bh1750_creat(&iic,BH1750_ADDR_H);
*        if(NULL == bh1750.this)
*        {
*            log_error("bh1750 creat failed.");
*        }
*************************************************/
c_oled oled_create(c_my_iic* iic,u8 type);

#endif  
	 



