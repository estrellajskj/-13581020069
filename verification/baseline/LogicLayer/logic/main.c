/*main.c 主程序*/

#include "main.h"
static TaskHandle_t      my_logic_handle ;  /*任务句柄*/

int main()
{
	BaseType_t xReturn = pdPASS;
	//	初始化HAL库、时钟、定时器
	HAL_Init();                    	 	//初始化HAL库   
	Stm32_Clock_Init(RCC_PLL_MUL9);   	//设置时钟,72M
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_TIM1_CLK_ENABLE();
	__HAL_RCC_TIM2_CLK_ENABLE();
	__HAL_RCC_TIM3_CLK_ENABLE();
	__HAL_RCC_TIM4_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();		//使能io复用时钟	
	
	delay_init(72);               		//初始化延时函数
	log_init();
	SystemIsr_PriorityInit();
	
	//	创建任务
    xReturn = xTaskCreate((TaskFunction_t )my_task_init,          
                          (const char* )"my_task_init",           
                          (uint16_t )128,                          
                          (void* )NULL,                             
                          (UBaseType_t )1,                          
                          (TaskHandle_t* )&my_logic_handle);  
    
    if(pdPASS == xReturn)               
		vTaskStartScheduler(); 
	
	return(1);
}




