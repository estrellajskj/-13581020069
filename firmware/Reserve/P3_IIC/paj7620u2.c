#include "driver_conf.h"
#include "paj7620u2.h"
#include "paj7620u2_cfg.h"
#include "delay.h"

#define DEBUG 1  // USART2_Puts("LUX-Failed!");

//选择PAJ7620U2 BANK区域
void paj7620u2_selectBank(bank_e bank)
{
	switch(bank)
	{
		case BANK0: GS_Write_Byte(PAJ_REGITER_BANK_SEL,PAJ_BANK0);break;//BANK0寄存器区域
		case BANK1: GS_Write_Byte(PAJ_REGITER_BANK_SEL,PAJ_BANK1);break;//BANK1寄存器区域
	}
			
}

//PAJ7620U2唤醒
u8 paj7620u2_wakeup(void)
{ 
	u8 data=0x0a;
	GS_WakeUp();//唤醒PAJ7620U2
	delay_ms(5);//唤醒时间>400us
	GS_WakeUp();//唤醒PAJ7620U2
	delay_ms(5);//唤醒时间>400us
	paj7620u2_selectBank(BANK0);//进入BANK0寄存器区域
	data = GS_Read_Byte(0x00);//读取状态
	if(data!=0x20) return 0; //唤醒失败
	
	return 1;
}

//PAJ7620U2初始化
//返回值：0:失败 1:成功
u8 paj7620u2_init(void)
{
	u8 i;
	u8 status;
	
	GS_i2c_init();//IIC初始化
  status = paj7620u2_wakeup();//唤醒PAJ7620U2
	if(!status) return 0;
	paj7620u2_selectBank(BANK0);//进入BANK0寄存器区域
	for(i=0;i<INIT_SIZE;i++)
	{
		GS_Write_Byte(init_Array[i][0],init_Array[i][1]);//初始化PAJ7620U2
	}
  paj7620u2_selectBank(BANK0);//切换回BANK0寄存器区域
	
	paj7620u2_selectBank(BANK0);//进入BANK0寄存器区域
	for(i=0;i<GESTURE_SIZE;i++)
	{
		GS_Write_Byte(gesture_arry[i][0],gesture_arry[i][1]);//手势识别模式初始化
	}
	paj7620u2_selectBank(BANK0);//切换回BANK0寄存器区域
	
	return 1;
}

//手势识别测试
void Gesture_test(void)
{
	u8 i;
  u8 status;
	u8 data[2]={0x00};
	u16 gesture_data;
	paj7620u2_selectBank(BANK0);//进入BANK0寄存器区域
	for(i=0;i<GESTURE_SIZE;i++)
	{
		GS_Write_Byte(gesture_arry[i][0],gesture_arry[i][1]);//手势识别模式初始化
	}
	paj7620u2_selectBank(BANK0);//切换回BANK0寄存器区域
	i=100;
	while(1)
	{
//		GS_Write_Byte(PAJ_SET_INT_FLAG1,0X00);//关闭手势识别中断输出
//		GS_Write_Byte(PAJ_SET_INT_FLAG2,0X00);
		status = GS_Read_nByte(PAJ_GET_INT_FLAG1,2,&data[0]);//读取手势状态			
		if(!status)
		{   
			gesture_data =(u16)data[1]<<8 | data[0];
			if(gesture_data) 
			{
//				switch(gesture_data)
//				{
//					case GES_UP:              USART2_Puts("UP!"); UART_Send_Enter(); break; //向上
//					case GES_DOWM:            USART2_Puts("DOWN!"); UART_Send_Enter(); break; //向下
//					case GES_LEFT:            USART2_Puts("LEFT!"); UART_Send_Enter(); break; //向左
//					case GES_RIGHT:           USART2_Puts("RIGHT!"); UART_Send_Enter(); break; //向右
//					case GES_FORWARD:         USART2_Puts("FORWARD!"); UART_Send_Enter(); break; //向前
//					case GES_BACKWARD:        USART2_Puts("BACKWARD!"); UART_Send_Enter(); break; //向后
//					case GES_CLOCKWISE:       USART2_Puts("CLOCKWISE!"); UART_Send_Enter(); break; //顺时针
//					case GES_COUNT_CLOCKWISE: USART2_Puts("COUNT_CLOCKWISE!"); UART_Send_Enter(); break; //逆时针
//					case GES_WAVE:            USART2_Puts("WAVE!"); UART_Send_Enter(); break; //挥动
//					default:  break;
//					
//				}	
			}
		}
		delay_ms(50);
		i--;   
	}
}

//接近检测测试
//void Ps_test(void)
//{
//    u8 i;
//	u8 data[2]={0x00};
//	u8 obj_brightness=0;
//	u16 obj_size=0;
//	
//	paj7620u2_selectBank(BANK0);//进入BANK0寄存器区域
//	for(i=0;i<PROXIM_SIZE;i++)
//	{
//		GS_Write_Byte(proximity_arry[i][0],proximity_arry[i][1]);//接近检测模式初始化
//	}
//	paj7620u2_selectBank(BANK0);//返回BANK0寄存器区域
//	i=0;
//	
//	while(1)
//	{	
//		obj_brightness = GS_Read_Byte(PAJ_GET_OBJECT_BRIGHTNESS);//读取物体亮度
//		data[0] = GS_Read_Byte(PAJ_GET_OBJECT_SIZE_1);//读取物体大小
//		data[1] = GS_Read_Byte(PAJ_GET_OBJECT_SIZE_2);
//		obj_size = ((u16)data[1] & 0x0f)<<8 | data[0];
//		USART2_Puts("obj_brightness: ");
//		UART_Put_Num(obj_brightness);
//		UART_Send_Enter();
//		USART2_Puts("obj_size: ");
//		UART_Put_Num(obj_size);
//		UART_Send_Enter();
//		
//		delay_ms(100);
//		i++;
//	}
//}

//u8 Gesture_Get(void)
//{
//	u8 i;
//  u8 status,res=9;
//	u8 data[2]={0x00};
//	u16 gesture_data;
//	paj7620u2_selectBank(BANK0);//进入BANK0寄存器区域
//	for(i=0;i<GESTURE_SIZE;i++)
//	{
//		GS_Write_Byte(gesture_arry[i][0],gesture_arry[i][1]);//手势识别模式初始化
//	}
//	paj7620u2_selectBank(BANK0);//切换回BANK0寄存器区域
//	i = 100;
//	while(i)
//	{
//		status = GS_Read_nByte(PAJ_GET_INT_FLAG1,2,&data[0]);//读取手势状态			
//		if(!status)
//		{   
//			gesture_data =(u16)data[1]<<8 | data[0];
//			if(gesture_data) 
//			{
//				switch(gesture_data)
//				{
//					case GES_UP:              USART2_Puts("UP!"); UART_Send_Enter(); res = 0; break; //向上
//					case GES_DOWM:            USART2_Puts("DOWN!"); UART_Send_Enter(); res = 1; break; //向下
//					case GES_LEFT:            USART2_Puts("LEFT!"); UART_Send_Enter(); res = 2; break; //向左
//					case GES_RIGHT:           USART2_Puts("RIGHT!"); UART_Send_Enter(); res = 3; break; //向右
//					case GES_FORWARD:         USART2_Puts("FORWARD!"); UART_Send_Enter(); res = 4; break; //向前
//					case GES_BACKWARD:        USART2_Puts("BACKWARD!"); UART_Send_Enter(); res = 5; break; //向后
//					case GES_CLOCKWISE:       USART2_Puts("CLOCKWISE!"); UART_Send_Enter(); res = 6; break; //顺时针
//					case GES_COUNT_CLOCKWISE: USART2_Puts("COUNT_CLOCKWISE!"); UART_Send_Enter(); res = 7; break; //逆时针
//					case GES_WAVE:            USART2_Puts("WAVE!"); UART_Send_Enter(); res = 8; break; //挥动
//					default:  res = 9; break;
//				}	
//			}
//		}
//		i--;
//	}
//	return res;
//}

