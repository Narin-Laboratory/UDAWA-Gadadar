/**
 * UDAWA - Universal Digital Agriculture Watering Assistant
 * Firmware for Actuator 4Ch UDAWA Board (Gadadar)
 * Licensed under aGPLv3
 * Researched and developed by PRITA Research Group & Narin Laboratory
 * prita.undiknas.ac.itD | narin.co.itD
**/
#include "main.h"
#define S1_TX 32
#define S1_RX 4

using namespace libudawa;
Settings mySettings;
Adafruit_BME280 bme;

BaseType_t xReturnedWsSendTelemetry;
BaseType_t xReturnedWsSendSensors;
BaseType_t xReturnedRecPowerUsage;
BaseType_t xReturnedRecWeatherData;
BaseType_t xReturnedPublishSwitch;
BaseType_t xReturnedRelayControl;

TaskHandle_t xHandleWsSendTelemetry = NULL;
TaskHandle_t xHandleWsSendSensors = NULL;
TaskHandle_t xHandleRecPowerUsage = NULL;
TaskHandle_t xHandleRecWeatherData = NULL;
TaskHandle_t xHandlePublishSwitch = NULL;
TaskHandle_t xHandleRelayControl = NULL;

void setup()
{
  long startMillis = millis();

  processSharedAttributeUpdateCb = &attUpdateCb;
  onTbDisconnectedCb = &onTbDisconnected;
  onTbConnectedCb = &onTbConnected;
  processSaveSettingsCb = &saveSettings;
  processSetPanicCb = &setPanic;
  processGenericClientRPCCb = &genericClientRPC;
  plannedRebootOnEnableCb = &onReboot;
  emitAlarmCb = &onAlarm;
  onSyncClientAttrCb = &onSyncClientAttr;
  #ifdef USE_WEB_IFACE
  wsEventCb = &onWsEvent;
  #endif
  startup();
  loadSettings();
  syncConfigCoMCU();
  if(String(config.model) == String("Generic")){
    strlcpy(config.model, "Gadadar", sizeof(config.model));
  }
  stateReset(0);

  tb.setBufferSize(1024);

  #ifdef USE_WEB_IFACE
  xQueuePZEMMessage = xQueueCreate( 1, sizeof( struct PZEMMessage ) );
  xQueueBME280Message = xQueueCreate( 1, sizeof( struct BME280Message ) );
  #endif

  mySettings.flag_bme280 = bme.begin(0x76);
  if(!mySettings.flag_bme280){
    log_manager->warn(PSTR(__func__),PSTR("BME weather sensor failed to initialize!\n"));
  }else{
    if(xHandleRecWeatherData == NULL){
      xReturnedRecWeatherData = xTaskCreatePinnedToCore(recWeatherDataTR, PSTR("recWeatherData"), 6144, NULL, 1, &xHandleRecWeatherData, 1);
      if(xReturnedRecWeatherData == pdPASS){
        log_manager->warn(PSTR(__func__), PSTR("Task recWeatherData has been created.\n"));
      }
    }
  }

  if(xHandleRecPowerUsage == NULL){
    xReturnedRecPowerUsage = xTaskCreatePinnedToCore(recPowerUsageTR, PSTR("recPowerUsage"), 6144, NULL, 1, &xHandleRecPowerUsage, 1);
    if(xReturnedRecPowerUsage == pdPASS){
      log_manager->warn(PSTR(__func__), PSTR("Task recPowerUsage has been created.\n"));
    }
  }

  if(xHandlePublishSwitch == NULL){
    xReturnedPublishSwitch = xTaskCreatePinnedToCore(publishSwitchTR, PSTR("publishSwitch"), 4096, NULL, 1, &xHandlePublishSwitch, 1);
    if(xReturnedPublishSwitch == pdPASS){
      log_manager->warn(PSTR(__func__), PSTR("Task publishSwitch has been created.\n"));
    }
  }

  if(xHandleRelayControl == NULL){
    xReturnedRelayControl = xTaskCreatePinnedToCore(relayControlTR, PSTR("relayControl"), 6144, NULL, 1, &xHandleRelayControl, 1);
    if(xReturnedRelayControl == pdPASS){
      log_manager->warn(PSTR(__func__), PSTR("Task relayControl has been created.\n"));
    }
  }

  #ifdef USE_WEB_IFACE
  if(xHandleWsSendTelemetry == NULL){
    xReturnedWsSendTelemetry = xTaskCreatePinnedToCore(wsSendTelemetryTR, PSTR("wsSendTelemetry"), 3072, NULL, 1, &xHandleWsSendTelemetry, 1);
    if(xReturnedWsSendTelemetry == pdPASS){
      log_manager->warn(PSTR(__func__), PSTR("Task wsSendTelemetry has been created.\n"));
    }
  }

  if(xHandleWsSendSensors == NULL){
    xReturnedWsSendSensors = xTaskCreatePinnedToCore(wsSendSensorsTR, PSTR("wsSendSensors"), 5120, NULL, 1, &xHandleWsSendSensors, 1);
    if(xReturnedWsSendSensors == pdPASS){
      log_manager->warn(PSTR(__func__), PSTR("Task wsSendSensors has been created.\n"));
    }
  }
  #endif

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void loop()
{
  udawa();
}

void recPowerUsageTR(void *arg){
  Stream_Stats<float> _volt_;
  Stream_Stats<float> _amp_;
  Stream_Stats<float> _watt_;
  Stream_Stats<float> _freq_;
  Stream_Stats<float> _pf_;

  unsigned long timerCalcPowerUsage = millis();
  unsigned long timerRecPowerUsage = millis();
  unsigned long timerAlarm = millis();
  
  while(true){
   
    HardwareSerial PZEMSerial(1);
    PZEM004Tv30 PZEM(PZEMSerial, S1_RX, S1_TX);

    float volt = PZEM.voltage();
    float amp = PZEM.current();
    float watt = PZEM.power();
    float ener = PZEM.energy();
    float freq = PZEM.frequency();
    float pf = PZEM.pf();

    #ifdef USE_WEB_IFACE
    if( xQueuePZEMMessage != NULL && (config.wsCount > 0)){
      PZEMMessage PZEMMsg;
      PZEMMsg.amp = amp; PZEMMsg.watt = watt; PZEMMsg.ener = ener; PZEMMsg.freq = freq; PZEMMsg.pf = pf; PZEMMsg.volt = volt;
      if( xQueueSend( xQueuePZEMMessage, &PZEMMsg, ( TickType_t ) 1000 ) != pdPASS )
      {
          log_manager->debug(PSTR(__func__), PSTR("Failed to fill PZEMMsg. Queue is full. \n"));
      }
    }
    #endif
        
    if(tb.connected() && config.provSent){
      StaticJsonDocument<DOCSIZE_MIN> doc;
      float ener;
      unsigned long now = millis();
      if( (now - timerCalcPowerUsage) > (mySettings.itPc * 1000)){
        
        doc[PSTR("_volt")] = volt;
        doc[PSTR("_amp")] = amp;
        doc[PSTR("_watt")] = watt;
        doc[PSTR("_freq")] = freq;
        doc[PSTR("_pf")] = pf;
        doc[PSTR("_ener")] = ener;
        
        tbSendAttribute(doc);
        doc.clear();

        timerCalcPowerUsage = now;
      }

      now = millis();
      if( (now - timerRecPowerUsage) > (mySettings.itP * 1000) ){
        doc[PSTR("volt")] = _volt_.Get_Average();
        doc[PSTR("amp")] = _amp_.Get_Average();
        doc[PSTR("watt")] = _watt_.Get_Average();
        doc[PSTR("freq")] = _freq_.Get_Average();
        doc[PSTR("pf")] = _pf_.Get_Average();
        doc[PSTR("ener")] = ener;

        if(tbSendTelemetry(doc)){
          _volt_.Clear(); _amp_.Clear(); _watt_.Clear(); _freq_.Clear(); _pf_.Clear();
        }
        
        timerRecPowerUsage = now;
      }
    }

    unsigned long now = millis();
    if( (now - timerAlarm) > 30000 ){
      if(volt <= 0){
        setAlarm(121, 1, 5, 1000);
      }

      uint8_t ACTIVE_CH_COUNTER = 0;
      for(uint8_t i = 0; i < 4; i++){
        if(mySettings.dutyState[i] == mySettings.ON){
          ACTIVE_CH_COUNTER++;
        }

        if(mySettings.stateOnTs[i] > 0){
          if(millis() - mySettings.stateOnTs[i] > 3000000){
            if(i = 0){
              setAlarm(211, 1, 10, 1000);
            }
            else if(i = 0){
              setAlarm(212, 1, 10, 1000);
            }
            else if(i = 0){
              setAlarm(213, 1, 10, 1000);
            }
            else if(i = 0){
              setAlarm(214, 1, 10, 1000);
            }
          }
        }
      }

      if(ACTIVE_CH_COUNTER > 0 && (int)watt < 10){
        setAlarm(221, 1, 10, 1000);
      }

      if(ACTIVE_CH_COUNTER == 0 && (int)watt > 10){
        setAlarm(222, 1, 10, 1000);
      }
      timerAlarm = now;
    }

    vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
  }
}

void recWeatherDataTR(void *arg){
  Stream_Stats<float> _celc_;
  Stream_Stats<float> _rh_;
  Stream_Stats<float> _hpa_;
  Stream_Stats<float> _alt_;
  
  unsigned long timerCalcWeatherData = millis();
  unsigned long timerRecWeatherData = millis();

  while(true){
    float celc = 0.0;
    float rh = 0.0;
    float hpa = 0.0;
    float alt = 0.0;

    if(!mySettings.flag_bme280){
      setAlarm(111, 1, 5, 1000);
    }
    else{      
      celc = bme.readTemperature();
      rh = bme.readHumidity();
      hpa = bme.readPressure() / 100.0F;
      alt = bme.readAltitude(mySettings.seaHpa);

      _celc_.Add(celc);
      _rh_.Add(rh);
      _hpa_.Add(hpa);
      _alt_.Add(alt);
      
      #ifdef USE_WEB_IFACE
      if( xQueueBME280Message != NULL && (config.wsCount > 0) ){
        BME280Message BME280Msg;
        BME280Msg.celc = celc; BME280Msg.rh = rh; BME280Msg.hpa = hpa; BME280Msg.alt = alt;
        if( xQueueSend( xQueueBME280Message, &BME280Msg, ( TickType_t ) 1000 ) != pdPASS )
        {
            log_manager->debug(PSTR(__func__), PSTR("Failed to fill BME280Msg. Queue is full. \n"));
        }
      }
      #endif

      if(config.provSent && tb.connected() && config.fIoT){
        StaticJsonDocument<DOCSIZE_MIN> doc;

        unsigned long now = millis();
        if( (now - timerCalcWeatherData) > (mySettings.itWc * 1000) ){
          doc[PSTR("_celc")] = celc;
          doc[PSTR("_rh")] = rh;
          doc[PSTR("_hpa")] = hpa;
          doc[PSTR("_alt")] = alt;

          tbSendAttribute(doc);
          doc.clear();

          timerCalcWeatherData = now;
        }

        if( (now - timerRecWeatherData) > (mySettings.itW * 1000) ){
          doc[PSTR("celc")] = _celc_.Get_Average();
          doc[PSTR("rh")] = _rh_.Get_Average();
          doc[PSTR("hpa")] = _hpa_.Get_Average();
          doc[PSTR("alt")] = _alt_.Get_Average();

          if(tbSendTelemetry(doc)){
            _celc_.Clear(); _rh_.Clear(); _hpa_.Clear(); _alt_.Clear(); 
          }

          timerRecWeatherData = now;
        }
      }
    }
    vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
  }
}

void loadSettings()
{
  long startMillis = millis();

  StaticJsonDocument<DOCSIZE> doc;
  readSettings(doc, settingsPath);

  if(doc["cpM"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["cpM"].as<JsonArray>()) { mySettings.cpM[index] = v.as<uint8_t>(); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.cpM); i++) { mySettings.cpM[i] = 0; } }

  if(doc["cp1A"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["cp1A"].as<JsonArray>()) { mySettings.cp1A[index] = v.as<uint8_t>(); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.cp1A); i++) { mySettings.cp1A[i] = 0; } }

  if(doc["cp1B"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["cp1B"].as<JsonArray>()) { mySettings.cp1B[index] = v.as<unsigned long>(); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.cp1B); i++) { mySettings.cp1B[i] = 0; } }  

  if(doc["cp2A"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["cp2A"].as<JsonArray>()) { mySettings.cp2A[index] = v.as<unsigned long>(); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.cp2A); i++) { mySettings.cp2A[i] = 0; } }

  if(doc["cp2B"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["cp2B"].as<JsonArray>()) { mySettings.cp2B[index] = v.as<unsigned long>(); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.cp2B); i++) { mySettings.cp2B[i] = 0; } }

  if(doc["cp3A"] != nullptr) {
    for(uint8_t i=0; i<countof(mySettings.cp3A); i++)
    {
      serializeJson(doc["cp3A"][i], mySettings.cp3A[i]);
    }
  }


  if(doc["cp4A"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["cp4A"].as<JsonArray>()) { mySettings.cp4A[index] = v.as<unsigned long>(); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.cp4A); i++) { mySettings.cp4A[i] = 0; } }

  if(doc["cp4B"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["cp4B"].as<JsonArray>()) { mySettings.cp4B[index] = v.as<unsigned long>(); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.cp4B); i++) { mySettings.cp4B[i] = 0; } }

  if(doc["pR"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["pR"].as<JsonArray>()) { mySettings.pR[index] = v.as<uint8_t>(); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.pR); i++) { mySettings.pR[i] = 7+i; } }

  if(doc["ON"] != nullptr) { mySettings.ON = doc["ON"].as<uint8_t>(); } else { mySettings.ON = 1; }

  if(doc["itP"] != nullptr){mySettings.itP = doc["itP"].as<uint16_t>();}
  else{mySettings.itP = 900;}

  if(doc["itW"] != nullptr){mySettings.itW = doc["itW"].as<uint16_t>();}
  else{mySettings.itW = 300;}

  if(doc["itD"] != nullptr){mySettings.itD = doc["itD"].as<uint16_t>();}
  else{mySettings.itD = 15;}

  if(doc["itPc"] != nullptr){mySettings.itPc = doc["itPc"].as<uint16_t>();}
  else{mySettings.itPc = 3;}

  if(doc["itWc"] != nullptr){mySettings.itWc = doc["itWc"].as<uint16_t>();}
  else{mySettings.itWc = 3;}


  for(uint8_t i = 0; i < countof(mySettings.dutyCounter); i++) { mySettings.dutyCounter[i] = 86400; }

  if(doc["seaHpa"] != nullptr){mySettings.seaHpa = doc["seaHpa"].as<float>();}
  else{mySettings.seaHpa = 1019.00;}

  if(doc["lbl"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["lbl"].as<JsonArray>()) { strlcpy(mySettings.lbl[index], v.as<const char*>(), sizeof(mySettings.lbl[index])); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.lbl); i++) { strlcpy(mySettings.lbl[i], "unnamed", sizeof(mySettings.lbl[i])); } }

  String tmp;
  if(config.logLev >= 4){serializeJson(doc, tmp);}
  log_manager->debug(PSTR(__func__), "Loaded settings:\n %s \n", tmp.c_str());

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void saveSettings()
{
  long startMillis = millis();

  StaticJsonDocument<DOCSIZE> doc;

  JsonArray cp1A = doc.createNestedArray("cp1A");
  for(uint8_t i=0; i<countof(mySettings.cp1A); i++)
  {
    cp1A.add(mySettings.cp1A[i]);
  }

  JsonArray cp1B = doc.createNestedArray("cp1B");
  for(uint8_t i=0; i<countof(mySettings.cp1B); i++)
  {
    cp1B.add(mySettings.cp1B[i]);
  }

  JsonArray cp2A = doc.createNestedArray("cp2A");
  for(uint8_t i=0; i<countof(mySettings.cp2A); i++)
  {
    cp2A.add(mySettings.cp2A[i]);
  }

  JsonArray cp2B = doc.createNestedArray("cp2B");
  for(uint8_t i=0; i<countof(mySettings.cp2B); i++)
  {
    cp2B.add(mySettings.cp2B[i]);
  }

  JsonArray cp3A = doc.createNestedArray("cp3A");
  for(uint8_t i=0; i<countof(mySettings.cp3A); i++)
  {
    String index = String("cp3A") + String(i + 1);
    StaticJsonDocument<DOCSIZE_MIN> data;
    DeserializationError err = deserializeJson(data, mySettings.cp3A[i]);
    if(err == DeserializationError::Ok){
      cp3A.add(data);
    }else{
      log_manager->verbose(PSTR(__func__),PSTR("%s setting parse failed: %s (%s).\n"), index.c_str(), mySettings.cp3A[i].c_str(), err.c_str());
    }
  }

  JsonArray cp4A = doc.createNestedArray("cp4A");
  for(uint8_t i=0; i<countof(mySettings.cp4A); i++)
  {
    cp4A.add(mySettings.cp4A[i]);
  }

  JsonArray cp4B = doc.createNestedArray("cp4B");
  for(uint8_t i=0; i<countof(mySettings.cp4B); i++)
  {
    cp4B.add(mySettings.cp4B[i]);
  }

  JsonArray pR = doc.createNestedArray("pR");
  for(uint8_t i=0; i<countof(mySettings.pR); i++)
  {
    pR.add(mySettings.pR[i]);
  }

  JsonArray lbl = doc.createNestedArray("lbl");
  for(uint8_t i=0; i<countof(mySettings.lbl); i++)
  {
    lbl.add(mySettings.lbl[i]);
  }

  doc["ON"] = mySettings.ON;
  doc["itP"] = mySettings.itP;
  doc["itW"] = mySettings.itW;
  doc["itPc"] = mySettings.itPc;
  doc["itWc"] = mySettings.itWc;
  doc["itD"] = mySettings.itD;

  JsonArray cpM = doc.createNestedArray("cpM");
  for(uint8_t i=0; i<countof(mySettings.cpM); i++)
  {
    cpM.add(mySettings.cpM[i]);
  }

  doc["seaHpa"] = mySettings.seaHpa;

  writeSettings(doc, settingsPath);
  String tmp;
  if(config.logLev >= 4){serializeJson(doc, tmp);}
  log_manager->debug(PSTR(__func__), "Written settings:\n %s \n", tmp.c_str());

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void setSwitch(String ch, String state)
{
  //long startMillis = millis();

  bool fState = 0;
  uint8_t pR = 0;
  uint8_t itD = 0;

  if(ch == String("ch1")){itD = 0; pR = mySettings.pR[0]; mySettings.dutyState[0] = (state == String("ON")) ? mySettings.ON : 1 - mySettings.ON; mySettings.publishSwitch[0] = true;}
  else if(ch == String("ch2")){itD = 1; pR = mySettings.pR[1]; mySettings.dutyState[1] = (state == String("ON")) ? mySettings.ON : 1 - mySettings.ON; mySettings.publishSwitch[1] = true;}
  else if(ch == String("ch3")){itD = 2; pR = mySettings.pR[2]; mySettings.dutyState[2] = (state == String("ON")) ? mySettings.ON : 1 - mySettings.ON; mySettings.publishSwitch[2] = true;}
  else if(ch == String("ch4")){itD = 3; pR = mySettings.pR[3]; mySettings.dutyState[3] = (state == String("ON")) ? mySettings.ON : 1 - mySettings.ON; mySettings.publishSwitch[3] = true;}

  if(state == String("ON"))
  {
    fState = mySettings.ON;
    mySettings.stateOnTs[itD] = millis();
  }
  else
  {
    fState = 1 - mySettings.ON;
    mySettings.stateOnTs[itD] = 0;
  }

  setCoMCUPin(pR, 1, OUTPUT, 0, fState);
  log_manager->warn(PSTR(__func__), "Relay %s was set to %s / %d.\n", ch, state, (int)fState);
  
  //log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void relayControlTR(void *arg){
  while(true){
    relayControlCP1();
    relayControlCP2();
    relayControlCP3();
    relayControlCP4();

    vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
  }
}

void relayControlCP1(){
  for(uint8_t i = 0; i < countof(mySettings.pR); i++)
  {
    if (mySettings.cp1B[i] < 2){mySettings.cp1B[i] = 2;} //safenet
    if(mySettings.cp1A[i] != 0 && mySettings.cpM[i] == 1)
    {
      if( mySettings.dutyState[i] == mySettings.ON )
      {
        if( mySettings.cp1A[i] != 100 && (millis() - mySettings.dutyCounter[i] ) >= (float)(( ((float)mySettings.cp1A[i] / 100) * (float)mySettings.cp1B[i]) * 1000))
        {
          mySettings.dutyState[i] = 1 - mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "OFF");
          mySettings.dutyCounter[i] = millis();
        }
      }
      else
      {
        if( mySettings.cp1A[i] != 0 && (millis() - mySettings.dutyCounter[i] ) >= (float) ( ((100 - (float) mySettings.cp1A[i]) / 100) * (float) mySettings.cp1B[i]) * 1000)
        {
          mySettings.dutyState[i] = mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "ON");
          mySettings.dutyCounter[i] = millis();
        }
      }
    }
  }
}

void relayControlCP2(){
  for(uint8_t i = 0; i < countof(mySettings.pR); i++)
  {
    if(mySettings.cp2B[i] > 0 && mySettings.cpM[i] == 2){
      if(mySettings.cp2A[i] <= (rtc.getEpoch()) && (mySettings.cp2B[i]) >=
        (rtc.getEpoch() - mySettings.cp2A[i]) && mySettings.dutyState[i] != mySettings.ON){
          mySettings.dutyState[i] = mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "ON");
      }
      else if(mySettings.dutyState[i] == mySettings.ON && (mySettings.cp2B[i]) <=
        (rtc.getEpoch() - mySettings.cp2A[i])){
          mySettings.dutyState[i] = 1 - mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "OFF");
      }
    }
  }
}

void relayControlCP3(){
  for(uint8_t i = 0; i < countof(mySettings.pR); i++){
    if(mySettings.cpM[i] == 3){
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, mySettings.cp3A[i]);
      if(error != DeserializationError::Ok){
        log_manager->warn(PSTR(__func__),PSTR("Failed to parse JSON for CH%d: %s\n"),i+1, mySettings.cp3A[i].c_str());
        delay(1000);
        break;
      }
      bool flag_isInTimeWindow = false;
      int activeTimeWindowCounter = 0;
      for(JsonVariant v : doc.as<JsonArray>())
      {
        int currHour = rtc.getHour(true);
        int currHourToSec = currHour * 3600;
        int currMinute = rtc.getMinute();
        int currMinuteToSec = currMinute * 60;
        int currSecond = rtc.getSecond();
        String currDT = rtc.getDateTime();
        int currentTimeInSec = currHourToSec + currMinuteToSec + currSecond;

        int d = v["d"].as<int>();
        int targetHour = v["h"].as<int>();
        int targetHourToSec = targetHour * 3600;
        int targetMinute = v["i"].as<int>();
        int targetMinuteToSec =targetMinute * 60;
        int targetSecond =v["s"].as<int>();
        int targetTimeInSec = targetHourToSec + targetMinuteToSec + targetSecond;

        int activationOffset = targetTimeInSec - currentTimeInSec;
        int deactivationOffset = activationOffset + d;
        int activeTimeWindow = deactivationOffset - activationOffset;
        flag_isInTimeWindow = ( activationOffset <= 0 && deactivationOffset >= 0 ) ? true : false;
        const char * isInTimeWindow = flag_isInTimeWindow ? "TRUE" : "FALSE";

        if(flag_isInTimeWindow){
          activeTimeWindowCounter++;
        }
      }
      if (mySettings.dutyState[i] != mySettings.ON && activeTimeWindowCounter > 0){
        mySettings.dutyState[i] = mySettings.ON;
        String ch = "ch" + String(i+1);
        setSwitch(ch, "ON");
      }else if(mySettings.dutyState[i] == mySettings.ON && activeTimeWindowCounter < 1) {
        mySettings.dutyState[i] = 1 - mySettings.ON;
        String ch = "ch" + String(i+1);
        setSwitch(ch, "OFF");
      }
    }
  }
}

void relayControlCP4(){
  for(uint8_t i = 0; i < countof(mySettings.pR); i++)
  {
    if(mySettings.cp4A[i] != 0 && mySettings.cp4B[i] != 0 && mySettings.cpM[i] == 4)
    {
      if( mySettings.dutyState[i] == 1 - mySettings.ON && (millis() - mySettings.cp4BTs[i]) > (mySettings.cp4A[i] * 1000) ){
        mySettings.dutyState[i] = mySettings.ON;
        String ch = "ch" + String(i+1);
        setSwitch(ch, "ON");
        mySettings.cp4BTs[i] = millis();
      }
      if( mySettings.dutyState[i] == mySettings.ON && (millis() - mySettings.cp4BTs[i]) > (mySettings.cp4B[i] * 1000) ){
        mySettings.dutyState[i] = 1 - mySettings.ON;
        String ch = "ch" + String(i+1);
        setSwitch(ch, "OFF");
      }
    }
  } 
}

void attUpdateCb(const Shared_Attribute_Data &data)
{
  long startMillis = millis();

  if(data["cp1A1"] != nullptr){
    if( xSemaphoreSettings != NULL ){
      if( xSemaphoreTake( xSemaphoreSettings, ( TickType_t ) 1000 ) == pdTRUE )
      {
        mySettings.cp1A[0] = data["cp1A1"].as<uint8_t>();
        xSemaphoreGive( xSemaphoreSettings );
      }
      else
      {
        log_manager->verbose(PSTR(__func__), PSTR("No semaphore available.\n."));
      }
    }

    if(data["cp1A1"].as<uint8_t>() == 0)
    {
      setSwitch(String("ch1"), String("OFF"));
    }
  }
  if(data["cp1A2"] != nullptr)
  {
    if( xSemaphoreSettings != NULL ){
      if( xSemaphoreTake( xSemaphoreSettings, ( TickType_t ) 1000 ) == pdTRUE )
      {
        mySettings.cp1A[1] = data["cp1A2"].as<uint8_t>();
        xSemaphoreGive( xSemaphoreSettings );
      }
      else
      {
        log_manager->verbose(PSTR(__func__), PSTR("No semaphore available.\n."));
      }
    }
    if(data["cp1A2"].as<uint8_t>() == 0)
    {
      setSwitch(String("ch2"), String("OFF"));
    }
  }
  if(data["cp1A3"] != nullptr)
  {
    if( xSemaphoreSettings != NULL ){
      if( xSemaphoreTake( xSemaphoreSettings, ( TickType_t ) 1000 ) == pdTRUE )
      {
        mySettings.cp1A[2] = data["cp1A3"].as<uint8_t>();
        xSemaphoreGive( xSemaphoreSettings );
      }
      else
      {
        log_manager->verbose(PSTR(__func__), PSTR("No semaphore available.\n."));
      }
    }
    if(data["cp1A3"].as<uint8_t>() == 0)
    {
      setSwitch(String("ch3"), String("OFF"));
    }
  }
  if(data["cp1A4"] != nullptr)
  {
    if( xSemaphoreSettings != NULL ){
      if( xSemaphoreTake( xSemaphoreSettings, ( TickType_t ) 1000 ) == pdTRUE )
      {
        mySettings.cp1A[3] = data["cp1A4"].as<uint8_t>();
        xSemaphoreGive( xSemaphoreSettings );
      }
      else
      {
        log_manager->verbose(PSTR(__func__), PSTR("No semaphore available.\n."));
      }
    }
    if(data["cp1A4"].as<uint8_t>() == 0)
    {
      setSwitch(String("ch4"), String("OFF"));
    }
  }

  if( xSemaphoreSettings != NULL ){
    if( xSemaphoreTake( xSemaphoreSettings, ( TickType_t ) 1000 ) == pdTRUE )
    {
      if(data["cp1B1"] != nullptr){mySettings.cp1B[0] = data["cp1B1"].as<unsigned long>();}
      if(data["cp1B2"] != nullptr){mySettings.cp1B[1] = data["cp1B2"].as<unsigned long>();}
      if(data["cp1B3"] != nullptr){mySettings.cp1B[2] = data["cp1B3"].as<unsigned long>();}
      if(data["cp1B4"] != nullptr){mySettings.cp1B[3] = data["cp1B4"].as<unsigned long>();}

      if(data["cp3A1"] != nullptr){mySettings.cp3A[0] = data["cp3A1"].as<String>();}
      if(data["cp3A2"] != nullptr){mySettings.cp3A[1] = data["cp3A2"].as<String>();}
      if(data["cp3A3"] != nullptr){mySettings.cp3A[2] = data["cp3A3"].as<String>();}
      if(data["cp3A4"] != nullptr){mySettings.cp3A[3] = data["cp3A4"].as<String>();}

      if(data["cp2A1"] != nullptr){
        uint64_t micro = data["cp2A1"].as<uint64_t>();
        uint32_t micro_high = micro >> 32;
        uint32_t micro_low = micro & MAX_INT;
        mySettings.cp2A[0] = micro2milli(micro_high, micro_low);
      }
      if(data["cp2A2"] != nullptr){
        uint64_t micro = data["cp2A2"].as<uint64_t>();
        uint32_t micro_high = micro >> 32;
        uint32_t micro_low = micro & MAX_INT;
        mySettings.cp2A[1] = micro2milli(micro_high, micro_low);
      }
      if(data["cp2A3"] != nullptr){
        uint64_t micro = data["cp2A3"].as<uint64_t>();
        uint32_t micro_high = micro >> 32;
        uint32_t micro_low = micro & MAX_INT;
        mySettings.cp2A[2] = micro2milli(micro_high, micro_low);
      }
      if(data["cp2A4"] != nullptr){
        uint64_t micro = data["cp2A4"].as<uint64_t>();
        uint32_t micro_high = micro >> 32;
        uint32_t micro_low = micro & MAX_INT;
        mySettings.cp2A[3] = micro2milli(micro_high, micro_low);
      }

      if(data["cp2B1"] != nullptr){mySettings.cp2B[0] = data["cp2B1"].as<unsigned long>();}
      if(data["cp2B2"] != nullptr){mySettings.cp2B[1] = data["cp2B2"].as<unsigned long>();}
      if(data["cp2B3"] != nullptr){mySettings.cp2B[2] = data["cp2B3"].as<unsigned long>();}
      if(data["cp2B4"] != nullptr){mySettings.cp2B[3] = data["cp2B4"].as<unsigned long>();}

      if(data["cp4A1"] != nullptr){mySettings.cp4A[0] = data["cp4A1"].as<unsigned long>();}
      if(data["cp4A2"] != nullptr){mySettings.cp4A[1] = data["cp4A2"].as<unsigned long>();}
      if(data["cp4A3"] != nullptr){mySettings.cp4A[2] = data["cp4A3"].as<unsigned long>();}
      if(data["cp4A4"] != nullptr){mySettings.cp4A[3] = data["cp4A4"].as<unsigned long>();}

      if(data["cp4B1"] != nullptr){mySettings.cp4B[0] = data["cp4B1"].as<unsigned long>();}
      if(data["cp4B2"] != nullptr){mySettings.cp4B[1] = data["cp4B2"].as<unsigned long>();}
      if(data["cp4B3"] != nullptr){mySettings.cp4B[2] = data["cp4B3"].as<unsigned long>();}
      if(data["cp4B4"] != nullptr){mySettings.cp4B[3] = data["cp4B4"].as<unsigned long>();}

      if(data["pR1"] != nullptr){mySettings.pR[0] = data["pR1"].as<uint8_t>();}
      if(data["pR2"] != nullptr){mySettings.pR[1] = data["pR2"].as<uint8_t>();}
      if(data["pR3"] != nullptr){mySettings.pR[2] = data["pR3"].as<uint8_t>();}
      if(data["pR4"] != nullptr){mySettings.pR[3] = data["pR4"].as<uint8_t>();}

      if(data["ON"] != nullptr){mySettings.ON = data["ON"].as<bool>();}
      if(data["itP"] != nullptr){mySettings.itP = data["itP"].as<uint16_t>();}
      if(data["itW"] != nullptr){mySettings.itW = data["itW"].as<uint16_t>();}
      if(data["itPc"] != nullptr){mySettings.itPc = data["itPc"].as<uint16_t>();}
      if(data["itWc"] != nullptr){mySettings.itWc = data["itWc"].as<uint16_t>();}
      if(data["itD"] != nullptr){mySettings.itD = data["itD"].as<uint16_t>();}

      if(data["cpM1"] != nullptr){mySettings.cpM[0] = data["cpM1"].as<uint8_t>();}
      if(data["cpM2"] != nullptr){mySettings.cpM[1] = data["cpM2"].as<uint8_t>();}
      if(data["cpM3"] != nullptr){mySettings.cpM[2] = data["cpM3"].as<uint8_t>();}
      if(data["cpM4"] != nullptr){mySettings.cpM[3] = data["cpM4"].as<uint8_t>();}

      if(data["lbl1"] != nullptr){strlcpy(mySettings.lbl[0], data["lbl1"].as<const char*>(), sizeof(mySettings.lbl[0]));}
      if(data["lbl2"] != nullptr){strlcpy(mySettings.lbl[1], data["lbl2"].as<const char*>(), sizeof(mySettings.lbl[1]));}
      if(data["lbl3"] != nullptr){strlcpy(mySettings.lbl[2], data["lbl3"].as<const char*>(), sizeof(mySettings.lbl[2]));}
      if(data["lbl4"] != nullptr){strlcpy(mySettings.lbl[3], data["lbl4"].as<const char*>(), sizeof(mySettings.lbl[3]));}

      if(data["seaHpa"] != nullptr){mySettings.seaHpa = data["seaHpa"].as<float>();}
      xSemaphoreGive( xSemaphoreSettings );
    }
    else
    {
      log_manager->verbose(PSTR(__func__), PSTR("No semaphore available.\n."));
    }
  }

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void onTbConnected(){
  mySettings.publishSwitch[0] = 1;
  mySettings.publishSwitch[1] = 1;
  mySettings.publishSwitch[2] = 1;
  mySettings.publishSwitch[3] = 1;
}

void onTbDisconnected(){
 
}

void onReboot(){}

void setPanic(const RPC_Data &data){
    long startMillis = millis();

    if(data[PSTR("st")] != nullptr){
        StaticJsonDocument<DOCSIZE_MIN> doc;
        doc[PSTR("method")] = PSTR("sCfg");
        if(strcmp(data[PSTR("st")], PSTR("ON")) == 0){
            doc[PSTR("fP")] = 1;
            configcomcu.fP = 1;
            //setAlarm(666, 1, 1, 10000);
            stateReset(1);
        }
        else{
            doc[PSTR("fP")] = 0;
            configcomcu.fP = 0;
        }
        serialWriteToCoMcu(doc, 0);
    }

    log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

RPC_Response genericClientRPC(const RPC_Data &data){
    long startMillis = millis();

    StaticJsonDocument<DOCSIZE_MIN> payload;    
    DeserializationError err = deserializeJson(payload, data);

    if(err == DeserializationError::Ok){
        if(payload[PSTR("cmd")] != nullptr){
            const char * cmd = payload["cmd"].as<const char *>();
            log_manager->verbose(PSTR(__func__), PSTR("Received command: %s\n"), cmd);

            if(strcmp(cmd, PSTR("setSwitch")) == 0){
                if(payload[PSTR("arg")][PSTR("ch")] != nullptr && payload[PSTR("arg")][PSTR("st")] != nullptr){
                    setSwitch(payload[PSTR("arg")][PSTR("ch")].as<String>(), payload[PSTR("arg")][PSTR("st")].as<String>());
                }
            }
        }   
    }
    else{
        log_manager->verbose(PSTR(__func__), PSTR("Failed to parse JSON: %s\n"), err.c_str());
        if(data[PSTR("cmd")] != nullptr){
            const char * cmd = data["cmd"].as<const char *>();
            log_manager->verbose(PSTR(__func__), PSTR("Received command: %s\n"), cmd);

            if(strcmp(cmd, PSTR("setSwitch")) == 0){
                if(data[PSTR("arg")][PSTR("ch")] != nullptr && data[PSTR("arg")][PSTR("st")] != nullptr){
                    setSwitch(data[PSTR("arg")][PSTR("ch")].as<String>(), data[PSTR("arg")][PSTR("st")].as<String>());
                }
            }
        }  
    }

    StaticJsonDocument<JSON_OBJECT_SIZE(1)> doc;
    doc[PSTR("rpc")] = PSTR("OK");

    log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
    return RPC_Response(doc);
}

void stateReset(bool resetOpMode){
    if(resetOpMode){
        mySettings.cpM[0] = 0;
        mySettings.cpM[1] = 0;
        mySettings.cpM[2] = 0;
        mySettings.cpM[3] = 0;
        log_manager->verbose(PSTR(__func__), PSTR("Relay operations are changed to manual.\n"));
    }
    setSwitch("ch1", "OFF");
    setSwitch("ch2", "OFF");
    setSwitch("ch3", "OFF");
    setSwitch("ch4", "OFF");
    log_manager->verbose(PSTR(__func__), PSTR("Relay operations are disabled.\n"));
}

void deviceTelemetry(){
    if(config.provSent && tb.connected() && config.fIoT){
      StaticJsonDocument<DOCSIZE_MIN> doc;
      
      doc[PSTR("uptime")] = millis(); 
      doc[PSTR("heap")] = heap_caps_get_free_size(MALLOC_CAP_8BIT); 
      doc[PSTR("rssi")] = WiFi.RSSI(); 
      doc[PSTR("dt")] = rtc.getEpoch(); 

      tbSendAttribute(doc);
    }
}

void onAlarm(int code){

}

void publishSwitchTR(void * arg){
  unsigned long timerDeviceTelemetry = millis();
  while(true){
    for (uint8_t i = 0; i < sizeof(mySettings.dutyState); i++){
      if(mySettings.publishSwitch[i]){
          String chName = "ch" + String(i+1);
          int state = (int)mySettings.dutyState[i] == mySettings.ON ? 1 : 0;

          char buffer[10];
          StaticJsonDocument<DOCSIZE_MIN> doc;
          doc[chName.c_str()] = state;
          serializeJson(doc, buffer);
          
          if(config.fIoT && tb.connected() && config.provSent){
              tbSendTelemetry(doc);
          }

          #ifdef USE_WEB_IFACE
          if(config.fIface && config.wsCount > 0){
              ws.broadcastTXT(buffer);
          }
          #endif

          mySettings.publishSwitch[i] = false;
      }
    }

    unsigned long now = millis();
    if((now - timerDeviceTelemetry) > (mySettings.itD * 1000)){
      deviceTelemetry();
      timerDeviceTelemetry = now;
    }

    vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
  }
}

void onSyncClientAttr(uint8_t direction){
    long startMillis = millis();

    StaticJsonDocument<DOCSIZE_MIN> doc;
    

    if(tb.connected() && (direction == 0 || direction == 1)){
      doc[PSTR("cp1A1")] = mySettings.cp1A[0];
      doc[PSTR("cp1A2")] = mySettings.cp1A[1];
      doc[PSTR("cp1A3")] = mySettings.cp1A[2];
      doc[PSTR("cp1A4")] = mySettings.cp1A[3];
      doc[PSTR("cp1B1")] = mySettings.cp1B[0];
      doc[PSTR("cp1B2")] = mySettings.cp1B[1];
      doc[PSTR("cp1B3")] = mySettings.cp1B[2];
      doc[PSTR("cp1B4")] = mySettings.cp1B[3];
      tbSendAttribute(doc);
      doc.clear();
      doc[PSTR("cp2A1")] = (uint64_t)mySettings.cp2A[0] * 1000;
      doc[PSTR("cp2A2")] = (uint64_t)mySettings.cp2A[1] * 1000;
      doc[PSTR("cp2A3")] = (uint64_t)mySettings.cp2A[2] * 1000;
      doc[PSTR("cp2A4")] = (uint64_t)mySettings.cp2A[3] * 1000;
      doc[PSTR("cp2B1")] = mySettings.cp2B[0];
      doc[PSTR("cp2B2")] = mySettings.cp2B[1];
      doc[PSTR("cp2B3")] = mySettings.cp2B[2];
      doc[PSTR("cp2B4")] = mySettings.cp2B[3];
      tbSendAttribute(doc);
      doc.clear();
      doc[PSTR("cp4A1")] = mySettings.cp4A[0];
      doc[PSTR("cp4A2")] = mySettings.cp4A[1];
      doc[PSTR("cp4A3")] = mySettings.cp4A[2];
      doc[PSTR("cp4A4")] = mySettings.cp4A[3];
      doc[PSTR("cp4B1")] = mySettings.cp4B[0];
      doc[PSTR("cp4B2")] = mySettings.cp4B[1];
      doc[PSTR("cp4B3")] = mySettings.cp4B[2];
      doc[PSTR("cp4B4")] = mySettings.cp4B[3];
      tbSendAttribute(doc);
      doc.clear();
      doc[PSTR("pR1")] = mySettings.pR[0];
      doc[PSTR("pR2")] = mySettings.pR[1];
      doc[PSTR("pR3")] = mySettings.pR[2];
      doc[PSTR("pR4")] = mySettings.pR[3];
      doc[PSTR("ON")] = mySettings.ON;
      tbSendAttribute(doc);
      doc.clear();
      doc[PSTR("cp3A1")] = mySettings.cp3A[0].c_str();
      tbSendAttribute(doc);
      doc.clear();
      doc[PSTR("cp3A2")] = mySettings.cp3A[1].c_str();
      tbSendAttribute(doc);
      doc.clear();
      doc[PSTR("cp3A3")] = mySettings.cp3A[2].c_str();
      tbSendAttribute(doc);
      doc.clear();
      doc[PSTR("cp3A4")] = mySettings.cp3A[3].c_str();
      tbSendAttribute(doc);
      doc.clear();
      doc[PSTR("lbl1")] = mySettings.lbl[0];
      doc[PSTR("lbl2")] = mySettings.lbl[1];
      doc[PSTR("lbl3")] = mySettings.lbl[2];
      doc[PSTR("lbl4")] = mySettings.lbl[3];
      doc[PSTR("itP")] = mySettings.itP;
      doc[PSTR("itW")] = mySettings.itW;
      doc[PSTR("itPc")] = mySettings.itPc;
      doc[PSTR("itWc")] = mySettings.itWc;
      doc[PSTR("itD")] = mySettings.itD;
      tbSendAttribute(doc);
      doc.clear();
      doc[PSTR("cpM1")] = mySettings.cpM[0];
      doc[PSTR("cpM2")] = mySettings.cpM[1];
      doc[PSTR("cpM3")] = mySettings.cpM[2];
      doc[PSTR("cpM4")] = mySettings.cpM[3];
      doc[PSTR("seaHpa")] = mySettings.seaHpa;
      tbSendAttribute(doc);
      doc.clear();
    }

    #ifdef USE_WEB_IFACE
    if(config.wsCount > 0 && (direction == 0 || direction == 2)){
      char buffer[1024];
      JsonObject cp1A = doc.createNestedObject("cp1A");
      cp1A[PSTR("cp1A1")] = mySettings.cp1A[0];
      cp1A[PSTR("cp1A2")] = mySettings.cp1A[1];
      cp1A[PSTR("cp1A3")] = mySettings.cp1A[2];
      cp1A[PSTR("cp1A4")] = mySettings.cp1A[3];
      serializeJson(doc, buffer);
      ws.broadcastTXT(buffer);
      doc.clear();
      JsonObject cp1B = doc.createNestedObject("cp1B");
      cp1B[PSTR("cp1B1")] = mySettings.cp1B[0];
      cp1B[PSTR("cp1B2")] = mySettings.cp1B[1];
      cp1B[PSTR("cp1B3")] = mySettings.cp1B[2];
      cp1B[PSTR("cp1B4")] = mySettings.cp1B[3];
      serializeJson(doc, buffer);
      ws.broadcastTXT(buffer);
      doc.clear();
      JsonObject cp2A = doc.createNestedObject("cp2A");
      cp2A[PSTR("cp2A1")] = (uint64_t)mySettings.cp2A[0] * 1000;
      cp2A[PSTR("cp2A2")] = (uint64_t)mySettings.cp2A[1] * 1000;
      cp2A[PSTR("cp2A3")] = (uint64_t)mySettings.cp2A[2] * 1000;
      cp2A[PSTR("cp2A4")] = (uint64_t)mySettings.cp2A[3] * 1000;
      serializeJson(doc, buffer);
      ws.broadcastTXT(buffer);
      doc.clear();
      JsonObject cp2B = doc.createNestedObject("cp2B");
      cp2B[PSTR("cp2B1")] = mySettings.cp2B[0];
      cp2B[PSTR("cp2B2")] = mySettings.cp2B[1];
      cp2B[PSTR("cp2B3")] = mySettings.cp2B[2];
      cp2B[PSTR("cp2B4")] = mySettings.cp2B[3];
      serializeJson(doc, buffer);
      ws.broadcastTXT(buffer);
      doc.clear();
      JsonObject cp4A = doc.createNestedObject("cp4A");
      cp4A[PSTR("cp4A1")] = mySettings.cp4A[0];
      cp4A[PSTR("cp4A2")] = mySettings.cp4A[1];
      cp4A[PSTR("cp4A3")] = mySettings.cp4A[2];
      cp4A[PSTR("cp4A4")] = mySettings.cp4A[3];
      serializeJson(doc, buffer);
      ws.broadcastTXT(buffer);
      doc.clear();
      JsonObject cp4B = doc.createNestedObject("cp4B");
      cp4B[PSTR("cp4B1")] = mySettings.cp4B[0];
      cp4B[PSTR("cp4B2")] = mySettings.cp4B[1];
      cp4B[PSTR("cp4B3")] = mySettings.cp4B[2];
      cp4B[PSTR("cp4B4")] = mySettings.cp4B[3];
      serializeJson(doc, buffer);
      ws.broadcastTXT(buffer);
      doc.clear();
      JsonObject cp3A = doc.createNestedObject("cp3A");
      cp3A[PSTR("cp3A1")] = mySettings.cp3A[0].c_str();
      cp3A[PSTR("cp3A2")] = mySettings.cp3A[1].c_str();
      cp3A[PSTR("cp3A3")] = mySettings.cp3A[2].c_str();
      cp3A[PSTR("cp3A4")] = mySettings.cp3A[3].c_str();
      serializeJson(doc, buffer);
      ws.broadcastTXT(buffer);
      doc.clear();
      JsonObject lbl = doc.createNestedObject("lbl");
      lbl[PSTR("lbl1")] = mySettings.lbl[0];
      lbl[PSTR("lbl2")] = mySettings.lbl[1];
      lbl[PSTR("lbl3")] = mySettings.lbl[2];
      lbl[PSTR("lbl4")] = mySettings.lbl[3];
      serializeJson(doc, buffer);
      ws.broadcastTXT(buffer);
      doc.clear();
      JsonObject cpM = doc.createNestedObject("cpM");
      cpM[PSTR("cpM1")] = mySettings.cpM[0];
      cpM[PSTR("cpM2")] = mySettings.cpM[1];
      cpM[PSTR("cpM3")] = mySettings.cpM[2];
      cpM[PSTR("cpM4")] = mySettings.cpM[3];
      doc[PSTR("ch1")] = (int)mySettings.dutyState[0] == mySettings.ON ? 1 : 0;
      doc[PSTR("ch2")] = (int)mySettings.dutyState[1] == mySettings.ON ? 1 : 0;
      doc[PSTR("ch3")] = (int)mySettings.dutyState[2] == mySettings.ON ? 1 : 0;
      doc[PSTR("ch4")] = (int)mySettings.dutyState[3] == mySettings.ON ? 1 : 0;
      serializeJson(doc, buffer);
      ws.broadcastTXT(buffer);
    }
    #endif
    
    log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

#ifdef USE_WEB_IFACE
void onWsEvent(const JsonObject &doc){
    long startMillis = millis();

    if(doc["evType"] == nullptr){
        log_manager->debug(PSTR(__func__), "Event type not found.\n");
        return;
    }
    int evType = doc["evType"].as<int>();


    if(evType == (int)WStype_CONNECTED){
      syncClientAttr(2);
    }
    if(evType == (int)WStype_DISCONNECTED){
        if(config.wsCount < 1){
            log_manager->debug(PSTR(__func__),PSTR("No WS client is active. \n"));
        }
    }
    else if(evType == (int)WStype_TEXT){
        if(doc["cmd"] == nullptr){
            log_manager->debug(PSTR(__func__), "Command not found.\n");
            return;
        }
        const char* cmd = doc["cmd"].as<const char*>();
        if(strcmp(cmd, (const char*) "attr") == 0){
          processSharedAttributeUpdate(doc);
          syncClientAttr(1);
        }
        else if(strcmp(cmd, (const char*) "saveSettings") == 0){
          saveSettings();
        }
        else if(strcmp(cmd, (const char*) "configSave") == 0){
          configSave();
        }
        else if(strcmp(cmd, (const char*) "setPanic") == 0){
          doc[PSTR("st")] = configcomcu.fP ? "OFF" : "ON";
          processSetPanic(doc);
        }
        else if(strcmp(cmd, (const char*) "reboot") == 0){
          plannedReboot(5);
        }
        else if(strcmp(cmd, (const char*) "setSwitch") == 0){
          setSwitch(doc["ch"].as<String>(), doc["state"].as<int>() == 1 ? "ON" : "OFF");
        }
    }

    log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void wsSendTelemetryTR(void *arg){
  while(true){
    if(config.fIface && config.wsCount > 0){
      char buffer[DOCSIZE_MIN];
      StaticJsonDocument<DOCSIZE_MIN> doc;
      JsonObject devTel = doc.createNestedObject("devTel");
      devTel[PSTR("heap")] = heap_caps_get_free_size(MALLOC_CAP_8BIT);
      devTel[PSTR("rssi")] = WiFi.RSSI();
      devTel[PSTR("uptime")] = millis()/1000;
      devTel[PSTR("dt")] = rtc.getEpoch();
      devTel[PSTR("dts")] = rtc.getDateTime();
      serializeJson(doc, buffer);
      ws.broadcastTXT(buffer);
    }
    vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
  }
}

void wsSendSensorsTR(void *arg){
  while(true){
    if(config.fIface && config.wsCount > 0){
      char buffer[DOCSIZE_MIN];
      StaticJsonDocument<DOCSIZE_MIN> doc;
  
      if( xQueuePZEMMessage != NULL ){
        PZEMMessage PZEMMsg;
        if( xQueueReceive( xQueuePZEMMessage,  &( PZEMMsg ), ( TickType_t ) 1000 ) == pdPASS )
        {
          JsonObject pzem = doc.createNestedObject("pzem");
          pzem[PSTR("volt")] = round2(PZEMMsg.volt);
          pzem[PSTR("amp")] = round2(PZEMMsg.amp);
          pzem[PSTR("watt")] = round2(PZEMMsg.watt);
          pzem[PSTR("ener")] = round2(PZEMMsg.ener);
          pzem[PSTR("freq")] = round2(PZEMMsg.freq);
          pzem[PSTR("pf")] = round2(PZEMMsg.pf)*100;
          serializeJson(doc, buffer);
          ws.broadcastTXT(buffer);
          doc.clear();
        }
      } 

      if( xQueueBME280Message != NULL ){
        BME280Message BME280Msg;
        if( xQueueReceive( xQueueBME280Message,  &( BME280Msg ), ( TickType_t ) 1000 ) == pdPASS )
        {
          JsonObject bme280 = doc.createNestedObject("bme280");
          bme280[PSTR("celc")] = round2(BME280Msg.celc);
          bme280[PSTR("rh")] = round2(BME280Msg.rh);
          bme280[PSTR("hpa")] = round2(BME280Msg.hpa);
          bme280[PSTR("alt")] = round2(BME280Msg.alt);
          serializeJson(doc, buffer);
          ws.broadcastTXT(buffer);
        }
      } 
    }
    vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
  }
}
#endif