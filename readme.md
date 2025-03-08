# 概述
- 这是ESP8266-VFD-Clock的源码，由Arduino C编写
- 本项目的制作记录详见[ESP8266 VFD时钟 制作记录|Jacky Zhang的博客](https://jackyzhang26.github.io/2024/02/15/ESP8266%20VFD%E6%97%B6%E9%92%9F%20%E5%88%B6%E4%BD%9C%E8%AE%B0%E5%BD%95/)
- 电路由立创EDA设计，发布在[立创开源硬件平台](https://oshwhub.com/jackyzhang5/esp32c3-vfd_copy)
# 内容
- vfd08
    - vfd08.ino ----商家提供的VFD模块显示驱动示例程序
- VFD_Clock-final.ino ----本项目的源码，补充了一些VFD显示驱动相关的函数
# 功能构想
- [x] 网络对时
- [x] AP配网
    - [] 在AP配网界面添加一些配置功能，e.g. NTP授时服务器设定等
- [x] 自动亮度调节
- [x] 功能切换
- [x] OTA(未测试)
- [ ] 网络获取天气
- [ ] 深度睡眠模式
- [ ] 其他物联网功能...