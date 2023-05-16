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
#define CURRENT_FIRMWARE_VERSION "0.0.7"
#define DOCSIZE 2048
#define DOCSIZE_MIN 512
#define DOCSIZE_SETTINGS 4096
#define USE_SERIAL2
#define USE_WEB_IFACE
#define USE_WIFI_OTA
#define STACKSIZE_WIFIKEEPER 2000
#define STACKSIZE_SETALARM 3700
#define STACKSIZE_WIFIOTA 4096
#define STACKSIZE_TB 12000
#define STACKSIZE_IFACE 9000
#define STACKSIZE_RECWEATHERDATA 3100
#define STACKSIZE_RECPOWERUSAGE 3100
#define STACKSIZE_PUBLISHSWITCH 6000
#define STACKSIZE_RELAYCONTROL 9000
#define STACKSIZE_WSSENDTELEMETRY 2200
#define STACKSIZE_WSSENDSENSORS 3000

#include <libudawa.h>
#include <TimeLib.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Statistical.h>

const char* settingsPath = "/settings.json";
struct Settings
{
    uint8_t cpM[4];
    uint8_t cp1A[4];
    unsigned long cp1B[4];
    uint8_t pR[4];
    String cp3A[4];
    uint8_t ON;
    bool dutyState[4];
    unsigned long stateOnTs[4];
    unsigned long dutyCounter[4];
    uint16_t itP = 900;
    uint16_t itW = 300;
    uint16_t itD = 60;
    uint16_t itPc = 1;
    uint16_t itWc = 1;
    uint16_t itDc = 1;
    uint32_t cp2A[4];
    unsigned long cp2B[4];
    uint32_t cp4A[4];
    unsigned long cp4B[4];
    unsigned long cp4BTs[4];
    bool publishSwitch[4] = {true, true, true, true};

    bool flag_bme280 = false;
    float seaHpa = 1019.00;

    char lbl[4][16];
};

#ifdef USE_WEB_IFACE
struct PZEMMessage
{
    float volt;
    float amp;
    float watt;
    float ener;
    float freq;
    float pf;
};
QueueHandle_t xQueuePZEMMessage;

struct BME280Message
{
    float celc;
    float rh;
    float hpa;
    float alt;
};
QueueHandle_t xQueueBME280Message;
#endif

void loadSettings();
void saveSettings();
void relayControlCP1();
void relayControlCP2();
void relayControlCP3();
void relayControlCP4();
void relayControlTR(void *arg);
void setSwitch(String  ch, String state);
void recPowerUsageTR(void *arg);
void recWeatherDataTR(void *arg);
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
void wsSendSensorsTR(void *arg);
#endif
void publishSwitchTR(void * arg);
void onMQTTUpdateStart();
void onMQTTUpdateEnd();
#endif