#ifndef __MATRIX_KEYBOARD_H__
#define __MATRIX_KEYBOARD_H__

/*按键列表*/
typedef struct __MATRIX_IO
{
    GPIO_TypeDef* gpio    ;    /*按键所在GPIO分组*/
    uint32_t      pin     ;    /*按键所在PIN脚*/
}matrix_io_t;

typedef struct __C_MATRIX c_matrix;

typedef struct __C_MATRIX
{
    void* this;
	
    /************************************************* 
    * Function: set_callback 
    * Description: 设置键盘的回调函数
    * Input : <this>      键盘对象
    *         <callback>  回调函数
    * Output: <无> 
    * Return: <E_OK>     获取成功
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         int key_board_call(uint8_t num,u8 state)
    *         {
    *             log_inform("KEY: d",key_name,state);
    *             return E_OK;
    *         }
    *
    *         ret = board.set_callback(&board,key_board_call);
    *         if(E_OK != ret)
    *         {
    *             log_error("Callback set failed.");
    *         }
    *************************************************/  
    int (*set_callback)(c_matrix* this,int (u8 row,u8 col));
}c_matrix;


c_matrix matrix_keyboard_create(matrix_io_t *io_list);

#endif

