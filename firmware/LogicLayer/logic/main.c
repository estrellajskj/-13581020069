/*main.c 主程序*/

#include "main.h"
static TaskHandle_t      my_logic_handle ;  /*任务句柄*/

static void led_init_early(void)
{
	GPIO_InitTypeDef gpio = {0};
	__HAL_RCC_GPIOB_CLK_ENABLE();
	gpio.Pin = GPIO_PIN_1;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_PULLUP;
	gpio.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &gpio);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
}

static void led_blink_once(u8 code)
{
	u8 i;
	for (i = 0; i < code; ++i) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
		HAL_Delay(150);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
		HAL_Delay(180);
	}
	HAL_Delay(500);
}

void firmware_fatal(u8 code)
{
	for (;;) led_blink_once(code);
}

void vApplicationMallocFailedHook(void)
{
	firmware_fatal(7);
}

int main()
{
	BaseType_t xReturn = pdPASS;
	//	初始化HAL库、时钟、定时器
	HAL_Init();
	led_init_early();
	if (Stm32_Clock_Init(RCC_PLL_MUL9) == 0) firmware_fatal(2);
	if (sys_clock_fallback) led_blink_once(1);
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_TIM1_CLK_ENABLE();
	__HAL_RCC_TIM2_CLK_ENABLE();
	__HAL_RCC_TIM3_CLK_ENABLE();
	__HAL_RCC_TIM4_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();		//使能io复用时钟	
	
	delay_init((u8)(SystemCoreClock / 1000000U));	//跟随实际HCLK配置微秒延时
	log_init();
	SystemIsr_PriorityInit();
	
	//	创建任务
    xReturn = xTaskCreate((TaskFunction_t )my_task_init,          
                          (const char* )"my_task_init",           
                          (uint16_t )512,
                          (void* )NULL,                             
                          (UBaseType_t )1,                          
                          (TaskHandle_t* )&my_logic_handle);  
    
	if (xReturn != pdPASS) firmware_fatal(3);
	vTaskStartScheduler();
	firmware_fatal(3); /* Scheduler should not return when its idle task starts. */
	return 0;
}




