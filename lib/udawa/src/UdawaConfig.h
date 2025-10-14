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

  char logIP[16] = DEFAULT_LOG_IP;
  uint16_t logPort = logPort;

  bool LEDOn = false;
  uint8_t pinLEDR = pinLEDR;
  uint8_t pinLEDG = pinLEDG;
  uint8_t pinLEDB = pinLEDB;
  uint8_t pinBuzz = pinBuzz;

  #ifdef USE_CO_MCU
  uint8_t s2rx = s2rx;
  uint8_t s2tx = s2tx;
  uint16_t coMCUBuzzFreq = 1600;
  bool coMCUFBuzzer = true;
  uint8_t coMCUPinBuzzer = 2;
  uint8_t coMCUPinLEDR = coMCUPinLEDR;
  uint8_t coMCUPinLEDG = coMCUPinLEDG;
  uint8_t coMCUPinLEDB = coMCUPinLEDB;
  uint8_t coMCULON = coMCULON;
  uint8_t coMCURelayPin[4] = {7, 8, 8, 10};
  #endif
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