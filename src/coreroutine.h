#ifndef COREROUTINE_H
#define COREROUTINE_H

#include "main.h"

struct AlarmMessage
{
    uint16_t code;
    uint8_t color; 
    int32_t blinkCount; 
    uint16_t blinkDelay;
};


#ifdef USE_IOT
struct IoTState{
    TaskHandle_t xHandleIoT = NULL;
    BaseType_t xReturnedIoT;
    SemaphoreHandle_t xSemaphoreThingsboard = NULL;
    bool fSharedAttributesSubscribed = false;
    bool fRebootRPCSubscribed = false;
    bool fStateSaveRPCSubscribed = false;
    bool fSetRelayRPCSubscribed = false;
    bool fFSUpdateRPCSubscribed = false;
    bool fIoTCurrentFWSent = false;
    bool fIoTUpdateRequestSent = false;
    bool fIoTUpdateStarted = false;
    bool fSyncAttributeRPCSubscribed = false;
};
#endif

extern ESP32Time RTC;
#ifdef USE_HW_RTC
extern ErriezDS3231 hwRTC;
#endif
extern TaskHandle_t xHandleAlarm;
extern BaseType_t xReturnedAlarm;
extern QueueHandle_t xQueueAlarm;
extern TaskHandle_t xHandlePowerSensor;
extern TaskHandle_t xHandleRelayControl;

#ifdef USE_CO_MCU
extern SemaphoreHandle_t xSemaphoreSerialCoMCUWrite;
extern SemaphoreHandle_t xSemaphoreSerialCoMCURead;
#endif

#ifdef USE_IOT
extern IoTState iotState;
#ifdef USE_IOT_SECURE
    extern WiFiClientSecure tcpClient;
#else
    extern WiFiClient tcpClient;
#endif
extern Arduino_MQTT_Client mqttClient;
// The SDK setup with 128 bytes for JSON payload and 32 fields for JSON object
extern ThingsBoard tb;
#endif

#ifdef USE_LOCAL_WEB_INTERFACE
    extern AsyncWebServer http;
    extern AsyncWebSocket ws;
    extern SemaphoreHandle_t xSemaphoreWSBroadcast;
    extern std::map<uint32_t, bool> wsClientAuthenticationStatus;
    extern std::map<IPAddress, unsigned long> wsClientAuthAttemptTimestamps; 
    extern std::map<uint32_t, String> wsClientSalts;
#endif

void reboot(int countDown);
void coreroutineSetFInit(bool fInit);
void coreroutineSetup();
void coreroutineLoop();
void coreroutineDoInit();
void coreroutineStartServices();
void coreroutineStopServices();
void coreroutineCrashStateTruthKeeper(uint8_t direction);
void coreroutineRTCUpdate(long ts);
void coreroutineSetAlarm(uint16_t code, uint8_t color, int32_t blinkCount, uint16_t blinkDelay);
static void coreroutineAlarmTaskRoutine(void *arg);
void coreroutineSetLEDBuzzer(uint8_t color, uint8_t isBlink, int32_t blinkCount, uint16_t blinkDelay);
#ifdef USE_WIFI_OTA
    void coreroutineOnWiFiOTAStart();
    void coreroutineOnWiFiOTAEnd();
    void coreroutineOnWiFiOTAProgress(unsigned int progress, unsigned int total);
    void coreroutineOnWiFiOTAError(ota_error_t error);
#endif
void coreroutineFSDownloader();
void coreroutineSaveAllStorage();
void coreroutineSyncClientAttr(uint8_t direction);
#ifdef USE_LOCAL_WEB_INTERFACE
    String hmacSha256(String htP, String salt);
    void wsBcast(JsonDocument &doc);
    void coreroutineOnWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
#endif
#ifdef USE_IOT
    void coreroutineProcessTBProvResp(const JsonDocument &data);
    void coreroutineRunIoT();
    void coreroutineIoTProvRequestTimedOut();
    bool iotSendAttr(JsonDocument &doc);
    bool iotSendTele(JsonDocument &doc);
    void iotUpdateStartingCallback();
    void iotProgressCallback(const size_t & currentChunk, const size_t & totalChuncks);
    void iotFinishedCallback(const bool & success);
#endif

#ifdef USE_CO_MCU
void coreroutineSerialWriteToCoMcu(JsonDocument &doc, bool isRpc, int wait = 50);
void coreroutineSerialReadFromCoMcu(JsonDocument &doc);
void coreroutineSyncConfigCoMCU();
void coreroutineCoMCUSetBuzzer(int32_t beepCount, uint16_t beepDelay);
void coreroutineCoMCUSetLed(uint8_t r, uint8_t g, uint8_t b, uint8_t isBlink, int32_t blinkCount, uint16_t blinkDelay);
void coreroutineSetCoMCUPin(uint8_t pin, uint8_t op, uint8_t mode, uint16_t aval, uint8_t state);
#endif

void coreroutinePowerSensorTaskRoutine(void *arg);
void coreroutineRelayControlTaskRoutine(void *arg);
void coreroutineSetRelay(uint8_t index, bool output);

#endif