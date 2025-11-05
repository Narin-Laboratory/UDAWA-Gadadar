#ifndef PARAMS_H
#define PARAMS_H
#include <Arduino.h>

#define SERIAL_BAUD_RATE 115200UL
#define COMPILED __DATE__ " " __TIME__
#define CURRENT_FIRMWARE_TITLE "Gadadar"
#define CURRENT_FIRMWARE_VERSION "0.0.1"

#define USE_WIFI_OTA

//#define USE_CO_MCU

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
static const uint8_t pinLEDR = 12;
static const uint8_t pinLEDG = 14;
static const uint8_t pinLEDB = 27;
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

/**
 * @brief UDAWA Common Alarm Code Definition
 * 110 Light sensor
 * 110 = The light sensor failed to initialize; please check the module integration and wiring.
 * 111 = The light sensor measurement is abnormal; please check the module integrity.
 * 112 = The light sensor measurement is showing an extreme value; please monitor the device's operation closely.
 *
 * 120 Weather sensor
 * 120 = The weather sensor failed to initialize; please check the module integration and wiring.
 * 121 = The weather sensor measurement is abnormal; The ambient temperature is out of range.
 * 122 = The weather sensor measurement is showing an extreme value; The ambient temperature is exceeding safe threshold; please monitor the device's operation closely.
 * 123 = The weather sensor measurement is showing an extreme value; The ambient temperature is less than safe threshold; please monitor the device's operation closely.
 * 124 = The weather sensor measurement is abnormal; The ambient humidity is out of range.
 * 125 = The weather sensor measurement is showing an extreme value; The ambient humidity is exceeding safe threshold; please monitor the device's operation closely.
 * 126 = The weather sensor measurement is showing an extreme value; The ambient humidity is below safe threshold; please monitor the device's operation closely.
 * 127 = The weather sensor measurement is abnormal; The barometric pressure is out of range.
 * 128 = The weather sensor measurement is showing an extreme value; The barometric pressure is more than safe threshold; please monitor the device's operation closely.
 * 129 = The weather sensor measurement is showing an extreme value; The barometric pressure is less than safe threshold; please monitor the device's operation closely.
 *
 * 130 SD Card
 * 130 = The SD Card failed to initialize; please check the module integration and wiring.
 * 131 = The SD Card failed to attatch; please check if the card is inserted properly.
 * 132 = The SD Card failed to create log file; please check if the card is ok.
 * 133 = The SD Card failed to write to the log file; please check if the card is ok.
 * * 140 AC Power sensor
 * 140 = The power sensor failed to initialize; please check the module integration and wiring.
 * 141 = The power sensor measurement is abnormal; The voltage reading is out of range.
 * 142 = The power sensor measurement is abnormal; The current reading is out of range.
 * 143 = The power sensor measurement is abnormal; The power reading is out of range.
 * 144 = The power sensor measurement is abnormal; The power factor and frequency reading is out of range.
 * 145 = The power sensor measurement is showing an overlimit; Please check the connected instruments.
 * * 150 Real Time Clock
 * 150 = The device timing information is incorrect; please update the device time manually. Any function that requires precise timing will malfunction!
 * * 210 Switch Relay
 * 210 = Switch number one is active, but the power sensor detects no power utilization. Please check the connected instrument to prevent failures.
 * 211 = Switch number two is active, but the power sensor detects no power utilization. Please check the connected instrument to prevent failures.
 * 212 = Switch number three is active, but the power sensor detects no power utilization. Please check the connected instrument to prevent failures.
 * 213 = Switch number four is active, but the power sensor detects no power utilization. Please check the connected instrument to prevent failures.
 * 214 = All switches are inactive, but the power sensor detects large power utilization. Please check the device relay module to prevent relay malfunction.
 * 215 = Switch numner one is active for more than safe duration!
 * 216 = Switch number two is active for more than safe duration!
 * 217 = Switch number three is active for more than safe duration!
 * 218 = Switch number four is active for more than safe duration! 
 *
 * 220 IOExtender
 * 220 = The IOExtender failed to initialize; please check the module integration and wiring.
 * 
 * 230 Water Quality Sensors (TDS/EC)
 * 230 = The TDS sensor failed to initialize; please check the ADS1115 module integration and wiring.
 * 231 = The TDS sensor measurement is abnormal; The TDS reading is out of range.
 * 232 = The TDS sensor measurement is showing an extreme value; The TDS is exceeding safe threshold (water quality poor).
 * 233 = The TDS sensor measurement is showing an extreme value; The TDS is below safe threshold (water too pure/unstable).
 * 234 = The EC sensor measurement is abnormal; The EC reading is out of range.
 * 235 = The EC sensor measurement is showing an extreme value; The EC is exceeding safe threshold.
 * 236 = The EC sensor measurement is showing an extreme value; The EC is below safe threshold.
 * 
 * 240 Water Temperature Sensor (DS18B20)
 * 240 = The water temperature sensor failed to initialize; please check the DS18B20 wiring and pull-up resistor.
 * 241 = The water temperature measurement is abnormal; The temperature reading is out of range.
 * 242 = The water temperature is exceeding safe threshold; Water may be too hot for intended use.
 * 243 = The water temperature is below safe threshold; Water may be too cold or frozen.
 * */
 
// Standardized Alarm Codes
// Prefer using these constants with coreroutineSetAlarm(ALARM_..., color, blinkCount, blinkDelay)
enum AlarmCode : uint16_t {
	// Generic / none
	ALARM_NONE                            = 0,
	// 110 Light sensor
	ALARM_LIGHT_SENSOR_INIT_FAIL           = 110,
	ALARM_LIGHT_SENSOR_MEAS_ABNORMAL       = 111,
	ALARM_LIGHT_SENSOR_MEAS_EXTREME        = 112,

	// 120 Weather sensor
	ALARM_WEATHER_SENSOR_INIT_FAIL         = 120,
	ALARM_WEATHER_TEMP_OUT_OF_RANGE        = 121,
	ALARM_WEATHER_TEMP_EXCEED_HIGH         = 122,
	ALARM_WEATHER_TEMP_BELOW_LOW           = 123,
	ALARM_WEATHER_HUMID_OUT_OF_RANGE       = 124,
	ALARM_WEATHER_HUMID_EXCEED_HIGH        = 125,
	ALARM_WEATHER_HUMID_BELOW_LOW          = 126,
	ALARM_WEATHER_PRESS_OUT_OF_RANGE       = 127,
	ALARM_WEATHER_PRESS_EXCEED_HIGH        = 128,
	ALARM_WEATHER_PRESS_BELOW_LOW          = 129,

	// 130 SD Card
	ALARM_SD_INIT_FAIL                     = 130,
	ALARM_SD_ATTACH_FAIL                   = 131,
	ALARM_SD_CREATE_LOG_FAIL               = 132,
	ALARM_SD_WRITE_LOG_FAIL                = 133,

	// 140 AC Power sensor
	ALARM_AC_SENSOR_INIT_FAIL              = 140,
	ALARM_AC_VOLTAGE_OUT_OF_RANGE          = 141,
	ALARM_AC_CURRENT_OUT_OF_RANGE          = 142,
	ALARM_AC_POWER_OUT_OF_RANGE            = 143,
	ALARM_AC_PF_FREQ_OUT_OF_RANGE          = 144,
	ALARM_AC_OVERLIMIT                     = 145,

	// 150 Real Time Clock
	ALARM_RTC_TIME_INCORRECT               = 150,

	// 210 Switch Relay
	ALARM_SWITCH1_ACTIVE_NO_POWER          = 210,
	ALARM_SWITCH2_ACTIVE_NO_POWER          = 211,
	ALARM_SWITCH3_ACTIVE_NO_POWER          = 212,
	ALARM_SWITCH4_ACTIVE_NO_POWER          = 213,
	ALARM_ALL_SWITCHES_OFF_POWER_DETECTED  = 214,
	ALARM_SWITCH1_ACTIVE_TOO_LONG          = 215,
	ALARM_SWITCH2_ACTIVE_TOO_LONG          = 216,
	ALARM_SWITCH3_ACTIVE_TOO_LONG          = 217,
	ALARM_SWITCH4_ACTIVE_TOO_LONG          = 218,

	// 220 IO Extender
	ALARM_IOEXTENDER_INIT_FAIL             = 220,

	// 230 Water Quality Sensors (TDS/EC)
	ALARM_TDS_SENSOR_INIT_FAIL             = 230,
	ALARM_TDS_OUT_OF_RANGE                 = 231,
	ALARM_TDS_EXCEED_HIGH                  = 232,
	ALARM_TDS_BELOW_LOW                    = 233,
	ALARM_EC_OUT_OF_RANGE                  = 234,
	ALARM_EC_EXCEED_HIGH                   = 235,
	ALARM_EC_BELOW_LOW                     = 236,

	// 240 Water Temperature Sensor (DS18B20)
	ALARM_WATER_TEMP_SENSOR_INIT_FAIL      = 240,
	ALARM_WATER_TEMP_OUT_OF_RANGE          = 241,
	ALARM_WATER_TEMP_EXCEED_HIGH           = 242,
	ALARM_WATER_TEMP_BELOW_LOW             = 243,
};
#endif
