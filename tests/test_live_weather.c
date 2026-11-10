#define ENV_HOST_TEST
#include "../firmware/LogicLayer/logic/my_logic.c"

int main(int argc, char **argv)
{
    FILE *file;
    ipd_parser parser = {0};
    char payload[2048], header[32];
    size_t size, i;
    int temperature, code;
    if (argc != 2) return 2;
    file = fopen(argv[1], "rb");
    if (!file) return 2;
    size = fread(payload, 1, sizeof(payload), file);
    if (ferror(file) || !feof(file) || !size) { fclose(file); return 2; }
    fclose(file);
    snprintf(header, sizeof(header), "+IPD,%u:", (unsigned)size);
    for (i = 0; header[i]; ++i) ipd_feed(&parser, (unsigned char)header[i]);
    for (i = 0; i < size; ++i) ipd_feed(&parser, (unsigned char)payload[i]);
    if (!http_weather(&parser, &temperature, &code)) {
        fprintf(stderr, "FAIL: live weather response rejected\n");
        return 1;
    }
    printf("PASS: live HTTP/IPD weather response, temperature=%d C, code=%d\n",
           temperature, code);
    return 0;
}
