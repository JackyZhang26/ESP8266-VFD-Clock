#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <stdio.h>

Ticker ticker;

int8_t din   = 12; // DA SDI SPI数据输入
uint8_t clk   = 14; // CK CLK SPI时钟
uint8_t cs    = 15; // CS SPI 片选
uint8_t Reset = 4; // RS VFD屏幕的复位 低电平有效，正常使用拉高/无reset是因为模组内置RC 硬件复位电路
//uint8_t en    = 0; // EN VFD模组电源部分使能，EN 高电平使能，建议置高后100ms 以上再进行 VFD初始化命令发送，避免模块电源还没稳定就发出命令，若不用此功能，直接VCC短接/无EN 是因为模组EN与VCC已经内部链接好
const int analogInPin = A0;
int lightValue = 0;
int lightCommand = 0xff;
volatile int lightMode = 0;
volatile int mode = 0;
volatile int timeForm = 0;

const char *ntpServer1 = "ntp.ustc.edu.cn";
const char *ntpServer2 = "ntp.ntsc.ac.cn";
const char *ntpServer3 = "ntp.aliyun.com";
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;

unsigned char  ziku_data[][5]  ={  //字模文件取模应由左下角开始，先由下至上，再由左至右
 0x24,0x3A,0x2B,0x7E,0x2A, // 0年
 0x40,0x3F,0x15,0x55,0x7F, // 1月
 0x00,0x7F,0x49,0x49,0x7F, // 2日
 0x3E,0x2A,0x3E,0x54,0x7F, // 3时
 0x48,0x2C,0x1A,0x49,0x7A, // 4分
 0x00,0x00,0x00,0x00,0x00, // 5空白
 0x41,0x22,0x10,0x08,0x07, // 7
 0x08,0x44,0x26,0x15,0x0c, // 8
 0x14,0x14,0x7f,0x0a,0x0a, // 9
 };
 
void spi_write_data(unsigned char w_data)
{
  unsigned char i;
  for (i = 0; i < 8; i++)
  {
    digitalWrite(clk, LOW);
    //若使用处理速度较快的 arduino设备 如ESP32 请加一些延时，VFD SPI时钟频率如手册描述 0.5MHZ
    if ( (w_data & 0x01) == 0x01)
    {
      digitalWrite(din, HIGH);
    }
    else
    {
      digitalWrite(din, LOW);
    }
    w_data >>= 1;
    
    digitalWrite(clk, HIGH);
  }
}

void VFD_cmd(unsigned char command)
{
  digitalWrite(cs, LOW);
  spi_write_data(command);
  digitalWrite(cs, HIGH);
  delayMicroseconds(5);
}

void VFD_show(void)
{
  digitalWrite(cs, LOW);//开始传输
  spi_write_data(0xe8);//开显示命令
  digitalWrite(cs, HIGH); //停止传输
}

void VFD_init()
{
  //SET HOW MANY digtal numbers
  //设置显示位数
  digitalWrite(cs, LOW);
  spi_write_data(0xe0);
  delayMicroseconds(5);
  spi_write_data(0x05);//6 digtal 0x05 // 8 digtal 0x07//16 digtal 0x0f
  digitalWrite(cs, HIGH);
  delayMicroseconds(5);

  //set bright
  //设置亮度
  digitalWrite(cs, LOW);
  spi_write_data(0xe4);
  delayMicroseconds(5);
  spi_write_data(lightCommand);//0xff max//0x01 min 
  digitalWrite(cs, HIGH);
  delayMicroseconds(5);
}

/******************************
  在指定位置打印一个字符(用户自定义,所有CG-ROM中的)
  x:位置;chr:要显示的字符编码
*******************************/
void VFD_WriteOneChar(unsigned char x, unsigned char chr)
{
  digitalWrite(cs, LOW);  //开始传输
  spi_write_data(0x20 + x); //地址寄存器起始位置
  spi_write_data(chr);//显示内容数据
  digitalWrite(cs, HIGH); //停止传输
  VFD_show();
}
/******************************
  在指定位置打印字符串
  (仅适用于英文,标点,数字)
  x:位置;str:要显示的字符串
*******************************/
void VFD_WriteStr(unsigned char x, char *str)
{
  digitalWrite(cs, LOW);  //开始传输
  spi_write_data(0x20 + x); //地址寄存器起始位置
  while (*str)
  {
    spi_write_data(*str); //ascii与对应字符表转换
    str++;
  }
  digitalWrite(cs, HIGH); //停止传输
  VFD_show();
}

/******************************
  清屏(by J.Z.)
*******************************/
void VFD_clear(){
  digitalWrite(Reset, LOW);
  delayMicroseconds(5);
  digitalWrite(Reset, HIGH);
  VFD_init();
}

/******************************
  在指定位置滚动显示字符串(by J.Z.)
  delaytime:滚动间隔时间;*str:要显示的字符串
*******************************/
void VFD_RollStr(int delaytime,char *str)
{
  int i,len;
  len = strlen(str);
  for(i=6;i>=0;i--){
    VFD_WriteStr(i,str);
    delay(delaytime);
    VFD_clear();
  }
  for(i=1;i<len;i++){
    VFD_WriteStr(0,str+i);
    delay(delaytime);
    VFD_clear();
  }
}

/******************************
  在指定位置打印自定义字符
  
  x:位置，有ROOM位置;*s:要显示的字符字模
*******************************/
void VFD_WriteUserFont(unsigned char x, unsigned char y,unsigned char *s)
{
  unsigned char i=0;
  unsigned char ii=0;
  digitalWrite(cs, LOW); //开始传输 
    spi_write_data(0x40+y);//地址寄存器起始位置
    for(i=0;i<7;i++)
    spi_write_data(s[i]);
  digitalWrite(cs, HIGH); //停止传输

  digitalWrite(cs, LOW);
  spi_write_data(0x20+x);
  spi_write_data(0x00+y);   
  digitalWrite(cs, HIGH);

  VFD_show();
}

/******************************
  显示关于信息功能
*******************************/
void printAboutInfo(){
  VFD_RollStr(200,"VFDCLK by J.Z.");
}

/******************************
  亮度设置功能
*******************************/
void lightSetting(){
  attachInterrupt(digitalPinToInterrupt(5), changeLightMode, FALLING);
  VFD_clear();
  switch(lightMode){
    case 0:
    ticker.attach(1,autoChangeLight);
    VFD_WriteStr(0,"auto");
    break;
    case 1:
    ticker.detach();
    lightCommand=0x40;
    VFD_WriteStr(0,"111111");
    break;
    case 2:
    ticker.detach();
    lightCommand=0x80;
    VFD_WriteStr(0,"222222");
    break;
    case 3:
    ticker.detach();
    lightCommand=0xc0;
    VFD_WriteStr(0,"333333");
    break;
    case 4:
    ticker.detach();
    lightCommand=0xff;
    VFD_WriteStr(0,"444444");
    break;
  }
  delay(1000);
  detachInterrupt(digitalPinToInterrupt(5));
}

/******************************
  显示日历功能
*******************************/
void printCalendar(){
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    VFD_RollStr(200,"Failed to obtain time");
    return;
  }
  char year[5]={0};
  char month[3]={0};
  char day[3]={0};
  VFD_clear();
  if(timeinfo.tm_sec%2){
    sprintf(year, "%5d", timeinfo.tm_year+1900);
    VFD_WriteUserFont(5,5,ziku_data[0]);
    VFD_WriteStr(0,year);
  }
  else{
    sprintf(month, "%2d", timeinfo.tm_mon+1);
    sprintf(day, "%2d", timeinfo.tm_mday);
    VFD_WriteUserFont(2,2,ziku_data[1]);
    VFD_WriteUserFont(5,5,ziku_data[2]);
    VFD_WriteStr(0,month);
    VFD_WriteStr(3,day);
  }
  delay(2000);
}

/******************************
  显示时间功能
*******************************/
void printTime(){
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    VFD_RollStr(200,"Failed to obtain time");
    return;
  }
  char hour[3]={0};
  char min[3]={0};
  char sec[3]={0};
  /*
  strcpy(hour,String(timeinfo.tm_hour).c_str());
  strcpy(min,String(timeinfo.tm_min).c_str());
  strcpy(sec,String(timeinfo.tm_sec).c_str());
  */
  sprintf(hour,"%02d",timeinfo.tm_hour);
  sprintf(min,"%02d",timeinfo.tm_min);
  sprintf(sec,"%02d",timeinfo.tm_sec);
  attachInterrupt(digitalPinToInterrupt(5), changeTimeForm, FALLING);
  VFD_clear();
  switch(timeForm){
    case 0:
    VFD_WriteStr(0,hour);
    VFD_WriteStr(2,min);
    VFD_WriteStr(4,sec);
    break;
    case 1:
    VFD_WriteStr(0,hour);
    VFD_WriteStr(3, min);
    VFD_WriteUserFont(2, 2, ziku_data[3]);
    VFD_WriteUserFont(5, 5, ziku_data[4]);
    break;
    case 2:
    VFD_WriteStr(0,hour);
    if(timeinfo.tm_sec%2){
      VFD_WriteOneChar(2, ':');
    }
    VFD_WriteStr(3,min);
    VFD_WriteOneChar(5, map(timeinfo.tm_sec,0,59,16,21));
    break;
  }
  delay(1000);
  detachInterrupt(digitalPinToInterrupt(5));
}

/***********
  初始化，包含了Wifi连接等设定
*************/
void setup() {
  pinMode(clk, OUTPUT);
  pinMode(din, OUTPUT);
  pinMode(cs, OUTPUT);
  pinMode(Reset, OUTPUT);
  delayMicroseconds(100);
  digitalWrite(Reset, LOW);
  delayMicroseconds(5);
  digitalWrite(Reset, HIGH);
  VFD_init();

  ticker.attach(1, autoChangeLight);

  attachInterrupt(digitalPinToInterrupt(0), changeMode, FALLING);

  WiFiManager wifiManager;

  wifiManager.setAPCallback(configModeCallback);

  wifiManager.autoConnect();
  short int i=0;
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    VFD_WriteOneChar(i, '.');
    i++;
    if(i>=6){
      i=0;
    }
  }
  VFD_RollStr(200,"WiFi connected!");

  ArduinoOTA.begin();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);
}

/***********
  主循环
*************/
void loop() {
  switch(mode){
    case 0:
    printTime();
    break;
    case 1:
    printCalendar();
    break;
    case 2:
    lightSetting();
    break;
    case 3:
    attachInterrupt(digitalPinToInterrupt(5), timeCalibrate, FALLING);
    printAboutInfo();
    detachInterrupt(digitalPinToInterrupt(5));
    break;
    case 4:
    VFD_WriteStr(0, "OTA");
    ArduinoOTA.handle();
    break;
  }
}

/***********
  模式切换
*************/
IRAM_ATTR void changeMode(){
  mode++;
  if(mode>=5){
    mode=0;
  }
}
/***********
  亮度模式切换
*************/
IRAM_ATTR void changeLightMode(){
  lightMode++;
  if(lightMode>=5){
    lightMode=0;
  }
}
/***********
  时间格式切换
*************/
IRAM_ATTR void changeTimeForm(){
  timeForm++;
  if(timeForm>=3){
    timeForm=0;
  }
}
/***********
  亮度自动调节
*************/
void autoChangeLight(){
  lightValue = analogRead(analogInPin);
  lightCommand = map(lightValue,0,1024,0,255);
}
/***********
  进入WiFi配置模式
*************/
void configModeCallback (WiFiManager *myWiFiManager) {
  VFD_RollStr(200,"Entered config mode");
  VFD_RollStr(200,const_cast<char*>((myWiFiManager->getConfigPortalSSID()).c_str()));
}
/***********
  时间校准
*************/
IRAM_ATTR void timeCalibrate(){
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);
}