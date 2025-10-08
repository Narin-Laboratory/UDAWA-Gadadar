#ifndef UDAWACONFIG_H
#define UDAWACONFIG_H

#include <ArduinoJson.h>
#include <UdawaLogger.h>
#include <FS.h>
#include <LittleFS.h>
#include "secret.h"
#include "params.h"
#include <functional> 
#include <vector>

struct UdawaConfigStruct{
  bool fInit;
  char hwid[16];
  char name[64];
  char model[64];
  char group[64];
  uint8_t logLev;

  char tbAddr[64];
  uint16_t tbPort;
  char wssid[64];
  char wpass[64];
  char dssid[64];
  char dpass[64];
  char upass[64];
  #ifdef USE_IOT
  bool fIoT;
  char accTkn[32];
  bool provSent;
  char provDK[32];
  char provDS[32];
  #endif
  char binURL[192];

  int gmtOff;

  bool fWOTA;
  bool fWeb;
  char hname[64];
  char htU[64];
  char htP[64];

  char logIP[16] = "255.255.255.255";
  uint16_t logPort = 29514;

  bool LEDOn = false;
  uint8_t pinLEDR = 27;
  uint8_t pinLEDG = 14;
  uint8_t pinLEDB = 12;
  uint8_t pinBuzz = 32;
};

extern SemaphoreHandle_t xSemaphoreConfig; 

class UdawaConfig{
    public:
        UdawaConfig(const char* path);
        bool begin();
        bool load();        
        bool save();
        UdawaConfigStruct state;
    private:
        UdawaLogger *_logger = UdawaLogger::getInstance(LogLevel::VERBOSE);
        const char *_path;
        JsonDocument _data;
        
};

class GenericConfig{
  public:
    GenericConfig(const char* path);
    bool load(JsonDocument &data);
    bool save(JsonDocument &data);
  private:
    UdawaLogger *_logger = UdawaLogger::getInstance(LogLevel::VERBOSE);
    const char *_path;
    JsonDocument _data;
};

#endif