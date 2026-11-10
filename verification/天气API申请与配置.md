# 心知天气 API 申请与配置

当前固件适配心知天气 V3 天气实况接口 `/v3/weather/now.json`。
查询城市由 `firmware/LogicLayer/logic/pin_conf.h` 的 `WEATHER_CITY` 配置，当前为杭州 `hangzhou`。

## 账号操作

1. 打开 https://www.seniverse.com/signup 注册，或使用已有账号登录。
2. 注册页需要邮箱、账号密码、手机号和短信验证码，由账号持有人本人完成。
3. 进入 https://www.seniverse.com/dashboard ，在“产品管理”添加包含“天气实况”权限的 API 产品。优先查看控制台当前提供的免费选项。
4. 打开产品详情，查看 API 密钥。本固件使用“私钥”（参数 `key`），不是“公钥”（参数 `uid`）。
5. 把私钥填入 `pin_conf.h` 中的 `WEATHER_API_KEY` 双引号内，重新编译并烧录新 HEX。账号密码和短信验证码不需要填入工程。

## 当前网络参数

- GitHub 源码版 Wi-Fi 名称、密码和 API Key 已清空，需要在 `pin_conf.h` 中填写。
- 使用 2.4GHz 网络，ESP8266 为 AT 固件，USART2 波特率 115200。
- 查询城市默认为杭州；私钥由使用者自行配置，不要提交到 GitHub。
- 固件每 10 分钟查询一次天气。将 API Key 清空时可以尝试连接 Wi-Fi 和校时，但不查询天气。
- 当前请求为 TCP/HTTP，端口 80。已在电脑验证账号权限、HTTP 200 响应和固件解析；HTTPS-only 或 chunked 响应目前不能直接使用。
- 免费天气实况按官方文档返回天气文字、天气代码和气温，能提供当前程序所用字段。天气页城市旁显示 `Seniverse` 作为心知天气来源标注。
- 天气页现在把现象代码转成“晴、多云、阴、小雨、雷阵雨、雪、雾”等中文名称，并保留温度。未知代码显示“未知”，无有效数据或超过 30 分钟显示“天气暂不可用”；此项是实况而非降雨预报。需同步更新 STM32 HEX 与 TFT 中文字库。
- 仅完成编译不代表板上已连接 Wi-Fi 或查到真实天气。

## 实际接口验证

2026-11-06 电脑通过直连 TCP 发送与固件相同的 HTTP/1.0 请求，返回 HTTP 200，城市为 `Hangzhou`。响应共 683 字节，非 chunked，JSON 天气数据的最后更新时间为 `2026-11-06T14:44:11+08:00`，杭州温度 17°C、天气代码 30。

`tests/test_live_weather.c` 将捕获的真实 HTTP 响应包装为 ESP8266 `+IPD` 字节流，调用固件现有 `ipd_feed` 与 `http_weather` 验证通过。响应快照为 `verification/weather-live-response.http`，测试摘要为 `verification/weather-live-test.json`，两者不含请求私钥。这不代表实体模块已完成联网。

## 官方参考

- 查看密钥：https://docs.seniverse.com/api/start/key.html
- 天气实况：https://docs.seniverse.com/api/weather/now.html
- 天气代码：https://docs.seniverse.com/api/start/code.html
- 使用说明：https://docs.seniverse.com/api/start/start.html
