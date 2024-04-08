#pragma once
#include <ArduinoJson.h>


class Gao_Weatehr {
  public:
    Gao_Weatehr();
  public:
    String getbtcString();
    float getbtcFloat();
    void httpBitcoin();
    
  private:
    String btcString;
    float btcFloat;


    
};
