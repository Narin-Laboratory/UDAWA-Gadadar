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

#define DOCSIZE 2048
#define DOCSIZE_MIN 384
#define USE_SERIAL2
#include <libudawa.h>
#include <TimeLib.h>
#include <HardwareSerial.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>


#define CURRENT_FIRMWARE_TITLE "Gadadar"
#define CURRENT_FIRMWARE_VERSION "0.0.1"

const char* settingsPath = "/settings.json";
struct Settings
{
    uint8_t rlyCtrlMd[4];
    uint8_t dtCyc[4];
    unsigned long dtRng[4];
    uint8_t dtCycFS[4];
    unsigned long dtRngFS[4];
    uint8_t pin[4];
    uint32_t lastUpdated;
    uint8_t ON;
    bool dutyState[4];
    unsigned long dutyCounter[4];
    uint16_t intvRecPwrUsg = 1;
    uint16_t intvRecWthr = 1;
    uint16_t intvDevTel = 1;
    uint32_t rlyActDT[4];
    uint32_t rlyActIT[4];
    unsigned long rlyActDr[4];
    unsigned long rlyActITOn[4];
    unsigned long rlyActITOnTs[4];
    bool publishSwitch[4] = {true, true, true, true};

    bool flag_bme280 = false;
    float seaHpa = 1019.00;

    bool flag_syncClientAttr = 0;

    float _volt = 0.0;
    float _amp = 0.0;
    float _watt = 0.0;
    float _ener = -1;
    float _freq = 0.0;
    float _pf = 0.0;

    String httpUname;
    String httpPass;
};

callbackResponse processSaveConfig(const callbackData &data);
callbackResponse processSaveSettings(const callbackData &data);
callbackResponse processSharedAttributesUpdate(const callbackData &data);
callbackResponse processSyncClientAttributes(const callbackData &data);
callbackResponse processReboot(const callbackData &data);
callbackResponse processSetSwitch(const callbackData &data);
callbackResponse processGetSwitchCh1(const callbackData &data);
callbackResponse processGetSwitchCh2(const callbackData &data);
callbackResponse processGetSwitchCh3(const callbackData &data);
callbackResponse processGetSwitchCh4(const callbackData &data);
callbackResponse processSaveConfigCoMCU(const callbackData &data);
callbackResponse processSetPanic(const callbackData &data);
callbackResponse processBridge(const callbackData &data);
callbackResponse processResetConfig(const callbackData &data);
callbackResponse processUpdateSpiffs(const callbackData &data);

void loadSettings();
void saveSettings();
void relayControlBydtCyc();
void relayControlByDateTime();
void relayControlByIntrvl();
void syncClientAttributes();
void publishDeviceAttributes();
void publishDeviceTelemetry();
void setSwitch(String  ch, String state);
void publishSwitch();
void recPowerUsage();
void recWeatherData();
uint8_t activeBeep();
uint32_t micro2milli(uint32_t hi, uint32_t lo);
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void wsSend(StaticJsonDocument<DOCSIZE_MIN> &doc);
void wsSend(StaticJsonDocument<DOCSIZE_MIN> &doc, AsyncWebSocketClient * client);
void wsSendTelemetry();
void wsSendSensors();
void wsSendAttributes();
void updateSpiffs();
double round2(double value);

#endif