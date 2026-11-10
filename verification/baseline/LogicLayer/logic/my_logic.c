#include "app_conf.h"
#include "pin_conf.h"

#ifdef APP_MY_APP_ENABLED
#define MODULE_NAME       "main logic"

#ifdef MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

c_switch buzzer = {0};               //蜂鸣器

c_dht11 dht11 = {0};	//dht11传感器
float temp = 25, humi = 50;	//温湿度
int temp_thresh = 30, humi_thresh = 70;	//温湿度阈值
u8 temp_alarm = 0, humi_alarm = 0;	//温湿度是否报警

float air_quality = 0.0f, air_quality_thresh = 30, airV = 0.0f;    //空气质量、空气质量阈值、ADC电压
u8 air_quality_alarm = 0;	//空气质量是否报警

c_my_iic iic_ds3231 = {0};     //ds3231 iic
c_ds3231 ds3231 = {0};    //  时钟模块
time_type time_init = {0, 0, 9, 1, 1, 1, 24}; //  初始时间
time_type time_now = {0, 0, 9, 1, 1, 1, 24}; //  当前时间

c_switch led = {0};               //led
float light = 0.0f, light_thresh = 2.0f;    //光强、光强阈值
u8 light_alarm = 0;	//光强是否报警


#define DATA_LEN 64	//	上传数据长度
#define TASK1_UPDATE_TIME_MS  500	//	任务1间隔时间
static void my_task1(void* param)
{
	//函数返回值
	u8 ret = 0;

	//dht11传感器初始化
	dht11 = dht11_create(DHT11_GRP, DHT11_PIN);
	if (NULL == dht11.this)
	{
		log_error("dht11 creat failed.");
	}

	//初始化空气质量ADC
	my_adc.init(MQ135_ADC);

	//DS3231 iic初始化
	iic_ds3231 = iic_creat(DS3231_SDA_GRP, DS3231_SDA_PIN, DS3231_SCL_GRP, DS3231_SCL_PIN);
	if(NULL == iic_ds3231.this)
	{
		log_error("iic creat failed.");
	}
	//ds3231时钟模块初始化
	ds3231 = ds3231_creat(&iic_ds3231);
	if(E_OK != ret)
	{
		log_error("ds3231 init failed.");
	}
	ds3231.set(&ds3231, &time_init);    //只初始化一次，烧录一次后注释掉本行代码或删除，如果有同步时间的需求，也需要调用此函数

	//led初始化
	led = switch_create(LED_GRP, LED_PIN);
	if(NULL == led.this)
	{
		log_error("led creat failed.");
	}
	//关闭led
	led.set(&led, SWITCH_LOW);

	//蜂鸣器初始化
	buzzer = switch_create(BUZZER_GRP, BUZZER_PIN);
	if(NULL == buzzer.this)
	{
		log_error("buzzer creat failed.");
	}
	//关闭蜂鸣器
	buzzer.set(&buzzer, SWITCH_LOW);

	//初始化光敏ADC
	my_adc.init(GM_ADC);

	while(1)
	{
		//获取温湿度
		ret = dht11.get(&dht11, &temp, &humi);
		if(E_OK != ret)
		{
			log_error("dht11 get failed.");
		}
		if(temp > temp_thresh)
			temp_alarm = 1;
		else
			temp_alarm = 0;

		if(humi > humi_thresh)
			humi_alarm = 1;
		else
			humi_alarm = 0;

		//获取当前空气质量
		my_adc.get(MQ135_ADC, &airV);
		air_quality = pow(11.5428 * 7.04 * airV / (3.3 - airV), (1 / 0.6549));
		if(air_quality > air_quality_thresh)
			air_quality_alarm = 1;
		else
			air_quality_alarm = 0;

		//获取当前时间
		ret = ds3231.get(&ds3231, &time_now);
		if(E_OK != ret)
		{
			log_error("ds3231 get failed.");
		}

		//获取当前光强
		my_adc.get(GM_ADC, &light);
		if(light > light_thresh)
			light_alarm = 1;
		else
			light_alarm = 0;

		//报警
		if (0)
			buzzer.set(&buzzer, SWITCH_LOW);

		vTaskDelay(TASK1_UPDATE_TIME_MS);
	}

}

//	从esp328266（tcp）获取的数据
char g_8266_tcp_recv_buf[256] = {0};
int g_8266_tcp_recv_flag = 0;
//	esp328266（tcp）串口回调函数
int esp8266_tcp_recv_call(void* param,const u8* data,u16 data_len)    
{
	if (g_8266_tcp_recv_flag == 0)
	{
		memset(g_8266_tcp_recv_buf, 0, 256);
		memcpy(g_8266_tcp_recv_buf, data, data_len);
		g_8266_tcp_recv_flag = 1;
	}
	else
	{
		memcpy(g_8266_tcp_recv_buf + strlen(g_8266_tcp_recv_buf), data, data_len);
	}
}
//	解析esp8266 tcp命令
void parse_8266_tcp_cmd()
{
	int len = strlen(g_8266_tcp_recv_buf);
	if (g_8266_tcp_recv_flag == 0 || g_8266_tcp_recv_buf[len - 2] != '\r' || g_8266_tcp_recv_buf[len - 1] != '\n')
		return;

	char *token = strtok(g_8266_tcp_recv_buf, "\r\n");
	char* ptr = NULL;
	int value = 0;
	while (token != NULL)
	{
		//	解析数据
		if (NULL != (ptr = strstr((char*)token, "AT+COMMAND1=")))
		{
			sscanf(ptr, "AT+COMMAND1=%d,OK\r\n", &value);  
		}
		else if (NULL != (ptr = strstr((char*)token, "AT+COMMAND2=")))
		{
			sscanf(ptr,"AT+COMMAND2=%d,OK\r\n", &value);  
		}
		token = strtok(NULL, "\r\n");
	}

	g_8266_tcp_recv_flag = 0;
}

#define TASK2_UPDATE_TIME_MS  100	//	任务2间隔时间
static void my_task2(void* param)
{
	while(1)
	{
		parse_8266_tcp_cmd();
		vTaskDelay(TASK2_UPDATE_TIME_MS);
	}
}
#define TASK3_UPDATE_TIME_MS  2500	//	任务3间隔时间
static void my_task3(void* param)
{
	u8 ret = 0;   //函数返回值
	//	待发送的数据
	char send_buf[DATA_LEN] = {0};
	char send_buf_alarm[DATA_LEN] = {0};

	//esp8266模块初始化
	c_esp8266_wifi   esp8266_wifi = {0};
	esp8266_wifi = esp8266_wifi_create(ESP8266_TCP_UART_ID, WIFI_NAME, WIFI_PASSWORD);
	//设置串口回调函数
	ret = esp8266_wifi.set_callback(&esp8266_wifi, NULL, esp8266_tcp_recv_call);
	while(1)
	{
		//清空发送数据字符串数组
		memset(send_buf, 0, DATA_LEN);
		//拼接要发送的字符串
		//snprintf(send_buf, DATA_LEN, "AT+DATA=XXX,OK\r\n");
		memset(send_buf_alarm, 0, DATA_LEN);
		//snprintf(send_buf_alarm, DATA_LEN, "AT+MATA=XXX,OK\r\n");
		esp8266_wifi.send(&esp8266_wifi, (const u8*)send_buf, strlen(send_buf));

		vTaskDelay(300);
		esp8266_wifi.send(&esp8266_wifi, (const u8*)send_buf_alarm, strlen(send_buf_alarm));

		vTaskDelay(TASK3_UPDATE_TIME_MS);
	}
}

static TaskHandle_t      g_task1_handle ;  		 /*任务1句柄*/
static TaskHandle_t      g_task2_handle ;  		 /*任务2句柄*/
static TaskHandle_t      g_task3_handle ;  		 /*任务3句柄*/

//任务初始化
void my_task_init(void)
{
	taskENTER_CRITICAL();
	BaseType_t os_ret;
#if 1
	/*创建任务1*/
	if(NULL == g_task1_handle)
	{
		os_ret = xTaskCreate((TaskFunction_t )my_task1,   
			(const char*    )MODULE_NAME, 
			(uint16_t       )MY_APP_1_STK, 
			(void*          )NULL,	 
			(UBaseType_t    )MY_APP_1_PRO,		 
			(TaskHandle_t*  )&g_task1_handle); 		 

		if(pdPASS != os_ret)
		{
			log_error("Task init filed,ret=%d",(int)os_ret);
			return;
		}
}
#endif

#if 1
	/*创建任务2*/
	if(NULL == g_task2_handle)
	{
		os_ret = xTaskCreate((TaskFunction_t )my_task2,   
			(const char*    )MODULE_NAME, 
			(uint16_t       )MY_APP_2_STK, 
			(void*          )NULL,	 
			(UBaseType_t    )MY_APP_2_PRO,		 
			(TaskHandle_t*  )&g_task2_handle); 		 

		if(pdPASS != os_ret)
		{
			log_error("Task init filed,ret=%d",(int)os_ret);
			return;
		}
}
#endif

#if 1
	/*创建任务3*/
	if(NULL == g_task3_handle)
	{
		os_ret = xTaskCreate((TaskFunction_t )my_task3,   
			(const char*    )MODULE_NAME, 
			(uint16_t       )MY_APP_3_STK, 
			(void*          )NULL,	 
			(UBaseType_t    )MY_APP_3_PRO,		 
			(TaskHandle_t*  )&g_task3_handle); 		 

		if(pdPASS != os_ret)
		{
			log_error("Task init filed,ret=%d",(int)os_ret);
			return;
		}
}
#endif

	vTaskDelete(NULL);
	taskEXIT_CRITICAL();
}
#endif
