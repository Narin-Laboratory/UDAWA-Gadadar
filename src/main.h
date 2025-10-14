#ifndef MAIN_H
#define MAIN_H

#define countof(a) (sizeof(a) / sizeof(a[0]))

#include "params.h"
#include "secret.h"

#include "UdawaConfig.h"
#include "UdawaWiFiHelper.h"
#include "UdawaLogger.h"
#include "UdawaSerialLogger.h"
#include "UdawaWiFiLogger.h"
#include <LittleFS.h>
#include <NTPClient.h>
#include <ESP32Time.h>
#ifdef USE_I2C
#include <Wire.h>
#endif
#ifdef USE_HW_RTC
#include <ErriezDS3231.h>
#endif
#ifdef USE_WIFI_OTA
#include <ArduinoOTA.h>
#endif
#include <ESPmDNS.h>
#include <ArduinoHttpClient.h>
#include <Update.h>
#ifdef USE_LOCAL_WEB_INTERFACE
#include <../lib/Crypto/src/Crypto.h>
#include <../lib/Crypto/src/SHA256.h>
#include <mbedtls/sha256.h>
#include <base64.h>
#include <map>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "mbedtls/md.h"
#include "mbedtls/base64.h"
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#endif

#ifdef USE_IOT
#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include <Provision.h>
#include <Attribute_Request.h>
#include <Shared_Attribute_Callback.h>
#include <Shared_Attribute_Update.h>
#include <OTA_Update_Callback.h>
#include <RPC_Callback.h>
#include <Server_Side_RPC.h>
#include <OTA_Firmware_Update.h>
#include <Espressif_Updater.h>
#ifdef USE_IOT_SECURE
#include <WiFiClientSecure.h>
#else
#include <WiFiClient.h>
#endif
#ifdef THINGSBOARD_ENABLE_STREAM_UTILS
#include <StreamUtils.h>
#endif
#endif

#include <PZEM004Tv30.h>
#ifndef USE_CO_MCU
#include "PCF8575.h"
#endif
#include "storage.h"
#include "logging.h"
#include "networking.h"
#include "coreroutine.h"

#endif