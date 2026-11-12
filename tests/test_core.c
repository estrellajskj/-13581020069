#define ENV_HOST_TEST
#include "../firmware/LogicLayer/logic/my_logic.c"
#include "../firmware/LogicLayer/logic/pin_conf.h"

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while (0)

static void test_time(void)
{
    env_time t = {2024,2,29,7,30,0};
    CHECK(time_valid(&t)); CHECK(alarm_due(&t,1,7,30,0));
    CHECK(!alarm_due(&t,1,7,30,20240229)); CHECK(!alarm_due(&t,0,7,30,0));
    CHECK(!alarm_due(&t,1,7,31,0));
    t.year=2023; CHECK(!time_valid(&t));
    t.year=2024; t.month=4; t.day=31; CHECK(!time_valid(&t));
    t.day=30; t.second=60; CHECK(!time_valid(&t));
    CHECK(parse_sntp("+CIPSNTPTIME:Fri Oct 23 15:30:04 2026",&t));
    CHECK(t.year==2026 && t.month==10 && t.hour==15 && t.second==4);
    CHECK(!parse_sntp("+CIPSNTPTIME:Thu Jan 01 00:00:00 1970",&t));
    CHECK(!parse_sntp("+CIPSNTPTIME:Fri Foo 23 15:30:04 2026",&t));
}
static void test_sensors(void)
{
    const int xs[]={100,1000,2000}, ys[]={0,70,200};
    const int bad[]={100,100,2000};
    co2_parser p={0};
    unsigned char frame[]={0xff,0x86,0x03,0xe9,0,0,0,0,0};
    unsigned i,sum=0; int ppm=0,v;
    CHECK(interpolate(1000,xs,ys)==70); CHECK(interpolate(1500,xs,ys)==135);
    CHECK(interpolate(99,xs,ys)==-1); CHECK(interpolate(2001,xs,ys)==-1);
    CHECK(interpolate(100,bad,ys)==-1);
    CHECK(relative_light(2800,2800,200)==0); CHECK(relative_light(200,2800,200)==100);
    CHECK(relative_light(3300,2800,200)==0); CHECK(relative_light(0,2800,200)==100);
    CHECK(relative_light(0,0,0)==-1);
    CHECK(!threshold_alarm(0,1,1300,CO2_ALARM_PPM,CO2_CLEAR_PPM));
    CHECK(threshold_alarm(0,1,1301,CO2_ALARM_PPM,CO2_CLEAR_PPM));
    CHECK(threshold_alarm(1,1,1251,CO2_ALARM_PPM,CO2_CLEAR_PPM));
    CHECK(!threshold_alarm(1,1,1250,CO2_ALARM_PPM,CO2_CLEAR_PPM));
    CHECK(!threshold_alarm(1,0,2000,CO2_ALARM_PPM,CO2_CLEAR_PPM));
    CHECK(!threshold_alarm(0,1,70,70,65)); CHECK(threshold_alarm(0,1,71,70,65));
    for(i=1;i<8;++i)sum+=frame[i]; frame[8]=(unsigned char)(0-sum);
    CHECK(!co2_feed(&p,0,&ppm));
    for(i=0;i<8;++i) CHECK(!co2_feed(&p,frame[i],&ppm));
    CHECK(co2_feed(&p,frame[8],&ppm)); CHECK(ppm==1001);
    frame[8]^=1;
    for(i=0;i<9;++i) CHECK(!co2_feed(&p,frame[i],&ppm));
    CHECK(number("100",10,100,&v) && v==100);
    CHECK(!number("100x",10,100,&v)); CHECK(!number("",0,1,&v));
    CHECK(!number("-1",0,100,&v)); CHECK(!number("1000",0,100,&v));
}
static void test_hcho_concentration(void)
{
    const int xs[]={100,500,1500}, rising[]={0,70,200}, falling[]={200,70,0};
    const int empty[]={0,0,0}, nonmonotonic[]={0,100,70};
    const int bad_adc[]={0,500,1500}, out_of_bounds[]={0,70,10001};
    char text[32];
    /* Synthetic pairs exercise the algorithm; they are not sensor calibration. */
    CHECK(calibration_valid(xs,rising)); CHECK(calibration_valid(xs,falling));
    CHECK(!calibration_valid(empty,empty)); CHECK(!calibration_valid(xs,nonmonotonic));
    CHECK(!calibration_valid(bad_adc,rising)); CHECK(!calibration_valid(xs,out_of_bounds));
    CHECK(interpolate(500,xs,falling)==70); CHECK(interpolate(1000,xs,falling)==35);
    CHECK(interpolate(1500,xs,falling)==0); CHECK(interpolate(99,xs,falling)==-1);
    CHECK(interpolate(1501,xs,falling)==-1);
    hcho_text(text,sizeof(text),1,1,0,1,71); CHECK(!strcmp(text,"\xB4\xFD\xB1\xEA\xB6\xA8"));
    hcho_text(text,sizeof(text),0,1,1,1,71); CHECK(!strcmp(text,"-- mg/m3"));
    hcho_text(text,sizeof(text),1,0,1,1,71); CHECK(!strcmp(text,"WARM mg/m3"));
    hcho_text(text,sizeof(text),1,1,1,0,71); CHECK(!strcmp(text,"-- mg/m3"));
    hcho_text(text,sizeof(text),1,1,1,1,-1); CHECK(!strcmp(text,"-- mg/m3"));
    hcho_text(text,sizeof(text),1,1,1,1,0); CHECK(!strcmp(text,"0.000 mg/m3"));
    hcho_text(text,sizeof(text),1,1,1,1,70); CHECK(!strcmp(text,"0.070 mg/m3"));
    hcho_text(text,sizeof(text),1,1,1,1,71); CHECK(!strcmp(text,"0.071 mg/m3"));
    hcho_text(text,sizeof(text),1,1,1,1,1234); CHECK(!strcmp(text,"1.234 mg/m3"));
    hcho_text(text,sizeof(text),1,1,1,1,10000); CHECK(!strcmp(text,"10.000 mg/m3"));
    CHECK(HCHO_ALARM_UG_M3==70 && HCHO_CLEAR_UG_M3==65);
    CHECK(!threshold_alarm(0,1,70,HCHO_ALARM_UG_M3,HCHO_CLEAR_UG_M3));
    CHECK(threshold_alarm(0,1,71,HCHO_ALARM_UG_M3,HCHO_CLEAR_UG_M3));
    CHECK(threshold_alarm(1,1,66,HCHO_ALARM_UG_M3,HCHO_CLEAR_UG_M3));
    CHECK(!threshold_alarm(1,1,65,HCHO_ALARM_UG_M3,HCHO_CLEAR_UG_M3));
    CHECK(!threshold_alarm(1,0,71,HCHO_ALARM_UG_M3,HCHO_CLEAR_UG_M3));
    CHECK(!MQ138_CALIBRATED);
}
static void test_sy_co2_protocol(void)
{
    const unsigned char command[] = {0xff,0x01,0x86,0,0,0,0,0,0x79};
    const unsigned char sample[] = {0xff,0x86,0x01,0x8a,0x1b,0x85,0,1,0x4e};
    unsigned char frame[9];
    co2_parser parser = {0};
    int ppm = -1, result = 0;
    unsigned i;
    CHECK(CO2_PROTOCOL == 1); CHECK(CO2_BAUD == 9600U);
    CHECK(!memcmp(co2_query,command,sizeof(command)));
    /* Manufacturer A2.0 page 6 example, including nonzero reserved bytes. */
    for (i = 0; i < sizeof(sample) - 1; ++i) CHECK(!co2_feed(&parser,sample[i],&ppm));
    CHECK(ppm == -1);
    CHECK(co2_feed(&parser,sample[8],&ppm)); CHECK(ppm == 394);
    CHECK(!threshold_alarm(0,1,ppm,CO2_ALARM_PPM,CO2_CLEAR_PPM));
    for (i = 0; i < 2; ++i) {
        unsigned j;
        for (j = 0; j < sizeof(sample); ++j) result += co2_feed(&parser,sample[j],&ppm);
    }
    CHECK(result == 2 && ppm == 394);
    memcpy(frame,sample,sizeof(frame)); frame[8] ^= 1;
    for (i = 0; i < sizeof(frame); ++i) CHECK(!co2_feed(&parser,frame[i],&ppm));
    CHECK(ppm == 394);
    /* A partial frame followed by a new header must not consume the new frame. */
    CHECK(!co2_feed(&parser,0xff,&ppm)); CHECK(!co2_feed(&parser,0x86,&ppm));
    CHECK(!co2_feed(&parser,0,&ppm)); CHECK(!co2_feed(&parser,0,&ppm));
    for (i = 0; i < sizeof(sample) - 1; ++i) CHECK(!co2_feed(&parser,sample[i],&ppm));
    CHECK(co2_feed(&parser,sample[8],&ppm)); CHECK(ppm == 394);
    /* A valid checksum alone cannot make an out-of-range concentration valid. */
    memcpy(frame,sample,sizeof(frame)); frame[2]=0x27; frame[3]=0x11; frame[8]=0xa1;
    for (i = 0; i < sizeof(frame); ++i) CHECK(!co2_feed(&parser,frame[i],&ppm));
    CHECK(ppm == 394);
}
static void test_weather(void)
{
    const char *json="{\"results\":[{\"location\":{\"name\":\"Beijing\"},\"now\":{\"text\":\"Cloudy\",\"code\":\"4\",\"temperature\":\"18\"}}]}";
    char response[800], stream[900], header[64], display[72];
    ipd_parser parser;
    int t=0,code=0; unsigned i;
    CHECK(weather_parse(json,&t,&code)); CHECK(t==18 && code==4);
    weather_text(display,sizeof(display),1,23,0);
    CHECK(!strcmp(display,"\xC7\xE7 / 23 C"));
    CHECK(!strcmp(weather_condition(1),weather_condition(0)));
    CHECK(!strcmp(weather_condition(4),"\xB6\xE0\xD4\xC6"));
    CHECK(!strcmp(weather_condition(9),"\xD2\xF5"));
    weather_text(display,sizeof(display),1,18,13);
    CHECK(!strcmp(display,"\xD0\xA1\xD3\xEA / 18 C"));
    weather_text(display,sizeof(display),1,-10,22);
    CHECK(!strcmp(display,"\xD0\xA1\xD1\xA9 / -10 C"));
    CHECK(!strcmp(weather_condition(30),"\xCE\xED"));
    CHECK(!strcmp(weather_condition(31),"\xF6\xB2"));
    CHECK(!strcmp(weather_condition(99),"\xCE\xB4\xD6\xAA"));
    CHECK(!strcmp(weather_condition(-1),weather_condition(99)));
    weather_text(display,sizeof(display),0,23,0);
    CHECK(!strcmp(display,"\xCC\xEC\xC6\xF8\xD4\xDD\xB2\xBB\xBF\xC9\xD3\xC3"));
    /* Seven full-width glyphs plus temperature fit the 296-pixel field. */
    weather_text(display,sizeof(display),1,-80,12);
    CHECK(strlen(display) == 22 && strlen(display) * 8 <= 296);
    CHECK(weather_parse("{\"results\":[{\"now\":{\"code\":0,\"temperature\":-10}}]}",&t,&code));
    CHECK(t==-10 && code==0);
    CHECK(!weather_parse("{\"temperature\":18,\"code\":4}",&t,&code));
    CHECK(!weather_parse("{\"results\":[{\"now\":{\"code\":0,\"temperature\":\"18x\"}}]}",&t,&code));
    CHECK(!weather_parse("{\"results\":[{\"now\":{\"code\":0,\"temperature\":100}}]}",&t,&code));
    CHECK(!weather_parse("{\"results\":[]}",&t,&code));
    CHECK(!weather_parse("{\"results\":[{\"now\":{\"code\":4}",&t,&code));
    CHECK(!strcmp(clothing(18,4),"Light jacket / long sleeves"));
    CHECK(!strcmp(clothing(30,11),"Rainwear / umbrella"));
    snprintf(response,sizeof(response),"HTTP/1.0 200 OK\r\nContent-Length: %u\r\n\r\n%s",(unsigned)strlen(json),json);
    memset(&parser,0,sizeof(parser));
    snprintf(stream,sizeof(stream),"WIFI CONNECTED\r\n+IPD,%u:%s\r\nCLOSED\r\n",(unsigned)strlen(response),response);
    for(i=0;stream[i];++i)ipd_feed(&parser,(unsigned char)stream[i]);
    CHECK(http_weather(&parser,&t,&code)); CHECK(t==18 && code==4);
    /* Two payload records, including a split in the HTTP header. */
    memset(&parser,0,sizeof(parser));
    snprintf(header,sizeof(header),"+IPD,17:");
    for(i=0;header[i];++i)ipd_feed(&parser,(unsigned char)header[i]);
    for(i=0;i<17;++i)ipd_feed(&parser,(unsigned char)response[i]);
    snprintf(header,sizeof(header),"+IPD,%u:",(unsigned)strlen(response)-17);
    for(i=0;header[i];++i)ipd_feed(&parser,(unsigned char)header[i]);
    for(i=17;response[i];++i)ipd_feed(&parser,(unsigned char)response[i]);
    CHECK(http_weather(&parser,&t,&code));
    parser.remaining=1; CHECK(!http_weather(&parser,&t,&code)); parser.remaining=0;
    parser.overflow=1; CHECK(!http_weather(&parser,&t,&code)); parser.overflow=0;
    parser.http[9]='5'; CHECK(!http_weather(&parser,&t,&code));
    memset(&parser,0,sizeof(parser));
    for(i=0;i<20;++i)ipd_feed(&parser,(unsigned char)stream[i]);
    CHECK(!http_weather(&parser,&t,&code));
    memset(&parser,0,sizeof(parser));
    strcpy(parser.http,"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n");
    CHECK(!http_weather(&parser,&t,&code));
}

static void test_raw_voltage_alarm(void)
{
    raw_voltage_alarm state = {0};
    CHECK(!raw_low_alarm(&state,0,1,107,500,800,2));
    CHECK(!raw_low_alarm(&state,1,1,1500,500,800,2));
    CHECK(!raw_low_alarm(&state,1,1,107,500,800,2));
    CHECK(!raw_low_alarm(&state,1,1,1500,500,800,2));
    CHECK(!raw_low_alarm(&state,1,1,500,500,800,2));
    CHECK(!raw_low_alarm(&state,1,1,500,500,800,2));
    CHECK(!raw_low_alarm(&state,1,1,499,500,800,2));
    CHECK(raw_low_alarm(&state,1,1,107,500,800,2));
    CHECK(raw_low_alarm(&state,1,1,650,500,800,2));
    CHECK(raw_low_alarm(&state,1,1,800,500,800,2));
    CHECK(raw_low_alarm(&state,1,1,650,500,800,2));
    CHECK(raw_low_alarm(&state,1,1,800,500,800,2));
    CHECK(!raw_low_alarm(&state,1,1,1500,500,800,2));
    CHECK(!raw_low_alarm(&state,1,1,107,500,800,2));
    CHECK(!raw_low_alarm(&state,1,0,107,500,800,2));
    CHECK(!raw_low_alarm(&state,1,1,107,500,800,2));
    CHECK(raw_low_alarm(&state,1,1,107,500,800,2));
    CHECK(!raw_low_alarm(&state,0,1,107,500,800,2));
    CHECK(!raw_low_alarm(&state,1,1,107,800,500,2));
    CHECK(!raw_low_alarm(&state,1,1,107,500,800,0));
}
int main(void)
{
    CHECK(BUZZER_OUTPUT_ENABLED == 1);
    CHECK(!buzzer_phase(0,0,300));
    CHECK(!buzzer_phase(1,0,299));
    CHECK(buzzer_phase(1,0,300));
    CHECK(!buzzer_phase(1,0,600));
    CHECK(buzzer_phase(0,1,900));
    CHECK(buzzer_phase(1,1,900));
    CHECK(!buzzer_phase(0,0,900));
    test_time(); test_sensors(); test_hcho_concentration(); test_sy_co2_protocol(); test_weather(); test_raw_voltage_alarm();
    printf("PASS: %u checks\n",checks); return 0;
}
