#include "coreroutine.h"
#include <PZEM004Tv30.h>
#include "PCF8575.h"

PCF8575 IOExtender(0x20);

ESP32Time RTC(0);
#ifdef USE_HW_RTC
ErriezDS3231 hwRTC;
#endif

TaskHandle_t xHandleAlarm = NULL;
BaseType_t xReturnedAlarm;
QueueHandle_t xQueueAlarm = xQueueCreate( 10, sizeof( struct AlarmMessage ) );
TaskHandle_t xHandlePowerSensor = NULL;
TaskHandle_t xHandleRelayControl = NULL;

#ifdef USE_LOCAL_WEB_INTERFACE
AsyncWebServer http(80);
AsyncWebSocket ws(PSTR("/ws"));
std::map<uint32_t, bool> wsClientAuthenticationStatus;
std::map<IPAddress, unsigned long> wsClientAuthAttemptTimestamps; 
std::map<uint32_t, String> wsClientSalts;
SemaphoreHandle_t xSemaphoreWSBroadcast = NULL;
#endif

#ifdef USE_IOT
IoTState iotState;
#ifdef USE_IOT_SECURE
WiFiClientSecure tcpClient;
#else
WiFiClient tcpClient;
#endif
Arduino_MQTT_Client mqttClient(tcpClient);
Provision<> IAPIProv;
Server_Side_RPC<> IAPIRPC;
Shared_Attribute_Update<> IAPISharedAttr;
Attribute_Request<> IAPISharedAttrReq;
Espressif_Updater<> IoTUpdater;
OTA_Firmware_Update<> IAPIOta;
ThingsBoard tb(mqttClient);
#endif

void reboot(int countDown = 0){
  crashState.plannedRebootCountDown = countDown;
  crashState.fPlannedReboot = true;
}

void coreroutineSetup(){
    #ifdef USE_I2C
    Wire.begin();
    Wire.setClock(400000);
    #endif

    coreroutineCrashStateTruthKeeper(1);
    if(crashState.rtcp < SAFE_CHECKPOINT_TIME){
        crashState.crashCnt++;
        if(crashState.crashCnt >= MAX_CRASH_COUNTER){
            crashState.fSafeMode = true;
            logger->warn(PSTR(__func__), PSTR("** SAFEMODE ACTIVATED **\n"));
        }
    }
    if(crashState.fSafeMode){
      logger->warn(PSTR(__func__), PSTR("** SAFEMODE ACTIVATED **\n"));
    }
    logger->debug(PSTR(__func__), PSTR("Runtime Counter: %d, Crash Counter: %d, Safemode Status: %s\n"), crashState.rtcp, crashState.crashCnt, crashState.fSafeMode ? PSTR("ENABLED") : PSTR("DISABLED"));
    
    if (!crashState.fSafeMode){
      logger->info(PSTR(__func__), PSTR("Hardware ID: %s\n"), config.state.hwid);

      #ifdef USE_WIFI_LOGGER
      wiFiLogger->setConfig(config.state.logIP, config.state.logPort, WIFI_LOGGER_BUFFER_SIZE);
      #endif

    if(xHandleAlarm == NULL){
        xReturnedAlarm = xTaskCreatePinnedToCore(coreroutineAlarmTaskRoutine, PSTR("coreroutineAlarmTaskRoutine"), ALARM_STACKSIZE, NULL, 1, &xHandleAlarm, 1);
        if(xReturnedAlarm == pdPASS){
          logger->warn(PSTR(__func__), PSTR("Task alarmTaskRoutine has been created.\n"));
        }
      }
    if(xHandlePowerSensor == NULL){
        appState.xReturnedPowerSensor = xTaskCreatePinnedToCore(coreroutinePowerSensorTaskRoutine, PSTR("powerSensor"), 4096, NULL, 1, &appState.xHandlePowerSensor, 1);
        if(appState.xReturnedPowerSensor == pdPASS){
            logger->warn(PSTR(__func__), PSTR("Task powerSensor has been created.\n"));
        }
    }
    if(xHandleRelayControl == NULL){
        appState.xReturnedRelayControl = xTaskCreatePinnedToCore(coreroutineRelayControlTaskRoutine, PSTR("relayControl"), 4096, NULL, 1, &appState.xHandleRelayControl, 1);
        if(appState.xReturnedRelayControl == pdPASS){
            logger->warn(PSTR(__func__), PSTR("Task relayControl has been created.\n"));
        }
    }
    }
    coreroutineSetAlarm(0, 0, 3, 50);

    crashState.rtcp = 0;

    #ifdef USE_LOCAL_WEB_INTERFACE
    if(xSemaphoreWSBroadcast == NULL){xSemaphoreWSBroadcast = xSemaphoreCreateMutex();}
    #endif

    #ifdef USE_IOT
    tb.Set_Buffering_Size(IOT_BUFFERING_SIZE);
    tb.Set_Maximum_Stack_Size(IOT_DEFAULT_MAX_STACK_SIZE);
    tb.Set_Buffer_Size(IOT_RECEIVE_BUFFER_SIZE, IOT_SEND_BUFFER_SIZE);

    logger->debug(PSTR(__func__), PSTR("ThingsBoard buffer size after: %u, stack: %u, resp: %u, send: %u, recv: %u\n"),
      tb.Get_Buffering_Size(),
      tb.Get_Maximum_Stack_Size(),
      tb.Get_Max_Response_Size(),
      tb.Get_Send_Buffer_Size(), 
      tb.Get_Receive_Buffer_Size());

    if(iotState.xSemaphoreThingsboard == NULL){iotState.xSemaphoreThingsboard = xSemaphoreCreateMutex();}
    #ifdef USE_IOT_SECURE
    tcpClient.setCACert(CA_CERT);
    #endif

    tb.Subscribe_API_Implementation(IAPIProv);
    tb.Subscribe_API_Implementation(IAPIRPC);
    tb.Subscribe_API_Implementation(IAPISharedAttr);
    tb.Subscribe_API_Implementation(IAPISharedAttrReq);
    tb.Subscribe_API_Implementation(IAPIOta);
    #endif
}

void coreroutineLoop(){
    unsigned long now = millis();

    wiFiHelper.run();

    #ifdef USE_WIFI_OTA
    ArduinoOTA.handle();
    #endif

    #ifdef USE_LOCAL_WEB_INTERFACE
    ws.cleanupClients();
    #endif

    #ifdef USE_IOT
    if(WiFi.getMode() == WIFI_MODE_STA && WiFi.isConnected()){
      coreroutineRunIoT();
    }
    #endif

    if( !crashState.crashStateCheckedFlag && (now - crashState.crashStateCheckTimer) > SAFEMODE_CLEAR_DURATION * 1000 ){
      crashState.fSafeMode = false;
      crashState.crashCnt = 0;
      logger->info(PSTR(__func__), PSTR("fSafeMode & Crash Counter cleared! Try to reboot normally.\n"));
      coreroutineCrashStateTruthKeeper(2);
      crashState.crashStateCheckedFlag = true;
    }

    if( (now - crashState.lastRecordedDatetimeSavedTimer) > FSTIMESAVER_INTERVAL * 3600 * 1000 ){
      crashState.lastRecordedDatetimeSavedTimer = now;
      coreroutineCrashStateTruthKeeper(2);
      logger->verbose(PSTR(__func__), PSTR("Crash state saved.\n"));
    }

    if(crashState.fPlannedReboot){
      if( now - crashState.plannedRebootTimer > 1000){
        if(crashState.plannedRebootCountDown <= 0){
          logger->warn(PSTR(__func__), PSTR("Reboting...\n"));
          ESP.restart();
        }
        crashState.plannedRebootCountDown--;
        logger->warn(PSTR(__func__), PSTR("Planned reboot in %d.\n"), crashState.plannedRebootCountDown);
        crashState.plannedRebootTimer = now;
      }
    }

    if(crashState.fStartServices){
      crashState.fStartServices = false;
      coreroutineStartServices();
    }

    if(crashState.fStopServices){
      crashState.fStopServices = false;
      coreroutineStopServices();
    }

    if(crashState.fDoInit){
      crashState.fDoInit = false;
      coreroutineDoInit();
    }

    if(appState.fsaveAppRelay){
        JsonDocument doc;
        storageConvertAppRelay(doc, false);
        appRelaysGC.save(doc);
        appState.fsaveAppRelay = false;
        logger->debug(PSTR(__func__), PSTR("Saved app relay state.\n"));
    }

    if(appState.fsyncClientAttributes){
        coreroutineSyncClientAttr(3); // 3 means both ws and iot
        appState.fsyncClientAttributes = false;
        logger->debug(PSTR(__func__), PSTR("Synced client attributes.\n"));
    }

    if(appState.fSaveAppState){
        JsonDocument doc;
        storageConvertAppState(doc, false);
        appStateGC.save(doc);
        appState.fSaveAppState = false;
        logger->debug(PSTR(__func__), PSTR("Saved app state.\n"));
    }

    if (appState.fPanic) {
        if (!appState.panic_action_taken) {
            logger->warn(PSTR(__func__), PSTR("Panic mode is activated.\n"));
            for(uint8_t i = 0; i < 4; i++){
                logger->warn(PSTR(__func__), PSTR("Relay %d is turned off and changed to manual.\n"), i+1);
                relays[i].mode = 0;
                coreroutineSetRelay(i, false);
            }
            appState.panic_action_taken = true;
        }
    } else {
        if (appState.panic_action_taken) {
            logger->info(PSTR(__func__), PSTR("Panic mode deactivated.\n"));
            appState.panic_action_taken = false; // Reset if panic is turned off
        }
    }

    #ifdef USE_LOCAL_WEB_INTERFACE
      if(ws.count() > 0 && (now - appState.lastWebBcast > (appConfig.intvWeb * 1000))){
        JsonDocument doc;
        JsonObject sysInfo = doc[PSTR("sysInfo")].to<JsonObject>();

        sysInfo[PSTR("heap")] = ESP.getFreeHeap();
        sysInfo[PSTR("uptime")] = now;
        sysInfo[PSTR("datetime")] = RTC.getDateTime();
        sysInfo[PSTR("rssi")] = wiFiHelper.rssiToPercent(WiFi.RSSI());

        wsBcast(doc);
        appState.lastWebBcast = now;
      }
    #endif

    #ifdef USE_IOT
      if(now - appState.lastAttrBcast > (appConfig.intvAttr * 1000)){
        JsonDocument doc;

        doc[PSTR("heap")] = ESP.getFreeHeap();
        doc[PSTR("uptime")] = now;
        doc[PSTR("datetime")] = RTC.getDateTime();
        doc[PSTR("rssi")] = wiFiHelper.rssiToPercent(WiFi.RSSI());
        iotSendAttr(doc);
        appState.lastAttrBcast = now;
      }
    #endif
}

void coreroutineDoInit(){
    logger->warn(PSTR(__func__), PSTR("Starting services setup protocol!\n"));
    if (!MDNS.begin(config.state.hname)) {
    logger->error(PSTR(__func__), PSTR("Error setting up MDNS responder!\n"));
    }
    else{
    logger->debug(PSTR(__func__), PSTR("mDNS responder started at %s\n"), config.state.hname);
    }

    MDNS.addService("http", "tcp", 80);

    #ifdef USE_WIFI_OTA
    if(config.state.fWOTA){
        logger->debug(PSTR(__func__), PSTR("Starting WiFi OTA at %s\n"), config.state.hname);
        ArduinoOTA.setHostname(config.state.hname);
        ArduinoOTA.setPasswordHash(config.state.upass);

        ArduinoOTA.onStart(coreroutineOnWiFiOTAStart);
        ArduinoOTA.onEnd(coreroutineOnWiFiOTAEnd);
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        coreroutineOnWiFiOTAProgress(progress, total);
        });
        ArduinoOTA.onError([](ota_error_t error) {
            coreroutineOnWiFiOTAError(error);
        });
        ArduinoOTA.begin();
    }
    #endif

    #ifdef USE_LOCAL_WEB_INTERFACE
    logger->debug(PSTR(__func__), PSTR("Starting web service...\n"));
    http.serveStatic("/", LittleFS, "/ui").setDefaultFile("index.html");
    http.serveStatic("/css/pico.blue.min.css", LittleFS, "/ui/css/pico.blue.min.css");
    http.serveStatic("/css/index.css", LittleFS, "/ui/css/index.css");
    http.serveStatic("/assets/bundle.js", LittleFS, "/ui/assets/bundle.js");


    ws.onEvent([](AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
        coreroutineOnWsEvent(server, client, type, arg, data, len);
    });

    http.addHandler(&ws);
    http.begin();
    logger->debug(PSTR(__func__), PSTR("Web service started.\n"));
    #endif
}

void coreroutineStartServices(){
    #ifdef USE_WIFI_LOGGER
    logger->addLogger(wiFiLogger);
    #endif
    coreroutineRTCUpdate(0);
    #ifdef USE_WIFI_OTA
    if(config.state.fWOTA){
        logger->debug(PSTR(__func__), PSTR("Starting WiFi OTA at %s\n"), config.state.hname);
        ArduinoOTA.setHostname(config.state.hname);
        ArduinoOTA.setPasswordHash(config.state.upass);

        ArduinoOTA.onStart(coreroutineOnWiFiOTAStart);
        ArduinoOTA.onEnd(coreroutineOnWiFiOTAEnd);
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        coreroutineOnWiFiOTAProgress(progress, total);
        });
        ArduinoOTA.onError([](ota_error_t error) {
            coreroutineOnWiFiOTAError(error);
        });
        ArduinoOTA.begin();
    }
    #endif

    if(!MDNS.begin(config.state.hname)) {
        logger->error(PSTR(__func__), PSTR("Error setting up MDNS responder!\n"));
    }
    else{
        logger->debug(PSTR(__func__), PSTR("mDNS responder started at %s\n"), config.state.hname);
    }

    MDNS.addService("http", "tcp", 80);

    #ifdef USE_LOCAL_WEB_INTERFACE
    if(config.state.fWeb && !crashState.fSafeMode){
      logger->debug(PSTR(__func__), PSTR("Starting web wervice...\n"));
      http.serveStatic("/", LittleFS, "/ui").setDefaultFile("index.html");
      http.serveStatic("/css/pico.blue.min.css", LittleFS, "/ui/css/pico.blue.min.css");
      http.serveStatic("/css/index.css", LittleFS, "/ui/css/index.css");
      http.serveStatic("/assets/bundle.js", LittleFS, "/ui/assets/bundle.js");

      ws.onEvent([](AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
          coreroutineOnWsEvent(server, client, type, arg, data, len);
      });

      http.addHandler(&ws);
      http.begin();
      logger->debug(PSTR(__func__), PSTR("Web service started.\n"));
    }
    #endif
}

void coreroutineStopServices(){
    #ifdef USE_WIFI_LOGGER
    logger->addLogger(wiFiLogger);
    #endif

    #ifdef USE_WIFI_OTA
    if(config.state.fWOTA){
        logger->debug(PSTR(__func__), PSTR("Stopping WiFi OTA...\n"), config.state.hname);
        ArduinoOTA.end();
    }
    #endif

    logger->error(PSTR(__func__), PSTR("Stopping MDNS...\n"));
    MDNS.end();


    #ifdef USE_LOCAL_WEB_INTERFACE
    logger->error(PSTR(__func__), PSTR("Stopping HTTP...\n"));
    http.end();
    #endif
}

void coreroutineCrashStateTruthKeeper(uint8_t direction){
  JsonDocument doc;
  crashState.rtcp = millis();

  if(direction == 1 || direction == 3){
    crashStateConfig.load(doc);
    crashState.rtcp = doc[PSTR("rtcp")];
    crashState.crashCnt = doc[PSTR("crashCnt")];
    crashState.fSafeMode = doc[PSTR("fSafeMode")];
    crashState.lastRecordedDatetime = doc[PSTR("lastRecordedDatetime")];
  } 

  if(direction == 2 || direction == 3){
    doc[PSTR("rtcp")] = crashState.rtcp;
    doc[PSTR("crashCnt")] = crashState.crashCnt;
    doc[PSTR("fSafeMode")] = crashState.fSafeMode;
    doc[PSTR("lastRecordedDatetime")] = RTC.getEpoch();
    crashStateConfig.save(doc);
  }
  


}

void coreroutineRTCUpdate(long ts){
  #ifdef USE_HW_RTC
  crashState.fRTCHwDetected = false;
  if(!hwRTC.begin()){
    logger->error(PSTR(__func__), PSTR("RTC module not found. Any function that requires precise timing will malfunction! \n"));
    logger->warn(PSTR(__func__), PSTR("Trying to recover last recorded time from flash file...! \n"));
    RTC.setTime(crashState.lastRecordedDatetime);
    logger->debug(PSTR(__func__), PSTR("Updated time via last recorded time from flash file: %s\n"), RTC.getDateTime().c_str());
  }
  else{
    crashState.fRTCHwDetected = true;
    hwRTC.setSquareWave(SquareWaveDisable);
  }
  #endif
  if(ts == 0){
    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP, "pool.ntp.org");
    timeClient.setTimeOffset(config.state.gmtOff);
    bool ntpSuccess = timeClient.update();
    if (ntpSuccess){
      long epochTime = timeClient.getEpochTime();
      RTC.setTime(epochTime);
      logger->debug(PSTR(__func__), PSTR("Updated time via NTP: %s GMT Offset:%d (%d) \n"), RTC.getDateTime().c_str(), config.state.gmtOff, config.state.gmtOff / 3600);
      #ifdef USE_HW_RTC
      if(crashState.fRTCHwDetected){
        logger->debug(PSTR(__func__), PSTR("Updating RTC HW from NTP...\n"));
        hwRTC.setDateTime(RTC.getHour(), RTC.getMinute(), RTC.getSecond(), RTC.getDay(), RTC.getMonth()+1, RTC.getYear(), RTC.getDayofWeek());
        logger->debug(PSTR(__func__), PSTR("Updated RTC HW from NTP with epoch %d | H:I:S W D-M-Y. -> %d:%d:%d %d %d-%d-%d\n"), 
        hwRTC.getEpoch(), RTC.getHour(), RTC.getMinute(), RTC.getSecond(), RTC.getDayofWeek(), RTC.getDay(), RTC.getMonth()+1, RTC.getYear());
      }
      #endif
    }else{
      #ifdef USE_HW_RTC
      if(crashState.fRTCHwDetected){
        logger->debug(PSTR(__func__), PSTR("Updating RTC from RTC HW with epoch %d.\n"), hwRTC.getEpoch());
        RTC.setTime(hwRTC.getEpoch());
        logger->debug(PSTR(__func__), PSTR("Updated time via RTC HW: %s GMT Offset:%d (%d) \n"), RTC.getDateTime().c_str(), config.state.gmtOff, config.state.gmtOff / 3600);
      }
      #endif
    }
  }else{
      RTC.setTime(ts);
      logger->debug(PSTR(__func__), PSTR("Updated time via timestamp: %s\n"), RTC.getDateTime().c_str());
  }
}

void coreroutineSetAlarm(uint16_t code, uint8_t color, int32_t blinkCount, uint16_t blinkDelay){
  if( xQueueAlarm != NULL ){
    AlarmMessage alarmMsg;
    alarmMsg.code = code; alarmMsg.color = color; alarmMsg.blinkCount = blinkCount; alarmMsg.blinkDelay = blinkDelay;
    if( xQueueSend( xQueueAlarm, &alarmMsg, ( TickType_t ) 1000 ) != pdPASS )
    {
        logger->debug(PSTR(__func__), PSTR("Failed to set alarm. Queue is full. \n"));
    }
  }
}

void coreroutineAlarmTaskRoutine(void *arg){
  pinMode(config.state.pinLEDR, OUTPUT);
  pinMode(config.state.pinLEDG, OUTPUT);
  pinMode(config.state.pinLEDB, OUTPUT);
  pinMode(config.state.pinBuzz, OUTPUT);
  while(true){
    if( xQueueAlarm != NULL ){
      AlarmMessage alarmMsg;
      if( xQueueReceive( xQueueAlarm,  &( alarmMsg ), ( TickType_t ) 100 ) == pdPASS )
      {
        if(alarmMsg.code > 0){
          JsonDocument doc;
          JsonObject alarm = doc[PSTR("alarm")].to<JsonObject>();
          alarm[PSTR("code")] = alarmMsg.code;
          alarm[PSTR("time")] = RTC.getDateTime();

          #ifdef USE_LOCAL_WEB_INTERFACE
          wsBcast(doc);
          #endif
        }
        coreroutineSetLEDBuzzer(alarmMsg.color, alarmMsg.blinkCount > 0 ? true : false, alarmMsg.blinkCount, alarmMsg.blinkDelay);
        logger->debug(PSTR(__func__), PSTR("Alarm code: %d, color: %d, blinkCount: %d, blinkDelay: %d\n"), alarmMsg.code, alarmMsg.color, alarmMsg.blinkCount, alarmMsg.blinkDelay);
        vTaskDelay((const TickType_t) (alarmMsg.blinkCount * alarmMsg.blinkDelay) / portTICK_PERIOD_MS);
      }
    }
    vTaskDelay((const TickType_t) 100 / portTICK_PERIOD_MS);
  }
}

void coreroutineSetLEDBuzzer(uint8_t color, uint8_t isBlink, int32_t blinkCount, uint16_t blinkDelay){
  uint8_t r, g, b;
  switch (color)
  {
  //Auto by network
  case 0:
    if(false){
      r = config.state.LEDOn == false ? true : false;
      g = config.state.LEDOn == false ? true : false;
      b = config.state.LEDOn;
    }
    else if(WiFi.status() == WL_CONNECTED && WiFi.getMode() == WIFI_MODE_STA){
      r = config.state.LEDOn == false ? true : false;
      g = config.state.LEDOn;
      b = config.state.LEDOn == false ? true : false;
    }
    else if(WiFi.status() == WL_CONNECTED && WiFi.getMode() == WIFI_MODE_AP && WiFi.softAPgetStationNum() > 0){
      r = config.state.LEDOn == false ? true : false;
      g = config.state.LEDOn;
      b = config.state.LEDOn == false ? true : false;
    }
    else{
      r = config.state.LEDOn;
      g = config.state.LEDOn == false ? true : false;
      b = config.state.LEDOn == false ? true : false;
    }
    break;
  //RED
  case 1:
    r = config.state.LEDOn;
    g = config.state.LEDOn == false ? true : false;
    b = config.state.LEDOn == false ? true : false;
    break;
  //GREEN
  case 2:
    r = config.state.LEDOn == false ? true : false;
    g = config.state.LEDOn;
    b = config.state.LEDOn == false ? true : false;
    break;
  //BLUE
  case 3:
    r = config.state.LEDOn == false ? true : false;
    g = config.state.LEDOn == false ? true : false;
    b = config.state.LEDOn;
    break;
  default:
    r = config.state.LEDOn;
    g = config.state.LEDOn;
    b = config.state.LEDOn;
  }

  if(isBlink){
    int32_t blinkCounter = 0;
    while (blinkCounter < blinkCount)
    {
      digitalWrite(config.state.pinLEDR, config.state.LEDOn == false ? true : false);
      digitalWrite(config.state.pinLEDG, config.state.LEDOn == false ? true : false);
      digitalWrite(config.state.pinLEDB, config.state.LEDOn == false ? true : false);
      digitalWrite(config.state.pinBuzz, HIGH);
      //logger->debug(PSTR(__func__), PSTR("Blinking LED and Buzzing, blinkDelay: %d, blinkCount: %d\n"), blinkDelay, blinkCount);
      vTaskDelay(pdMS_TO_TICKS(blinkDelay));
      digitalWrite(config.state.pinLEDR, r);
      digitalWrite(config.state.pinLEDG, g);
      digitalWrite(config.state.pinLEDB, b);
      digitalWrite(config.state.pinBuzz, LOW);
      //logger->debug(PSTR(__func__), PSTR("Stop Blinking LED and Buzzing.\n"));
      vTaskDelay(pdMS_TO_TICKS(blinkDelay));
      blinkCounter++;
    }
  }
  else{
    digitalWrite(config.state.pinLEDR, r);
    digitalWrite(config.state.pinLEDG, g);
    digitalWrite(config.state.pinLEDB, b);
  }
  
}

void coreroutineSaveAllStorage() {
    JsonDocument doc;

    // Save appConfig
    storageConvertAppConfig(doc, false);
    appConfigGC.save(doc);
    doc.clear();

    // Save appState
    storageConvertAppState(doc, false);
    appStateGC.save(doc);
    doc.clear();

    // Save appRelays
    storageConvertAppRelay(doc, false);
    appRelaysGC.save(doc);
    doc.clear();

    // Save crashStateConfig
    coreroutineCrashStateTruthKeeper(2);

    // Save main config (UdawaConfig)
    config.save();
}

static void coreroutineFSDownloaderTask(void *arg){
  coreroutineFSDownloader();
  vTaskDelete(NULL);
}

void coreroutineFSDownloader() {
    coreroutineSaveAllStorage();
    // We need a WiFiClient for ArduinoHttpClient
    WiFiClient wifi;

    // --- New URL Parsing Logic ---
    char host[128];
    char path[128];
    uint16_t port = 80; // Default to port 80

    // Use sscanf to parse the URL from your configuration
    // This format string looks for "http://", then captures the host (up to '/'),
    // an optional port (:port), and the rest as the path.
    if (sscanf(config.state.binURL, "http://%99[^:]:%hu/%99[^\n]", host, &port, path) == 3) {
        // Successfully parsed host, port, and path
    } else if (sscanf(config.state.binURL, "http://%99[^/]/%99[^\n]", host, path) == 2) {
        // Successfully parsed host and path, port remains 80
    } else {
        logger->error(PSTR(__func__), PSTR("Failed to parse URL: %s\n"), config.state.binURL);
        return;
    }
    // Prepend a '/' to the path if it's missing, as required by HttpClient
    if (path[0] != '/') {
        String tempPath = String("/") + String(path);
        strncpy(path, tempPath.c_str(), sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0'; // Ensure null termination
    }
    // --- End of New URL Parsing Logic ---

    // Create the HttpClient instance with the WiFiClient and parsed server details
    HttpClient http(wifi, host, port);

    logger->info(PSTR(__func__), PSTR("Downloading SPIFFS from host: %s, port: %d, path: %s\n"), host, port, path);

    // Make the GET request
    http.get(path);

    // ... (the rest of your function remains exactly the same) ...
    
    // Get the status code from the response
    int httpCode = http.responseStatusCode();

    int64_t updateSize = 0;
    String contentType;
    String acceptRange;

    if (httpCode == 200) { // HTTP_CODE_OK
        while (http.headerAvailable()) {
            String headerName = http.readHeaderName();
            String headerValue = http.readHeaderValue();

            if (headerName.equalsIgnoreCase("Content-Length")) {
                updateSize = headerValue.toInt();
            } else if (headerName.equalsIgnoreCase("Content-Type")) {
                contentType = headerValue;
            } else if (headerName.equalsIgnoreCase("Accept-Ranges")) {
                acceptRange = headerValue;
            }
        }

        if (acceptRange == "bytes") {
            logger->info(PSTR(__func__), PSTR("This server supports resume!\n"));
        } else {
            logger->info(PSTR(__func__), PSTR("This server does not support resume!\n"));
        }

    } else {
        logger->error(PSTR(__func__), PSTR("Server responded with HTTP Status %d.\n"), httpCode);
        return;
    }

    updateSize = http.contentLength();

    if (updateSize <= 0) {
        logger->error(PSTR(__func__), PSTR("Response is empty or invalid! updateSize: %d, contentType: %s\n"), (int)updateSize, contentType.c_str());
        reboot(3);
        return;
    }

    logger->info(PSTR(__func__), PSTR("updateSize: %d, contentType: %s\n"), (int)updateSize, contentType.c_str());
    
    bool canBegin = Update.begin(updateSize, U_SPIFFS);

    if (!canBegin) {
        logger->warn(PSTR(__func__), PSTR("Not enough space to begin OTA, partition size mismatch?\n"));
        Update.abort();
        reboot(3);
        return;
    }

    Update.onProgress([](size_t progress, size_t size) {
      JsonDocument doc;
      doc[PSTR("FSUpdate")][PSTR("status")] = PSTR("progress");
      
      // Calculate progress to fit in the 80% to 100% range
      size_t calculated_progress = (size_t)((size * 0.80) + (progress * 0.20));
      
      doc[PSTR("FSUpdate")][PSTR("progress")] = calculated_progress;
      doc[PSTR("FSUpdate")][PSTR("total")] = size;
      wsBcast(doc);
    });

    logger->info(PSTR(__func__), PSTR("Begin LittleFS OTA. This may take 2 - 5 mins to complete. Things might be quiet for a while.. Patience!\n"));
    JsonDocument doc;
    doc[PSTR("FSUpdate")][PSTR("status")] = PSTR("start");
    wsBcast(doc);
    doc.clear();

    size_t written = Update.writeStream(http);

    if (written == updateSize) {
        logger->info(PSTR(__func__), PSTR("Written : %d successfully.\n"), (int)written);
    } else {
        logger->warn(PSTR(__func__), PSTR("Written only : %d / %d. Premature end of stream? Error: %s\n"), (int)written, (int)updateSize, Update.errorString());
        Update.abort();
        coreroutineSaveAllStorage();
        doc[PSTR("FSUpdate")][PSTR("status")] = PSTR("fail");
        doc[PSTR("FSUpdate")][PSTR("error")] = Update.errorString();
        wsBcast(doc);
        reboot(3);
        return;
    }

    if (!Update.end()) {
        logger->warn(PSTR(__func__), PSTR("An Update Error Occurred: %d\n"), Update.getError());
        coreroutineSaveAllStorage();
        doc[PSTR("FSUpdate")][PSTR("status")] = PSTR("fail");
        doc[PSTR("FSUpdate")][PSTR("error")] = Update.getError();
        wsBcast(doc);
        reboot(3);
        return;
    }
    
    if (Update.isFinished()) {
        logger->info(PSTR(__func__), PSTR("Update completed successfully.\n"));
        coreroutineSaveAllStorage();
        doc[PSTR("FSUpdate")][PSTR("status")] = PSTR("success");
        wsBcast(doc);
        delay(1000); // Ensure configuration is saved before reboot
        reboot(3);
    } else {
        coreroutineSaveAllStorage();
        logger->warn(PSTR(__func__), PSTR("Update not finished! Something went wrong!\n"));
        doc[PSTR("FSUpdate")][PSTR("status")] = PSTR("fail");
        doc[PSTR("FSUpdate")][PSTR("error")] = "Update not finished";
        wsBcast(doc);
        reboot(3);
    }

    reboot(3);
}

#ifdef USE_WIFI_OTA
void coreroutineOnWiFiOTAStart(){
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
    type = "sketch";
    } else { // U_SPIFFS
    type = "filesystem";
    }
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    LittleFS.end();
    logger->debug(PSTR(""), PSTR("Start updating %s.\n"), type.c_str());
}

void coreroutineOnWiFiOTAEnd(){
    logger->debug(PSTR(__func__), PSTR("\n Finished.\n"));
}

void coreroutineOnWiFiOTAProgress(unsigned int progress, unsigned int total){
    logger->debug(PSTR(__func__), PSTR("Progress: %u%%\n"), (progress / (total / 100)));
}

void coreroutineOnWiFiOTAError(ota_error_t error){
    logger->error(PSTR(__func__), PSTR("Error[%u]: "), error);
    if (error == OTA_AUTH_ERROR) {
    logger->error(PSTR(""),PSTR("Auth Failed\n"));
    } else if (error == OTA_BEGIN_ERROR) {
    logger->error(PSTR(""),PSTR("Begin Failed\n"));
    } else if (error == OTA_CONNECT_ERROR) {
    logger->error(PSTR(""),PSTR("Connect Failed\n"));
    } else if (error == OTA_RECEIVE_ERROR) {
    logger->error(PSTR(""),PSTR("Receive Failed\n"));
    } else if (error == OTA_END_ERROR) {
    logger->error(PSTR(""),PSTR("End Failed\n"));
    }
}
#endif

#ifdef USE_LOCAL_WEB_INTERFACE
String hmacSha256(String htP, String salt) {
  char outputBuffer[65]; // 2 characters per byte + null terminator

  // Convert input strings to UTF-8 byte arrays 
  std::vector<uint8_t> apiKeyUtf8(htP.begin(), htP.end());
  std::vector<uint8_t> saltUtf8(salt.begin(), salt.end());

  // Calculate the HMAC
  unsigned char hmac[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1); // Set HMAC mode
  mbedtls_md_hmac_starts(&ctx, apiKeyUtf8.data(), apiKeyUtf8.size());
  mbedtls_md_hmac_update(&ctx, saltUtf8.data(), saltUtf8.size());
  mbedtls_md_hmac_finish(&ctx, hmac);
  mbedtls_md_free(&ctx); 

  // Convert the hash to a hex string (with leading zeros)
  for (int i = 0; i < 32; i++) {
    sprintf(&outputBuffer[i * 2], "%02x", hmac[i]);
  }

  // Null terminate the string
  outputBuffer[64] = '\0';
  return String(outputBuffer);  
}


void coreroutineOnWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  IPAddress clientIP = client->remoteIP();
  JsonDocument doc;
  switch(type) {
    case WS_EVT_DISCONNECT:
    {
      logger->verbose(PSTR(__func__), PSTR("Client disconnected.\n"));
      wsClientAuthenticationStatus.erase(client->id());
      wsClientAuthAttemptTimestamps.erase(clientIP);
      wsClientSalts.erase(client->id());  // Remove salt on disconnect
      break;     
    }
    case WS_EVT_CONNECT:
    {
      logger->verbose(PSTR(__func__), PSTR("New client arrived [%s]\n"), clientIP.toString().c_str());
      wsClientAuthenticationStatus[client->id()] = false;
      wsClientAuthAttemptTimestamps[clientIP] = millis();

      if(config.state.fInit){
        // Generate a random salt
        unsigned char salt[16];
        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context ctr_drbg;
        const char *pers = "ws_salt";

        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers));
        mbedtls_ctr_drbg_random(&ctr_drbg, salt, sizeof(salt));
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);

        String saltHex;
        for (int i = 0; i < sizeof(salt); i++) {
            char hex[3];
            sprintf(hex, "%02x", salt[i]);
            saltHex += hex;
        }
        
        // Store the salt for this client
        wsClientSalts[client->id()] = saltHex;

        // Send salt to the client
          doc.clear();
          doc[PSTR("setSalt")][PSTR("salt")] = saltHex;
          doc[PSTR("setSalt")][PSTR("name")] = config.state.name;
          doc[PSTR("setSalt")][PSTR("model")] = config.state.model;
          doc[PSTR("setSalt")][PSTR("group")] = config.state.group;
          String message;
          serializeJson(doc, message);
          doc.clear();
          client->text(message);
          break;
        }
      break;
    }
    case WS_EVT_DATA:
    {
      doc.clear();
      DeserializationError err = deserializeJson(doc, (const char*)data, len);
      /*if(err != DeserializationError::Ok){
        logger->error(PSTR(__func__), PSTR("Failed to parse JSON.\n"));
        return;
      }*/
      // If client is not authenticated, check credentials
      if(!wsClientAuthenticationStatus[client->id()] && config.state.fInit) {
        logger->verbose(PSTR(__func__), PSTR("Client is NOT authenticated (%i) AND fInit is TRUE (%i)\n"), wsClientAuthenticationStatus[client->id()], config.state.fInit);
        unsigned long currentTime = millis();
        unsigned long lastAttemptTime = wsClientAuthAttemptTimestamps[clientIP];

        /**if (currentTime - lastAttemptTime < 1000) {
          // Too many attempts in short time, block this IP for blockInterval
          //_wsClientAuthAttemptTimestamps[clientIP] = currentTime + WS_BLOCKED_DURATION - WS_RATE_LIMIT_INTERVAL;
          logger->verbose(PSTR(__func__), PSTR("Too many authentication attempts. Blocking for %d seconds. Rate limit %d.\n"), WS_BLOCKED_DURATION / 1000, WS_RATE_LIMIT_INTERVAL);
          //client->close();
          return;
        }**/

        if (err != DeserializationError::Ok) {
          //client->printf(PSTR("{\"status\": {\"code\": 400, \"msg\": \"Bad request.\"}}"));
          //_wsClientAuthAttemptTimestamps[clientIP] = currentTime;
          return;
        }
        else{
          if(doc[PSTR("auth")][PSTR("salt")] == nullptr || doc[PSTR("auth")][PSTR("hash")] == nullptr){
            //client->printf(PSTR("{\"status\": {\"code\": 400, \"msg\": \"Bad request.\"}}"));
            //_wsClientAuthAttemptTimestamps[clientIP] = currentTime;
            return;
          }

          String clientAuth = doc[PSTR("auth")][PSTR("hash")].as<String>();
          String clientSalt = doc[PSTR("auth")][PSTR("salt")].as<String>();
          //logger->debug(PSTR(__func__), PSTR("\n\tserver: %s\n\tclient: %s\n\tkey: %s\n\tsalt: %s\n"), _auth.c_str(), auth.c_str(), config.state.htP, salt.c_str());
          
          if (wsClientSalts[client->id()] == clientSalt) {
              // Compute expected HMAC with stored salt
              String expectedAuth = hmacSha256(config.state.htP, wsClientSalts[client->id()]);

              // Check if the HMACs match
              //logger->verbose(PSTR(__func__), PSTR("\nhtP:\t%s \n\nclientAuth:\t%s\n\nexpectedAuth:\t%s\n"), config.state.htP, clientAuth.c_str(), expectedAuth.c_str());
              if (clientAuth == expectedAuth) {
                  wsClientAuthenticationStatus[client->id()] = true;
                  logger->verbose(PSTR(__func__), PSTR("Client authenticated successfully.\n"));
                  client->printf(PSTR("{\"status\": {\"code\": 200, \"msg\": \"Authorized.\", \"model\": \"%s\"}}"), config.state.model);
              } else {
                  logger->warn(PSTR(__func__), PSTR("Authentication failed.\n"));
                  client->printf(PSTR("{\"status\": {\"code\": 401, \"msg\": \"Authorization failed.\", \"model\": \"%s\"}}"), config.state.model);
              }
          } else {
              logger->warn(PSTR(__func__), PSTR("Salt mismatch or expired.\n"));
              client->printf(PSTR("{\"status\": {\"code\": 401, \"msg\": \"Salt mismatch or expired.\", \"model\": \"%s\"}}"), config.state.model);
          }
          // Update timestamp for rate limiting
          wsClientAuthAttemptTimestamps[clientIP] = currentTime;
          return;
        }
      }
      else {
        // The client is already authenticated or fInit is false, you can process the received data
        //...

      if (doc[PSTR("setConfig")].is<JsonObject>()) {
        if (doc[PSTR("setConfig")][PSTR("cfg")].is<JsonObject>()) {
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("wssid")].is<const char*>() && strlen(doc[PSTR("setConfig")][PSTR("cfg")][PSTR("wssid")].as<const char*>()) > 0) {
          strlcpy(config.state.wssid, doc[PSTR("setConfig")][PSTR("cfg")][PSTR("wssid")].as<const char*>(), sizeof(config.state.wssid));
          logger->debug(PSTR(__func__), PSTR("wssid: %s\n"), doc[PSTR("setConfig")][PSTR("cfg")][PSTR("wssid")].as<const char*>());
          }
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("wpass")].is<const char*>() && strlen(doc[PSTR("setConfig")][PSTR("cfg")][PSTR("wpass")].as<const char*>()) > 0) {
          strlcpy(config.state.wpass, doc[PSTR("setConfig")][PSTR("cfg")][PSTR("wpass")].as<const char*>(), sizeof(config.state.wpass));
          logger->debug(PSTR(__func__), PSTR("wpass: %s\n"), doc[PSTR("setConfig")][PSTR("cfg")][PSTR("wpass")].as<const char*>());
          }
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("gmtOff")].is<int>()) {
          config.state.gmtOff = doc[PSTR("setConfig")][PSTR("cfg")][PSTR("gmtOff")].as<int>();
          logger->debug(PSTR(__func__), PSTR("gmtOff: %d\n"), config.state.gmtOff);  // Display as integer
          }
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("group")].is<const char*>() && strlen(doc[PSTR("setConfig")][PSTR("cfg")][PSTR("group")].as<const char*>()) > 0) {
          strlcpy(config.state.group, doc[PSTR("setConfig")][PSTR("cfg")][PSTR("group")].as<const char*>(), sizeof(config.state.group));
          logger->debug(PSTR(__func__), PSTR("group: %s\n"), doc[PSTR("setConfig")][PSTR("cfg")][PSTR("group")].as<const char*>());
          }
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("name")].is<const char*>() && strlen(doc[PSTR("setConfig")][PSTR("cfg")][PSTR("name")].as<const char*>()) > 0) {
          strlcpy(config.state.name, doc[PSTR("setConfig")][PSTR("cfg")][PSTR("name")].as<const char*>(), sizeof(config.state.name));
          logger->debug(PSTR(__func__), PSTR("name: %s\n"), doc[PSTR("setConfig")][PSTR("cfg")][PSTR("name")].as<const char*>());
          }
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("hname")].is<const char*>() && strlen(doc[PSTR("setConfig")][PSTR("cfg")][PSTR("hname")].as<const char*>()) > 0) {
          strlcpy(config.state.hname, doc[PSTR("setConfig")][PSTR("cfg")][PSTR("hname")].as<const char*>(), sizeof(config.state.hname));
          logger->debug(PSTR(__func__), PSTR("hname: %s\n"), doc[PSTR("setConfig")][PSTR("cfg")][PSTR("hname")].as<const char*>());
          }
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("htP")].is<const char*>() && strlen(doc[PSTR("setConfig")][PSTR("cfg")][PSTR("htP")].as<const char*>()) > 0) {
          strlcpy(config.state.htP, doc[PSTR("setConfig")][PSTR("cfg")][PSTR("htP")].as<const char*>(), sizeof(config.state.htP));
          logger->debug(PSTR(__func__), PSTR("htP: %s\n"), doc[PSTR("setConfig")][PSTR("cfg")][PSTR("htP")].as<const char*>());
          }
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("binURL")].is<const char*>() && strlen(doc[PSTR("setConfig")][PSTR("cfg")][PSTR("binURL")].as<const char*>()) > 0) {
          strlcpy(config.state.binURL, doc[PSTR("setConfig")][PSTR("cfg")][PSTR("binURL")].as<const char*>(), sizeof(config.state.binURL));
          logger->debug(PSTR(__func__), PSTR("binURL: %s\n"), doc[PSTR("setConfig")][PSTR("cfg")][PSTR("binURL")].as<const char*>());
          }
          #ifdef USE_IOT
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("tbAddr")].is<const char*>() && strlen(doc[PSTR("setConfig")][PSTR("cfg")][PSTR("tbAddr")].as<const char*>()) > 0) {
          strlcpy(config.state.tbAddr, doc[PSTR("setConfig")][PSTR("cfg")][PSTR("tbAddr")].as<const char*>(), sizeof(config.state.tbAddr));
          logger->debug(PSTR(__func__), PSTR("tbAddr: %s\n"), doc[PSTR("setConfig")][PSTR("cfg")][PSTR("tbAddr")].as<const char*>());
          }
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("tbPort")].is<uint16_t>()) {
          config.state.tbPort = doc[PSTR("setConfig")][PSTR("cfg")][PSTR("tbPort")].as<uint16_t>();
          logger->debug(PSTR(__func__), PSTR("tbPort: %d\n"), config.state.tbPort); 
          }
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("fIoT")].is<bool>()) {
          config.state.fIoT = doc[PSTR("setConfig")][PSTR("cfg")][PSTR("fIoT")].as<bool>();
          logger->debug(PSTR(__func__), PSTR("fIoT: %d\n"), config.state.fIoT); 
          }
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("provDK")].is<const char*>() && strlen(doc[PSTR("setConfig")][PSTR("cfg")][PSTR("provDK")].as<const char*>()) > 0) {
          strlcpy(config.state.provDK, doc[PSTR("setConfig")][PSTR("cfg")][PSTR("provDK")].as<const char*>(), sizeof(config.state.provDK));
          logger->debug(PSTR(__func__), PSTR("provDK: %s\n"), doc[PSTR("setConfig")][PSTR("cfg")][PSTR("provDK")].as<const char*>());
          }
          if (doc[PSTR("setConfig")][PSTR("cfg")][PSTR("provDS")].is<const char*>() && strlen(doc[PSTR("setConfig")][PSTR("cfg")][PSTR("provDS")].as<const char*>()) > 0) {
          strlcpy(config.state.provDS, doc[PSTR("setConfig")][PSTR("cfg")][PSTR("provDS")].as<const char*>(), sizeof(config.state.provDS));
          logger->debug(PSTR(__func__), PSTR("provDS: %s\n"), doc[PSTR("setConfig")][PSTR("cfg")][PSTR("provDS")].as<const char*>());
          }
          #endif
        }
        config.save();
        }

        else if(doc[PSTR("getConfig")].is<const char*>()){
          coreroutineSyncClientAttr(1);
        }

        else if(doc[PSTR("getAvailableWiFi")].is<const char*>()){
            JsonDocument doc;
            File file = LittleFS.open(PSTR("/WiFiList.json"), FILE_READ);
            if (file) {
            // Deserialize the file's content into a temporary JsonDocument
            JsonDocument wifiListDoc;
            DeserializationError error = deserializeJson(wifiListDoc, file);
            file.close();

            if (error == DeserializationError::Ok) {
              // Create the final structure
              doc[PSTR("WiFiList")] = wifiListDoc.as<JsonVariant>();
              wsBcast(doc);
            } else {
              logger->error(PSTR(__func__), PSTR("Failed to parse WiFiList.json: %s\n"), error.c_str());
            }
            } else {
            logger->error(PSTR(__func__), PSTR("Could not open WiFiList.json for reading.\n"));
            }
        }

        else if(doc[PSTR("setFInit")].is<JsonObject>()){
          if(doc[PSTR("setFInit")][PSTR("fInit")].is<bool>()){
            coreroutineSetFInit(doc[PSTR("setFInit")][PSTR("fInit")].as<bool>());
            coreroutineSyncClientAttr(1);
          }
          reboot(3);
        }

        else if(doc[PSTR("setRTCUpdate")].is<JsonObject>()){
          if(doc[PSTR("setRTCUpdate")][PSTR("ts")].is<unsigned long>()){
            coreroutineRTCUpdate(doc[PSTR("setRTCUpdate")][PSTR("ts")].as<unsigned long>());
          }
        }

        else if(doc[PSTR("reboot")].is<int>()){
          reboot(doc[PSTR("reboot")].as<int>());
        }

        else if(doc[PSTR("FSUpdate")].is<bool>()){
          xTaskCreate(
              coreroutineFSDownloaderTask,    // Function that implements the task.
              "FSDownloader",                 // Text name for the task.
              8192,                           // Stack size in words, not bytes.
              NULL,                           // Parameter passed into the task.
              1,                              // Priority at which the task is created.
              NULL                            // Used to pass out the created task's handle.
          );
        }

        else if(doc[PSTR("setRelayState")].is<JsonObject>()){
            if(doc[PSTR("setRelayState")][PSTR("pin")].is<uint8_t>() && doc[PSTR("setRelayState")][PSTR("state")].is<bool>()){
                coreroutineSetRelay(doc[PSTR("setRelayState")][PSTR("pin")].as<uint8_t>(), doc[PSTR("setRelayState")][PSTR("state")].as<bool>());
            }
        }
        else if(doc[PSTR("resetPowerSensor")].is<bool>()){
            appState.fResetPowerSensor = true;
        }
        else if(doc[PSTR("setRelay")].is<JsonObject>() && doc[PSTR("setRelay")][PSTR("relay")].is<JsonObject>() && doc[PSTR("setRelay")][PSTR("index")].is<uint8_t>()){
            uint8_t index = doc[PSTR("setRelay")][PSTR("index")].as<uint8_t>();
            if(index < 4){
                if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("pin")].is<uint8_t>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("pin")].is<const char*>()){relays[index].pin = doc[PSTR("setRelay")][PSTR("relay")][PSTR("pin")].as<uint8_t>();}
                if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("mode")].is<uint8_t>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("mode")].is<const char*>()){relays[index].mode = doc[PSTR("setRelay")][PSTR("relay")][PSTR("mode")].as<uint8_t>();}
                if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("wattage")].is<uint16_t>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("wattage")].is<const char*>()){relays[index].wattage = doc[PSTR("setRelay")][PSTR("relay")][PSTR("wattage")].as<uint16_t>();}
                if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("dutyCycle")].is<uint8_t>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("dutyCycle")].is<const char*>()){relays[index].dutyCycle = doc[PSTR("setRelay")][PSTR("relay")][PSTR("dutyCycle")].as<uint8_t>();}
                if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("dutyRange")].is<unsigned long>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("dutyRange")].is<const char*>()){relays[index].dutyRange = doc[PSTR("setRelay")][PSTR("relay")][PSTR("dutyRange")].as<unsigned long>();}
                if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("autoOff")].is<unsigned long>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("autoOff")].is<const char*>()){relays[index].autoOff = doc[PSTR("setRelay")][PSTR("relay")][PSTR("autoOff")].as<unsigned long>();}
                if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("label")].is<String>()){relays[index].label = doc[PSTR("setRelay")][PSTR("relay")][PSTR("label")].as<String>();}
                if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("overrunInSec")].is<uint16_t>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("overrunInSec")].is<const char*>()){relays[index].overrunInSec = doc[PSTR("setRelay")][PSTR("relay")][PSTR("overrunInSec")].as<uint16_t>();}
                if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("datetime")].is<unsigned long>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("datetime")].is<const char*>()){relays[index].datetime = doc[PSTR("setRelay")][PSTR("relay")][PSTR("datetime")].as<unsigned long>();}
                if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("duration")].is<unsigned long>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("duration")].is<const char*>()){relays[index].duration = doc[PSTR("setRelay")][PSTR("relay")][PSTR("duration")].as<unsigned long>();}
                if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")].is<JsonArray>()){
                    for(uint8_t j = 0; j < maxTimers; j++){
                        if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")][j][PSTR("h")].is<uint8_t>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")][j][PSTR("h")].is<const char*>()){relays[index].timers[j].hour = doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")][j][PSTR("h")].as<uint8_t>();}
                        if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")][j][PSTR("i")].is<uint8_t>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")][j][PSTR("i")].is<const char*>()){relays[index].timers[j].minute = doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")][j][PSTR("i")].as<uint8_t>();}
                        if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")][j][PSTR("s")].is<uint8_t>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")][j][PSTR("s")].is<const char*>()){relays[index].timers[j].second = doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")][j][PSTR("s")].as<uint8_t>();}
                        if(doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")][j][PSTR("d")].is<unsigned long>() || doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")][j][PSTR("d")].is<const char*>()){relays[index].timers[j].duration = doc[PSTR("setRelay")][PSTR("relay")][PSTR("timers")][j][PSTR("d")].as<unsigned long>();}
                    }
                }
            }
            appState.fsaveAppRelay = true;
            appState.fsyncClientAttributes = true;
        }
        else if(doc[PSTR("fPanic")].is<bool>()){
            appState.fPanic = doc[PSTR("fPanic")].as<bool>();
        }
      }
      doc.clear();
    }
    break;
    case WS_EVT_ERROR:
      {
        logger->warn(PSTR(__func__), PSTR("ws [%u] error\n"), client->id());
        return;
      }
      break;	
  }
}

void wsBcast(JsonDocument &doc){
  if(config.state.fWeb){
    if( xSemaphoreWSBroadcast != NULL){
      if( xSemaphoreTake( xSemaphoreWSBroadcast, ( TickType_t ) 1000 ) == pdTRUE )
      {
        String buffer;
        serializeJson(doc, buffer);
        ws.textAll(buffer);
        //logger->verbose(PSTR(__func__), PSTR("Broadcasting message: %s\n"), buffer.c_str());
        xSemaphoreGive( xSemaphoreWSBroadcast );
      }
      else
      {
        logger->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
      }
    }
  }
}
#endif

void coreroutineSyncClientAttr(uint8_t direction){
  JsonDocument doc;
  String ip = WiFi.localIP().toString();
  
  if(direction == 1 || direction == 3){ // Send to client
    #ifdef USE_LOCAL_WEB_INTERFACE
    // Send original attr and cfg objects for UI compatibility
    doc.clear();
    JsonObject attr = doc[PSTR("attr")].to<JsonObject>();
    attr[PSTR("ipad")] = ip.c_str();
    attr[PSTR("compdate")] = COMPILED;
    attr[PSTR("fmTitle")] = CURRENT_FIRMWARE_TITLE;
    attr[PSTR("fmVersion")] = CURRENT_FIRMWARE_VERSION;
    attr[PSTR("stamac")] = WiFi.macAddress();
    attr[PSTR("apmac")] = WiFi.softAPmacAddress();
    attr[PSTR("flFree")] = ESP.getFreeSketchSpace();
    attr[PSTR("fwSize")] = ESP.getSketchSize();
    attr[PSTR("flSize")] = ESP.getFlashChipSize();
    attr[PSTR("dSize")] = LittleFS.totalBytes();
    attr[PSTR("dUsed")] = LittleFS.usedBytes();
    attr[PSTR("sdkVer")] = ESP.getSdkVersion();
    wsBcast(doc);

    doc.clear();
    JsonObject cfg = doc[PSTR("cfg")].to<JsonObject>();
    cfg[PSTR("name")] = config.state.name;
    cfg[PSTR("model")] = config.state.model;
    cfg[PSTR("group")] = config.state.group;
    cfg[PSTR("gmtOff")] = config.state.gmtOff;
    cfg[PSTR("hname")] = config.state.hname;
    cfg[PSTR("htP")] = config.state.htP;
    cfg[PSTR("wssid")] = config.state.wssid;
    cfg[PSTR("wpass")] = config.state.wpass;
    cfg[PSTR("fInit")] = config.state.fInit;
    cfg[PSTR("binURL")] = config.state.binURL;
    wsBcast(doc);

    doc.clear();
    JsonArray availableRelayModes = doc[F("availableRelayMode")].to<JsonArray>();
    for (uint8_t i = 0; i < countof(availableRelayMode); i++) {
      availableRelayModes.add(availableRelayMode[i]);
    }
    wsBcast(doc);

    // Send new app-specific configs
    doc.clear();
    storageConvertAppConfig(doc, false);
    wsBcast(doc);

    doc.clear();
    storageConvertAppState(doc, false);
    wsBcast(doc);

    doc.clear();
    storageConvertAppRelay(doc, false);
    wsBcast(doc);

    doc.clear();
    storageConvertUdawaConfig(doc, false);
    wsBcast(doc);
    #endif
  }

  if(direction == 2 || direction == 3){ // Send to IoT
      #ifdef USE_IOT
      // Send appConfig
      doc.clear();
      storageConvertAppConfig(doc, false);
      iotSendAttr(doc);

      // Send appState
      doc.clear();
      storageConvertAppState(doc, false);
      iotSendAttr(doc);

      // Send appRelays
      doc.clear();
      storageConvertAppRelay(doc, false);
      iotSendAttr(doc);

      // Send udawaConfig
      doc.clear();
      storageConvertUdawaConfig(doc, false);
      iotSendAttr(doc);
      #endif
  }
}

void coreroutineSetFInit(bool fInit){
  config.state.fInit = fInit;
  config.save();

  #ifdef USE_LOCAL_WEB_INTERFACE
    if(config.state.fWeb && !crashState.fSafeMode){
      JsonDocument doc;
      doc[PSTR("setFinishedSetup")][PSTR("fInit")] = config.state.fInit;
      wsBcast(doc);
    }
  #endif
}


void coreroutinePowerSensorTaskRoutine(void *arg) {
    float volt, amp, watt, freq, pf, ener, ener_start_period;

    // Variables for telemetry data aggregation
    float volt_sum = 0, amp_sum = 0, watt_sum = 0, pf_sum = 0, freq_sum = 0;
    float volt_min_period = 999, volt_max_period = 0;
    float amp_min_period = 999, amp_max_period = 0;
    float pf_min_period = 999, pf_max_period = 0;
    float freq_min_period = 999, freq_max_period = 0;
    long reading_count = 0;

    unsigned long timerTelemetry = millis();
    unsigned long timerAttribute = millis();
    unsigned long timerWebIface = millis();
    unsigned long timerAlarm = millis();

    // Initial reading to set the starting energy point
    HardwareSerial initialPzemSerial(1);
    PZEM004Tv30 initialPzem(initialPzemSerial, appConfig.s1rx, appConfig.s1tx);
    ener = initialPzem.energy();
    if (isnan(ener)) ener = 0;
    ener_start_period = ener;


    while (true) {
        HardwareSerial pzemSerial(1);
        PZEM004Tv30 pzem(pzemSerial, appConfig.s1rx, appConfig.s1tx);

        bool fFailureReadings = false;

        if (appConfig.fPowerSensorDummy) {
            appState.fPowerSensor = true;
            float total_wattage = 0;
            for(uint8_t i = 0; i < 4; i++) {
                if (relays[i].state) {
                    total_wattage += relays[i].wattage;
                }
            }

            if (total_wattage > 0) {
                 watt = total_wattage + (rand() % 10 - 5);
            } else {
                 watt = 5.0 + (rand() % 10 / 10.0); // Standby power 5-6W
            }

            volt = 220.0 + (rand() % 20 - 10);
            amp = watt / volt;
            freq = 50.0 + (rand() % 5 / 10.0 - 0.2);
            pf = 0.95 + (rand() % 10 / 100.0);
            ener += watt / 3600000.0;

        } else {
            appState.fPowerSensor = !isnan(pzem.voltage());
            if (appState.fPowerSensor) {
                volt = pzem.voltage();
                amp = pzem.current();
                watt = pzem.power();
                freq = pzem.frequency();
                pf = pzem.pf();
                ener = pzem.energy();
            } else {
                fFailureReadings = true;
            }
        }

        if (appState.fPowerSensor && !fFailureReadings) {
            if (isnan(volt) || isnan(amp) || isnan(watt) || isnan(freq) || isnan(pf) ||
                isnan(ener) || volt < 0.0 || volt > 1000.0 || amp < 0.0 || amp > 100.0 || watt < .0 ||
                watt > 22000.0 || freq < 0.0 || freq > 100.0) {
                fFailureReadings = true;
            }
        }

        unsigned long now = millis();
        if (appState.fPowerSensor && !fFailureReadings) {
            // Accumulate for aggregation
            volt_sum += volt;
            amp_sum += amp;
            watt_sum += watt;
            pf_sum += pf;
            freq_sum += freq;

            if (volt < volt_min_period) volt_min_period = volt;
            if (volt > volt_max_period) volt_max_period = volt;
            if (amp < amp_min_period) amp_min_period = amp;
            if (amp > amp_max_period) amp_max_period = amp;
            if (pf < pf_min_period) pf_min_period = pf;
            if (pf > pf_max_period) pf_max_period = pf;
            if (freq < freq_min_period) freq_min_period = freq;
            if (freq > freq_max_period) freq_max_period = freq;

            reading_count++;

            JsonDocument doc;

            #ifdef USE_LOCAL_WEB_INTERFACE
            if ((now - timerWebIface) > (appConfig.intvWeb * 1000)) {
                JsonObject powerSensor = doc[PSTR("powerSensor")].to<JsonObject>();
                powerSensor[PSTR("volt")] = volt;
                powerSensor[PSTR("amp")] = amp;
                powerSensor[PSTR("watt")] = watt;
                powerSensor[PSTR("freq")] = freq;
                powerSensor[PSTR("pf")] = pf;
                powerSensor[PSTR("ener")] = ener;
                wsBcast(doc);
                doc.clear();
                timerWebIface = now;
            }
            #endif

            #ifdef USE_IOT
            if ((now - timerAttribute) > (appConfig.intvAttr * 1000)) {
                doc[PSTR("_volt")] = volt;
                doc[PSTR("_amp")] = amp;
                doc[PSTR("_watt")] = watt;
                doc[PSTR("_freq")] = freq;
                doc[PSTR("_pf")] = pf;
                doc[PSTR("_ener")] = ener;
                iotSendAttr(doc);
                doc.clear();
                timerAttribute = now;
            }

            if ((now - timerTelemetry) > (appConfig.intvTele * 1000)) {
                if (reading_count > 0) {
                    float energy_consumed_kwh = ener - ener_start_period;

                    doc[PSTR("volt_avg")] = volt_sum / reading_count;
                    doc[PSTR("volt_min")] = volt_min_period;
                    doc[PSTR("volt_max")] = volt_max_period;

                    doc[PSTR("amp_avg")] = amp_sum / reading_count;
                    doc[PSTR("amp_min")] = amp_min_period;
                    doc[PSTR("amp_max")] = amp_max_period;

                    doc[PSTR("pf_avg")] = pf_sum / reading_count;
                    doc[PSTR("pf_min")] = pf_min_period;
                    doc[PSTR("pf_max")] = pf_max_period;

                    doc[PSTR("freq_avg")] = freq_sum / reading_count;
                    doc[PSTR("freq_min")] = freq_min_period;
                    doc[PSTR("freq_max")] = freq_max_period;

                    doc[PSTR("watt_avg")] = watt_sum / reading_count;
                    doc[PSTR("ener_total_kwh")] = ener;
                    doc[PSTR("energy_consumed_wh")] = energy_consumed_kwh * 1000.0;
                    iotSendTele(doc);
                    doc.clear();

                    // Reset accumulators for next period
                    volt_sum = 0; watt_sum = 0; amp_sum = 0; pf_sum = 0; freq_sum = 0;
                    volt_min_period = 999; volt_max_period = 0;
                    amp_min_period = 999; amp_max_period = 0;
                    pf_min_period = 999; pf_max_period = 0;
                    freq_min_period = 999; freq_max_period = 0;
                    reading_count = 0;
                    ener_start_period = ener;
                }
                timerTelemetry = now;
            }
            #endif
        }

        if ((now - timerAlarm) > (appConfig.powerSensorAlarmTimer * 1000)) {
            if (!appState.fPowerSensor) {
                coreroutineSetAlarm(140, 1, 5, 1000);
            } else if (!fFailureReadings) {
                if (volt < 180 || volt > 260) coreroutineSetAlarm(141, 1, 5, 1000);
                if (amp < 0 || amp > 100) coreroutineSetAlarm(142, 1, 5, 1000);
                if (watt < 0 || watt > appConfig.maxWatt) coreroutineSetAlarm(143, 1, 5, 1000);
                if (pf < 0 || pf > 1) coreroutineSetAlarm(144, 1, 5, 1000);
                if (freq < 48 || freq > 52) coreroutineSetAlarm(144, 1, 5, 1000);
                if (watt > appConfig.maxWatt || volt > 275) coreroutineSetAlarm(145, 1, 5, 1000);

                uint8_t activeRelayCounter = 0;
                for (uint8_t i = 0; i < 4; i++) {
                    if (relays[i].state == true) {
                        activeRelayCounter++;
                        if (watt < (relays[i].wattage * 0.1)) coreroutineSetAlarm(210 + i, 1, 5, 1000);
                        if (relays[i].overrunInSec != 0 && (millis() - relays[i].lastActive) > relays[i].overrunInSec * 1000) {
                            coreroutineSetAlarm(215 + i, 1, 5, 1000);
                        }
                    }
                }
                if (activeRelayCounter == 0 && watt > 10) coreroutineSetAlarm(214, 1, 5, 1000);
            }
            timerAlarm = now;
        }

        if (appState.fResetPowerSensor) {
            appState.fResetPowerSensor = false;
            logger->warn(PSTR(__func__), PSTR("Resetting power sensor energy counter.\n"));
            if (!appConfig.fPowerSensorDummy) {
                pzem.resetEnergy();
            }
            ener = 0;
            ener_start_period = 0;
        }

        appState.powerSensorTaskRoutineLastActivity = millis();
        vTaskDelay((const TickType_t)1000 / portTICK_PERIOD_MS);
    }
}

void coreroutineRelayControlTaskRoutine(void *arg){
  for(uint8_t i = 0; i < 4; i++){
    IOExtender.pinMode(relays[i].pin, OUTPUT);
    logger->verbose(PSTR(__func__), PSTR("Relay %d initialized as output.\n"), relays[i].pin);
  }

  appState.fIOExtender =  IOExtender.begin();

  if(!appState.fIOExtender){
    //logger->error(PSTR(__func__), PSTR("Failed to initialize IOExtender!\n"));
  }
  else{
    logger->verbose(PSTR(__func__), PSTR("IOExtender initialized.\n"));
  }

  for(uint8_t i = 0; i < 4; i++){
    if(relays[i].mode == 0){
      coreroutineSetRelay(i, relays[i].state);
      logger->debug(PSTR(__func__), PSTR("Relay %d is initialized as %d.\n"), i+1, relays[i].state);
    }
  }

  unsigned long timerAlarm = millis();
  while (true)
  {
    if(!appState.fIOExtender){
      for(uint8_t i = 0; i < 4; i++){
        IOExtender.pinMode(relays[i].pin, OUTPUT);
      }
      appState.fIOExtender = IOExtender.begin();
    }

    vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
    if(appState.fPanic){continue;}
    unsigned long now = millis();

    /* Start alarm section */
    if(now - timerAlarm > 30000){
      if(!appState.fIOExtender){
        coreroutineSetAlarm(220, 1, 10, 500);
      }

      timerAlarm = now;
    }
    /* End alarm section */

    for(uint8_t i = 0; i < 4; i++){
      /* Start Manual Control Mode*/
      if(relays[i].mode == 0 && relays[i].autoOff != 0){
        if( (now - relays[i].lastActive) >= ( relays[i].autoOff * 1000 ) && relays[i].state == true){
          coreroutineSetRelay(i, false);
          logger->debug(PSTR(__func__), PSTR("Relay %d is turned off due to autoOff.\n"), i+1);
        }
      }
      /* End Manual Control Mode*/
      /* Start Duty Cycle Control Mode */
      if(relays[i].dutyRange < 2){relays[i].dutyRange = 2;} //safenet
      if(relays[i].dutyCycle != 0 && relays[i].mode == 1) {
        if( relays[i].state == true )
        {
          if( relays[i].dutyCycle != 100 && (now - relays[i].lastChanged ) >= (float)(( ((float)relays[i].dutyCycle / 100) * (float)relays[i].dutyRange) * 1000))
          {
            coreroutineSetRelay(i, false);
            relays[i].state = false;
            relays[i].lastChanged = now;
          }
        }
        else
        {
          if( relays[i].dutyCycle != 0 && (now - relays[i].lastChanged ) >= (float) ( ((100 - (float) relays[i].dutyCycle) / 100) * (float)relays[i].dutyRange) * 1000)
          {
            coreroutineSetRelay(i, true);
            relays[i].state = true;
            relays[i].lastChanged = now;
          }
        }
      }
      /* End Duty Cycle Control Mode */
      /* Start Time Daily Control Mode */
      if(relays[i].mode == 2){
        bool flag_isInTimeWindow = false;
        int activeTimeWindowCounter = 0;
        for(uint8_t j = 0; j < maxTimers; j++)
        {
          int currHour = RTC.getHour(true);
          int currHourToSec = currHour * 3600;
          int currMinute = RTC.getMinute();
          int currMinuteToSec = currMinute * 60;
          int currSecond = RTC.getSecond();
          String currDT = RTC.getDateTime();
          int currentTimeInSec = currHourToSec + currMinuteToSec + currSecond;

          int duration = relays[i].timers[j].duration;
          int targetHour = relays[i].timers[j].hour;
          int targetHourToSec = targetHour * 3600;
          int targetMinute = relays[i].timers[j].minute;
          int targetMinuteToSec =targetMinute * 60;
          int targetSecond = relays[i].timers[j].second;
          int targetTimeInSec = targetHourToSec + targetMinuteToSec + targetSecond;

          int activationOffset = targetTimeInSec - currentTimeInSec;
          int deactivationOffset = activationOffset + duration;
          int activeTimeWindow = deactivationOffset - activationOffset;
          flag_isInTimeWindow = ( activationOffset <= 0 && deactivationOffset >= 0 ) ? true : false;
          const char * isInTimeWindow = flag_isInTimeWindow ? "TRUE" : "FALSE";

          //logger->debug(PSTR(__func__), PSTR("Relay %d timer index %d hour %d minute %d second %d duration %d IsInTImeWindow %s activeTimeWindowCounter %d\n"), i+1, j, targetHour, targetMinute, targetSecond, duration, isInTimeWindow, activeTimeWindowCounter);
          if(flag_isInTimeWindow){
            activeTimeWindowCounter++;
          }
        }
        if (relays[i].state == false && activeTimeWindowCounter > 0){
          relays[i].state = true;
          coreroutineSetRelay(i, true);
        }else if(relays[i].state == true && activeTimeWindowCounter < 1) {
          relays[i].state = false;
          coreroutineSetRelay(i, false);
        }
      }
      /* Stop Time Daily Control Mode */
      /* Start Exact Datetime Control Mode */
      if(relays[i].duration > 0 && relays[i].mode == 3){
        if(relays[i].datetime <= (RTC.getEpoch()) && (relays[i].duration) >=
          (RTC.getEpoch() - relays[i].datetime) && relays[i].state == false){
            relays[i].state = true;
            coreroutineSetRelay(i, true);
        }
        else if(relays[i].state == true && (relays[i].duration) <=
          (RTC.getEpoch() - relays[i].datetime)){
            relays[i].state = false;
            coreroutineSetRelay(i, false);
        }
      }
      /* Start Exact Datetime Control Mode */
    }

    appState.relayControlTaskRoutineLastActivity = millis();
  }
}

void coreroutineSetRelay(uint8_t index, bool output){
  if(index < 4){
    if(output){
      IOExtender.digitalWrite(relays[index].pin, appConfig.relayON);
      relays[index].state = true;
      relays[index].lastActive = millis();
      relays[index].lastChanged = millis();
      logger->debug(PSTR(__func__), PSTR("Relay %d is ON. %d was written to relay.\n"), index+1, appConfig.relayON);
      appState.fsyncClientAttributes = true;
      appState.fsaveAppRelay = true;
    }
    else{
      IOExtender.digitalWrite(relays[index].pin, !appConfig.relayON);
      relays[index].state = false;
      relays[index].lastChanged = millis();
      logger->debug(PSTR(__func__), PSTR("Relay %d is OFF. %d was written to relay.\n"), index+1, !appConfig.relayON);
      appState.fsyncClientAttributes = true;
      appState.fsaveAppRelay = true;
    }

    #ifdef USE_IOT
    JsonDocument doc;
    doc[(String("ch") + String(index+1)).c_str()] = relays[index].state;
    iotSendTele(doc);
    #endif
  }
}

#ifdef USE_IOT
void coreroutineProcessTBProvResp(const JsonDocument &data){
  if( iotState.xSemaphoreThingsboard != NULL && WiFi.isConnected() && !config.state.provSent && tb.connected()){
    if( xSemaphoreTake( iotState.xSemaphoreThingsboard, ( TickType_t ) 1000 ) == pdTRUE )
    {
      constexpr char CREDENTIALS_TYPE[] PROGMEM = "credentialsType";
      constexpr char CREDENTIALS_VALUE[] PROGMEM = "credentialsValue";
      String _data;
      serializeJson(data, _data);
      logger->verbose(PSTR(__func__),PSTR("Received device provision response: %s\n"), _data.c_str());

      if (strncmp(data["status"], "SUCCESS", strlen("SUCCESS")) != 0) {
        logger->error(PSTR(__func__),PSTR("Provision response contains the error: (%s)\n"), data["errorMsg"].as<const char*>());
      }
      else
      {
        if (strncmp(data[CREDENTIALS_TYPE], PSTR("ACCESS_TOKEN"), strlen(PSTR("ACCESS_TOKEN"))) == 0) {
          strlcpy(config.state.accTkn, data[CREDENTIALS_VALUE].as<std::string>().c_str(), sizeof(config.state.accTkn));
          config.state.provSent = true;  
          config.save();
          logger->verbose(PSTR(__func__),PSTR("Access token provision response saved.\n"));
        }
        else if (strncmp(data[CREDENTIALS_TYPE], PSTR("MQTT_BASIC"), strlen(PSTR("MQTT_BASIC"))) == 0) {
          /*auto credentials_value = data[CREDENTIALS_VALUE].as<JsonObjectConst>();
          credentials.client_id = credentials_value[CLIENT_ID].as<std::string>();
          credentials.username = credentials_value[CLIENT_USERNAME].as<std::string>();
          credentials.password = credentials_value[CLIENT_PASSWORD].as<std::string>();*/
        }
        else {
          logger->warn(PSTR(__func__),PSTR("Unexpected provision credentialsType: (%s)\n"), data[CREDENTIALS_TYPE].as<const char*>());

        }
      }

      // Disconnect from the cloud client connected to the provision account, because it is no longer needed the device has been provisioned
      // and we can reconnect to the cloud with the newly generated credentials.
      if (tb.connected()) {
        tb.disconnect();
      }
      xSemaphoreGive( iotState.xSemaphoreThingsboard );
    }
    else
    {
      logger->error(PSTR(__func__), PSTR("No semaphore available.\n"));
    }
  }
}

void coreroutineIoTProvRequestTimedOut(){
  logger->verbose(PSTR(__func__), PSTR("Provisioning request timed out.\n"));
}

void iotFinishedCallback(const bool & success){
  if(success){
    logger->info(PSTR(__func__), PSTR("IoT OTA Update done!\n"));
    reboot(10);
  }
  else{
    logger->warn(PSTR(__func__), PSTR("IoT OTA Update failed!\n"));
    reboot(10);
  }
}

void iotProgressCallback(const size_t & currentChunk, const size_t & totalChuncks){
   if( xSemaphoreTake( iotState.xSemaphoreThingsboard, ( TickType_t ) 5000 ) == pdTRUE ) {
    logger->debug(PSTR(__func__), PSTR("IoT OTA Progress: %.2f%%\n"),  static_cast<float>(currentChunk * 100U) / totalChuncks);
    xSemaphoreGive( iotState.xSemaphoreThingsboard );   
  }
}

void iotUpdateStartingCallback(){
  logger->info(PSTR(__func__), PSTR("IoT OTA Update starting...\n"));
  // It's a good practice to stop services that might interfere with the update
  // For example, web server or other network services.
  // coreroutineStopServices(); // You might want to call this if needed.
  //LittleFS.end(); // Unmount filesystem to be safe
}

void coreroutineRunIoT(){
  if(!config.state.provSent){
      if (tb.connect(config.state.tbAddr, PSTR("provision"), config.state.tbPort)) {
        const Provision_Callback provisionCallback(Access_Token(), &coreroutineProcessTBProvResp, config.state.provDK, config.state.provDS, config.state.hwid, IOT_PROVISIONING_TIMEOUT * 1000000, &coreroutineIoTProvRequestTimedOut);
        if(IAPIProv.Provision_Request(provisionCallback))
        {
          logger->info(PSTR(__func__),PSTR("Connected to provisioning server: %s:%d. Sending provisioning response: DK: %s, DS: %s, Id: %s \n"),  
            config.state.tbAddr, config.state.tbPort, config.state.provDK, config.state.provDS, config.state.hwid);
        }
        else{
          logger->warn(PSTR(__func__), PSTR("Provision request failed: %s:%d DK:%s DS:%s ID:%S\n"), config.state.tbAddr, config.state.tbPort, config.state.provDK, config.state.provDS, config.state.hwid);
        }
      }
      else
      {
        logger->warn(PSTR(__func__),PSTR("Failed to connect to provisioning server: %s:%d\n"),  config.state.tbAddr, config.state.tbPort);
      }
      unsigned long timer = millis();
      while(true){
        tb.loop();
        if(config.state.provSent || (millis() - timer) > 10000){break;}
        vTaskDelay((const TickType_t)10 / portTICK_PERIOD_MS);
      }
    }
    else{
      if(!tb.connected() && WiFi.isConnected())
      {
        logger->warn(PSTR(__func__),PSTR("IoT disconnected!\n"));
        //onTbDisconnectedCb();
        logger->info(PSTR(__func__),PSTR("Connecting to broker %s:%d\n"), config.state.tbAddr, config.state.tbPort);
        uint8_t tbDisco = 0;
        const uint8_t maxRetries = 12;
        const TickType_t retryDelay = 5000 / portTICK_PERIOD_MS;
        while(!tb.connect(config.state.tbAddr, config.state.accTkn, config.state.tbPort, config.state.hwid)){  
          tbDisco++;
          logger->warn(PSTR(__func__),PSTR("Failed to connect to IoT Broker %s (%d)\n"), config.state.tbAddr, tbDisco);
          if(tbDisco >= maxRetries){
            config.state.provSent = false;
            tbDisco = 0;
            break;
          }
          vTaskDelay(retryDelay); 
        }

        if(tb.connected()){
          if(!iotState.fSharedAttributesSubscribed){
            Shared_Attribute_Callback coreroutineThingsboardSharedAttributesUpdateCallback(
                [](const JsonVariantConst& data) {
                    logger->verbose(PSTR(__func__), PSTR("Received shared attribute update(s): \n"));
                    serializeJson(data, Serial);

                    JsonDocument doc;
                    DeserializationError error = deserializeJson(doc, data.as<String>());
                    if (error) {
                        logger->error(PSTR(__func__), PSTR("Failed to parse shared attributes: %s\n"), error.c_str());
                        return;
                    }

                    storageConvertAppConfig(doc, true);
                    storageConvertAppState(doc, true);
                    storageConvertAppRelay(doc, true);
                    storageConvertUdawaConfig(doc, true);
                }
            );
            iotState.fSharedAttributesSubscribed = IAPISharedAttr.Shared_Attributes_Subscribe(coreroutineThingsboardSharedAttributesUpdateCallback);
            if (iotState.fSharedAttributesSubscribed){
              logger->verbose(PSTR(__func__), PSTR("Thingsboard shared attributes update subscribed successfuly.\n"));
            }
            else{
              logger->warn(PSTR(__func__), PSTR("Failed to subscribe Thingsboard shared attributes update.\n"));
            }
          }

          if(!iotState.fSetRelayRPCSubscribed){
            RPC_Callback setRelayCallback("setRelay", [](const JsonVariantConst& params, JsonDocument& result) {
              serializeJsonPretty(params, Serial);
                if (params.is<JsonObjectConst>()) {
                  JsonObjectConst paramObj = params.as<JsonObjectConst>();
                  if (paramObj[PSTR("pin")].is<uint8_t>() && paramObj[PSTR("state")].is<bool>()) {
                    uint8_t pin = paramObj[PSTR("pin")].as<uint8_t>();
                    bool state = paramObj[PSTR("state")].as<bool>();
                    coreroutineSetRelay(pin, state);
                    result["status"] = "success";
                  } else {
                    result["status"] = "error";
                    result["error"] = "Invalid parameters";
                  }
                } else {
                  result["status"] = "error";
                  result["error"] = "Invalid request format";
                }
            });
            iotState.fSetRelayRPCSubscribed = IAPIRPC.RPC_Subscribe(setRelayCallback);
            if(iotState.fSetRelayRPCSubscribed){
              logger->verbose(PSTR(__func__), PSTR("setRelay RPC subscribed successfuly.\n"));
            }
            else{
              logger->warn(PSTR(__func__), PSTR("Failed to subscribe setRelay RPC.\n"));
            }
          }
          if(!iotState.fRebootRPCSubscribed){
            RPC_Callback rebootCallback("reboot", [](const JsonVariantConst& params, JsonDocument& result) {
                int countdown = 0;
                if (params.is<int>()) {
                    countdown = params.as<int>();
                }
                reboot(countdown);
                result["status"] = "rebooting";
            });
            iotState.fRebootRPCSubscribed = IAPIRPC.RPC_Subscribe(rebootCallback); // Pass the callback directly
            if(iotState.fRebootRPCSubscribed){
              logger->verbose(PSTR(__func__), PSTR("reboot RPC subscribed successfuly.\n"));
            }
            else{
              logger->warn(PSTR(__func__), PSTR("Failed to subscribe reboot RPC.\n"));
            }
          }

          if(!iotState.fConfigSaveRPCSubscribed){
            RPC_Callback configSaveCallback("configSave", [](const JsonVariantConst& params, JsonDocument& result) {
                config.save();
                result["status"] = "config saved";
            });
            iotState.fConfigSaveRPCSubscribed = IAPIRPC.RPC_Subscribe(configSaveCallback); // Pass the callback directly
            if(iotState.fConfigSaveRPCSubscribed){
              logger->verbose(PSTR(__func__), PSTR("configSave RPC subscribed successfuly.\n"));
            }
            else{
              logger->warn(PSTR(__func__), PSTR("Failed to subscribe configSave RPC.\n"));
            }
          }

          if(!iotState.fFSUpdateRPCSubscribed){
            RPC_Callback fsUpdateCallback("FSUpdate", [](const JsonVariantConst& params, JsonDocument& result) {
                xTaskCreate(
                    coreroutineFSDownloaderTask,    // Function that implements the task.
                    "FSDownloader",                 // Text name for the task.
                    8192,                           // Stack size in words, not bytes.
                    NULL,                           // Parameter passed into the task.
                    1,                              // Priority at which the task is created.
                    NULL                            // Used to pass out the created task's handle.
                );
                result["status"] = "FS Update task started";
            });
            iotState.fFSUpdateRPCSubscribed = IAPIRPC.RPC_Subscribe(fsUpdateCallback); // Pass the callback directly
            if(iotState.fFSUpdateRPCSubscribed){
              logger->verbose(PSTR(__func__), PSTR("FSUpdate RPC subscribed successfuly.\n"));
            }
            else{
              logger->warn(PSTR(__func__), PSTR("Failed to subscribe FSUpdate RPC.\n"));
            }
          }

          #ifdef USE_IOT_OTA
          const OTA_Update_Callback coreroutineIoTUpdaterOTACallback(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &IoTUpdater, &iotFinishedCallback, &iotProgressCallback, &iotUpdateStartingCallback, IOT_OTA_UPDATE_FAILURE_RETRY, IOT_OTA_UPDATE_PACKET_SIZE);
          if (IAPIOta.Start_Firmware_Update(coreroutineIoTUpdaterOTACallback)) {
              logger->debug(PSTR(__func__), PSTR("Initial firmware update check started.\n"));
          } else {
              logger->error(PSTR(__func__), PSTR("Initial firmware update check failed to start.\n"));
          }
          if (IAPIOta.Subscribe_Firmware_Update(coreroutineIoTUpdaterOTACallback)) {
              logger->debug(PSTR(__func__), PSTR("Subscribed to firmware updates.\n"));
          } else {
              logger->error(PSTR(__func__), PSTR("Failed to subscribe to firmware updates.\n"));
          }
          #endif

          coreroutineSyncClientAttr(2);          
          logger->info(PSTR(__func__),PSTR("IoT Connected!\n"));
        }
      }
    }

    tb.loop();
}


bool iotSendAttr(JsonDocument &doc){
  bool res = false;
  if( iotState.xSemaphoreThingsboard != NULL && WiFi.isConnected() && config.state.provSent && tb.connected() && config.state.accTkn != NULL){
    if( xSemaphoreTake( iotState.xSemaphoreThingsboard, ( TickType_t ) 10000 ) == pdTRUE )
    {
      res = tb.Send_Attribute_Json(doc);
      xSemaphoreGive( iotState.xSemaphoreThingsboard );
    }
    else
    {
      logger->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
    }
  }
  return res;
}

bool iotSendTele(JsonDocument &doc){
  bool res = false;
  if( iotState.xSemaphoreThingsboard != NULL && WiFi.isConnected() && config.state.provSent && tb.connected() && config.state.accTkn != NULL){
    if( xSemaphoreTake( iotState.xSemaphoreThingsboard, ( TickType_t ) 10000 ) == pdTRUE )
    {
      res = tb.Send_Telemetry_Json(doc);
      xSemaphoreGive( iotState.xSemaphoreThingsboard );
    }
    else
    {
      logger->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
    }
  }
  return res;
}
#endif