#ifndef __FRM1213_H__
#define __FRM1213_H__



#define FRM1213_BAUDRATE    115200   /*frm1213的串口波特率*/

typedef struct __FRM1213 c_frm1213;

typedef struct __FRM1213
{
    void* this;

    int (*add)    (c_frm1213* this);   /*添加人脸*/
    int (*del_all)(c_frm1213* this);   /*删除所有人脸*/
    int (*det)    (c_frm1213* this);   /*人脸检测*/
}c_frm1213;

c_frm1213 frm1213_creat(u8 uart_id);

#endif


