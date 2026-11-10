#include "app_conf.h"
#include "pin_conf.h"

#ifdef APP_MY_APP_ENABLED
#define MODULE_NAME       "main logic"

#ifdef MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

c_switch buzzer = {0};               //蜂鸣器

c_key_board 	board;  //键盘对象，检测引脚电平
c_dht11 dht11 = {0};	//dht11传感器
float temp = 25, humi = 50;	//温湿度
int temp_thresh = 30, humi_thresh = 70;	//温湿度阈值
u8 temp_alarm = 0, humi_alarm = 0;	//温湿度是否报警

c_ds18b20 ds18b20 = {0};	//ds18b20传感器
float ds18b20_temp = 25.0f, ds18b20_temp_thresh = 37;	//温度,温度阈值
u8 ds18b20_temp_alarm = 0;	//温度是否报警

float soil_humi = 0.0f, soil_humi_thresh = 30, soil_humiV = 0.0f;    //土壤湿度、土壤湿度阈值、ADC电压
u8 soil_humi_alarm = 0;	//土壤湿度是否报警

c_my_iic iic_oled;      //oled液晶屏iic
c_oled oled;      //oled液晶屏


c_ec800_msg ec800_msg = {0};      //SIM800模块

char odata[50] = {0};      //要发送的短信数据

char tel[15] = "15000000000";      //接收短信的手机号
float ch4 = 0.0f, ch4_thresh = 30, ch4V = 0.0f;    //甲烷浓度、甲烷浓度阈值、ADC电压
u8 ch4_alarm = 0;	//甲烷是否报警

c_my_iic iic_sgp30 = {0};     //sgp30传感器iic
c_sgp30 sgp30;     //sgp30模块
u16 co2 = 0, voc = 0; //  co2浓度和voc浓度
int co2_thresh = 50, voc_thresh = 10; //  cco2浓度和voc浓度阈值
u8 co2_alarm = 0, voc_alarm = 0;	//co2、voc是否报警

float co = 0.0f, co_thresh = 30, coV = 0.0f;    //co浓度、co浓度阈值、ADC电压
u8 co_alarm = 0;	//co浓度是否报警

float smoke = 0.0f, smoke_thresh = 30, smokeV = 0.0f;    //烟雾浓度、烟雾浓度阈值、ADC电压
u8 smoke_alarm = 0;	//烟雾浓度是否报警

c_my_iic iic_bmp180 = {0};     //bmp180 iic
c_bmp180 bmp180 = {0};   //大气压传感器
float t = 0.0f,alt = 0.0f; //当前温度 单位 摄氏度,当前海拔 单位 米
u32 pa = 0;              //当前气压 单位 帕斯卡
c_my_iic iic_mlx90614 = {0};     //mlx90614传感器iic
c_mlx90614 mlx90614 = {0};     //mlx90614红外测温模块
float mlx90614_temp = 0.0f, env = 25.0f, mlx90614_temp_thresh = 10; //  体温、环境温度和和体温阈值
u8 mlx90614_temp_alarm = 0;	//温度是否报警

#include <cmath>
float ntcV = 0.0f, ntcR = 0.0f, ntc_temp = 0.0f, ntc_temp_thresh = 0.0f;    //电压、温度、温度阈值
#define T25 298.15    //25摄氏度对应的绝对温度
#define R25 100    //25摄氏度时的电阻值
#define B   3950

float get_temp(float r)
{
	float N1, N2, N3, N4;
	N1 = (log(R25) - log(r)) / B;
	N2 = 1 / T25 - N1;
	N3 = 1 / N2;
	N4 = N3 - 273.15;
	return N4;
}
u8 ntc_alarm = 0;	//ntc温度是否报警

c_my_iic iic_bh1750 = {0};     //光照强度传感器iic
c_bh1750    bh1750;      //光照强度传感器
u16 lux = 0; //  光照强度
int lux_thresh = 200; //  光照强度阈值
u8 lux_alarm = 0;	//光照是否报警

c_gp2y  gp2y  = {0};     //pm2.5模块
float pm2_5 = 0.0f, pm2_5_thresh = 100;       //pm2.5值、pm2.5阈值
u8 pm2_5_alarm = 0;	//pm2.5是否报警

float air_quality = 0.0f, air_quality_thresh = 30, airV = 0.0f;    //空气质量、空气质量阈值、ADC电压
u8 air_quality_alarm = 0;	//空气质量是否报警

float alcohol = 0.0f, alcohol_thresh = 30, alcoholV = 0.0f;    //酒精浓度、酒精浓度阈值、ADC电压
u8 alcohol_alarm = 0;	//酒精是否报警

c_gps gps = {0};     //gps模块
nmea_gga gps_data = {0};     //gps数据

c_my_iic iic_mpu6050 = {0};     //mpu6050 iic
float x = 0.0f, y = 0.0f,z = 0.0f;    //x、y、z三轴数据
u8 fall_down = 0; //是否摔倒
c_hx711 hx711 = {0};     //hx711模块
float weight = 0.0f, weight_thresh = 100;    //重量,重量阈值
u8 weight_alarm = 0;	//重量是否报警

float pressure = 0.0f, pressure_thresh = 30, pressureV = 0.0f;    //气/血压、气/血压阈值、ADC电压
u8 pressure_alarm = 0;	//气/血压是否报警

c_my_iic iic_max30102 = {0};     //max30102传感器iic
u32 heart = 0, blood_o2 = 0;    //心率、血氧
int heart_thresh = 70, blood_o2_thresh = 0;    //心率、血氧阈值
u8 heart_alarm = 0, blood_o2_alarm = 0;	//心率、血氧是否报警

c_ph  ph  = {0};
float ph_value = 0.0f, ph_thresh = 9, phV = 0.0f;    //ph、ph阈值、ADC电压
u8 ph_alarm = 0;	//ph是否报警

float turbidity = 0.0f, turbidity_thresh = 30, turbidityV = 0.0f;    //浊度、浊度阈值、ADC电压
u8 turbidity_alarm = 0;	//浊度是否报警

float voice = 0.0f, voice_thresh = 30, voiceV = 0.0f;    //分贝、分贝阈值、ADC电压
u8 voice_alarm = 0;	//是否噪音报警

//手势识别相关头文件引入
#include "paj7620u2.h"
#include "paj7620u2_iic.h"

u16 gesture_data = 0;             //手势识别传感器解析后的数据
u8 gesture_status = 0;             //手势识别状态
u8 data[2] = {0x00};             //手势识别传感器读取到的数据
//	rc522回调函数
int rfid_callback(void* param,u8 event,const u8* data)    
{
	char str_buf[50] = {0};
	if(RC522_FIND == event)
	{
		log_inform("Get card:%#X,%#X,%#X,%#X.",data[0], data[1], data[2], data[3]);

		/*对比卡号，合法卡*/
		if(0x91 == data[0])
		{
		}
		/*非法卡*/
		else
		{
		}
	}

	return E_OK;
}

c_my_iic iic_ina219;      //功率计ina219 iic
c_ina219 ina219;      //功率计ina219
u32 mv = 0, mw = 0, ma = 0, ma_thresh = 100; 
u8 ma_alarm = 0;	//是否电流过大报警

c_my_iic iic_ds3231 = {0};     //ds3231 iic
c_ds3231 ds3231 = {0};    //  时钟模块
time_type time_init = {0, 0, 9, 1, 1, 1, 24}; //  初始时间
time_type time_now = {0, 0, 9, 1, 1, 1, 24}; //  当前时间

#include "lcd_init.h"
#include "lcd.h"
#include "pic.h"
#include "touch.h"
c_ws2812 ws2812 = {0};              //ws2812模块
c_switch led = {0};               //led
c_jq8400 jq8400 = {0};              //jq8400语音播报模块
u8 jq8400_state = JQ8400_IDLE;              //jq8400语音播报模块
c_servo sg90 = {0};               //舵机
c_switch relay = {0};               //继电器
c_switch fan = {0};               //风扇
c_switch pump = {0};               //水泵
c_switch jrp = {0};               //加热片
c_switch whp = {0};               //雾化片
c_uln2003 uln2003 = {0};              //步进电机
c_switch motor = {0};               //马达
c_my_iic iic_tcs34725 = {0};     //颜色识别传感器iic
c_tcs34725    tcs34725;      //tcs34725传感器
u16 r = 0,g = 0,b = 0, c = 0; //  红色值 绿色值 蓝色值 白色值

float light = 0.0f, light_thresh = 2.0f;    //光强、光强阈值
u8 light_alarm = 0;	//光强是否报警

c_tft18 tft18 = {0};      //tft液晶屏

c_hc_sr04 hc_sr04 = {0};     //超声传感器
float dis = 0.0f, dis_thresh = 30;    //距离、距离阈值
u8 dis_alarm = 0;	//是否距离过近报警

float com_gas = 0.0f, com_gas_thresh = 30, com_gasV = 0.0f;    //可燃气体浓度、可燃气体浓度阈值、ADC电压
u8 com_gas_alarm = 0;	//可燃气体浓度是否报警

#include "flash.h"
// 假设的 Flash 读写函数
#define HISTORY_RECORD_COUNT 5
typedef struct {
	int type;	//0 长度 1 面积 2 体积
	u16 val;   //数值
} Data; 

typedef struct {
	Data data[HISTORY_RECORD_COUNT];	//数据
	u16 write_flag; //写入标志位
} HistoryRecord; 
// 每个历史记录包含距离、面积和体积 
HistoryRecord history_records; 
// 定义 Flash 存储起始地址 
#define FLASH_START_ADDRESS ((uint32_t)0X08018000) 
c_switch r_led1 = {0};               //红灯1
c_switch g_led1 = {0};               //绿灯1
c_switch y_led1 = {0};               //黄灯1
c_switch r_led2 = {0};               //红灯2
c_switch g_led2 = {0};               //绿灯2
c_switch y_led2 = {0};               //黄灯2
float nh3 = 0.0f, nh3_thresh = 30, nh3V = 0.0f;    //氨气浓度、氨气浓度阈值、ADC电压
u8 nh3_alarm = 0;	//氨气浓度是否报警

c_my_iic iic_amg8833;      //热成像amg8833 iic
c_amg8833 amg8833;      //热成像amg8833
float pixels[64]; //8*8温度矩阵
float ecg_value  = 0.0f;    //心电

u8 flag_infrared = 0;  //红外是否被触发
int flag_wind = 0;  //风速传感器电平触发次数
u8 wind_level = 0;	//风速级别
char wind_direction[2] = {"N"};	//风向
u8 speed = 0; //速度
u8 flag_lean = 0; //是否倾斜
u8 flag_fire = 0; //是否检测到火焰
int key = 0; //红外遥控按键
u8 flag_person = 0;   //是否检测到人
u8 flag_zhendong = 0; //是否检测到震动
int count = 0; //流量计转动圈数
u8 flag_menci = 0;    //门磁是否触发
u8 flag_rain = 0; //是否检测到雨水
u8 flag_water = 0; //是否检测到水
int breathe_num = 0; //呼吸次数
//键盘回调函数    
int key_board_call(const char* key_name, u8 state)    
{
	//红外触发
	if(strstr(key_name,"infrared")!=NULL && state == KEY_PRESS)
		flag_infrared = 1;
	//红外对管触发
	if(strstr(key_name,"hwdg")!=NULL && state == KEY_PRESS)
		speed++;
	//风速传感器触发
	if(strstr(key_name,"wind")!=NULL && state == KEY_PRESS)
		flag_wind++;
	//风向为北
	if(strstr(key_name,"north")!=NULL && state == KEY_PRESS)
		wind_direction[0] = 'N';

	//风向为南
	if(strstr(key_name,"south")!=NULL && state == KEY_PRESS)
		wind_direction[0] = 'S';

	//风向为西
	if(strstr(key_name,"west")!=NULL && state == KEY_PRESS)
		wind_direction[0] = 'W';

	//风向为东
	if(strstr(key_name,"east")!=NULL && state == KEY_PRESS)
		wind_direction[0] = 'E';

	//检测到倾斜
	if(strstr(key_name,"lean")!=NULL && state == KEY_PRESS)
		flag_lean = 1;
	//检测到未倾斜
	else if(strstr(key_name,"lean")!=NULL && state == KEY_UP)
		flag_lean = 0;

	//火焰触发
	if(strstr(key_name,"fire")!=NULL && state == KEY_PRESS)
		flag_fire = 1;
	//火焰未触发
	else if(strstr(key_name,"fire")!=NULL && state == KEY_UP)
		flag_fire = 0;

	//人体感应触发
	if(strstr(key_name,"renti")!=NULL && state == KEY_PRESS)
		flag_person = 1;
	//人体感应未触发
	else if(strstr(key_name,"renti")!=NULL && state == KEY_UP)
		flag_person = 0;

	//震动模块触发
	if(strstr(key_name,"zhendong")!=NULL && state == KEY_PRESS)
		flag_zhendong = 1;
	//震动模块未触发
	else if(strstr(key_name,"zhendong")!=NULL && state == KEY_UP)
		flag_zhendong = 0;

	//流量计触发
	if(strstr(key_name,"flow")!=NULL && state == KEY_PRESS)
		count++;
	//门磁模块触发
	if(strstr(key_name,"menci")!=NULL && state == KEY_PRESS)
		flag_menci = 1;
	//门磁模块未触发
	else if(strstr(key_name,"menci")!=NULL && state == KEY_UP)
		flag_menci = 0;

	//检测到雨水
	if(strstr(key_name,"rain")!=NULL && state == KEY_PRESS)
		flag_rain = 1;
	//检测到无水
	else if(strstr(key_name,"rain")!=NULL && state == KEY_UP)
		flag_rain = 0;

	//检测到水
	if(strstr(key_name,"water")!=NULL && state == KEY_PRESS)
		flag_water = 1;
	//检测到无水
	else if(strstr(key_name,"water")!=NULL && state == KEY_UP)
		flag_water = 0;

	//检测到呼吸
	if(strstr(key_name,"breathe")!=NULL && state == KEY_PRESS)
		breathe_num++;
	return E_OK;
}

c_matrix    matrix = {0}; //矩阵键盘对象
//	矩阵键盘回调函数
int matrix_board_callback(u8 row,u8 col)    
{
	log_inform("row: %d, col: %d", row, col);
	if(row == 0 && col == 0)
	{
	}
	else if(row == 0 && col == 1)
	{
	}
	else if(row == 0 && col == 2)
	{
	}
	else if(row == 0 && col == 3)
	{
	}
	else if(row == 1 && col == 0)
	{
	}
	else if(row == 1 && col == 1)
	{
	}
	else if(row == 1 && col == 2)
	{
	}
	else if(row == 1 && col == 3)
	{
	}
	else if(row == 2 && col == 0)
	{
	}
	else if(row == 2 && col == 1)
	{
	}
	else if(row == 2 && col == 2)
	{
	}
	else if(row == 2 && col == 3)
	{
	}
	else if(row == 3 && col == 0)
	{
	}
	else if(row == 3 && col == 1)
	{
	}
	else if(row == 3 && col == 2)
	{
	}
	else if(row == 3 && col == 3)
	{
	}
	return E_OK;
}

char  screen_dis_msg[20]= {"20206304     "};
//	语音识别串口回调函数
int voice_uart_recv_call(void* param, const u8* data, u16 data_len)    
{
	//命令1
	//如果用的自学习，就用data[0],如果是自己单独配置的固件，就用data[3]
	if (data[0] == 0x01)
	{
	}
	//命令2
	else if (data[0] == 0x02)
	{
	}
}
//	二维码串口回调函数
int qr_uart_recv_call(void* param, const u8* data, u16 data_len)    
{
	log_inform("%s",data);
}
//	k210串口回调函数
int k210_uart_recv_call(void* param, const u8* data, u16 data_len)    
{
	log_inform("%s",data);
	char* ptr = NULL;	
	//解析数据
	if(NULL != (ptr = strstr((char*)data,"class1")))
	{
	}
	if(NULL != (ptr = strstr((char*)data,"class2")))
	{
	}
}

int tds = 0, tds_thresh;//当前tds、 tds阈值
u8 tds_alarm = 0;	//tds是否报警

u8 tds_cmd[6] = {0xa0, 0x00, 0x00, 0x00, 0x00, 0xa0};    //  tds查询命令
//	tds串口回调函数
int tds_uart_recv_call(void* param, const u8* data, u16 data_len)
{
	if(data[0]==0xaa) 
	{
		tds = data[1] * 256 + data[2]; 
		log_inform("%s",data);
	}
}

float water_o2 = 0, water_o2_thresh = 3;//溶解氧、溶解氧阈值
u8 water_o2_alarm = 0;	//溶解氧是否报警

u8 water_o2_cmd[] = {0x01, 0x03, 0x00, 0x02, 0x00, 0x01, 0x25, 0xCA};    //  溶解氧查询命令
//	溶解氧串口回调函数
int water_o2_uart_recv_call(void* param, const u8* data, u16 data_len)
{
	log_inform("%s", data);
	for(int i = 0; i<data_len - 5; i++)
	{
		if(data[i]==0x01 && data[i + 1] == 0x03 && data[i + 2] == 0x02) 
		{
			water_o2 = ((u16)(data[3] << 8) + (data[i + 4])) / 100.0f;
		}
	}
}

float o2 = 0, o2_thresh = 3;//氧气、氧气阈值
u8 o2_alarm = 0;	//氧气是否报警

//	氧气串口回调函数
int o2_uart_recv_call(void* param, const u8* data, u16 data_len)
{
	log_inform("%s", data);
	if(data_len >= 6)
	{
		if(data[0] == 0xff && data[1] == 0x01 && data[2] == 0x07) 
		{
			o2 = (data[4] * 256 + data[5]) / 10.0;
		}
	}
}

int flag_person = 0;//是否有人

int heart = 0, heart_thresh = 95;	//心率、心率阈值

int breathe_frequency = 0, breathe_frequency_thresh = 30;	//呼吸频率、呼吸频率阈值
//	毫米雷达波串口回调函数
int radar_uart_recv_call(void* param, const u8* data, u16 data_len)
{
	log_inform("%s", data);
	//心率
	if (data[0] == 0x53 && data[1] == 0x59 && data[2] == 0x85 && data[3] == 0x02)
		heart = data[6];
	//呼吸
	else if (data[0] == 0x53 && data[1] == 0x59 && data[2] == 0x81 && data[3] == 0x02)
		breathe_frequency = data[6];
	//是否有人
	else if (data[0] == 0x53 && data[1] == 0x59 && data[2] == 0x80 && data[3] == 0x02)
		flag_person = data[6];
	return E_OK;
}

#define DATA_LEN 64	//	上传数据长度
#define TASK1_UPDATE_TIME_MS  500	//	任务1间隔时间
static void my_task1(void* param)
{
	//函数返回值
	u8 ret = 0;
	//小车底盘初始化
	chassis_init(PWA_TIMER, PWA_TIMER_CH, PWB_TIMER, PWB_TIMER_CH, AIN1_GPIO, AIN1_PIN, AIN2_GPIO, AIN2_PIN,
		BIN1_GPIO, BIN1_PIN, BIN2_GPIO, BIN2_PIN, STBY_GPIO, STBY_PIN);
	//x、y轴状态
	u8 x_status = 1, y_status = 1;

	//oled传感器iic初始化
	iic_oled = iic_creat(OLED_SDA_GRP, OLED_SDA_PIN, OLED_SCL_GRP, OLED_SCL_PIN);
	if(NULL == iic_oled.this)
	{
		log_error("iic creat failed.");
	}
	//oled显示屏初始化
	oled = oled_create(&iic_oled, OLED_TYPE_96);
	if(NULL == oled.this)
	{
		log_error("oled creat failed.");
	}
	oled.display_clear(&oled);

	//触摸屏初始化，引脚定义在lcd_init.c里面
	LCD_Init();
	LCD_Fill(0,0,LCD_W,LCD_H,WHITE);
	TP_Init();

	//TFT显示屏初始化
	tft18 = tft18_create(TFT_SPI_CHANNEL, TFT18_DIR_V_REVERSE, TFT_RES_GRP, TFT_RES_PIN, TFT_DC_GRP, TFT_DC_PIN, TFT_CS_GRP, TFT_CS_PIN);
	if(NULL == tft18.this)
	{
		log_error("tft18 creat failed.");
	}
	tft18.clear(&tft18);

	//dht11传感器初始化
	dht11 = dht11_create(DHT11_GRP, DHT11_PIN);
	if (NULL == dht11.this)
	{
		log_error("dht11 creat failed.");
	}

	//ds8b20传感器初始化
	ds18b20 = ds18b20_create(DS18B20_GRP, DS18B20_PIN);
	if(NULL == ds18b20.this)
	{
		log_error("ds18b20 creat failed.");
	}

	//初始化土壤湿度ADC
	my_adc.init(SOIL_ADC);

	//氧气模块初始化，该模块作为串口使用
	my_uart.init(O2_UART_ID, 9600, 64);
	//设置串口回调函数
	my_uart.set_callback(O2_UART_ID, NULL, o2_uart_recv_call);

	//su-03t模块初始化，该模块作为串口使用
	my_uart.init(SU03T_UART_ID, 115200, 64);
	//设置串口回调函数
	my_uart.set_callback(SU03T_UART_ID, NULL, voice_uart_recv_call);

	//短信模块ec800初始化
	ec800_msg = ec800_msg_creat(EC800_MSG_UART_ID);
	if(NULL == ec800_msg.this)
	{
		log_error("ec800_msg creat failed.");
	}
	strcpy(odata, "test message");

	//初始化甲烷ADC
	my_adc.init(MQ4_ADC);

	//sgp30传感器iic初始化
	iic_sgp30 = iic_creat(SGP30_SDA_GRP, SGP30_SDA_PIN, SGP30_SCL_GRP, SGP30_SCL_PIN);
	if(NULL == iic_sgp30.this)
	{
		log_error("iic creat failed.");
	}
	//sgp30传感器初始化
	sgp30 = sgp30_creat(&iic_sgp30);
	if(NULL == sgp30.this)
	{
		log_error("sgp30 creat failed.");
	}
	//启动sgp30传感器检测
	sgp30.start(&sgp30);

	//初始化co浓度ADC
	my_adc.init(MQ7_ADC);

	//初始化烟雾浓度ADC
	my_adc.init(MQ2_ADC);

	//bmp180初始化
	iic_bmp180 = iic_creat(BMP180_SDA_GRP, BMP180_SDA_PIN, BMP180_SCL_GRP, BMP180_SCL_PIN);
	if(NULL == iic_bmp180.this)
	{
		log_error("iic creat failed.");
	}
	//bmp180传感器初始化
	bmp180 = bmp180_creat(&iic_bmp180);
	if(NULL == bmp180.this)
	{
		log_error("bmp180 create failed.");
	}

	//mlx90614传感器iic初始化
	iic_mlx90614 = iic_creat(MLX90614_SDA_GRP, MLX90614_SDA_PIN, MLX90614_SCL_GRP, MLX90614_SCL_PIN);
	if(NULL == iic_mlx90614.this)
	{
		log_error("iic creat failed.");
	}
	//mlx90614传感器初始化
	mlx90614 = mlx90614_creat(&iic_mlx90614, MLX90614_DEFAULT_ADDR);
	if(NULL == mlx90614.this)
	{
		log_error("mlx90614 creat failed.");
	}

	//初始化NTC传感器ADC
	my_adc.init(NTC_ADC);

	//bh1750传感器iic初始化
	iic_bh1750 = iic_creat(BH1750_SDA_GRP, BH1750_SDA_PIN, BH1750_SCL_GRP, BH1750_SCL_PIN);
	if(NULL == iic_bh1750.this)
	{
		log_error("iic creat failed.");
	}
	//bh1750传感器初始化
	bh1750 = bh1750_creat(&iic_bh1750, BH1750_ADDR_H);
	if(NULL == bh1750.this)
	{
		log_error("bh1750 creat failed.");
	}
	//启动bh1750传感器检测
	bh1750.start(&bh1750, BH1750_MODE_H);

	//pm2.5传感器初始化
	gp2y = gp2y_create(GP2Y_CHANNLEL_GRP, GP2Y_TRIG_GRP, GP2Y_TRIG_PIN);
	if(NULL == gp2y.this)
	{
		log_error("gp2y creat failed.");
	}

	//初始化空气质量ADC
	my_adc.init(MQ135_ADC);

	//初始化酒精ADC
	my_adc.init(MQ3_ADC);

	//gps传感器初始化
	gps = gps_creat(GPS_UART_ID);
	if(NULL == gps.this)
	{
		log_error("gps creat failed.");
	}

	//mpu6050 iic初始化
	iic_mpu6050 = iic_creat(MPU6050_SDA_GRP, MPU6050_SDA_PIN, MPU6050_SCL_GRP, MPU6050_SCL_PIN);
	if(NULL == iic_mpu6050.this)
	{
		log_error("iic creat failed.");
	}
	//mpu6050传感器初始化
	ret = mpu6050.init(&iic_mpu6050);
	if(E_OK != ret)
	{
		log_error("mpu6050 init failed.");
	}

	//hx711传感器初始化
	hx711 = hx711_create(HX711_ACK_GRP, HX711_ACK_PIN, HX711_DIO_GRP, HX711_DIO_PIN);
	if(NULL == hx711.this)
	{
		log_error("hx711 creat failed.");
	}

	//hx711传感器校零
	hx711.zero(&hx711);

	//初始化气/血压ADC
	my_adc.init(PRESSURE_ADC);

	//心率、血氧传感器iic初始化
	iic_max30102 = iic_creat(MAX30102_SDA_GRP, MAX30102_SDA_PIN, MAX30102_SCL_GRP, MAX30102_SCL_PIN);
	if(NULL == iic_max30102.this)
	{
		log_error("iic creat failed.");
	}
	//max3012传感器初始化
	ret = max30102.init(&iic_max30102);
	if(E_OK != ret)
	{
		log_error("max30102 init failed.");
	}

	//初始化PH
	ph = ph_create(PH_ADC);
	if(NULL == ph.this)
	{
		log_error("ph creat failed.");
	}
	//TDS模块初始化，该模块作为串口使用
	my_uart.init(TDS_UART_ID, 9600, 64);
	//设置串口回调函数
	my_uart.set_callback(TDS_UART_ID, NULL, tds_uart_recv_call);

	//初始化浊度ADC
	my_adc.init(TURB_ADC);

	//初始化声音分贝ADC
	my_adc.init(VOICE_ADC);

	//手势识别PAJ7620U2传感器初始化
	//手势识别默认引脚未A6 A7，如需修改，则在paj7620u2.c文件的GS_i2c_init函数里修改
	ret = paj7620u2_init();
	if(0 == ret)
	{
		log_error("paj7620 init failed.");
	}

	//指纹模块初始化
	as608_init();

	//二维码模块初始化，该模块作为串口使用
	my_uart.init(QR_UART_ID, 9600, 64);
	//设置串口回调函数
	my_uart.set_callback(QR_UART_ID, NULL, qr_uart_recv_call);

	//RC522初始化
	rc522_cfg cfg = {RC522_RST_GRP, RC522_RST_PIN, RC522_MISO_GRP, RC522_MISO_PIN, RC522_MOSI_GRP, RC522_MOSI_PIN, RC522_SCK_GRP, RC522_SCK_PIN, RC522_SDA_GRP, RC522_SDA_PIN};
	ret = rc522.init(&cfg);
	if(E_OK != ret)
	{
		log_error("rc522 init failed.");
	}
	rc522.set_callback(NULL, rfid_callback);

	//功率计INA219传感器iic初始化
	iic_ina219 = iic_creat(INA219_SDA_GRP, INA219_SDA_PIN, INA219_SCL_GRP, INA219_SCL_PIN);
	if(NULL == iic_ina219.this)
	{
		log_error("iic creat failed.");
	}
	//功率计ina219传感器初始化
	ina219 = ina219_creat(&iic_ina219);
	if(NULL == ina219.this)
	{
		log_error("ina219 creat failed.");
	}

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

	//矩阵键盘初始化
	matrix_io_t io_list[8]={MATRIX_1_GRP, MATRIX_1_PIN, MATRIX_2_GRP, MATRIX_2_PIN, MATRIX_3_GRP, MATRIX_3_PIN, MATRIX_4_GRP, MATRIX_4_PIN, MATRIX_5_GRP, MATRIX_5_PIN, MATRIX_6_GRP, MATRIX_6_PIN, MATRIX_7_GRP, MATRIX_7_PIN, MATRIX_8_GRP, MATRIX_8_PIN, };
	matrix = matrix_keyboard_create(io_list);
	if(NULL == matrix.this)
		log_error("matrix create failed.");
	else
		matrix.set_callback(&matrix,  matrix_board_callback);

	//WS2812初始化
	ws2812 = ws2812_create(WS2812_GRP, WS2812_PIN, 1);
	if(NULL == ws2812.this)
	{
		log_error("ws2812 creat failed.");
	}
	//设置RGB
	//ws2812.write_24Bits(&ws2812, 255, 255, 255);

	//K210模块初始化，该模块作为串口使用
	my_uart.init(K210_UART_ID, 115200, 64);
	//设置串口回调函数
	my_uart.set_callback(K210_UART_ID, NULL, k210_uart_recv_call);

	//led初始化
	led = switch_create(LED_GRP, LED_PIN);
	if(NULL == led.this)
	{
		log_error("led creat failed.");
	}
	//关闭led
	led.set(&led, SWITCH_LOW);

	//语音播报模块jq8400初始化
	jq8400 = jq8400_creat(JQ8400_UART_ID);
	if(NULL == jq8400.this)
	{
		log_error("jq8400 creat failed.");
	}
	//设置语音模块音量为30，音量可设置范围为0-30
	jq8400.volume(&jq8400, 30);
	//播放欢迎音频(播放音频前一般需要判断一下状态)，若不需要播放，则删除这几行代码
	jq8400.state(&jq8400, &jq8400_state);
	if(jq8400_state == JQ8400_IDLE)
		jq8400.play(&jq8400, false, 1);

	//舵机初始化
	sg90 = servo_create(SG90_TIMER, SG90_CHANNEL);
	if(NULL == sg90.this)
	{
		log_error("sg90 creat failed.");
	}
	//舵机初始化为0度角
	sg90.set(&sg90, 0.0f);

	//继电器初始化
	relay = switch_create(RELAY_GRP, RELAY_PIN);
	if(NULL == relay.this)
	{
		log_error("relay creat failed.");
	}
	//关闭继电器
	relay.set(&relay, SWITCH_LOW);

	//风扇初始化
	fan = switch_create(FAN_GRP, FAN_PIN);
	if(NULL == fan.this)
	{
		log_error("fan creat failed.");
	}
	//关闭风扇
	fan.set(&fan, SWITCH_LOW);

	//水泵初始化
	pump = switch_create(PUMP_GRP, PUMP_PIN);
	if(NULL == pump.this)
	{
		log_error("pump creat failed.");
	}
	//关闭水泵
	pump.set(&pump, SWITCH_LOW);

	//加热片初始化
	jrp = switch_create(JRP_GRP, JRP_PIN);
	if(NULL == jrp.this)
	{
		log_error("jrp creat failed.");
	}
	//关闭加热片
	jrp.set(&jrp, SWITCH_LOW);

	//雾化片初始化
	whp = switch_create(WHP_GRP, WHP_PIN);
	if(NULL == whp.this)
	{
		log_error("whp creat failed.");
	}
	//关闭雾化片
	whp.set(&whp, SWITCH_LOW);

	//步进电机初始化
	uln2003 = uln2003_creat(TYPE_1_PHASE, ULN_A_GRP, ULN_A_PIN, ULN_B_GRP, ULN_B_PIN, ULN_C_GRP, ULN_C_PIN, ULN_D_GRP, ULN_D_PIN);
	if(NULL == uln2003.this)
	{
		log_error("uln2003 create failed.");
	}

	//马达初始化
	motor = switch_create(MOTOR_GRP, MOTOR_PIN);
	if(NULL == motor.this)
	{
		log_error("motor creat failed.");
	}
	//关闭马达
	motor.set(&motor, SWITCH_LOW);

	//颜色识别TCS34725
	iic_tcs34725 = iic_creat(TCS34725_SDA_GRP, TCS34725_SDA_PIN, TCS34725_SCL_GRP, TCS34725_SCL_PIN);
	if(NULL == iic_tcs34725.this)
	{
		log_error("iic creat failed.");
	}
	//tcs34725传感器初始化
	tcs34725 = tcs34725_creat(&iic_tcs34725);
	if(NULL == tcs34725.this)
	{
		log_error("tcs34725 creat failed.");
	}
	//启动tcs34725传感器检测
	tcs34725.start(&tcs34725, TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X);

	//蜂鸣器初始化
	buzzer = switch_create(BUZZER_GRP, BUZZER_PIN);
	if(NULL == buzzer.this)
	{
		log_error("buzzer creat failed.");
	}
	//关闭蜂鸣器
	buzzer.set(&buzzer, SWITCH_LOW);

	//溶解氧模块初始化，该模块作为串口使用
	my_uart.init(WATER_O2_UART_ID, 9600, 64);
	//设置串口回调函数
	my_uart.set_callback(WATER_O2_UART_ID, NULL, water_o2_uart_recv_call);

	//初始化光敏ADC
	my_adc.init(GM_ADC);

	//点阵屏初始化，该模块作为串口使用
	my_uart.init(DZ_SCREEN_UART_ID, 9600, 64);

	//超声传感器iic初始化
	c_my_iic iic_hcsr04 = iic_creat(HCSR04_SDA_GRP, HCSR04_SDA_PIN, HCSR04_SCL_GRP, HCSR04_SCK_PIN);
	if(NULL == iic_hcsr04.this)
	{
		log_error("iic creat failed.");
	}

	//超声传感器初始化
	hc_sr04 = hc_sr04_creat(&iic_hcsr04);
	if(NULL == hc_sr04.this)
	{
		log_error("hc_sr04 creat failed.");
	}

	//初始化可燃气体浓度ADC
	my_adc.init(MQ9_ADC);

	history_records.write_flag = 0;
		// 从 Flash 读取历史记录
	STMFLASH_Read(FLASH_START_ADDRESS, (u16*)&history_records, sizeof(history_records)/2);	

	//红外遥控初始化
	remote_init(REMOTE_GRP, REMOTE_PIN);

	//毫米雷达波模块初始化，该模块作为串口使用
	my_uart.init(Radar_UART_ID, 115200, 64);
	//设置串口回调函数
	my_uart.set_callback(Radar_UART_ID, NULL, radar_uart_recv_call);

	//红绿灯1初始化
	r_led1 = switch_create(RED_LED_GRP1, RED_LED_PIN1);
	if(NULL == r_led1.this)
	{
		log_error("r_led creat failed.");
	}
	//关闭r_led1
	r_led1.set(&r_led1, SWITCH_LOW);
	g_led1 = switch_create(GREEN_LED_GRP1, GREEN_LED_PIN1);
	if(NULL == g_led1.this)
	{
		log_error("g_led creat failed.");
	}
	//关闭g_led1
	g_led1.set(&g_led1, SWITCH_LOW);
	y_led1 = switch_create(YELLOW_LED_GRP1, YELLOW_LED_PIN1);
	if(NULL == y_led1.this)
	{
		log_error("y_led creat failed.");
	}
	//关闭y_led1
	y_led1.set(&y_led1, SWITCH_LOW);

	//红绿灯2初始化
	r_led2 = switch_create(RED_LED_GRP2, RED_LED_PIN2);
	if(NULL == r_led2.this)
	{
		log_error("r_led creat failed.");
	}
	//关闭r_led2
	r_led2.set(&r_led2, SWITCH_LOW);
	g_led2 = switch_create(GREEN_LED_GRP2, GREEN_LED_PIN2);
	if(NULL == g_led2.this)
	{
		log_error("g_led creat failed.");
	}
	//关闭g_led2
	g_led2.set(&g_led2, SWITCH_LOW);
	y_led2 = switch_create(YELLOW_LED_GRP2, YELLOW_LED_PIN2);
	if(NULL == y_led2.this)
	{
		log_error("y_led creat failed.");
	}
	//关闭y_led2
	y_led2.set(&y_led2, SWITCH_LOW);

	//初始化氨气浓度ADC
	my_adc.init(MQ137_ADC);

	//热成像AMG8833 iic初始化
	iic_amg8833 = iic_creat(AMG8833_SDA_GRP, AMG8833_SDA_PIN, AMG8833_SCL_GRP, AMG8833_SCL_PIN);
	if(NULL == iic_amg8833.this)
	{
		log_error("iic creat failed.");
	}
	//热成像AMG8833传感器初始化
	amg8833 = amg8833_creat(&iic_amg8833);
	if(NULL == amg8833.this)
	{
		log_error("amg8833 creat failed.");
	}

	//初始化心电ADC
	my_adc.init(AD8232_ADC);

	/*键盘对象初始化*/
	key_list board_list[] = {
		//红外传感器
		{"infrared", INFRARED_GRP, INFRARED_PIN, KEY_PRESS_IS_ZERO  },
		//风速传感器
		{"wind", WIND_GRP, WIND_PIN, KEY_PRESS_IS_ZERO  },
		//风向传感器北
		{"north", NORTH_GRP, NORTH_PIN, KEY_PRESS_IS_ZERO  },
		//风向传感器南
		{"south", SOUTH_GRP, SOUTH_PIN, KEY_PRESS_IS_ZERO  },
		//风向传感器西
		{"west", WEST_GRP, WEST_PIN, KEY_PRESS_IS_ZERO  },
		//风向传感器东
		{"east", EAST_GRP, EAST_PIN, KEY_PRESS_IS_ZERO  },
		//红外对管传感器
		{"hwdg", HWDG_GRP, HWDG_PIN, KEY_PRESS_IS_ZERO  },
		//倾斜传感器
		{"lean", LEAN_GRP, LEAN_PIN, KEY_PRESS_IS_ZERO  },
		//火焰传感器
		{"fire", FIRE_GRP, FIRE_PIN, KEY_PRESS_IS_ZERO  },
		//人体感应传感器
		{"renti", RENTI_GRP, RENTI_PIN, KEY_PRESS_IS_ZERO  },
		//震动传感器
		{"zhendong", ZHENDONG_GRP, ZHENDONG_PIN, KEY_PRESS_IS_ZERO  },
		//流量计
		{"flow", FLOW_GRP, FLOW_PIN, KEY_PRESS_IS_ZERO  },
		//门磁传感器
		{"menci", MENCI_GRP, MENCI_PIN, KEY_PRESS_IS_ZERO  },
		//呼吸
		{"breathe", BREATHE_GRP, BREATHE_PIN, KEY_PRESS_IS_ZERO  },
		//雨水
		{"rain", RAIN_GRP, RAIN_PIN, KEY_PRESS_IS_ZERO  },
		//液位
		{"water", WATER_GRP, WATER_PIN, KEY_PRESS_IS_ZERO  },
	};
	/*创建键盘对象*/
	board = key_board_create(board_list,sizeof(board_list)/sizeof(key_list));
	if(NULL == board.this)
	{
		log_error("Board create failed.");
	}

	/*设置键盘回调函数*/
	ret = board.set_callback(&board,key_board_call);
	if(E_OK != ret)
	{
		log_error("Set callback failed.");
	}

	while(1)
	{
		tp_dev.scan(0);
		if(tp_dev.sta&TP_PRES_DOWN)//消抖
		{
			vTaskDelay(1);//延时消抖	
			log_inform("x: %d, y:%d", tp_dev.x[0], tp_dev.y[0]); 
		}
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

		tft18.str(&tft18, 0, 0, 12, "temp:%.1f %d  ", temp, temp_thresh);
		tft18.str(&tft18, 0, 20, 12, "humi:%.1f %d  ", humi, humi_thresh);

		oled.display_str(&oled, 0, 2, "temp:%.1f %d  ", temp, temp_thresh);
		oled.display_str(&oled, 0, 3, "humi:%.1f %d  ", humi, humi_thresh);

		//获取温度
		ret = ds18b20.get(&ds18b20, &ds18b20_temp);
		if(E_OK != ret)
		{
			log_error("ds18b20 get failed.");
		}
		if(ds18b20_temp > ds18b20_temp_thresh)
			ds18b20_temp_alarm = 1;
		else
			ds18b20_temp_alarm = 0;

		tft18.str(&tft18, 0, 80, 12, "temp:%.1f %d  ", ds18b20_temp, ds18b20_temp_thresh);

		oled.display_str(&oled, 0, 5, "temp:%.1f %d  ", ds18b20_temp, ds18b20_temp_thresh);

		//获取当前土壤湿度
		my_adc.get(SOIL_ADC, &soil_humiV);
		soil_humi = 100 - soil_humiV * 30;
		if(soil_humi > soil_humi_thresh)
			soil_humi_alarm = 1;
		else
			soil_humi_alarm = 0;

		tft18.str(&tft18, 0, 120, 12, "soil:%.1f %.1f  ", soil_humi, soil_humi_thresh);

		oled.display_str(&oled, 0, 7, "soil:%.1f %.1f  ", soil_humi, soil_humi_thresh);

		if(o2 < o2_thresh)
			o2_alarm = 1;
		else
			o2_alarm = 0;

		tft18.str(&tft18, 0, 160, 12, "o2:%d %d ", o2, o2_thresh);

		oled.display_str(&oled, 0, 9, "o2:%d %d ", o2, o2_thresh);


		//发送短信
		//ec800_msg.message(&ec800_msg, tel , odata);	
		//获取当前甲烷浓度
		my_adc.get(MQ4_ADC, &ch4V);
		ch4 = (100 / 3.3) * ch4V;
		if(ch4 > ch4_thresh)
			ch4_alarm = 1;
		else
			ch4_alarm = 0;

		tft18.str(&tft18, 0, 200, 12, "mq4:%.1f %.1f  ", ch4, ch4_thresh);

		oled.display_str(&oled, 0, 11, "mq4:%.1f %.1f  ", ch4, ch4_thresh);

		//获取CO2、VOC
		ret = sgp30.get(&sgp30, HUMIDITY_UN, &co2, &voc);
		if(E_OK != ret)
		{
			log_error("sgp30 get failed.");
		}
		if(co2 > co2_thresh)
			co2_alarm = 1;
		else
			co2_alarm = 0;

		if(voc > voc_thresh)
			voc_alarm = 1;
		else
			voc_alarm = 0;

		tft18.str(&tft18, 0, 240, 12, "co2:%d %d  ", co2, co2_thresh);
		tft18.str(&tft18, 0, 260, 12, "voc:%d %d  ", voc, voc_thresh);

		oled.display_str(&oled, 0, 14, "co2:%d %d  ", co2, co2_thresh);
		oled.display_str(&oled, 0, 15, "voc:%d %d  ", voc, voc_thresh);

		//获取当前co浓度
		my_adc.get(MQ7_ADC, &coV);
		co = pow(11.5428 * 7.04 * coV / (3.3 - coV), (1 / 0.6549));
		if(co > co_thresh)
			co_alarm = 1;
		else
			co_alarm = 0;

		tft18.str(&tft18, 0, 320, 12, "co:%.1f %.1f  ", co, co_thresh);

		oled.display_str(&oled, 0, 17, "co:%.1f %.1f  ", co, co_thresh);

		//获取当前烟雾浓度
		my_adc.get(MQ2_ADC, &smokeV);
		smoke = smokeV / 3.3f * 100.0f;
		if(smoke > smoke_thresh)
			smoke_alarm = 1;
		else
			smoke_alarm = 0;

		tft18.str(&tft18, 0, 360, 12, "smoke:%.1f %.1f  ", smoke, smoke_thresh);

		oled.display_str(&oled, 0, 19, "smoke:%.1f %.1f  ", smoke, smoke_thresh);

		tft18.str(&tft18, 0, 400, 12, "fire:%d  ", flag_fire);

		oled.display_str(&oled, 0, 21, "fire:%d  ", flag_fire);

		//获取大气压、海拔数据
		ret = bmp180.get(&bmp180, BMP180_P_STANDARD, &t, &alt, &pa);
		if(E_OK != ret)
		{
			log_error("bmp180 get failed.");
		}

		tft18.str(&tft18, 0, 440, 12, "atmos:%d  ", (int)pa);
		tft18.str(&tft18, 0, 460, 12, "alt:%.1f  ", (int)alt);
		oled.display_str(&oled, 0, 24, "atmos:%d  ", (int)pa);

		oled.display_str(&oled, 0, 25, "alt:%.1f  ", (int)alt);

		//获取体温
		ret = mlx90614.get(&mlx90614, &env, &mlx90614_temp);
		if(E_OK != ret)
		{
			log_error("mlx90614 get failed.");
		}
		if(mlx90614_temp > mlx90614_temp_thresh)
			mlx90614_temp_alarm = 1;
		else
			mlx90614_temp_alarm = 0;

		tft18.str(&tft18, 0, 520, 12, "temp:%.1f %.1f  ", mlx90614_temp, mlx90614_temp_thresh);

		oled.display_str(&oled, 0, 27, "temp:%d %d  ", mlx90614_temp, mlx90614_temp_thresh);

		//获取当前ntc传感器温度
		my_adc.get(NTC_ADC, &ntcV);
		ntcR = (3.3f - ntcV) / 3.3f * 100.0f;
		ntc_temp = get_temp(ntcR);
		if(ntc_temp > ntc_temp_thresh)
			ntc_alarm = 1;
		else
			ntc_alarm = 0;

		tft18.str(&tft18, 0, 560, 12, "temp:%.1f %.1f  ", ntc_temp, ntc_temp_thresh);

		oled.display_str(&oled, 0, 29, "temp:%.1f %.1f  ", ntc_temp, ntc_temp_thresh);

		//获取光照强度
		ret = bh1750.get(&bh1750, &lux);
		if(E_OK != ret)
		{
			log_error("bh1750 get failed.");
		}
		if(lux > lux_thresh)
			lux_alarm = 1;
		else
			lux_alarm = 0;

		tft18.str(&tft18, 0, 600, 12, "light:%d %d  ", lux, lux_thresh);

		oled.display_str(&oled, 0, 31, "light:%d %d  ", lux, lux_thresh);

		//获取pm2.5
		ret = gp2y.get(&gp2y, &pm2_5);
		if(E_OK != ret)
		{
			log_error("gp2y get failed.");
		}
		if(pm2_5 > pm2_5_thresh)
			pm2_5_alarm = 1;
		else
			pm2_5_alarm = 0;

		tft18.str(&tft18, 0, 640, 12, "pm25:%d %d  ", (int)pm2_5, (int)pm2_5_thresh);

		oled.display_str(&oled, 0, 33, "pm25:%d %d  ", (int)pm2_5, (int)pm2_5_thresh);

		//获取当前空气质量
		my_adc.get(MQ135_ADC, &airV);
		air_quality = pow(11.5428 * 7.04 * airV / (3.3 - airV), (1 / 0.6549));
		if(air_quality > air_quality_thresh)
			air_quality_alarm = 1;
		else
			air_quality_alarm = 0;

		tft18.str(&tft18, 0, 680, 12, "air:%.1f %.1f  ", air_quality, air_quality_thresh);

		oled.display_str(&oled, 0, 35, "air:%.1f %.1f  ", air_quality, air_quality_thresh);

		//获取当前酒精浓度
		my_adc.get(MQ3_ADC, &alcoholV);
		alcohol = (100 / 3.3) * alcoholV;
		if(alcohol > alcohol_thresh)
			alcohol_alarm = 1;
		else
			alcohol_alarm = 0;

		tft18.str(&tft18, 0, 720, 12, "alc:%.1f %.1f  ", alcohol, alcohol_thresh);

		oled.display_str(&oled, 0, 37, "alc:%.1f %.1f  ", alcohol, alcohol_thresh);

		//计算风速
		if (0 == flag_wind)
			wind_level = 0;
		else if (flag_wind / (TASK1_UPDATE_TIME_MS / 1000.0) < 3)
			wind_level = 1;
		else if (flag_wind / (TASK1_UPDATE_TIME_MS / 1000.0) < 6)
			wind_level = 2;
		else
			wind_level = 3;
		flag_wind = 0;
		tft18.str(&tft18, 0, 760, 12, "wind:%d  ", wind_level);

		oled.display_str(&oled, 0, 39, "wind:%d  ", wind_level);

		//获取经纬度
		ret = gps.get(&gps, &gps_data);
		if(E_OK != ret)
		{
			log_error("gps get failed.");
		}

		tft18.str(&tft18, 0, 800, 12, "long:%.3f  ", gps_data.o_gngga.o_longitude);
		tft18.str(&tft18, 0, 820, 12, "lat:%.3f  ", gps_data.o_gngga.o_latitude);

		oled.display_str(&oled, 0, 42, "long:%.3f  ", gps_data.o_gngga.o_longitude);
		oled.display_str(&oled, 0, 43, "lat:%.3f  ", gps_data.o_gngga.o_latitude);

		//获取陀螺仪数据
		//dmp_get_pedometer_step_count(&step);
		ret = mpu6050.get(&x, &y, &z);
		if(E_OK != ret)
		{
			log_error("mpu6050 get failed.");
		}

		//判断是否摔倒
		x_status = x < 30 && x > -30;
		y_status = y < 30 && y > -30;
		if(x_status && y_status)
			fall_down = 0;
		else
			fall_down = 1;
		tft18.str(&tft18, 0, 880, 12, "x:%d  ", (int)x);
		tft18.str(&tft18, 0, 900, 12, "y:%d  ", (int)y);
		tft18.str(&tft18, 0, 920, 12, "z:%d  ", (int)z);
		oled.display_str(&oled, 0, 47, "x:%d  ", (int)x);

		oled.display_str(&oled, 0, 48, "y:%d  ", (int)y);

		oled.display_str(&oled, 0, 49, "z:%d  ", (int)z);

		//获取当前重量
		ret = hx711.get(&hx711, &weight);
		if(E_OK != ret)
		{
			log_error("hx711 get failed.");
		}
		if(weight > weight_thresh)
			weight_alarm = 1;
		else
			weight_alarm = 0;

		tft18.str(&tft18, 0, 1000, 12, "weight:%.1f %.1f  ", weight, weight_thresh);

		oled.display_str(&oled, 0, 51, "weight:%.1f %.1f  ", weight, weight_thresh);

		//获取气/血压
		my_adc.get(PRESSURE_ADC, &pressureV);
		pressure = 48.471 * pressureV + 4.4168;
		if(pressure > pressure_thresh)
			pressure_alarm = 1;
		else
			pressure_alarm = 0;

		tft18.str(&tft18, 0, 1040, 12, "pre:%.1f %.1f  ", pressure, pressure_thresh);

		oled.display_str(&oled, 0, 53, "pre:%.1f %.1f  ", pressure, pressure_thresh);

		//获取心率、血氧
		ret = max30102.get(&heart, &blood_o2);
		if(E_OK != ret)
		{
			log_error("max30102 get failed.");
		}
		if(heart > heart_thresh)
			heart_alarm = 1;
		else
			heart_alarm = 0;

		if(blood_o2 > blood_o2_thresh)
			blood_o2_alarm = 1;
		else
			blood_o2_alarm = 0;

		tft18.str(&tft18, 0, 1080, 12, "heart:%d %d  ", heart, heart_thresh);
		tft18.str(&tft18, 0, 1100, 12, "o2:%d %d  ", blood_o2, blood_o2_thresh);

		oled.display_str(&oled, 0, 56, "heart:%d %d  ", heart, heart_thresh);
		oled.display_str(&oled, 0, 57, "o2:%d %d  ", blood_o2, blood_o2_thresh);

		//获取当前ph
		ret = ph.get(&ph, &ph_value);
		if(E_OK != ret)
		{
			log_error("ph get failed.");
		}
		if(ph_value > ph_thresh)
			ph_alarm = 1;
		else
			ph_alarm = 0;

		tft18.str(&tft18, 0, 1160, 12, "ph:%.1f %.1f  ", ph_value, ph_thresh);

		oled.display_str(&oled, 0, 59, "ph:%.1f %.1f  ", ph_value, ph_thresh);


		//发送tds查询指令
		my_uart.send(TDS_UART_ID, tds_cmd, 6);	
		if(tds > tds_thresh)
			tds_alarm = 1;
		else
			tds_alarm = 0;

		tft18.str(&tft18, 0, 1200, 12, "tds:%.1f %.1f  ", tds, tds_thresh);

		oled.display_str(&oled, 0, 61, "tds:%.1f %.1f  ", tds, tds_thresh);

		//获取当前浊度
		my_adc.get(TURB_ADC, &turbidityV);
		turbidity = -865.68* (-0.0192 * (0.0 / 10 - 25) + turbidityV) + 3047.19;
		if(turbidity > turbidity_thresh)
			turbidity_alarm = 1;
		else
			turbidity_alarm = 0;

		tft18.str(&tft18, 0, 1240, 12, "tur:%.1f %.1f  ", turbidity, turbidity_thresh);

		oled.display_str(&oled, 0, 63, "tur:%.1f %.1f  ", turbidity, turbidity_thresh);

		//获取当前分贝
		my_adc.get(VOICE_ADC, &voiceV);
		voice = fabs(voiceV - 1.65) * 41.328;
		if(voice > voice_thresh)
			voice_alarm = 1;
		else
			voice_alarm = 0;

		tft18.str(&tft18, 0, 1280, 12, "voice:%.1f %.1f  ", voice, voice_thresh);

		oled.display_str(&oled, 0, 65, "voice:%.1f %.1f  ", voice, voice_thresh);


		//读取手势状态
		gesture_status = GS_Read_nByte(PAJ_GET_INT_FLAG1,2,&data[0]);
		if(!gesture_status)
		{
			gesture_data =(u16)data[1]<<8 | data[0];
			if(gesture_data)
			{
				switch(gesture_data)
				{
					case GES_UP://向上
					break;

					case GES_DOWM://向下
					break;

					case GES_LEFT://向左
					break;

					case GES_RIGHT://向右
					break;

					case GES_FORWARD://向前
					break;

					case GES_BACKWARD://向后
					break;

					case GES_CLOCKWISE://顺时针
					break;

					case GES_COUNT_CLOCKWISE://逆时针
					break;

					case GES_WAVE://挥动
					break;
				}
			}
		}
		//常用接口：若不用，请删除对应代码，并删除多余注释
		//添加指纹
		//u8 ret = Add_FR(g_fr_num);  //若成功返回1 否则返回0
		////删除指纹
		//Del_FR();
		//刷指纹
		//press_FR(); //若校验成功返回0  若检测到是非法指纹返回-3 否则返回其他（-1或-2）

		//获取电流、电压、功率
		ret = ina219.get(&ina219, &mv, &ma, &mw);
		if(E_OK != ret)
		{
			log_error("ina219 get failed.");
		}
		if(ma > ma_thresh)
			ma_alarm = 1;
		else
			ma_alarm = 0;

		tft18.str(&tft18, 0, 1320, 12, "I:%d  ", ma, ma_thresh);

		oled.display_str(&oled, 0, 67, "I:%d %d  ", ma, ma_thresh);

		//获取当前时间
		ret = ds3231.get(&ds3231, &time_now);
		if(E_OK != ret)
		{
			log_error("ds3231 get failed.");
		}

		tft18.str(&tft18, 0, 1360, 12, "%02d:%02d:%02d ", time_now.o_house, time_now.o_min, time_now.o_sec);

		oled.display_str(&oled, 0, 69, "%02d:%02d:%02d ", time_now.o_house, time_now.o_min, time_now.o_sec);


		//步进电机正转一圈
		//uln2003.rotate(&uln2003, 360.0f, 7.3242f);
		//步进电机正转一圈
		//uln2003.rotate(&uln2003, 360.0f, -7.3242f);
		//步进电机停止
		//uln2003.stop(&uln2003);
		//获取颜色
		ret = tcs34725.get_rgbc(&tcs34725, &r, &g, &b, &c);
		if(E_OK != ret)
		{
			log_error("tcs34725 get failed.");
		}
		tft18.str(&tft18, 0, 1400, 12, "dir:%s ", wind_direction);

		oled.display_str(&oled, 0, 71, "dir:%s ", wind_direction);


		//发送溶解氧查询指令
		my_uart.send(WATER_O2_UART_ID, water_o2_cmd, 8);	
		if(water_o2 < water_o2_thresh)
			water_o2_alarm = 1;
		else
			water_o2_alarm = 0;

		tft18.str(&tft18, 0, 1440, 12, "w_o2:%d %d ", water_o2, water_o2_thresh);

		oled.display_str(&oled, 0, 73, "w_o2:%d %d ", water_o2, water_o2_thresh);

		//获取当前光强
		my_adc.get(GM_ADC, &light);
		if(light > light_thresh)
			light_alarm = 1;
		else
			light_alarm = 0;

		tft18.str(&tft18, 0, 1480, 12, "light:%.1f %.1f  ", light, light_thresh);

		oled.display_str(&oled, 0, 75, "light:%.1f %.1f  ", light, light_thresh);


		//给点阵屏发送显示指令
		my_uart.send(DZ_SCREEN_UART_ID, (const u8*)screen_dis_msg, strlen(screen_dis_msg));	

		//获取距离
		ret = hc_sr04.get(&hc_sr04, &dis);
		if(E_OK != ret)
		{
			log_error("hc_sr04 get failed.");
		}
		if(dis < dis_thresh)
			dis_alarm = 1;
		else
			dis_alarm = 0;

		tft18.str(&tft18, 0, 1520, 12, "dis:%.1f %d  ", dis, (int)dis_thresh);

		oled.display_str(&oled, 0, 77, "dis:%.1f %d  ", dis, (int)dis_thresh);

		//获取当前可燃气体浓度
		my_adc.get(MQ9_ADC, &com_gasV);
		com_gas = com_gasV / 3.3f * 100.0f;
		if(com_gas > com_gas_thresh)
			com_gas_alarm = 1;
		else
			com_gas_alarm = 0;

		tft18.str(&tft18, 0, 1560, 12, "gas:%.1f %.1f  ", com_gas, com_gas_thresh);

		oled.display_str(&oled, 0, 79, "com_gas:%.1f %.1f  ", com_gas, com_gas_thresh);

		//写入数据到flash
		//history_records.write_flag = 0x1F1F;
		STMFLASH_Write(FLASH_START_ADDRESS,(u16*)&history_records,sizeof(history_records)/2);	
		//扫描红外遥控按键
		key = remote_scan();
		//if (69 == key)
		//{
		//}
		//获取当前氨气浓度
		my_adc.get(MQ137_ADC, &nh3V);
		nh3 = (100 / 3.3f) * nh3V;
		if(nh3 > nh3_thresh)
			nh3_alarm = 1;
		else
			nh3_alarm = 0;

		tft18.str(&tft18, 0, 1600, 12, "nh3:%.1f %.1f  ", nh3, nh3_thresh);

		oled.display_str(&oled, 0, 81, "nh3:%.1f %.1f  ", nh3, nh3_thresh);

		//获取热成像温度矩阵
		ret = amg8833.get(&amg8833, pixels);
		if(E_OK == ret)
		{
			//打印数据
		}
		//获取当前心电数据
		my_adc.get(AD8232_ADC, &ecg_value );

		tft18.str(&tft18, 0, 1640, 12, "ecg:%.1f  ", ecg_value);

		oled.display_str(&oled, 0, 83, "ecg:%.1f  ", ecg_value);

		//前进
		//chassis_work(CHASSIS_GO, speed);
		//后退
		//chassis_work(CHASSIS_BACK, speed);
		//左转
		//chassis_work(CHASSIS_LEFT, speed);
		//右转
		//chassis_work(CHASSIS_RIGHT, speed);
		//停止
		//chassis_work(CHASSIS_STOP, speed);
		//报警
		if (0)
			buzzer.set(&buzzer, SWITCH_LOW);

		vTaskDelay(TASK1_UPDATE_TIME_MS);
	}

}


//	蓝牙分包发送数据
void ProBTSend(char * pcData, uint32_t uxLen)    
{
	int i = 0, len = 0;
	for (; i < uxLen; i += len)
	{
		if (i == 0)
			len = i + 11 < uxLen ? 11 : uxLen - i;
		else
			len = i + 18 < uxLen ? 18 : uxLen - i;
		my_uart.send(BT_UART_ID, (const u8*)pcData + i, len);
	}
}

//	从蓝牙串口获取的数据
char g_bt_recv_buf[256] = {0};
int g_bt_recv_flag = 0;
//	蓝牙串口回调函数
int bt_uart_recv_call(void* param,const u8* data,u16 data_len)    
{
	if (g_bt_recv_flag == 0)
	{
		memset(g_bt_recv_buf, 0, 256);
		memcpy(g_bt_recv_buf, data, data_len);
		g_bt_recv_flag = 1;
	}
	else
	{
		memcpy(g_bt_recv_buf + strlen(g_bt_recv_buf), data, data_len);
	}
}
//	从esp32cam串口获取的数据
char g_esp_recv_buf[256] = {0};
int g_esp_recv_flag = 0;
//	esp32cam串口回调函数
int esp_uart_recv_call(void* param,const u8* data,u16 data_len)    
{
	if (g_esp_recv_flag == 0)
	{
		memset(g_esp_recv_buf, 0, 256);
		memcpy(g_esp_recv_buf, data, data_len);
		g_esp_recv_flag = 1;
	}
	else
	{
		memcpy(g_esp_recv_buf + strlen(g_esp_recv_buf), data, data_len);
	}
}
//	从zigbee串口获取的数据
char g_zigbee_recv_buf[256] = {0};
int g_zigbee_recv_flag = 0;
//	zigbee串口回调函数
int zigbee_uart_recv_call(void* param,const u8* data,u16 data_len)    
{
	if (g_zigbee_recv_flag == 0)
	{
		memset(g_zigbee_recv_buf, 0, 256);
		memcpy(g_zigbee_recv_buf, data, data_len);
		g_zigbee_recv_flag = 1;
	}
	else
	{
		memcpy(g_zigbee_recv_buf + strlen(g_zigbee_recv_buf), data, data_len);
	}
}
//	从TTL串口获取的数据
char g_ttl_recv_buf[256] = {0};
int g_ttl_recv_flag = 0;
//	ttl串口回调函数
int ttl_uart_recv_call(void* param,const u8* data,u16 data_len)    
{
	if (g_ttl_recv_flag == 0)
	{
		memset(g_ttl_recv_buf, 0, 256);
		memcpy(g_ttl_recv_buf, data, data_len);
		g_ttl_recv_flag = 1;
	}
	else
	{
		memcpy(g_ttl_recv_buf + strlen(g_ttl_recv_buf), data, data_len);
	}
}
//	从lora串口获取的数据
char g_lora_recv_buf[256] = {0};
int g_lora_recv_flag = 0;
//	lora串口回调函数
int lora_uart_recv_call(void* param,const u8* data,u16 data_len)    
{
	if (g_lora_recv_flag == 0)
	{
		memset(g_lora_recv_buf, 0, 256);
		memcpy(g_lora_recv_buf, data, data_len);
		g_lora_recv_flag = 1;
	}
	else
	{
		memcpy(g_lora_recv_buf + strlen(g_lora_recv_buf), data, data_len);
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
//	解析蓝牙命令
void parse_bt_cmd()
{
	int len = strlen(g_bt_recv_buf);
	if (g_bt_recv_flag == 0 || g_bt_recv_buf[len - 2] != '\r' || g_bt_recv_buf[len - 1] != '\n')
		return;

	char *token = strtok(g_bt_recv_buf, "\r\n");
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

	g_bt_recv_flag = 0;
}
//	解析esp32cam命令
void parse_esp_cmd()
{
	int len = strlen(g_esp_recv_buf);
	if (g_esp_recv_flag == 0 || g_esp_recv_buf[len - 2] != '\r' || g_esp_recv_buf[len - 1] != '\n')
		return;

	char *token = strtok(g_esp_recv_buf, "\r\n");
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

	g_esp_recv_flag = 0;
}
//	解析zigbee命令
void parse_zigbee_cmd()
{
	int len = strlen(g_zigbee_recv_buf);
	if (g_zigbee_recv_flag == 0 || g_zigbee_recv_buf[len - 2] != '\r' || g_zigbee_recv_buf[len - 1] != '\n')
		return;

	char *token = strtok(g_zigbee_recv_buf, "\r\n");
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

	g_zigbee_recv_flag = 0;
}
//	解析ttl命令
void parse_ttl_cmd()
{
	int len = strlen(g_ttl_recv_buf);
	if (g_ttl_recv_flag == 0 || g_ttl_recv_buf[len - 2] != '\r' || g_ttl_recv_buf[len - 1] != '\n')
		return;

	char *token = strtok(g_ttl_recv_buf, "\r\n");
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

	g_ttl_recv_flag = 0;
}
//	解析lora命令
void parse_lora_cmd()
{
	int len = strlen(g_lora_recv_buf);
	if (g_lora_recv_flag == 0 || g_lora_recv_buf[len - 2] != '\r' || g_lora_recv_buf[len - 1] != '\n')
		return;

	char *token = strtok(g_lora_recv_buf, "\r\n");
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

	g_lora_recv_flag = 0;
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
//	解析阿里云命令
void parse_aliyun_cmd();

#define TASK2_UPDATE_TIME_MS  100	//	任务2间隔时间
static void my_task2(void* param)
{
	while(1)
	{
		//	解析阿里云命令
		parse_aliyun_cmd();
		parse_bt_cmd();
		parse_esp_cmd();
		parse_zigbee_cmd();
		parse_ttl_cmd();
		parse_lora_cmd();
		parse_8266_tcp_cmd();
		vTaskDelay(TASK2_UPDATE_TIME_MS);
	}
}
//OneNet回调函数
int onenet_callback(void* param, const char* msg)
{
	log_inform("Recv:%s.", msg);
	char* ptr = NULL;
	//解析从OneNet收到的数据
	if(NULL != (ptr = strstr((char*)msg,"data")))
	{
		//sscanf((char*)ptr,"data\":%d", &data);	
	}
	return E_OK;
}

#define TASK3_UPDATE_TIME_MS  2500	//	任务3间隔时间
static void my_task3(void* param)
{
	//待发送的数据
	char send_buf[DATA_LEN] = {0};
	mqtt_inform inform = {
	"version=2018-10-31&res=products%2FOPSYY588Pb%2Fdevices%2F4g_dev_001&et=1840088662&method=md5&sign=XAkDXcJp9ebCe6V%2BJ4WzDA%3D%3D",
	"OPSYY588Pb",
	"4g_dev_001",
	};
	c_ec800 ec800 = ec800_create(UART_4G_ID);
	ec800.ec800_callback(&ec800, NULL, onenet_callback);
	ec800.ec800_start(&ec800, &inform);
	int temp = 0;
	const char *prefix = "\"id\":\"123\",\"version\":\"1.0\",\"params\"";
	while(1)
	{
		memset(send_buf, 0, sizeof(send_buf));
		//prefix不能修改，要上传的数据按json格式加到花括号内，value值是否加引号，取决于onenet上字段类型，建议都用字符串，这样统一都加引号
		//sprintf((char *)send_buf, "%s:{\"temp\":{\"value\":\"%d\"}}", prefix, temp);
		ec800.ec800_send(&ec800,send_buf);
		vTaskDelay(TASK3_UPDATE_TIME_MS);
	}
}
//onenet回调函数
int onenet_callback(void* param, const char* msg)
{
	log_inform("Recv:%s.", msg);
	char* ptr = NULL;
	//解析从onenet收到的数据
	if(NULL != (ptr = strstr((char*)msg,"pattern")))
	{
		//sscanf((char*)ptr, "pattern\":\"%d", &mode);
	}
	return E_OK;
}

#define TASK3_UPDATE_TIME_MS  2500	//	任务3间隔时间
static void my_task3(void* param)
{
	c_esp8266 esp8266 = {0};
	sta_inform      sta = {
	STA_WIFI_NAME,
	STA_WIFI_PWD

	};

	onenet_inform  onenet = {
	"f6DqXFZBie",
	"test",
	//注意生成token用的key是设备详情里面的设备密匙，不是用户key！ version是2018-10-31 时间戳要选择一个至少当前时间往后一年的日期再生成时间戳 生成时间戳的网址 http://shijianchuo.wiicha.com/&wd=&eqid=e72a17be0076f6fd0000000265781901
	"version=2018-10-31&res=products%2Ff6DqXFZBie%2Fdevices%2Ftest&et=1840088662&method=md5&sign=coKiCPjZEGs0Gf8sc9gUKw%3D%3D"
	};

	esp8266 = esp8266_create(UART_8266_ONENET_ID);
	vTaskDelay(5000);
	esp8266.onenet_start(&esp8266,&sta,&onenet);
	//设置onenet回调
	esp8266.onenet_callback(&esp8266, NULL, onenet_callback);
	while(1)
	{
		//esp8266.onenet_send(&esp8266, ONENET_FLOAT ONENET_FLOAT_END, "val1", val1,
			//"val2", val2);
		vTaskDelay(TASK3_UPDATE_TIME_MS);
	}
}
c_esp8266 esp8266;
char g_aliyun_recv_buf[256] = {0};
int g_aliyun_recv_flag = 0;
//阿里云回调函数
int aliyun_callback(void* param,const u8* data,u16 data_len)  
{
	log_inform("Recv:%s.", data);
	if (g_aliyun_recv_flag == 0)
	{
		memset(g_aliyun_recv_buf, 0, 256);
		memcpy(g_aliyun_recv_buf, data, data_len);
		g_aliyun_recv_flag = 1;
	}
	else
	{
		if (strlen(g_aliyun_recv_buf) + data_len < 256)
			memcpy(g_aliyun_recv_buf + strlen(g_aliyun_recv_buf), data, data_len);
		else
			memset(g_aliyun_recv_buf, 0, 256);
}
	return E_OK;
}

	//	解析阿里云命令
void parse_aliyun_cmd()
{
	int len = strlen(g_aliyun_recv_buf);
	if (NULL == strstr(g_aliyun_recv_buf, "\r\n"))
		return;

	char *token = strtok(g_aliyun_recv_buf, "\r\n");
	char* ptr = NULL;
	int value = 0;
	while (token != NULL)
	{
		//查找关键字
		if(NULL != (ptr = strstr((char*)token, "target_temp")))
		{
			ptr = strstr((char*)token, "data");
			sscanf((char*)ptr, "data\":%d", &value);
		}
		token = strtok(NULL, "\r\n");
	}

	g_aliyun_recv_flag = 0;
}

#define DELAY_TIME 1500	//	上传数据间隔时长
#define TASK3_UPDATE_TIME_MS  1000	//	任务3间隔时间
static void my_task3(void* param)
{
	char buf[10] = {0};
	//阿里云创建数据流转可参考这个视频：https://www.bilibili.com/video/BV1kj41117Rz/?spm_id_from=333.880.my_history.page.click&vd_source=8c375f881434ec3b012fdf45a4ffe087 如果需要创建物模型（需要在云平台上查看数据），则在产品里创建，然后字段和代码里对应上就可以了
	esp8266 = esp8266_create(UART_8266_ALIYUN_ID);
	sta_inform sta = {STA_WIFI_NAME, STA_WIFI_PWD};	//wifi账号密码
	aliyun_inform aliyun = {
	"esp8266&k1h3j6aQls1",	//阿里云连接参数
	"ae936c4a072c236b177df2bad1fc9751c40886b3ea6d6ec74f6a3b902e7994ce",
	"k1h3j6aQls1.esp8266|securemode=2\\,signmethod=hmacsha256\\,timestamp=1718784830449|",
	"iot-06z009wuamz2x6c.mqtt.iothub.aliyuncs.com",
	"1883",
	"/k1h3j6aQls1/esp8266/user/esp8266",
	"//sys/k1h3j6aQls1/esp8266/thing/service/property/set"};
	esp8266.set_data_process_callback(&esp8266, NULL, aliyun_callback);
	esp8266.wifi_cfg(&esp8266, ESP8266_WIFI_STA, &sta, NULL);
	esp8266.aliyun_cfg(&esp8266, &aliyun);
	//设置回调函数
	while(1)
	{
		//temp这里是自定义的，和小程序/app/上位机对应好就行
		memset(buf, 0, sizeof(buf));
		//sprintf(buf,"%.1f", ds18b20_temp);
		//esp8266.aliyun_set(&esp8266, "temp", buf);
		vTaskDelay(TASK3_UPDATE_TIME_MS);
	}
}
//onenet回调函数
int onenet_callback(void* param, const char* msg)
{
	char* ptr = NULL;
	//解析从onenet收到的数据
	int thresh = 0;
	if(NULL != (ptr = strstr((char*)msg,"temp_thresh")))
	{
		sscanf((char*)ptr, "temp_thresh\":\"%d", &thresh);
	}
	return E_OK;
}

#define TASK3_UPDATE_TIME_MS  2500	//	任务3间隔时间
static void my_task3(void* param)
{
	c_bc260y bc260y = {0};
	onenet_inform  onenet = {
	"f6DqXFZBie",
	"test",
	//注意生成token用的key是设备详情里面的设备密匙，不是用户key！ version是2018-10-31 时间戳要选择一个至少当前时间往后一年的日期再生成时间戳 生成时间戳的网址 http://shijianchuo.wiicha.com/&wd=&eqid=e72a17be0076f6fd0000000265781901
	"version=2018-10-31&res=products%2Ff6DqXFZBie%2Fdevices%2Ftest&et=1840088662&method=md5&sign=coKiCPjZEGs0Gf8sc9gUKw%3D%3D"
	};

	bc260y = bc260y_create(UART_BC260Y_ONENET_ID);
	vTaskDelay(5000);
	bc260y.onenet_start(&bc260y, &onenet);
	//设置onenet回调
	bc260y.onenet_callback(&bc260y, NULL, onenet_callback);
	while(1)
	{
		//bc260y.onenet_send(&bc260y, ONENET_FLOAT ONENET_FLOAT_END, "val1", val1,
			//"val2", val2);
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
