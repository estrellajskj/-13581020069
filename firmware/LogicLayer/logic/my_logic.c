#ifdef ENV_HOST_TEST
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#else
#include "app_conf.h"
#include "pin_conf.h"
#endif
#define JSMN_STRICT
#define JSMN_STATIC
#include "../../ThirdPart/jsmn.h"

typedef struct {
    int year, month, day, hour, minute, second;
} env_time;

static int leap(int y) { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }
static int month_days(int y, int m)
{
    static const unsigned char days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m < 1 || m > 12) return 0;
    return days[m - 1] + (m == 2 && leap(y));
}
static int time_valid(const env_time *t)
{
    return t->year >= 2020 && t->year <= 2099 && t->month >= 1 && t->month <= 12 &&
        t->day >= 1 && t->day <= month_days(t->year, t->month) &&
        t->hour >= 0 && t->hour < 24 && t->minute >= 0 && t->minute < 60 &&
        t->second >= 0 && t->second < 60;
}
static unsigned day_key(const env_time *t)
{
    return (unsigned)(t->year * 10000 + t->month * 100 + t->day);
}
static int threshold_alarm(int previous, int valid, int value, int on, int off)
{
    if (!valid) return 0;
    if (value > on) return 1;
    if (value <= off) return 0;
    return previous;
}

typedef struct {
    int active;
    unsigned consecutive;
} raw_voltage_alarm;

static int raw_low_alarm(raw_voltage_alarm *state, int ready, int valid,
                         int mv, int on, int off, unsigned confirmation)
{
    int next;
    if (!ready || !valid || on >= off || !confirmation) {
        state->active = 0; state->consecutive = 0;
        return 0;
    }
    next = state->active ? mv < off : mv < on;
    if (next == state->active) state->consecutive = 0;
    else if (++state->consecutive >= confirmation) {
        state->active = next; state->consecutive = 0;
    }
    return state->active;
}
static int buzzer_phase(int gas, int clock, unsigned now)
{
    return (gas || clock) && (now / 300U % 2U);
}
static int alarm_due(const env_time *t, int enabled, int hour, int minute, unsigned last_day)
{
    return enabled && time_valid(t) && t->hour == hour && t->minute == minute && last_day != day_key(t);
}
static int parse_sntp(const char *s, env_time *t)
{
    char week[4], month[4];
    static const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    int i;
    const char *p = strstr(s, "+CIPSNTPTIME:");
    if (!p || sscanf(p, "+CIPSNTPTIME:%3s %3s %d %d:%d:%d %d", week, month,
        &t->day, &t->hour, &t->minute, &t->second, &t->year) != 7) return 0;
    t->month = 0;
    for (i = 0; i < 12; ++i) if (!strcmp(month, months[i])) t->month = i + 1;
    return time_valid(t);
}
static int relative_light(int mv, int dark, int bright)
{
    int value;
    if (dark == bright) return -1;
    value = (mv - dark) * 100 / (bright - dark);
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    return value;
}
static void mq_response_text(char *text, size_t size, int valid, int ready, int mv, int reference)
{
    int percent = reference > 0 ? relative_light(mv, reference, 0) : -1;
    if (!valid || percent < 0) snprintf(text, size, "-- %%");
    else snprintf(text, size, "%s %d%%", ready ? "REL" : "WARM", percent);
}
static int interpolate(int x, const int *xs, const int *ys)
{
    int i;
    if (!(xs[0] < xs[1] && xs[1] < xs[2]) ||
        !(ys[0] < ys[1] && ys[1] < ys[2]) || x < xs[0] || x > xs[2]) return -1;
    i = x <= xs[1] ? 0 : 1;
    return ys[i] + (int)((int32_t)(x - xs[i]) * (ys[i + 1] - ys[i]) / (xs[i + 1] - xs[i]));
}
static int number(const char *s, int min, int max, int *out)
{
    long v;
    char *end;
    if (!s || !*s || (*s != '-' && (*s < '0' || *s > '9'))) return 0;
    v = strtol(s, &end, 10);
    if (*end || v < min || v > max) return 0;
    *out = (int)v;
    return 1;
}

static int token_eq(const char *json, const jsmntok_t *t, const char *s)
{
    return t->type == JSMN_STRING && t->end - t->start == (int)strlen(s) &&
        !strncmp(json + t->start, s, (size_t)(t->end - t->start));
}
static int token_after(const jsmntok_t *tokens, int count, int i)
{
    int end = tokens[i].end;
    for (++i; i < count && tokens[i].start < end; ++i) {}
    return i;
}
static int member(const char *json, const jsmntok_t *tokens, int count, int object, const char *key)
{
    int i;
    if (object < 0 || object >= count || tokens[object].type != JSMN_OBJECT) return -1;
    for (i = object + 1; i + 1 < count && tokens[i].start < tokens[object].end;) {
        if (token_eq(json, &tokens[i], key)) return i + 1;
        i = token_after(tokens, count, i + 1);
    }
    return -1;
}
static int token_number(const char *json, const jsmntok_t *t, int min, int max, int *out)
{
    char buf[12];
    int len = t->end - t->start;
    if (len <= 0 || len >= (int)sizeof(buf)) return 0;
    memcpy(buf, json + t->start, (size_t)len); buf[len] = 0;
    return number(buf, min, max, out);
}
static int weather_parse(const char *json, int *temperature, int *code)
{
    static jsmntok_t tokens[96];
    jsmn_parser parser;
    int n, results, now, ti, ci;
    jsmn_init(&parser);
    n = jsmn_parse(&parser, json, strlen(json), tokens, 96);
    if (n < 1 || tokens[0].type != JSMN_OBJECT) return 0;
    results = member(json, tokens, n, 0, "results");
    if (results < 0 || tokens[results].type != JSMN_ARRAY || !tokens[results].size) return 0;
    now = member(json, tokens, n, results + 1, "now");
    ti = member(json, tokens, n, now, "temperature");
    ci = member(json, tokens, n, now, "code");
    return ti >= 0 && ci >= 0 && token_number(json, &tokens[ti], -80, 70, temperature) &&
        token_number(json, &tokens[ci], 0, 99, code);
}
/* Seniverse code table; GB2312 bytes match the screen's font encoding. */
static const char *weather_condition(int code)
{
    static const char *const names[] = {
        "\xC7\xE7", "\xC7\xE7", "\xC7\xE7", "\xC7\xE7",
        "\xB6\xE0\xD4\xC6", "\xC7\xE7\xBC\xE4\xB6\xE0\xD4\xC6",
        "\xC7\xE7\xBC\xE4\xB6\xE0\xD4\xC6", "\xB4\xF3\xB2\xBF\xB6\xE0\xD4\xC6",
        "\xB4\xF3\xB2\xBF\xB6\xE0\xD4\xC6", "\xD2\xF5",
        "\xD5\xF3\xD3\xEA", "\xC0\xD7\xD5\xF3\xD3\xEA",
        "\xC0\xD7\xD5\xF3\xD3\xEA\xB0\xE9\xD3\xD0\xB1\xF9\xB1\xA2",
        "\xD0\xA1\xD3\xEA", "\xD6\xD0\xD3\xEA", "\xB4\xF3\xD3\xEA",
        "\xB1\xA9\xD3\xEA", "\xB4\xF3\xB1\xA9\xD3\xEA", "\xCC\xD8\xB4\xF3\xB1\xA9\xD3\xEA",
        "\xB6\xB3\xD3\xEA", "\xD3\xEA\xBC\xD0\xD1\xA9", "\xD5\xF3\xD1\xA9",
        "\xD0\xA1\xD1\xA9", "\xD6\xD0\xD1\xA9", "\xB4\xF3\xD1\xA9", "\xB1\xA9\xD1\xA9",
        "\xB8\xA1\xB3\xBE", "\xD1\xEF\xC9\xB3", "\xC9\xB3\xB3\xBE\xB1\xA9",
        "\xC7\xBF\xC9\xB3\xB3\xBE\xB1\xA9", "\xCE\xED", "\xF6\xB2", "\xB7\xE7",
        "\xB4\xF3\xB7\xE7", "\xEC\xAB\xB7\xE7", "\xC8\xC8\xB4\xF8\xB7\xE7\xB1\xA9",
        "\xC1\xFA\xBE\xED\xB7\xE7", "\xC0\xE4", "\xC8\xC8"
    };
    if (code < 0 || (unsigned)code >= sizeof(names) / sizeof(names[0])) return "\xCE\xB4\xD6\xAA";
    return names[code];
}
static void weather_text(char *text, size_t size, int valid, int temperature, int code)
{
    if (valid) snprintf(text,size,"%s / %d C",weather_condition(code),temperature);
    else snprintf(text,size,"\xCC\xEC\xC6\xF8\xD4\xDD\xB2\xBB\xBF\xC9\xD3\xC3");
}
static const char *clothing(int t, int code)
{
    if ((code >= 10 && code <= 25) || (code >= 30 && code <= 32)) return "Rainwear / umbrella";
    if (t <= 5) return "Down jacket / warm layers";
    if (t <= 15) return "Coat / long sleeves";
    if (t <= 24) return "Light jacket / long sleeves";
    return "Light clothes / sun protection";
}

typedef struct {
    unsigned char data[9];
    unsigned used;
} co2_parser;
static const unsigned char co2_query[9] = {0xff,0x01,0x86,0,0,0,0,0,0x79};
static int co2_feed(co2_parser *p, unsigned char b, int *ppm)
{
    unsigned i, sum = 0;
    int value;
    if (!p->used && b != 0xff) return 0;
    p->data[p->used++] = b;
    if (p->used == 2 && b != 0x86) {
        p->used = b == 0xff ? 1 : 0;
        return 0;
    }
    if (p->used < 9) return 0;
    for (i = 1; i < 8; ++i) sum += p->data[i];
    value = p->data[2] * 256 + p->data[3];
    p->used = 0;
    if ((unsigned char)(0 - sum) == p->data[8] && value <= 10000) {
        *ppm = value;
        return 1;
    }
    /* Retain a possible next header after a truncated or damaged frame. */
    for (i = 1; i < sizeof(p->data); ++i) {
        if (p->data[i] == 0xff && (i == 8 || p->data[i + 1] == 0x86)) {
            p->used = sizeof(p->data) - i;
            memmove(p->data, p->data + i, p->used);
            break;
        }
    }
    return 0;
}

/* Feed +IPD payloads by advertised byte count, across arbitrary UART fragments. */
typedef struct {
    char prefix[16];
    unsigned used, remaining;
    int overflow;
    char http[2048];
    unsigned http_used;
} ipd_parser;
static void ipd_feed(ipd_parser *p, unsigned char b)
{
    unsigned length;
    char tail;
    if (p->remaining) {
        if (p->http_used + 1 < sizeof(p->http)) {
            p->http[p->http_used++] = (char)b; p->http[p->http_used] = 0;
        } else p->overflow = 1;
        --p->remaining;
        return;
    }
    if (!p->used) {
        if (b == '+') p->prefix[p->used++] = (char)b;
        return;
    }
    if (p->used + 1 >= sizeof(p->prefix)) { p->used = 0; return; }
    p->prefix[p->used++] = (char)b; p->prefix[p->used] = 0;
    if (p->used <= 5 && strncmp(p->prefix, "+IPD,", p->used)) {
        p->used = b == '+' ? 1 : 0;
        if (p->used) p->prefix[0] = '+';
    } else if (b == ':') {
        if (sscanf(p->prefix, "+IPD,%u%c", &length, &tail) == 2 && tail == ':' && length <= 65535U)
            p->remaining = length;
        p->used = 0;
    }
}
static int http_weather(ipd_parser *p, int *temp, int *code)
{
    const char *body, *length_header, *chunked;
    int length;
    if (p->overflow || p->remaining || strncmp(p->http, "HTTP/1.", 7) ||
        (strncmp(p->http + 8, " 200 ", 5))) return 0;
    body = strstr(p->http, "\r\n\r\n");
    if (!body) return 0;
    chunked = strstr(p->http, "Transfer-Encoding:");
    if (chunked && chunked < body) return 0; /* Request HTTP/1.0; fail closed for unsupported framing. */
    length_header = strstr(p->http, "Content-Length:");
    if (length_header && length_header < body) {
        if (sscanf(length_header, "Content-Length: %d", &length) != 1 || length < 0 ||
            strlen(body + 4) != (size_t)length) return 0;
    }
    return weather_parse(body + 4, temp, code);
}

#ifndef ENV_HOST_TEST
#ifdef APP_MY_APP_ENABLED

typedef struct {
    unsigned char *data;
    unsigned size;
    volatile unsigned head, tail, overflow;
} byte_ring;
static unsigned char wifi_bytes[1024], screen_bytes[128], co2_bytes[64];
static byte_ring wifi_ring = {wifi_bytes, sizeof(wifi_bytes), 0,0,0};
static byte_ring screen_ring = {screen_bytes, sizeof(screen_bytes), 0,0,0};
static byte_ring co2_ring = {co2_bytes, sizeof(co2_bytes), 0,0,0};
static c_dht11 dht;
static c_my_iic rtc_bus;
static c_ds3231 rtc;
static c_switch buzzer, led;
static env_time clock_now;
static int clock_ok, dht_ok, co2_ok, hcho_ok, weather_ok;
static int co2_ppm, hcho_ug, mq_mv, light_pct, light_ok, temperature, humidity;
static int weather_temp, weather_code;
static int mq_adc_ok, mq_warmed, mq_raw_alarm;
static raw_voltage_alarm mq_raw_state;
static unsigned co2_at, weather_at;
static int gas_alarm, alarm_latched, alarm_enabled, alarm_hour = 7, alarm_minute;
static unsigned alarm_day;
static int auto_dim = 1, manual_dim = 70, current_dim = -1, active_page;
static int brightness_pending = 1;
static int network_online;
static co2_parser co2_rx;
static ipd_parser http_rx;
volatile unsigned env_min_free_heap;
volatile unsigned env_stack_free_words;
volatile unsigned env_co2_rx_bytes, env_co2_last_rx_at, env_co2_rx_snapshot_size;
volatile unsigned env_co2_tx_queries, env_co2_valid_frames;
volatile unsigned char env_co2_rx_snapshot[16];
extern void firmware_fatal(u8 code);

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task; (void)name;
    firmware_fatal(8);
}

static int receive(void *arg, const u8 *data, u16 len)
{
    byte_ring *r = (byte_ring *)arg;
    unsigned i, next;
    for (i = 0; i < len; ++i) {
        next = (r->head + 1) % r->size;
        if (next == r->tail) { r->overflow = 1; break; }
        r->data[r->head] = data[i];
        __DMB(); r->head = next;
    }
    return E_OK;
}

static int co2_receive(void *arg, const u8 *data, u16 len)
{
    unsigned i, size = len < sizeof(env_co2_rx_snapshot) ? len : sizeof(env_co2_rx_snapshot);
    env_co2_rx_bytes += len;
    env_co2_last_rx_at = HAL_GetTick();
    env_co2_rx_snapshot_size = 0;
    for (i = 0; i < size; ++i) env_co2_rx_snapshot[i] = data[i];
    __DMB(); env_co2_rx_snapshot_size = size;
    return receive(arg, data, len);
}
static int take(byte_ring *r)
{
    int b;
    if (r->tail == r->head) return -1;
    b = r->data[r->tail];
    __DMB(); r->tail = (r->tail + 1) % r->size;
    return b;
}
static int screen_command(const char *command)
{
    unsigned char buf[132];
    unsigned n = strlen(command);
    if (n > sizeof(buf) - 3) return 0;
    memcpy(buf, command, n); memset(buf + n, 0xff, 3);
    return my_uart.send(SCREEN_UART_ID, buf, (u16)(n + 3)) == E_OK;
}
static void screen_text(const char *object, const char *text)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "%s.txt=\"%s\"", object, text);
    screen_command(cmd);
}
static void screen_value(const char *object, int value)
{
    char cmd[60];
    snprintf(cmd, sizeof(cmd), "%s.val=%d", object, value);
    screen_command(cmd);
}
static void screen_page(int page)
{
    char cmd[24];
    if (page < 0 || page > 3) return;
    snprintf(cmd, sizeof(cmd), "page %d", page);
    if (screen_command(cmd)) active_page = page;
}
static void clock_store(const env_time *t)
{
    time_type value;
    if (!time_valid(t) || !rtc.this) return;
    memset(&value, 0, sizeof(value));
    value.o_year = (u8)(t->year - 2000); value.o_month = (u8)t->month;
    value.o_day = (u8)t->day; value.o_house = (u8)t->hour;
    value.o_min = (u8)t->minute; value.o_sec = (u8)t->second;
    value.o_week = 1; /* Driver uses weekday 1..7; recomputed below. */
    {
        int y = t->year, m = t->month;
        static const int offsets[] = {0,3,2,5,0,3,5,1,4,6,2,4};
        if (m < 3) --y;
        value.o_week = (u8)((y + y/4 - y/100 + y/400 + offsets[m-1] + t->day) % 7 + 1);
    }
    if (rtc.set(&rtc, &value) == E_OK) { clock_now = *t; clock_ok = 1; }
}
static void clock_read(void)
{
    time_type t;
    env_time next;
    clock_ok = 0;
    if (!rtc.this && rtc_bus.this) rtc = ds3231_creat(&rtc_bus);
    if (!rtc.this || rtc.get(&rtc, &t) != E_OK) return;
    next.year = 2000 + t.o_year; next.month = t.o_month; next.day = t.o_day;
    next.hour = t.o_house; next.minute = t.o_min; next.second = t.o_sec;
    if (time_valid(&next)) { clock_now = next; clock_ok = 1; }
}
static void alarm_update(unsigned now)
{
    static int co2_high, hcho_high;
    if (co2_ok && now - co2_at > CO2_STALE_MS) co2_ok = 0;
    co2_high = threshold_alarm(co2_high,co2_ok,co2_ppm,CO2_ALARM_PPM,CO2_CLEAR_PPM);
    hcho_high = threshold_alarm(hcho_high,hcho_ok,hcho_ug,HCHO_ALARM_UG_M3,HCHO_CLEAR_UG_M3);
    gas_alarm = co2_high || hcho_high || mq_raw_alarm;
    if (clock_ok && alarm_due(&clock_now,alarm_enabled,alarm_hour,alarm_minute,alarm_day)) {
        alarm_day = day_key(&clock_now); alarm_latched = 1;
    }
    if (buzzer.this) buzzer.set(&buzzer, BUZZER_OUTPUT_ENABLED && buzzer_phase(gas_alarm,alarm_latched,now) ? SWITCH_HIGHT : SWITCH_LOW);
    if (led.this) led.set(&led, gas_alarm || alarm_latched ? SWITCH_HIGHT : SWITCH_LOW);
}
static void sensors_read(unsigned now)
{
    float temp, hum, v;
    const int xs[3] = {MQ138_CAL_MV_0, MQ138_CAL_MV_1, MQ138_CAL_MV_2};
    const int ys[3] = {MQ138_CAL_UG_0, MQ138_CAL_UG_1, MQ138_CAL_UG_2};
    dht_ok = dht.this && dht.get(&dht, &temp, &hum) == E_OK;
    if (dht_ok) { temperature = (int)(temp * 10); humidity = (int)(hum * 10); }
    hcho_ok = 0;
    mq_adc_ok = 0;
    if (now >= MQ138_WARMUP_MS) mq_warmed = 1;
    if (my_adc.get(MQ138_ADC, &v) == E_OK) {
        mq_mv = (int)(v * 1000);
        mq_adc_ok = mq_mv > 5 && mq_mv < 3250;
        if (MQ138_CALIBRATED && mq_warmed && mq_adc_ok) {
            hcho_ug = interpolate(mq_mv, xs, ys); hcho_ok = hcho_ug >= 0;
        }
    }
    mq_raw_alarm = raw_low_alarm(&mq_raw_state,
        MQ138_RAW_ALARM_ENABLED && !MQ138_CALIBRATED && mq_warmed, mq_adc_ok,
        mq_mv, MQ138_RAW_ALARM_ON_MV, MQ138_RAW_ALARM_OFF_MV, MQ138_RAW_CONFIRM_SAMPLES);
    light_ok = 0;
    if (my_adc.get(GM_ADC, &v) == E_OK) {
        light_pct = relative_light((int)(v * 1000), LIGHT_DARK_MV, LIGHT_BRIGHT_MV);
        light_ok = light_pct >= 0;
    }
    if (weather_ok && now - weather_at > WEATHER_STALE_MS) weather_ok = 0;
}
static int safe_clock_command(const char *s)
{
    env_time t;
    int n = 0;
    if (sscanf(s, "SETTIME=%d-%d-%dT%d:%d:%d%n", &t.year, &t.month, &t.day,
        &t.hour, &t.minute, &t.second, &n) != 6 || s[n]) return 0;
    if (!time_valid(&t)) return 0;
    clock_store(&t); return 1;
}
static int handle_screen(const char *s)
{
    int v, hh, mm, en, n = 0;
    if (!strncmp(s, "PAGE=", 5) && number(s + 5, 0, 3, &v)) screen_page(v);
    else if (!strncmp(s, "DIM=", 4) && number(s + 4, 10, 100, &v)) { manual_dim = v; auto_dim = 0; brightness_pending = 1; }
    else if (!strncmp(s, "AUTO=", 5) && number(s + 5, 0, 1, &v)) { auto_dim = v; brightness_pending = 1; }
    else if (!strcmp(s, "STOP")) alarm_latched = 0;
    else if (!strncmp(s, "SETTIME=", 8)) return safe_clock_command(s);
    else if (sscanf(s, "ALARM=%d:%d,%d%n", &hh, &mm, &en, &n) == 3 && !s[n] &&
        hh >= 0 && hh < 24 && mm >= 0 && mm < 60 && (en == 0 || en == 1)) {
        alarm_hour = hh; alarm_minute = mm; alarm_enabled = en; alarm_latched = 0;
        alarm_day = 0;
    } else return 0;
    return 1;
}
static void screen_receive(void)
{
    static char line[96];
    static unsigned used;
    static int dropping;
    int b;
    if (screen_ring.overflow) { used = 0; dropping = 1; screen_ring.overflow = 0; }
    while ((b = take(&screen_ring)) >= 0) {
        if (b == '\n') {
            if (!dropping && used) { line[used] = 0; handle_screen(line); }
            used = 0; dropping = 0;
        } else if (b != '\r') {
            if (b < 32 || b > 126 || used + 1 >= sizeof(line)) dropping = 1;
            if (!dropping) line[used++] = (char)b;
        }
    }
}
static void screen_refresh(void)
{
    static unsigned field;
    char text[72];
    int desired = auto_dim && light_ok ? SCREEN_MIN_BRIGHTNESS + light_pct * (100 - SCREEN_MIN_BRIGHTNESS) / 100 : manual_dim;
    if (brightness_pending || abs(desired - current_dim) >= 3) {
        snprintf(text, sizeof(text), "dim=%d", desired);
        if (screen_command(text)) { current_dim = desired; brightness_pending = 0; }
    }
    field = (field + 1) % 7;
    if (!active_page) {
        switch (field) {
        case 0: if (dht_ok) snprintf(text, sizeof(text), "%d.%d C", temperature/10, abs(temperature%10)); else strcpy(text,"-- C"); screen_text("tTemp",text); break;
        case 1: if (dht_ok) snprintf(text, sizeof(text), "%d.%d %%", humidity/10, humidity%10); else strcpy(text,"-- %%"); screen_text("tHum",text); break;
        case 2:
            if (co2_ok) snprintf(text,sizeof(text),"%d ppm",co2_ppm);
            else if (env_co2_rx_bytes) snprintf(text,sizeof(text),"RX:%lu",(unsigned long)env_co2_rx_bytes);
            else strcpy(text,"CO2 no RX");
            screen_text("tCO2",text); break;
        case 3:
            screen_text("l3","MQ-138 relative");
            mq_response_text(text,sizeof(text),mq_adc_ok,mq_warmed,mq_mv,MQ138_RESPONSE_REFERENCE_MV);
            screen_text("tHcho",text); break;
        case 4: if (clock_ok) snprintf(text,sizeof(text),"%02d:%02d:%02d",clock_now.hour,clock_now.minute,clock_now.second); else strcpy(text,"Time unset"); screen_text("tTime",text); break;
        case 5: if (light_ok) snprintf(text,sizeof(text),"%d %%",light_pct); else strcpy(text,"-- %%"); screen_text("tLight",text); break;
        default:
            screen_text("tStatus",mq_raw_alarm ? "MQ RAW ALARM" : gas_alarm ? "GAS ALARM" :
                alarm_latched ? "ALARM" : !mq_adc_ok ? "MQ138 ADC fault" : !mq_warmed ? "MQ138 warming up" :
                !CO2_PROTOCOL ? (env_co2_rx_bytes ? "CO2 protocol unknown" : "CO2 no RX / MQ raw") :
                !co2_ok ? "CO2 data invalid" : !MQ138_CALIBRATED ? "MQ138 uncalibrated" :
                !hcho_ok ? "HCHO outside cal range" : network_online ? "Online" : "Offline"); break;
        }
    } else if (active_page == 1) {
        if (field == 0) { weather_text(text,sizeof(text),weather_ok,weather_temp,weather_code); screen_text("tWeather",text); }
        if (field == 1) screen_text("tWear",weather_ok ? clothing(weather_temp,weather_code) : "--");
        if (field == 2) {
            snprintf(text,sizeof(text),"%.16s / Seniverse",WEATHER_CITY);
            screen_text("tCity",text);
        }
    } else if (active_page == 2) {
        if (field == 0) { if (clock_ok) snprintf(text,sizeof(text),"%04d-%02d-%02d %02d:%02d:%02d",clock_now.year,clock_now.month,clock_now.day,clock_now.hour,clock_now.minute,clock_now.second); else strcpy(text,"Time unset"); screen_text("tDate",text); }
        if (field == 1) { snprintf(text,sizeof(text),"%02d:%02d %s",alarm_hour,alarm_minute,alarm_enabled ? "ON" : "OFF"); screen_text("tAlarm",text); }
    } else if (active_page == 3) {
        if (field == 0) screen_value("nDim", current_dim);
        if (field == 1) screen_value("nLight",light_ok ? light_pct : 0);
        if (field == 2) screen_text("tMode",auto_dim ? "Auto" : "Manual");
    }
}

enum { NET_OFF, NET_AT, NET_ECHO, NET_MODE, NET_JOIN, NET_MUX, NET_NTP_CFG,
       NET_IDLE, NET_NTP, NET_CONNECT, NET_PROMPT, NET_RESPONSE, NET_RETRY };
static int net_state = NET_OFF, net_ok, net_error, net_prompt, net_closed, ntp_success;
static unsigned net_started, net_retry_at, last_ntp, last_weather;
static char at_line[192];
static unsigned at_used;
static int at_dropping;
static env_time network_time;
static int network_time_ready;

static void network_reset_flags(void)
{
    net_ok = net_error = net_prompt = net_closed = 0;
}
static int network_send(int state, const char *text, unsigned now)
{
    network_reset_flags();
    if (my_uart.send(WIFI_UART_ID,(const u8 *)text,(u16)strlen(text)) != E_OK) return 0;
    net_state = state; net_started = now; return 1;
}
static void network_fail(unsigned now)
{
    memset(&http_rx,0,sizeof(http_rx));
    at_used = 0; at_dropping = 0;
    net_state = NET_RETRY; network_online = 0; net_retry_at = now;
    weather_ok = 0;
}
static void network_line(const char *line)
{
    if (!strcmp(line,"OK") || !strcmp(line,"ALREADY CONNECTED")) net_ok = 1;
    if (!strcmp(line,"ERROR") || strstr(line,"FAIL") || strstr(line,"busy")) net_error = 1;
    if (!strcmp(line,"CLOSED")) net_closed = 1;
    if (!strcmp(line,"WIFI DISCONNECT")) net_error = 1;
    if (parse_sntp(line,&network_time)) { network_time_ready = 1; ntp_success = 1; }
}
static void network_receive(void)
{
    int b, payload;
    if (wifi_ring.overflow) {
        wifi_ring.overflow = 0; at_dropping = 1; at_used = 0; http_rx.overflow = 1; net_error = 1;
    }
    while ((b = take(&wifi_ring)) >= 0) {
        payload = http_rx.remaining != 0;
        ipd_feed(&http_rx,(unsigned char)b);
        if (payload) continue;
        if (b == '>') net_prompt = 1;
        if (b == '\n') {
            if (!at_dropping) { at_line[at_used] = 0; network_line(at_line); }
            at_used = 0; at_dropping = 0;
        } else if (b != '\r') {
            if (at_used + 1 < sizeof(at_line)) at_line[at_used++] = (char)b;
            else at_dropping = 1;
        }
        if (b == ':' && http_rx.remaining) at_used = 0;
    }
}
static void network_tick(unsigned now)
{
    char command[320];
    int n;
    network_receive();
    if (network_time_ready) {
        clock_store(&network_time); network_time_ready = 0; last_ntp = now;
    }
    if (net_state == NET_OFF) {
        if (!WIFI_NAME[0]) return;
        network_send(NET_AT,"AT\r\n",now); return;
    }
    if (net_state == NET_RETRY) {
        if (now - net_retry_at >= 30000U) network_send(NET_AT,"AT\r\n",now);
        return;
    }
    if (net_error) { network_fail(now); return; }
    if (net_state != NET_IDLE && now - net_started > (net_state == NET_JOIN ? 25000U : 12000U)) { network_fail(now); return; }
    switch (net_state) {
    case NET_AT: if (net_ok) network_send(NET_ECHO,"ATE0\r\n",now); break;
    case NET_ECHO: if (net_ok) network_send(NET_MODE,"AT+CWMODE=1\r\n",now); break;
    case NET_MODE:
        if (net_ok) {
            snprintf(command,sizeof(command),"AT+CWJAP=\"%s\",\"%s\"\r\n",WIFI_NAME,WIFI_PASSWORD);
            network_send(NET_JOIN,command,now);
        } break;
    case NET_JOIN: if (net_ok) network_send(NET_MUX,"AT+CIPMUX=0\r\n",now); break;
    case NET_MUX:
        if (net_ok) {
            snprintf(command,sizeof(command),"AT+CIPSNTPCFG=1,%d,\"%s\"\r\n",SNTP_TIMEZONE,SNTP_SERVER);
            network_send(NET_NTP_CFG,command,now);
        } break;
    case NET_NTP_CFG:
        if (net_ok) { network_online = 1; network_reset_flags(); net_state = NET_IDLE; last_ntp = now - 3600000U; last_weather = now - WEATHER_REFRESH_MS; }
        break;
    case NET_IDLE:
        if (now - last_ntp >= 3600000U) { last_ntp = now; ntp_success = 0; network_send(NET_NTP,"AT+CIPSNTPTIME?\r\n",now); }
        else if (WEATHER_API_KEY[0] && now - last_weather >= WEATHER_REFRESH_MS) {
            memset(&http_rx,0,sizeof(http_rx)); last_weather = now;
            snprintf(command,sizeof(command),"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",WEATHER_HOST,WEATHER_PORT);
            network_send(NET_CONNECT,command,now);
        }
        break;
    case NET_NTP:
        if (net_ok) {
            if (!ntp_success) last_ntp = now - 3600000U + 10000U;
            network_reset_flags(); net_state = NET_IDLE;
        } break;
    case NET_CONNECT:
        if (net_ok) {
            n = snprintf(command,sizeof(command),"GET /v3/weather/now.json?key=%s&location=%s&language=en&unit=c HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",WEATHER_API_KEY,WEATHER_CITY,WEATHER_HOST);
            if (n <= 0 || n >= (int)sizeof(command)) { network_fail(now); break; }
            snprintf(command,sizeof(command),"AT+CIPSEND=%d\r\n",n);
            network_send(NET_PROMPT,command,now);
        } break;
    case NET_PROMPT:
        if (net_prompt) {
            snprintf(command,sizeof(command),"GET /v3/weather/now.json?key=%s&location=%s&language=en&unit=c HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",WEATHER_API_KEY,WEATHER_CITY,WEATHER_HOST);
            network_send(NET_RESPONSE,command,now);
        } break;
    case NET_RESPONSE:
        if (net_closed) {
            if (http_weather(&http_rx,&weather_temp,&weather_code)) { weather_ok = 1; weather_at = now; }
            else weather_ok = 0;
            network_reset_flags(); net_state = NET_IDLE;
        } break;
    default: break;
    }
}

void my_task_init(void)
{
    unsigned now, sensor_at = 0, clock_at = 0, screen_at = 0, co2_poll_at = 0;
    int b, uart_faults = 0, uart_result;
    int screen_ready, wifi_ready, co2_ready;
    buzzer = switch_create(BUZZER_GRP,BUZZER_PIN);
    led = switch_create(LED_GRP,LED_PIN);
    if (buzzer.this) buzzer.set(&buzzer,SWITCH_LOW);
    if (led.this) led.set(&led,SWITCH_LOW);
    dht = dht11_create(DHT11_GRP,DHT11_PIN);
    my_adc.init(GM_ADC); my_adc.init(MQ138_ADC);
    rtc_bus = iic_creat(DS3231_SDA_GRP,DS3231_SDA_PIN,DS3231_SCL_GRP,DS3231_SCL_PIN);
    if (rtc_bus.this) rtc = ds3231_creat(&rtc_bus);
    clock_read(); /* Preserve a running, battery-backed RTC. */
    uart_result = my_uart.init(SCREEN_UART_ID,SCREEN_BAUD,256);
    screen_ready = uart_result == E_OK && my_uart.set_callback(SCREEN_UART_ID,&screen_ring,receive) == E_OK;
    if (!screen_ready) uart_faults |= 1;
    uart_result = my_uart.init(WIFI_UART_ID,WIFI_BAUD,256);
    wifi_ready = uart_result == E_OK && my_uart.set_callback(WIFI_UART_ID,&wifi_ring,receive) == E_OK;
    if (!wifi_ready) uart_faults |= 2;
    uart_result = my_uart.init(CO2_UART_ID,CO2_BAUD,256);
    co2_ready = uart_result == E_OK && my_uart.set_callback(CO2_UART_ID,&co2_ring,co2_receive) == E_OK;
    if (!co2_ready) uart_faults |= 4;
    if (uart_faults && led.this) {
        int code;
        for (code = 1; code <= 3; ++code) {
            int pulse;
            if (!(uart_faults & (1 << (code - 1)))) continue;
            for (pulse = 0; pulse < code + 3; ++pulse) {
                led.set(&led, SWITCH_HIGHT); vTaskDelay(pdMS_TO_TICKS(120));
                led.set(&led, SWITCH_LOW); vTaskDelay(pdMS_TO_TICKS(180));
            }
            vTaskDelay(pdMS_TO_TICKS(600));
        }
    }
    if (screen_ready) { vTaskDelay(pdMS_TO_TICKS(1000)); screen_command("bkcmd=0"); screen_page(0); }
    for (;;) {
        now = HAL_GetTick();
        screen_receive();
        if (co2_ring.overflow) { co2_ring.overflow = 0; co2_rx.used = 0; co2_ok = 0; }
        while (co2_ready && (b = take(&co2_ring)) >= 0) {
            if (CO2_PROTOCOL == 1 && co2_feed(&co2_rx,(unsigned char)b,&co2_ppm)) {
                co2_ok = 1; co2_at = now; ++env_co2_valid_frames;
            }
        }
        if (wifi_ready) network_tick(now);
        if (now - sensor_at >= 2000U) {
            sensor_at = now; sensors_read(now);
            env_min_free_heap = xPortGetMinimumEverFreeHeapSize();
            env_stack_free_words = uxTaskGetStackHighWaterMark(NULL);
        }
        if (now - clock_at >= 1000U) { clock_at = now; clock_read(); }
        if (co2_ready && CO2_PROTOCOL == 1 && now - co2_poll_at >= 2000U) {
            co2_poll_at = now;
            if (my_uart.send(CO2_UART_ID,co2_query,sizeof(co2_query)) == E_OK) ++env_co2_tx_queries;
        }
        alarm_update(now);
        if (screen_ready && now - screen_at >= 200U) { screen_at = now; screen_refresh(); }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
#endif
#endif
