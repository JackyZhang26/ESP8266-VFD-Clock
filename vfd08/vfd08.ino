//arduino  引脚接法,后面数字根据自己设备IO来更改
//本例子使用ESP8266-12
//本例子使用本店铺6位，8位，12位，16位 5x7 显示
uint8_t din   = 12; // DA SDI SPI数据输入
uint8_t clk   = 14; // CK CLK SPI时钟
uint8_t cs    = 15; // CS SPI 片选
uint8_t Reset = 4; // RS VFD屏幕的复位 低电平有效，正常使用拉高/无reset是因为模组内置RC 硬件复位电路
uint8_t en    = 0; // EN VFD模组电源部分使能，EN 高电平使能，建议置高后100ms 以上再进行 VFD初始化命令发送，避免模块电源还没稳定就发出命令，若不用此功能，直接VCC短接/无EN 是因为模组EN与VCC已经内部链接好

unsigned char  ziku_data[][5]  ={  //字模文件取模应由左下角开始，先由下至上，再由左至右
 0x24,0x3A,0x2B,0x7E,0x2A, // 0年
 0x40,0x3F,0x15,0x55,0x7F, // 1月
 0x00,0x7F,0x49,0x49,0x7F, // 2日
 0x3E,0x2A,0x3E,0x54,0x7F, // 3时
 0x48,0x2C,0x1A,0x49,0x7A, // 4分
 0x00,0x00,0x36,0x36,0x00, // 5：
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
  spi_write_data(0xe8);     //开显示命令
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
  spi_write_data(0xff);//0xff max//0x01 min 
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
  spi_write_data(chr + 0x30);//显示内容数据
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
  在指定位置点亮符号
  12位 模块 带符号 的 可以使用下面 函数 实现符号显示。
  ad_dat:位置，on_off_flag:点亮哪一个符号
*******************************/
void VFD_Write_ADRAM(unsigned char ad_dat , unsigned char on_off_flag)
{
  unsigned char ad_dat_temp;
  digitalWrite(cs, LOW);  //开始传输 
  spi_write_data(0x60 + ad_dat);  //ADRAM
  if(on_off_flag==1)//logo
  {spi_write_data(0x02);}
  else if(on_off_flag==2)//just :
  {spi_write_data(0x01);}
  else if(on_off_flag==3)//logo + :
  {spi_write_data(0x03);}
  else if(on_off_flag==0)//nothing
  {spi_write_data(0x00);}
  digitalWrite(cs, HIGH); //停止传输
  VFD_show();
}
void VFD_clear(){
  digitalWrite(Reset, LOW);
  delayMicroseconds(5);
  digitalWrite(Reset, HIGH);
  VFD_init();
}

void setup() {
  pinMode(en, OUTPUT);
  pinMode(clk, OUTPUT);
  pinMode(din, OUTPUT);
  pinMode(cs, OUTPUT);
  pinMode(Reset, OUTPUT);
  digitalWrite(en, HIGH);
  delayMicroseconds(100);
  digitalWrite(Reset, LOW);
  delayMicroseconds(5);
  digitalWrite(Reset, HIGH);
  VFD_init();
}

void loop() {
  int i;
  for(i=0;i<=6;i++){
    VFD_WriteStr(6-i,"1827..");
    delay(500);
  }
  VFD_clear();
  for(i=0;i<=6;i++){
    VFD_WriteStr(6-i,"5010..");
    delay(500);
  }
  VFD_clear();
  for(i=0;i<=6;i++){
    VFD_WriteStr(6-i,"2756..");
    delay(500);
  }
  VFD_clear();
  for(i=0;i<=6;i++){
    VFD_WriteStr(6-i,"2080..");
    delay(500);
  }
  VFD_clear();
  for(i=0;i<=6;i++){
    VFD_WriteStr(6-i,"BU_YAO");
    delay(500);
  }
  VFD_clear();
  for(i=0;i<=6;i++){
    VFD_WriteStr(6-i,"HUI_DA");
    delay(500);
  }
  VFD_clear();
  // put your main code here, to run repeatedly:
}
