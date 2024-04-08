#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <time.h>
#include <sys/time.h>
#include <coredecls.h>
#include "SSD1306Wire.h" 
#include "OLEDDisplayUi.h"
#include "Gao_Weatehr.h"
#include "WeatherStationImages.h"
#include "tw.h"
#include "String.h"

Gao_Weatehr Gao_WeatehrClient;

#define TZ 8      // 中国时区为8
#define DST_MN 0  // 默认为0
#define t_logoRotating 120

const int COIN_SECS=0.6;                          // 1秒更新bitcoin

const int I2C_DISPLAY_ADDRESS = 0x3c;  //I2c地址默认

#if defined(ESP8266)
const int SDA_PIN = 0;  //引脚连接
const int SDC_PIN = 2;  //
#endif
SSD1306Wire display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);  // 0.96寸用这个
OLEDDisplayUi ui(&display);

const String WDAY_NAMES[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thurday", "Friday", "Saturday" };                                      //星期

#define TZ_MN ((TZ)*60)  //时间换算
#define TZ_SEC ((TZ)*3600)
#define DST_SEC ((DST_MN)*60)

time_t now;  //实例化时间

bool first = true;                   //首次更新标志
long timeSinceLastWUpdate = 0;       //上次更新后的时间
long timeSinceLastCurrUpdate = 0;    //上次天气更新后的时间

float lastBTC=0;
float currBTC=0;
float remainderBTC=0;
float quotientBTC=0;
int flagBTC = 0;

void drawProgress(OLEDDisplay *display, int percentage, String label);  //提前声明函数
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);

void drawBitcoinPrice(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state);
void logoRotating();

 FrameCallback frames[] = {drawBitcoinPrice};
 int numberOfFrames = 1;

OverlayCallback overlays[] = { drawHeaderOverlay };  //覆盖回调函数
int numberOfOverlays = 1;                            //覆盖数

void setup() {
  Serial.begin(115200);
  Serial.println();

  // 屏幕初始化
  display.init();
  display.clear();
  display.display();

  display.flipScreenVertically();  //屏幕翻转
  display.setContrast(120);        //屏幕亮度

  webconnect();

  ui.setTargetFPS(30);  //刷新频率

  ui.setActiveSymbol(activeSymbole);      //设置活动符号
  ui.setInactiveSymbol(inactiveSymbole);  //设置非活动符号

  // 符号位置
  // 你可以把这个改成TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // 定义第一帧在栏中的位置
  ui.setIndicatorDirection(LEFT_RIGHT);

  // 屏幕切换方向
  // 您可以更改使用的屏幕切换方向 SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
//  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, numberOfFrames);  // 设置框架
//  ui.setTimePerFrame(4000);              //设置切换时间
  ui.setOverlays(overlays, numberOfOverlays);  //设置覆盖

  // UI负责初始化显示
  ui.init();
  display.flipScreenVertically();  //屏幕反转

  configTime(TZ_SEC, DST_SEC, "ntp.ntsc.ac.cn", "ntp1.aliyun.com");  //ntp获取时间，你也可用其他"pool.ntp.org","0.cn.pool.ntp.org","1.cn.pool.ntp.org","ntp1.aliyun.com"
}

void loop() {
  if (first) {  //首次加载
    updateDatas(&display);
    first = false;
  }
  if (millis() - timeSinceLastCurrUpdate > (1000L * COIN_SECS)) {  //bitcoin更新
    Gao_WeatehrClient.httpBitcoin();
    currBTC = Gao_WeatehrClient.getbtcFloat();
    remainderBTC = currBTC-lastBTC;
    quotientBTC = (remainderBTC / lastBTC)*100;
    Serial.printf("\n上次价格:%f",lastBTC);
    Serial.printf("\n本次价格:%f",currBTC);
    Serial.printf("\n涨跌幅:%-0.5f%%",quotientBTC);
    Serial.printf("\n最终结果:%f",remainderBTC);
    if(remainderBTC>0)
      flagBTC=0;
    else if(remainderBTC<0)
      flagBTC=1;
    else 
      flagBTC=2;
    lastBTC=currBTC;
    
    timeSinceLastCurrUpdate = millis();
  }
  int remainingTimeBudget = ui.update();  //剩余时间预;帧更新
  
  if (remainingTimeBudget > 0) {
    //你可以在这里工作如果你低于你的时间预算。
    delay(remainingTimeBudget);
  }
}

void webconnect() {  //Web配网，密码直连将其注释
  display.clear();
  display.drawXbm(0, 0, 128, 64, cqut); //显示重庆理工大学
  display.display();

  WiFiManager wifiManager;  //实例化WiFiManager
  wifiManager.setDebugOutput(true); //关闭Debug
  wifiManager.setConnectTimeout(10); //设置超时

 if (!wifiManager.autoConnect("WeatherClock")) {  //AP模式/设置wifi名称
   Serial.println("\n连接失败并超时");
   //重新设置并再试一次，或者让它进入深度睡眠状态
   ESP.restart();
   delay(1000);
 }
 Serial.println("\mconnected...yo");
 yield();
}

void drawProgress(OLEDDisplay *display, int percentage, String label) {  //绘制进度
  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);  //alignment 对齐
  display->setFont(ArialMT_Plain_10);            //设置字体
  display->drawString(64, 10, label);
  display->drawProgressBar(2, 28, 124, 10, percentage);
  display->display();
}

void updateDatas(OLEDDisplay *display) {                      //首次更新
  drawProgress(display, 0, "Geting Price from URL");
  Gao_WeatehrClient.httpBitcoin();
  
  drawProgress(display, 33, "Geting Weather From XiaRou");
  currBTC = Gao_WeatehrClient.getbtcFloat();
  lastBTC = Gao_WeatehrClient.getbtcFloat();
  
  drawProgress(display, 66, "Geting Bitcoin Price");
  delay(600);
  
  //学院logo旋转动画
  logoRotating();
  delay(500);
}

void drawBitcoinPrice(OLEDDisplay *display, OLEDDisplayUiState *state, int16_t x, int16_t y){  //第一帧：显示bitcoin
//  display->clear();
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(94,30,String(currBTC));
  
  display->drawString(94,0,String(remainderBTC)+'$');
  display->drawXbm(2,26,18,23, bitcoin);
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0,0,String(quotientBTC)+'%');

  switch(flagBTC)
  {
    case 0:display->drawXbm(98,18,24,24,up);break;
    case 1:display->drawXbm(98,18,24,24,down);break;
    case 2:display->drawXbm(98,18,24,12,equal);break;
  }
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState *state){  //绘图页眉覆盖
  now = time(nullptr);
  struct tm *timeInfo;
  timeInfo = localtime(&now);
  char buff[14];
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);

  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(6, 54, String(buff));

  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  String date = WDAY_NAMES[timeInfo->tm_wday];
  display->drawString(122, 54, date);
  
  display->drawHorizontalLine(0, 52, 128);//列、行、长
}

void logoRotating(){//院徽旋转logo动画
  display.clear();
  unsigned char (*p)[1024]=gImage_zcsl;
  for(int j=0;j<3;j++){
    for(uint i=0;i<9;i++)
    {
      display.drawXbm(0,0,128,64,*(p+i));
      delay(t_logoRotating);
      display.display();
      display.clear();
    } 
  }
}

