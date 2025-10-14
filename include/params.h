#ifndef PARAMS_H
#define PARAMS_H
#include <Arduino.h>

#define SERIAL_BAUD_RATE 115200UL
#define COMPILED __DATE__ " " __TIME__
#define CURRENT_FIRMWARE_TITLE "Gadadar"
#define CURRENT_FIRMWARE_VERSION "0.0.1"

#define USE_WIFI_OTA

#define USE_CO_MCU

#define USE_LOCAL_WEB_INTERFACE
#ifdef USE_LOCAL_WEB_INTERFACE
    #define WS_BLOCKED_DURATION 60000UL
    #define WS_RATE_LIMIT_INTERVAL 1000UL
#endif

#define USE_IOT
#ifdef USE_IOT
    #define THINGSBOARD_ENABLE_STREAM_UTILS true
    #define USE_IOT_SECURE
    #define USE_IOT_OTA
    #define IOT_RECEIVE_BUFFER_SIZE   4096UL       // was 2048
    #define IOT_SEND_BUFFER_SIZE      4096UL       // was 2048
    #define IOT_DEFAULT_MAX_STACK_SIZE     4096UL       // was 2048
    #define IOT_DEFAULT_MAX_RESPONSE_SIZE  4096UL       // was 2048
    #define IOT_MAX_ATTRIBUTES             128UL          // was 10 (adjust if needed)
    #define IOT_BUFFERING_SIZE             4096UL       // was 2048
    #define IOT_REQUEST_TIMEOUT_MICROSECONDS 3000000UL  // 3 seconds (was 3000 µs)
    #define IOT_OTA_UPDATE_FAILURE_RETRY 12U
    #define IOT_OTA_UPDATE_PACKET_SIZE 4096U
    #ifdef USE_IOT_SECURE
    #define IOT_STACKSIZE 9000UL
    #else
    #define IOT_STACKSIZE 6000UL
    #endif    
    #define THINGSBOARD_ENABLE_DYNAMIC true
    #define THINGSBOARD_ENABLE_OTA true
    #define THINGSBOARD_ENABLE_DEBUG false
    #define IOT_PROVISIONING_TIMEOUT 10
#endif

#define USE_WIFI_LOGGER
#ifdef USE_WIFI_LOGGER
#define WIFI_LOGGER_BUFFER_SIZE 256UL
#endif

#define USE_I2C
#ifdef USE_I2C
#define USE_HW_RTC
#endif

#define POWERSENSOR_STACKSIZE 8192UL
#define ALARM_STACKSIZE 8192UL
#define RELAYCONTROL_STACKSIZE 8192UL

#define JSON_DOC_SIZE_TINY 256
#define JSON_DOC_SIZE_SMALL 512
#define JSON_DOC_SIZE_MEDIUM 768
#define JSON_DOC_SIZE_LARGE 1024
#define JSON_DOC_SIZE_XLARGE 8096

#define IOEXTENDER_ADDRESS 0x20

#define MAX_CRASH_COUNTER 30
#define SAFEMODE_CLEAR_DURATION 300 // seconds
#define SAFE_CHECKPOINT_TIME 30 //seconds
#define FSTIMESAVER_INTERVAL 1 // hours

#ifdef USE_IOT_SECURE
static const int tbPort = 8883;
static constexpr char tbAddr[] PROGMEM = "prita.undiknas.ac.id";
#else
static const int tbPort = 1883;
static constexpr char tbAddr[] PROGMEM = "udawa.local";
#endif
static constexpr char binURL[] PROGMEM = "http://udawa.or.id/cdn/gadadar/littlefs.bin";
static constexpr char model[] PROGMEM = "Gadadar";
static constexpr char hname[] PROGMEM = "gadadar";
static constexpr char group[] PROGMEM = "Gadadar";
#define DEFAULT_LOG_IP "255.255.255.255"
static const uint8_t logLev = 5;
static const bool fIoT = true;
static const bool fWOTA = true;
static const bool fWeb = true;
static const int gmtOff = 28880;
static const uint16_t logPort = 29514;
static const bool fInit = false;
static const bool LEDOn = false;
static const uint8_t pinLEDR = 27;
static const uint8_t pinLEDG = 14;
static const uint8_t pinLEDB = 12;
static const uint8_t pinBuzz = 32;

const uint8_t s1tx = 26; //Neo 26, V3.1 33, V3 32
const uint8_t s1rx = 25; //Neo 25, V3.1 32, V3 4
const unsigned long intvWeb = 1;
const unsigned long intvAttr = 5;
const unsigned long intvTele = 900;
const int maxWatt = 2000;
const bool relayON = false;
const bool fPowerSensorDummy = false;
const unsigned long powerSensorAlarmTimer = 30;
const std::array<String, 4> availableRelayMode = {PSTR("Manual"), PSTR("Duty Cycle"), PSTR("Time Daily"), PSTR("Specific Datetime")};
const uint8_t maxTimers = 10;

#ifdef USE_CO_MCU
const uint8_t s2rx = 16;
const uint8_t s2tx = 17;
const uint16_t coMCUBuzzFreq = 1600;
const bool coMCUFBuzzer = true;
const uint8_t coMCUPinBuzzer = 2;
const uint8_t coMCUPinLEDR = 3;
const uint8_t coMCUPinLEDG = 5;
const uint8_t coMCUPinLEDB = 6;
const uint8_t coMCULON = 255;
#endif

#endif
