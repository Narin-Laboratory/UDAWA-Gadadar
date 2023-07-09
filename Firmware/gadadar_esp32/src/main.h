/**
 * UDAWA - Universal Digital Agriculture Watering Assistant
 * Firmware for Actuator 4Ch UDAWA Board (Gadadar)
 * Licensed under aGPLv3
 * Researched and developed by PRITA Research Group & Narin Laboratory
 * prita.undiknas.ac.id | narin.co.id
**/
#ifndef main_h
#define main_h
#include <Arduino.h>
static const char* CA_CERT PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIGSjCCBDKgAwIBAgIJAMxU3KljbiooMA0GCSqGSIb3DQEBCwUAMIGwMQswCQYD
VQQGEwJJRDENMAsGA1UECAwEQmFsaTEQMA4GA1UEBwwHR2lhbnlhcjEhMB8GA1UE
CgwYQ1YuIE5hcmF5YW5hIEluc3RydW1lbnRzMSQwIgYDVQQLDBtOYXJpbiBDZXJ0
aWZpY2F0ZSBBdXRob3JpdHkxFjAUBgNVBAMMDU5hcmluIFJvb3QgQ0ExHzAdBgkq
hkiG9w0BCQEWEGNlcnRAbmFyaW4uY28uaWQwIBcNMjAwMTE2MDUyMjM1WhgPMjA1
MDAxMDgwNTIyMzVaMIGwMQswCQYDVQQGEwJJRDENMAsGA1UECAwEQmFsaTEQMA4G
A1UEBwwHR2lhbnlhcjEhMB8GA1UECgwYQ1YuIE5hcmF5YW5hIEluc3RydW1lbnRz
MSQwIgYDVQQLDBtOYXJpbiBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkxFjAUBgNVBAMM
DU5hcmluIFJvb3QgQ0ExHzAdBgkqhkiG9w0BCQEWEGNlcnRAbmFyaW4uY28uaWQw
ggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQC7SU2ahwCe1KktoaUEQLmr
E91S2UwaqGvcGksy9j08GnI1NU1MpqsVrPSxuLQRr7ww2IG9hzKN0rKIhkXUfBCJ
X8/K7bxEkLl2yfcJjql90/EdAjWClo3CURM1zIqzxggkZKmdEGrPV/WGkYchmxuT
QvYDoPVLScXhN7NtTfzd3x/zWwe4WHg4THpfqeyE6vCLoNeDKvF2GsP0xsYtows8
pTjKH9gh0kFi+aYoVxjbH8KB78ktWAOo1T2db3KUF4/gYweYk4b/vS1egdDm821/
6qC7XrsnaApyRm73RKtmhAzldx9D1YqdVbFIx5oRlEg3+uI7hv/YD6Icfhazw1ql
Su8U7g8Ax8OPVdjdJ41lgkFs+OpY4GnfzjIzhvQ+kRIPyDQQaLwCxFZJZIa2jAnB
R5GzSM+Py0d+oStELotd0O3kLC7z4eFdxfRuaaQzofn/aUT1K7NsbG8V7rC3lG9P
8Jc+SU7zP/XSqVjKFTzRnIQ6C4WwkdWwS7uN2FSQBmlIw4EHaYcoMbQCVN2DcGFE
iMH+8/kp99UBKCu2MKq1zM+W0n+dNJ65EdeygOw580EIdNY5DPbpQTeNMt1idXF2
8C9jMyInGt6ZgZ5IfjExfDDIb6WYl3KRmZxqXjWCMg1e5TwMrbeoeg3R6kFY3lG4
TReG7Zjp1PQ3vxMC4ZWcpQIDAQABo2MwYTAdBgNVHQ4EFgQU4rkKARnad9D4bdWs
t3Jo8KKTxHAwHwYDVR0jBBgwFoAU4rkKARnad9D4bdWst3Jo8KKTxHAwDwYDVR0T
AQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAYYwDQYJKoZIhvcNAQELBQADggIBAJAY
pl1fdOG8GpvTIDts/H1CZbT6bF85+FnAV6abL/X4dIx/sQpg9TASAicyMzqUtDSb
2WaD3tkDvFhu7/vzG62x0tcpGw99Bxy0pkSkO9J3KrrxxiK21810aZnxOLZFuvgJ
2O/jugBK8MdCemBpmX93imgXSLJSXJf5/yVcETXFGUmf5p7Ze3Wdi0AxFvjPe2yN
D2MFvp2dtJ9mFaGaCG9v5wjyVVZM+oTGI9JzfNURq//qYWX+Tz9HJVNVeuYvEUnb
LXe9FVZej1+RVsBut9eCyo4GWOqMgRWp/dyMKz3shHFec0pc0fluo7yQH82OonoM
ZzkqgKVmkP5LVW0WqrDKbPTmpsq3ISYJwe7Msnu5D47iUnuc22axPzOH7ZRXE+2n
1Vkig4iYxz2IFZCwO3Ei9LxDlaJh+juHNnS0ziosDrTw0c/VWjkV+XwhRhfNq+Cx
crjMwThsIrz+JXrTihppMvSQhJHjIB/KoiUsa63qVsv6JA+yeBwvthwJ4Kl2ioDg
rH2VNKMU9e/dsWRqfRdUxH29pyJHQFjlv8MXWlFrKgoyrOLN2wkO2RJdKm0hblZW
vh+RH1AiwshFKw9rxdUXJBGGVgn5F0Ie4alDI8ehelOpmrZgFYMOCpcFSpJ5vbXM
ny6l9/duT2POAsUN5IwHGDu8b2NT+vCUQRFVHY31
-----END CERTIFICATE-----
)EOF";
#define CURRENT_FIRMWARE_TITLE "Gadadar"
#define CURRENT_FIRMWARE_VERSION "0.0.9"
#define DOCSIZE 2048
#define DOCSIZE_MIN 512
#define DOCSIZE_SETTINGS 4096
#define USE_SERIAL2
#define USE_WEB_IFACE
#define USE_ASYNC_WEB
#define USE_WIFI_OTA
//#define USE_SDCARD_LOG
#define USE_SPIFFS_LOG
#define STACKSIZE_WIFIKEEPER 3000
#define STACKSIZE_SETALARM 3700
#define STACKSIZE_WIFIOTA 4096
#define STACKSIZE_TB 12000
#define STACKSIZE_IFACE 3000
#define STACKSIZE_POWERSENSOR 4500
#define STACKSIZE_WEATHERSENSOR 4500
#define STACKSIZE_PUBLISHDEVTEL 6000
#define STACKSIZE_WSSENDTELEMETRY 6000
#define STACKSIZE_RELAYCONTROL 4500

#include <libudawa.h>
#include <TimeLib.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include <BME280I2C.h>
#include <Statistical.h>

const char* settingsPath = PSTR("/settings.json");
const char* statesPath = PSTR("/states.json");
struct Settings
{
    uint8_t pR[4];
    uint8_t ON;
    uint16_t itP = 900;
    uint16_t itW = 300;
    uint16_t itD = 60;
    uint16_t itPc = 1;
    uint16_t itWc = 1;
    uint16_t itDc = 1;
    uint8_t cpM[4];
    uint8_t cp1A[4];
    unsigned long cp1B[4];
    uint32_t cp2A[4];
    unsigned long cp2B[4];
    String cp3A[4];
    uint32_t cp4A[4];
    unsigned long cp4B[4];
    unsigned long cp4BTs[4];

    float seaHpa = 1019.00;

    char lbl[4][16];
};

struct States
{
    uint8_t cp0A[4];
    uint32_t cp0B[4];
    bool dutyState[4];
    unsigned long stateOnTs[4];
    unsigned long dutyCounter[4];

    bool publishSwitch[4] = {true, true, true, true};

    bool flag_weatherSensor = false;
    bool flag_powerSensor = false;
    bool flag_resetPowerSensor = false;
};
States myStates;

#ifdef USE_WEB_IFACE
struct WSPayloadPowerSensor
{
    float volt;
    float amp;
    float watt;
    float ener;
    float freq;
    float pf;
};
QueueHandle_t xQueueWsPayloadPowerSensor;

struct WSPayloadWeatherSensor
{
    float celc;
    float rh;
    float alt;
    float hpa;
};
QueueHandle_t xQueueWsPayloadWeatherSensor;
#endif

#define S1_TX 32
#define S1_RX 4

using namespace libudawa;
Settings mySettings;

BaseType_t xReturnedWsSendTelemetry;
BaseType_t xReturnedPowerSensor;
BaseType_t xReturnedWeatherSensor;
BaseType_t xReturnedPublishDevTel;
BaseType_t xReturnedRelayControl;

TaskHandle_t xHandleWsSendTelemetry = NULL;
TaskHandle_t xHandleWeatherSensor = NULL;
TaskHandle_t xHandlePowerSensor = NULL;
TaskHandle_t xHandlePublishDevTel = NULL;
TaskHandle_t xHandleRelayControl = NULL;

SemaphoreHandle_t xSemaphorePowerSensor = NULL;
SemaphoreHandle_t xSemaphoreStates = NULL;

unsigned long powerSensorTR_last_activity = millis();

void loadSettings();
void saveSettings();
void loadStates();
void saveStates();
void relayControlCP0();
void relayControlCP1();
void relayControlCP2();
void relayControlCP3();
void relayControlCP4();
void relayControlTR(void *arg);
void setSwitch(String  ch, String state);
void powerSensorTR(void *arg);
void weatherSensorTR(void *arg);
void attUpdateCb(const Shared_Attribute_Data &data);
void onTbConnected();
void onTbDisconnected();
void setPanic(const RPC_Data &data);
RPC_Response genericClientRPC(const RPC_Data &data);
void onReboot();
void stateReset(bool resetOpMode);
void onAlarm(int code);
void onSyncClientAttr(uint8_t direction);
#ifdef USE_WEB_IFACE
void onWsEvent(const JsonObject &data);
void wsSendTelemetryTR(void *arg);
#endif
void publishDeviceTelemetryTR(void * arg);
void onMQTTUpdateStart();
void onMQTTUpdateEnd();
void publishSwitch();

/**
 * @brief UDAWA Common Alarm Code Definition
 *   110 Light sensor
 *      110 = The light sensor failed to initialize; please check the module integration and wiring.
 *      111 = The light sensor measurement is abnormal; please check the module integrity.
 *      112 = The light sensor measurement is showing an extreme value; please monitor the device's operation closely.
 *
 *   120 Weather sensor
 *      120 = The weather sensor failed to initialize; please check the module integration and wiring.
 *      121 = The weather sensor measurement is abnormal; The ambient temperature is out of range.
 *      122 = The weather sensor measurement is showing an extreme value; The ambient temperature is exceeding 40°C; please monitor the device's operation closely.
 *      123 = The weather sensor measurement is showing an extreme value; The ambient temperature is less than 17°C; please monitor the device's operation closely.
 *      124 = The weather sensor measurement is abnormal; The ambient humidity is out of range.
 *      125 = The weather sensor measurement is showing an extreme value; The ambient humidity is nearly 100%; please monitor the device's operation closely.
 *      126 = The weather sensor measurement is showing an extreme value; The ambient humidity is below 20%; please monitor the device's operation closely.
 *      127 = The weather sensor measurement is abnormal; The barometric pressure is out of range.
 *      128 = The weather sensor measurement is showing an extreme value; The barometric pressure is more than 1010hPa; please monitor the device's operation closely.
 *      129 = The weather sensor measurement is showing an extreme value; The barometric pressure is less than 100hPa; please monitor the device's operation closely.
 *
 *   130 SD Card
 *      130 = The SD Card failed to initialize; please check the module integration and wiring.
 *      131 = The SD Card failed to attatch; please check if the card is inserted properly.
 *      132 = The SD Card failed to create log file; please check if the card is ok.
 *      133 = The SD Card failed to write to the log file; please check if the card is ok.
 * 
 *   140 AC Power sensor
 *      140 = The power sensor failed to initialize; please check the module integration and wiring.
 *      141 = The power sensor measurement is abnormal; The voltage reading is out of range.
 *      142 = The power sensor measurement is abnormal; The current reading is out of range.
 *      143 = The power sensor measurement is abnormal; The power reading is out of range.
 *      144 = The power sensor measurement is abnormal; The power factor and frequency reading is out of range.
 *      145 = The power sensor measurement is showing an overlimit; Please check the connected instruments.
 * 
 *   150 Real Time Clock
 *      150 = The device timing information is incorrect; please update the device time manually. Any function that requires precise timing will malfunction!
 * 
 *   210 Switch Relay
 *      211 = Switch number one is active, but the power sensor detects no power utilization. Please check the connected instrument to prevent failures.
 *      212 = Switch number two is active, but the power sensor detects no power utilization. Please check the connected instrument to prevent failures.
 *      213 = Switch number three is active, but the power sensor detects no power utilization. Please check the connected instrument to prevent failures.
 *      214 = Switch number four is active, but the power sensor detects no power utilization. Please check the connected instrument to prevent failures.
 *      215 = All switches are inactive, but the power sensor detects large power utilization. Please check the device relay module to prevent relay malfunction.
 *      216 = Switch numner one is active for more than one hour!
 *      217 = Switch number two is active for more than one hour!
 *      218 = Switch number three is active for more than one hour!
 *      219 = Switch number four is active for more than one hour! 
 * 
 */
#endif