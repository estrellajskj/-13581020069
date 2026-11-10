#ifndef __EC800_MSG_H__
#define __EC800_MSG_H__

#define EC800_MSG_BAUDRATE  115200

#define EC800_MSG_NUM_LEN       100
#define EC800_MSG_MSG_MAX_LEN   100

typedef struct __C_EC800_MSG c_ec800_msg;

typedef struct __C_EC800_MSG
{
    void* this;

    int(*state)  (const c_ec800_msg* this);
	
	/************************************************* 
	* Function: ec800_msg发送短信 
	* Description: 创建 ec800_msg 对象
	* Demo  :
	*         c_ec800_msg ec800_msg = ec800_msg_creat(MY_UART_2);
	*         if(NULL == ec800_msg.this)
	*         {
	*             log_error("ec800_msg creat failed.");
	*         }
	*************************************************/
    int(*message)(const c_ec800_msg* this,const char* num,const char* content);
	
}c_ec800_msg;

/************************************************* 
* Function: ec800_msg_creat 
* Description: 创建 ec800_msg 对象
* Demo  :
*         c_ec800_msg ec800_msg = ec800_msg_creat(MY_UART_2);
*         if(NULL == ec800_msg.this)
*         {
*             log_error("ec800_msg creat failed.");
*         }
*************************************************/
c_ec800_msg ec800_msg_creat(u8 uart_id);

#endif

