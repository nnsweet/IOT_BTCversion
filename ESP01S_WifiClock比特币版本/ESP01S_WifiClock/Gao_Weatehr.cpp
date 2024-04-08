#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "Gao_Weatehr.h"

Gao_Weatehr::Gao_Weatehr() {
  btcString="-1";
  btcFloat=0;
}

//获取BitCoin实时价格
void Gao_Weatehr::httpBitcoin()  { 
  String URL="https://api.coincap.io/v2/rates/bitcoin";//https://api.coincap.io/v2/rates/bitcoin
  //创建 HTTPClient 对象
  HTTPClient httpClient;
  //设置不加密访问
  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  //配置请求地址。此处也可以不使用端口号和PATH而单纯的
  httpClient.begin(*client,URL); 
  //配置请求头
  // httpClient.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:40.0) Gecko/20100101 Firefox/40.1");
  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.print("\nSend GET request to URL: ");
  Serial.println(URL);
  Serial.print("\nhttpCode: ");
  Serial.println(httpCode);
  if (httpCode == HTTP_CODE_OK) {
    system_soft_wdt_feed();
    String payload = httpClient.getString();
    DynamicJsonDocument  jsonBuffer(1024);
    deserializeJson(jsonBuffer, payload);
    JsonObject root = jsonBuffer.as<JsonObject>();
    String bitCoinPrice_= root["data"]["rateUsd"];
    
    btcString=bitCoinPrice_.substring(0,bitCoinPrice_.indexOf('.')+2);//对s1做分割
    btcFloat=btcString.toFloat();
    Serial.print("\nbtcString:  ");
    Serial.print(bitCoinPrice_);
    Serial.print("\nbtcFloat: ");
    Serial.print(btcFloat);
    
    jsonBuffer.clear();
  } else {
    Serial.println("\nBitcoin Server Respose Code:");
    Serial.println(httpCode);
  }
  //关闭ESP8266与服务器连接
  httpClient.end();
}

String Gao_Weatehr::getbtcString(){
  return btcString;
}
float Gao_Weatehr::getbtcFloat(){
  return btcFloat;
}
