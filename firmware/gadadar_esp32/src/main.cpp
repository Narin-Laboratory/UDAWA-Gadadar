/**
 * UDAWA - Universal Digital Agriculture Watering Assistant
 * Firmware for Actuator 4Ch UDAWA Board (Gadadar)
 * Licensed under aGPLv3
 * Researched and developed by PRITA Research Group & Narin Laboratory
 * prita.undiknas.ac.itD | narin.co.itD
**/
#include "main.h"

void setup()
{
  processSharedAttributeUpdateCb = &attUpdateCb;
  onTbDisconnectedCb = &onTbDisconnected;
  onTbConnectedCb = &onTbConnected;
  processSetPanicCb = &setPanic;
  processGenericClientRPCCb = &genericClientRPC;
  emitAlarmCb = &onAlarm;
  onSyncClientAttrCb = &onSyncClientAttr;
  onSaveSettings = &saveSettings;
  onSaveStates = &saveStates;
  #ifdef USE_WEB_IFACE
  wsEventCb = &onWsEvent;
  #endif
  onMQTTUpdateStartCb = &onMQTTUpdateStart;
  onMQTTUpdateEndCb = &onMQTTUpdateEnd;
  if(xSemaphorePowerSensor == NULL){xSemaphorePowerSensor = xSemaphoreCreateMutex();}
  if(xSemaphoreStates == NULL){xSemaphoreStates = xSemaphoreCreateMutex();}
  startup();
  if(!config.SM){
    loadSettings();
    loadStates();
    syncConfigCoMCU();
  }
  if(String(config.model) == String("Generic")){
    strlcpy(config.model, "Gadadar", sizeof(config.model));
  }
  stateReset(0);

  tb.setBufferSize(1024);

  Wire.begin();

  #ifdef USE_WEB_IFACE
  xQueueWsPayloadPowerSensor = xQueueCreate( 1, sizeof( struct WSPayloadPowerSensor ) );
  xQueueWsPayloadWeatherSensor = xQueueCreate( 1, sizeof( struct WSPayloadWeatherSensor ) );
  #endif

  if(xHandleWeatherSensor == NULL && !config.SM){
    xReturnedWeatherSensor = xTaskCreatePinnedToCore(weatherSensorTR, PSTR("weatherSensor"), STACKSIZE_WEATHERSENSOR, NULL, 1, &xHandleWeatherSensor, 1);
    if(xReturnedWeatherSensor == pdPASS){
      log_manager->warn(PSTR(__func__), PSTR("Task weatherSensor has been created.\n"));
    }
  }

  if(xHandlePowerSensor == NULL && !config.SM){
    xReturnedPowerSensor = xTaskCreatePinnedToCore(powerSensorTR, PSTR("powerSensor"), STACKSIZE_POWERSENSOR, NULL, 1, &xHandlePowerSensor, 1);
    if(xReturnedPowerSensor == pdPASS){
      log_manager->warn(PSTR(__func__), PSTR("Task powerSensor has been created.\n"));
    }
  }

  if(xHandlePublishDevTel == NULL && !config.SM){
    xReturnedPublishDevTel = xTaskCreatePinnedToCore(publishDeviceTelemetryTR, PSTR("publishDevTel"), STACKSIZE_PUBLISHDEVTEL, NULL, 1, &xHandlePublishDevTel, 1);
    if(xReturnedPublishDevTel == pdPASS){
      log_manager->warn(PSTR(__func__), PSTR("Task publishDevTel has been created.\n"));
    }
  }

  if(xHandleRelayControl == NULL && !config.SM){
    xReturnedRelayControl = xTaskCreatePinnedToCore(relayControlTR, PSTR("relayControl"), STACKSIZE_RELAYCONTROL, NULL, 1, &xHandleRelayControl, 1);
    if(xReturnedRelayControl == pdPASS){
      log_manager->warn(PSTR(__func__), PSTR("Task relayControl has been created.\n"));
    }
  }

  #ifdef USE_WEB_IFACE
  if(xHandleWsSendTelemetry == NULL && !config.SM){
    xReturnedWsSendTelemetry = xTaskCreatePinnedToCore(wsSendTelemetryTR, PSTR("wsSendTelemetry"), STACKSIZE_WSSENDTELEMETRY, NULL, 1, &xHandleWsSendTelemetry, 1);
    if(xReturnedWsSendTelemetry == pdPASS){
      log_manager->warn(PSTR(__func__), PSTR("Task wsSendTelemetry has been created.\n"));
    }
  }
  #endif
}

void loop(){
  udawa();
  if(!config.SM){
    if( (millis() - powerSensorTR_last_activity) > 30000){
      setAlarm(121, 1, 5, 1000);
      log_manager->warn(PSTR("powerSensorTR"), PSTR("Task is not responding for: %d\n"), ((millis() - powerSensorTR_last_activity)));
      powerSensorTR_last_activity = millis();
      reboot();
    }
  }
  vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
}

void powerSensorTR(void *arg){
  Stream_Stats<float> _volt_;
  Stream_Stats<float> _amp_;
  Stream_Stats<float> _watt_;
  Stream_Stats<float> _freq_;
  Stream_Stats<float> _pf_;

  float volt; float amp; float watt; float freq; float pf; float ener;  

  unsigned long timerTelemetry = millis();
  unsigned long timerAttribute = millis();
  unsigned long timerAlarm = millis();

  while (true)
  {
    HardwareSerial PZEMSerial(1);
    PZEM004Tv30 PZEM(PZEMSerial, S1_RX, S1_TX);
    myStates.flag_powerSensor = !isnan(PZEM.voltage());
    if(!myStates.flag_powerSensor){log_manager->warn(PSTR(__func__), PSTR("Failed to initialize powerSensor!\n"));}

    bool flag_failure_readings = false;
    if(myStates.flag_powerSensor){
      volt = PZEM.voltage();
      amp = PZEM.current();
      watt = PZEM.power();
      freq = PZEM.frequency();
      pf = PZEM.pf();
      ener = PZEM.energy();
    }

    if(isnan(volt) || isnan(amp) || isnan(watt) || isnan(freq) || isnan(pf) ||
    isnan(ener) || volt < 0.0 || volt > 1000.0 || amp < 0.0 || amp > 100.0 || watt < .0 || 
    watt > 22000.0 || freq < 0.0 || freq > 100.0 || ener > 9999.0 ){
      flag_failure_readings = true;
      //log_manager->debug(PSTR(__func__), PSTR("Weather sensor abnormal reading: %.2f, %.2f, %.2f, %.2f\n"), celc, rh, hpa, alt);
    }

    if(myStates.flag_powerSensor && !flag_failure_readings)
    {
      _volt_.Add(volt);
      _amp_.Add(amp);
      _watt_.Add(watt);
      _freq_.Add(freq);
      _pf_.Add(pf);
    }

    unsigned long now = millis();
    if(myStates.flag_powerSensor && !flag_failure_readings)
    {
      StaticJsonDocument<DOCSIZE_MIN> doc;
      char buffer[DOCSIZE_MIN];
      
      if( (now - timerAttribute) > (mySettings.itPc * 1000) && tb.connected() && config.provSent)
      {
        doc[PSTR("_volt")] = volt;
        doc[PSTR("_amp")] = amp;
        doc[PSTR("_watt")] = watt;
        doc[PSTR("_freq")] = freq;
        doc[PSTR("_pf")] = pf;
        doc[PSTR("_ener")] = ener;
        serializeJson(doc, buffer);
        tbSendAttribute(buffer);
        doc.clear();

        timerAttribute = now;
      }

      if( (now - timerTelemetry) > (mySettings.itP * 1000) )
      {
        float a = _volt_.Get_Average();
        float b = _amp_.Get_Average();
        float c = _watt_.Get_Average();
        float d = _pf_.Get_Average();
        float e = _freq_.Get_Average();
        if(!isnan(a) && a > 0.0 && a < 1000.0 && !isnan(b) && b >=0 && 
          b <= 100.0 && !isnan(c) && c >= 0 && c <= 22000.0 &&
          d > 0 && d < 100 && e > 0 && e < 100){
          doc[PSTR("volt")] = a;  
          doc[PSTR("amp")] = b; 
          doc[PSTR("watt")] = c;
          doc[PSTR("pf")] = d; 
          doc[PSTR("freq")] = e;
          #ifdef USE_DISK_LOG  
          writeCardLogger(doc);
          #endif
          if(tb.connected() && config.provSent)
          {
            serializeJson(doc, buffer);
            if(tbSendTelemetry(buffer)){
              
            }
          }
          _volt_.Clear(); _amp_.Clear(); _watt_.Clear(); 
          _freq_.Clear(); _pf_.Clear();
          doc.clear();
        }

        timerTelemetry = now;
      }

    }
  
    if( (now - timerAlarm) > 10000 )
    {
      if(!myStates.flag_powerSensor){setAlarm(140, 1, 5, 1000);}
      else{
        if(volt < 0 || volt > 1000){setAlarm(141, 1, 5, 1000);}
        if(amp < 0 || amp > 100){setAlarm(142, 1, 5, 1000);}
        if(watt < 0 || watt > 22000){setAlarm(143, 1, 5, 1000);}
        if(pf < 0 || pf > 100){setAlarm(144, 1, 5, 1000);}
        if(freq < 0 || freq > 100){setAlarm(144, 1, 5, 1000);}
        if(watt > 2000 || volt > 275){setAlarm(145, 1, 5, 1000);}

        uint8_t activeRelayCounter = 0;
        for(uint8_t i = 0; i < 4; i++){
          if(myStates.dutyState[i] == mySettings.ON &&
          watt < 6.0){setAlarm(211+i, 1, 5, 1000);}

          if(myStates.dutyState[i] == mySettings.ON){activeRelayCounter++;}

          if( myStates.dutyState[i] == mySettings.ON && 
            (millis() - myStates.stateOnTs[i]) > 3600000){
              setAlarm(216+i, 1, 5, 1000);
            }
        }

        if(activeRelayCounter == 0 && watt > 6){setAlarm(215, 1, 5, 1000);}
        
    
      }

      
      timerAlarm = now;
    }

    #ifdef USE_WEB_IFACE
    if( xQueueWsPayloadPowerSensor != NULL && (config.wsCount > 0) && myStates.flag_powerSensor && !flag_failure_readings)
    {
      WSPayloadPowerSensor payload;
      payload.volt = volt;
      payload.amp = amp;
      payload.watt = watt;
      payload.freq = freq;
      payload.pf = pf;
      payload.ener = ener;
      if( xQueueSend( xQueueWsPayloadPowerSensor, &payload, ( TickType_t ) 1000 ) != pdPASS )
      {
        log_manager->debug(PSTR(__func__), PSTR("Failed to fill WSPayloadPowerSensor. Queue is full. \n"));
      }
    }
    #endif

    if(myStates.flag_resetPowerSensor){
      myStates.flag_resetPowerSensor = 0;
      PZEM.resetEnergy();
    }

    powerSensorTR_last_activity = millis();
    vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
  }
}


void weatherSensorTR(void *arg){
  BME280I2C weatherSensor;
  myStates.flag_weatherSensor = weatherSensor.begin();
  if(!myStates.flag_weatherSensor){log_manager->warn(PSTR(__func__), PSTR("Failed to initialize weatherSensor!\n"));}

  Stream_Stats<float> _celc_;
  Stream_Stats<float> _rh_;
  Stream_Stats<float> _hpa_;
  Stream_Stats<float> _alt_;

  float celc; float rh; float hpa; float alt;  

  unsigned long timerTelemetry = millis();
  unsigned long timerAttribute = millis();
  unsigned long timerAlarm = millis();

  while (true)
  {
    bool flag_failure_readings = false;
    if(myStates.flag_weatherSensor){
      BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
      BME280::PresUnit presUnit(BME280::PresUnit_Pa);

      weatherSensor.read(hpa, celc, rh, tempUnit, presUnit);
      alt = hpa / mySettings.seaHpa;
      hpa = hpa / 100.0F;
    }

    if(isnan(celc) || isnan(rh) || isnan(hpa) || celc < -80.0 || celc > 300.0 || rh < 0.0 || rh > 100.0 || hpa < 700.0 || 
    hpa > 1100.0 || alt < 0.0 || alt > 10000.0){
      flag_failure_readings = true;
      //log_manager->debug(PSTR(__func__), PSTR("Weather sensor abnormal reading: %.2f, %.2f, %.2f, %.2f\n"), celc, rh, hpa, alt);
    }

    if(myStates.flag_weatherSensor && !flag_failure_readings)
    {
      _celc_.Add(celc);
      _rh_.Add(rh);
      _hpa_.Add(hpa);
      _alt_.Add(alt);
    }

    unsigned long now = millis();
    if(myStates.flag_weatherSensor && !flag_failure_readings)
    {
      StaticJsonDocument<DOCSIZE_MIN> doc;
      char buffer[DOCSIZE_MIN];
      
      if( (now - timerAttribute) > (mySettings.itWc * 1000) && tb.connected() && config.provSent)
      {
        doc[PSTR("_celc")] = celc;
        doc[PSTR("_rh")] = rh;
        doc[PSTR("_hpa")] = hpa;
        doc[PSTR("_alt")] = alt;
        serializeJson(doc, buffer);
        tbSendAttribute(buffer);
        doc.clear();

        timerAttribute = now;
      }

      if( (now - timerTelemetry) > (mySettings.itW * 1000) )
      {
        float a = _celc_.Get_Average();
        float b = _rh_.Get_Average();
        float c = _hpa_.Get_Average();
        float d = _alt_.Get_Average();
        if(!isnan(a) && a > -80.0 && a < 300.0 && !isnan(b) && b >=0 && b <= 100 && !isnan(c) && c > 0 && c < 1015){
          doc[PSTR("celc")] = a;  
          doc[PSTR("rh")] = b; 
          doc[PSTR("hpa")] = c;
          doc[PSTR("alt")] = d;
          #ifdef USE_DISK_LOG 
          writeCardLogger(doc);
          #endif
          if(tb.connected() && config.provSent)
          {
            serializeJson(doc, buffer);
            if(tbSendTelemetry(buffer)){
              
            }
          }
          _celc_.Clear(); _rh_.Clear(); _hpa_.Clear(); _alt_.Clear();
          doc.clear();
        }

        timerTelemetry = now;
      }

    }
  
    if( (now - timerAlarm) > 10000 )
    {
      if(!myStates.flag_weatherSensor){setAlarm(120, 1, 5, 1000);}
      else{
        if(celc < -80.0 || celc > 80.0){setAlarm(121, 1, 5, 1000);}
        if(celc > 45.0){setAlarm(122, 1, 5, 1000);}
        if(celc < 17.0){setAlarm(123, 1, 5, 1000);}
        if(rh < 1.0 || rh > 100.0){setAlarm(124, 1, 5, 1000);}
        if(rh >= 99.0){setAlarm(125, 1, 5, 1000);}
        if(rh <= 20.0){setAlarm(126, 1, 5, 1000);}
        if(hpa < 700.0 || hpa > 1013){setAlarm(125, 1, 5, 1000);}
        if(hpa <= 750.0){setAlarm(126, 1, 5, 1000);}
        if(hpa >= 1015.0){setAlarm(127, 1, 5, 1000);}
      }

      
      timerAlarm = now;
    }

    #ifdef USE_WEB_IFACE
    if( xQueueWsPayloadWeatherSensor != NULL && (config.wsCount > 0) && myStates.flag_weatherSensor && !flag_failure_readings)
    {
      WSPayloadWeatherSensor payload;
      payload.celc = celc;
      payload.hpa = hpa;
      payload.rh = rh;
      payload.alt = alt;
      if( xQueueSend( xQueueWsPayloadWeatherSensor, &payload, ( TickType_t ) 1000 ) != pdPASS )
      {
        log_manager->debug(PSTR(__func__), PSTR("Failed to fill WSPayloadWeatherSensor. Queue is full. \n"));
      }
    }
    #endif
    vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
  }
}


void loadSettings()
{
  StaticJsonDocument<DOCSIZE_SETTINGS> doc;
  readSettings(doc, settingsPath);

  if(doc["ON"] != nullptr) { mySettings.ON = doc["ON"].as<uint8_t>(); } else { mySettings.ON = 1; }

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
  else{
    for(uint8_t i=0; i<countof(mySettings.cp3A); i++)
    {
      mySettings.cp3A[i] = "[{\"h\": 0, \"i\": 0, \"s\": 0, \"d\": 0}]";
    }
  }


  if(doc["cp4A"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["cp4A"].as<JsonArray>()) { mySettings.cp4A[index] = v.as<unsigned long>(); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.cp4A); i++) { mySettings.cp4A[i] = 0; } }

  if(doc["cp4B"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["cp4B"].as<JsonArray>()) { mySettings.cp4B[index] = v.as<unsigned long>(); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.cp4B); i++) { mySettings.cp4B[i] = 0; } }

  if(doc["pR"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["pR"].as<JsonArray>()) { mySettings.pR[index] = v.as<uint8_t>(); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.pR); i++) { mySettings.pR[i] = 7+i; } }

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

  for(uint8_t i = 0; i < countof(myStates.dutyCounter); i++) { myStates.dutyCounter[i] = 86400; }

  if(doc["seaHpa"] != nullptr){mySettings.seaHpa = doc["seaHpa"].as<float>();}
  else{mySettings.seaHpa = 1019.00;}

  if(doc["lbl"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["lbl"].as<JsonArray>()) { strlcpy(mySettings.lbl[index], v.as<const char*>(), sizeof(mySettings.lbl[index])); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.lbl); i++) { strlcpy(mySettings.lbl[i], "unnamed", sizeof(mySettings.lbl[i])); } }
}

void loadStates()
{
  if( xSemaphoreStates != NULL && xSemaphoreTake( xSemaphoreStates, ( TickType_t ) 3000 ) == pdTRUE ){
    StaticJsonDocument<DOCSIZE_SETTINGS> doc;
    readSettings(doc, statesPath);
    if(doc["cp0A"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["cp0A"].as<JsonArray>()) { myStates.cp0A[index] = v.as<uint8_t>(); index++; } } 
    else { for(uint8_t i = 0; i < countof(myStates.cp0A); i++) { myStates.cp0A[i] = 0; } }

    if(doc["cp0B"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["cp0B"].as<JsonArray>()) { myStates.cp0B[index] = v.as<uint32_t>(); index++; } } 
    else { for(uint8_t i = 0; i < countof(myStates.cp0B); i++) { myStates.cp0B[i] = 0; } }
    
    xSemaphoreGive(xSemaphoreStates);
    log_manager->verbose(PSTR(__func__), PSTR("State loaded.\n"));
  }else{
    log_manager->verbose(PSTR(__func__), PSTR("State failed to load.\n"));
  }
}

void saveSettings()
{
  StaticJsonDocument<DOCSIZE_SETTINGS> doc;

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
}

void saveStates()
{
  if( xSemaphoreStates != NULL && xSemaphoreTake( xSemaphoreStates, ( TickType_t ) 3000 ) == pdTRUE ){
    StaticJsonDocument<DOCSIZE_SETTINGS> doc;

    JsonArray cp0A = doc.createNestedArray("cp0A");
    for(uint8_t i=0; i<countof(myStates.cp0A); i++)
    {
      cp0A.add(myStates.cp0A[i]);
    }

    JsonArray cp0B = doc.createNestedArray("cp0B");
    for(uint8_t i=0; i<countof(myStates.cp0B); i++)
    {
      cp0B.add(myStates.cp0B[i]);
    }

    writeSettings(doc, statesPath);
    xSemaphoreGive(xSemaphoreStates);
    log_manager->verbose(PSTR(__func__), PSTR("State saved.\n"));
  }
  else{
    log_manager->verbose(PSTR(__func__), PSTR("State failed to save.\n"));
  }
}

void setSwitch(String ch, String state)
{
  //long startMillis = millis();

  bool fState = 0;
  uint8_t pR = 0;
  uint8_t idx = 0;

  if(ch == String("ch1")){idx = 0; pR = mySettings.pR[0]; myStates.dutyState[0] = (state == String("ON")) ? mySettings.ON : 1 - mySettings.ON; myStates.publishSwitch[0] = true;}
  else if(ch == String("ch2")){idx = 1; pR = mySettings.pR[1]; myStates.dutyState[1] = (state == String("ON")) ? mySettings.ON : 1 - mySettings.ON; myStates.publishSwitch[1] = true;}
  else if(ch == String("ch3")){idx = 2; pR = mySettings.pR[2]; myStates.dutyState[2] = (state == String("ON")) ? mySettings.ON : 1 - mySettings.ON; myStates.publishSwitch[2] = true;}
  else if(ch == String("ch4")){idx = 3; pR = mySettings.pR[3]; myStates.dutyState[3] = (state == String("ON")) ? mySettings.ON : 1 - mySettings.ON; myStates.publishSwitch[3] = true;}

  if(state == String("ON"))
  {
    fState = mySettings.ON;
    myStates.stateOnTs[idx] = millis();

    if(mySettings.cpM[idx] == 0){
      myStates.cp0A[idx] = mySettings.ON;
      FLAG_SAVE_STATES = 1;
    }
  }
  else
  {
    fState = 1 - mySettings.ON;
    myStates.stateOnTs[idx] = 0;

    if(mySettings.cpM[idx] == 0){
      myStates.cp0A[idx] = 1 - mySettings.ON;
      FLAG_SAVE_STATES = 1;
    }
  }

  setCoMCUPin(pR, 1, OUTPUT, 0, fState);
  log_manager->warn(PSTR(__func__), "Relay %s was set to %s / %d.\n", ch, state, (int)fState);
  //log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void relayControlTR(void *arg){
  while(true){
    relayControlCP0();
    relayControlCP1();
    relayControlCP2();
    relayControlCP3();
    relayControlCP4();

    vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
  }
}

void relayControlCP0(){
  uint8_t activeCounter = 0;
  for(uint8_t i = 0; i < countof(mySettings.pR); i++)
  {
    if(mySettings.cpM[i] == 0 && myStates.cp0B[i] != 0)
    {
      unsigned long now = millis();
      if( (now - myStates.stateOnTs[i]) >= ( myStates.cp0B[i] * 1000 ) && myStates.cp0A[i] == mySettings.ON)
      {
        String ch = "ch" + String(i + 1);
        setSwitch(ch, "OFF");
        activeCounter++;
      }
    }
  }
  if(activeCounter >= 1)
  {
    FLAG_SAVE_STATES = 1;
  }
}

void relayControlCP1(){
  for(uint8_t i = 0; i < countof(mySettings.pR); i++)
  {
    if (mySettings.cp1B[i] < 2){mySettings.cp1B[i] = 2;} //safenet
    if(mySettings.cp1A[i] != 0 && mySettings.cpM[i] == 1)
    {
      if( myStates.dutyState[i] == mySettings.ON )
      {
        if( mySettings.cp1A[i] != 100 && (millis() - myStates.dutyCounter[i] ) >= (float)(( ((float)mySettings.cp1A[i] / 100) * (float)mySettings.cp1B[i]) * 1000))
        {
          myStates.dutyState[i] = 1 - mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "OFF");
          myStates.dutyCounter[i] = millis();
        }
      }
      else
      {
        if( mySettings.cp1A[i] != 0 && (millis() - myStates.dutyCounter[i] ) >= (float) ( ((100 - (float) mySettings.cp1A[i]) / 100) * (float) mySettings.cp1B[i]) * 1000)
        {
          myStates.dutyState[i] = mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "ON");
          myStates.dutyCounter[i] = millis();
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
        (rtc.getEpoch() - mySettings.cp2A[i]) && myStates.dutyState[i] != mySettings.ON){
          myStates.dutyState[i] = mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "ON");
      }
      else if(myStates.dutyState[i] == mySettings.ON && (mySettings.cp2B[i]) <=
        (rtc.getEpoch() - mySettings.cp2A[i])){
          myStates.dutyState[i] = 1 - mySettings.ON;
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
      if (myStates.dutyState[i] != mySettings.ON && activeTimeWindowCounter > 0){
        myStates.dutyState[i] = mySettings.ON;
        String ch = "ch" + String(i+1);
        setSwitch(ch, "ON");
      }else if(myStates.dutyState[i] == mySettings.ON && activeTimeWindowCounter < 1) {
        myStates.dutyState[i] = 1 - mySettings.ON;
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
      if( myStates.dutyState[i] == 1 - mySettings.ON && (millis() - mySettings.cp4BTs[i]) > (mySettings.cp4A[i] * 1000) ){
        myStates.dutyState[i] = mySettings.ON;
        String ch = "ch" + String(i+1);
        setSwitch(ch, "ON");
        mySettings.cp4BTs[i] = millis();
      }
      if( myStates.dutyState[i] == mySettings.ON && (millis() - mySettings.cp4BTs[i]) > (mySettings.cp4B[i] * 1000) ){
        myStates.dutyState[i] = 1 - mySettings.ON;
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
        log_manager->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
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
        log_manager->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
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
        log_manager->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
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
        log_manager->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
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
      if(data["cp0B1"] != nullptr){myStates.cp0B[0] = data["cp0B1"].as<unsigned long>();}
      if(data["cp0B2"] != nullptr){myStates.cp0B[1] = data["cp0B2"].as<unsigned long>();}
      if(data["cp0B3"] != nullptr){myStates.cp0B[2] = data["cp0B3"].as<unsigned long>();}
      if(data["cp0B4"] != nullptr){myStates.cp0B[3] = data["cp0B4"].as<unsigned long>();}

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
      log_manager->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
    }
  }

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void onTbConnected(){
  myStates.publishSwitch[0] = 1;
  myStates.publishSwitch[1] = 1;
  myStates.publishSwitch[2] = 1;
  myStates.publishSwitch[3] = 1;
}

void onTbDisconnected(){
 
}

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
  if( xSemaphoreTBSend != NULL){
    if( xSemaphoreTake( xSemaphoreTBSend, ( TickType_t ) 0 ) == pdTRUE )
    {
      if(data[PSTR("cmd")] != nullptr){
          const char * cmd = data["cmd"].as<const char *>();
          log_manager->verbose(PSTR(__func__), PSTR("Received command: %s\n"), cmd);

          if(strcmp(cmd, PSTR("setSwitch")) == 0){
            if(data[PSTR("arg")][PSTR("ch")] != nullptr && data[PSTR("arg")][PSTR("st")] != nullptr){
                setSwitch(data[PSTR("arg")][PSTR("ch")].as<String>(), data[PSTR("arg")][PSTR("st")].as<String>());
            }
          }
          else if(strcmp(cmd, PSTR("resetPZEM")) == 0){
            myStates.flag_resetPowerSensor = 1;
          }
      }  
      xSemaphoreGive( xSemaphoreTBSend );
    }
    else
    {
      log_manager->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
    }
  }
  return RPC_Response(PSTR("generic"), 1);
}

void stateReset(bool resetOpMode){
    if(resetOpMode){
        mySettings.cpM[0] = 0;
        mySettings.cpM[1] = 0;
        mySettings.cpM[2] = 0;
        mySettings.cpM[3] = 0;
        log_manager->verbose(PSTR(__func__), PSTR("Relay operations are changed to manual.\n"));
    }
    for(uint8_t i = 0; i < countof(mySettings.pR); i++)
    {
      if(configcomcu.fP){
        String ch = "ch" + String(i + 1);
        setSwitch(ch, "OFF");
        log_manager->verbose(PSTR(__func__), PSTR("%s operation are disabled in panic mode.\n"), ch.c_str());
      }
      else if(mySettings.cpM[i] == 0)
      {
        String ch = "ch" + String(i + 1);
        String state = myStates.cp0A[i] == mySettings.ON ? "ON" : "OFF";
        setSwitch(ch, state);
        log_manager->verbose(PSTR(__func__), PSTR("%s operation are %s.\n"), ch.c_str(), state.c_str());
      }
      else
      {
        String ch = "ch" + String(i + 1);
        setSwitch(ch, "OFF");
        log_manager->verbose(PSTR(__func__), PSTR("%s operation are disabled.\n"), ch.c_str());
      }
    }
}

void deviceTelemetry(){
    if(config.provSent && tb.connected() && config.fIoT){
      StaticJsonDocument<128> doc;
      char buffer[128];
      
      doc[PSTR("uptime")] = millis(); 
      doc[PSTR("heap")] = heap_caps_get_free_size(MALLOC_CAP_8BIT); 
      doc[PSTR("rssi")] = WiFi.RSSI(); 
      doc[PSTR("dt")] = rtc.getEpoch(); 

      serializeJson(doc, buffer);
      tbSendAttribute(buffer);
    }
}

void onAlarm(int code){
  char buffer[32];
  StaticJsonDocument<32> doc;
  doc[PSTR("alarm")] = code;
  serializeJson(doc, buffer);
  wsBroadcastTXT(buffer);
}

void publishDeviceTelemetryTR(void * arg){
  unsigned long timerDeviceTelemetry = millis();
  while(true){
    publishSwitch();
    
    unsigned long now = millis();
    if( (now - timerDeviceTelemetry) > mySettings.itD * 1000 ){
      deviceTelemetry();
      timerDeviceTelemetry = now;
    }
    
    vTaskDelay((const TickType_t) 300 / portTICK_PERIOD_MS);
  }
}

void publishSwitch(){
  for (uint8_t i = 0; i < sizeof(myStates.dutyState); i++){
    if(myStates.publishSwitch[i]){
        String chName = "ch" + String(i+1);
        int state = (int)myStates.dutyState[i] == mySettings.ON ? 1 : 0;

        char buffer[64];
        StaticJsonDocument<64> doc;
        doc[chName.c_str()] = state;
        serializeJson(doc, buffer);
        
        if(config.fIoT && tb.connected() && config.provSent){
          tbSendTelemetry(buffer);
        }

        #ifdef USE_WEB_IFACE
        if(config.fIface && config.wsCount > 0){
          wsBroadcastTXT(buffer);
        }
        #endif

        myStates.publishSwitch[i] = false;
    }
  }
}

void onSyncClientAttr(uint8_t direction){
    long startMillis = millis();

    StaticJsonDocument<1024> doc;
    char buffer[1024];
    

    if(tb.connected() && (direction == 0 || direction == 1)){
      doc[PSTR("cp0A1")] = myStates.cp0A[0];
      doc[PSTR("cp0A2")] = myStates.cp0A[1];
      doc[PSTR("cp0A3")] = myStates.cp0A[2];
      doc[PSTR("cp0A4")] = myStates.cp0A[3];
      doc[PSTR("cp0B1")] = myStates.cp0B[0];
      doc[PSTR("cp0B2")] = myStates.cp0B[1];
      doc[PSTR("cp0B3")] = myStates.cp0B[2];
      doc[PSTR("cp0B4")] = myStates.cp0B[3];
      serializeJson(doc, buffer);
      tbSendAttribute(buffer);
      doc.clear();
      doc[PSTR("cp1A1")] = mySettings.cp1A[0];
      doc[PSTR("cp1A2")] = mySettings.cp1A[1];
      doc[PSTR("cp1A3")] = mySettings.cp1A[2];
      doc[PSTR("cp1A4")] = mySettings.cp1A[3];
      doc[PSTR("cp1B1")] = mySettings.cp1B[0];
      doc[PSTR("cp1B2")] = mySettings.cp1B[1];
      doc[PSTR("cp1B3")] = mySettings.cp1B[2];
      doc[PSTR("cp1B4")] = mySettings.cp1B[3];
      serializeJson(doc, buffer);
      tbSendAttribute(buffer);
      doc.clear();
      doc[PSTR("cp2A1")] = (uint64_t)mySettings.cp2A[0] * 1000;
      doc[PSTR("cp2A2")] = (uint64_t)mySettings.cp2A[1] * 1000;
      doc[PSTR("cp2A3")] = (uint64_t)mySettings.cp2A[2] * 1000;
      doc[PSTR("cp2A4")] = (uint64_t)mySettings.cp2A[3] * 1000;
      doc[PSTR("cp2B1")] = mySettings.cp2B[0];
      doc[PSTR("cp2B2")] = mySettings.cp2B[1];
      doc[PSTR("cp2B3")] = mySettings.cp2B[2];
      doc[PSTR("cp2B4")] = mySettings.cp2B[3];
      serializeJson(doc, buffer);
      tbSendAttribute(buffer);
      doc.clear();
      doc[PSTR("cp4A1")] = mySettings.cp4A[0];
      doc[PSTR("cp4A2")] = mySettings.cp4A[1];
      doc[PSTR("cp4A3")] = mySettings.cp4A[2];
      doc[PSTR("cp4A4")] = mySettings.cp4A[3];
      doc[PSTR("cp4B1")] = mySettings.cp4B[0];
      doc[PSTR("cp4B2")] = mySettings.cp4B[1];
      doc[PSTR("cp4B3")] = mySettings.cp4B[2];
      doc[PSTR("cp4B4")] = mySettings.cp4B[3];
      serializeJson(doc, buffer);
      tbSendAttribute(buffer);
      doc.clear();
      doc[PSTR("pR1")] = mySettings.pR[0];
      doc[PSTR("pR2")] = mySettings.pR[1];
      doc[PSTR("pR3")] = mySettings.pR[2];
      doc[PSTR("pR4")] = mySettings.pR[3];
      doc[PSTR("ON")] = mySettings.ON;
      serializeJson(doc, buffer);
      tbSendAttribute(buffer);
      doc.clear();
      doc[PSTR("cp3A1")] = mySettings.cp3A[0];
      serializeJson(doc, buffer);
      tbSendAttribute(buffer);
      doc.clear();
      doc[PSTR("cp3A2")] = mySettings.cp3A[1];
      serializeJson(doc, buffer);
      tbSendAttribute(buffer);
      doc.clear();
      doc[PSTR("cp3A3")] = mySettings.cp3A[2];
      serializeJson(doc, buffer);
      tbSendAttribute(buffer);
      doc.clear();
      doc[PSTR("cp3A4")] = mySettings.cp3A[3];
      serializeJson(doc, buffer);
      tbSendAttribute(buffer);
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
      serializeJson(doc, buffer);
      tbSendAttribute(buffer);
      doc.clear();
      doc[PSTR("cpM1")] = mySettings.cpM[0];
      doc[PSTR("cpM2")] = mySettings.cpM[1];
      doc[PSTR("cpM3")] = mySettings.cpM[2];
      doc[PSTR("cpM4")] = mySettings.cpM[3];
      doc[PSTR("seaHpa")] = mySettings.seaHpa;
      serializeJson(doc, buffer);
      tbSendAttribute(buffer);
      doc.clear();
    }

    #ifdef USE_WEB_IFACE
    if(config.wsCount > 0 && (direction == 0 || direction == 2)){
      JsonObject cp0A = doc.createNestedObject("cp0A");
      cp0A[PSTR("cp0A1")] = myStates.cp0A[0];
      cp0A[PSTR("cp0A2")] = myStates.cp0A[1];
      cp0A[PSTR("cp0A3")] = myStates.cp0A[2];
      cp0A[PSTR("cp0A4")] = myStates.cp0A[3];
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
      doc.clear();
      JsonObject cp0B = doc.createNestedObject("cp0B");
      cp0B[PSTR("cp0B1")] = myStates.cp0B[0];
      cp0B[PSTR("cp0B2")] = myStates.cp0B[1];
      cp0B[PSTR("cp0B3")] = myStates.cp0B[2];
      cp0B[PSTR("cp0B4")] = myStates.cp0B[3];
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
      doc.clear();
      JsonObject cp1A = doc.createNestedObject("cp1A");
      cp1A[PSTR("cp1A1")] = mySettings.cp1A[0];
      cp1A[PSTR("cp1A2")] = mySettings.cp1A[1];
      cp1A[PSTR("cp1A3")] = mySettings.cp1A[2];
      cp1A[PSTR("cp1A4")] = mySettings.cp1A[3];
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
      doc.clear();
      JsonObject cp1B = doc.createNestedObject("cp1B");
      cp1B[PSTR("cp1B1")] = mySettings.cp1B[0];
      cp1B[PSTR("cp1B2")] = mySettings.cp1B[1];
      cp1B[PSTR("cp1B3")] = mySettings.cp1B[2];
      cp1B[PSTR("cp1B4")] = mySettings.cp1B[3];
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
      doc.clear();
      JsonObject cp2A = doc.createNestedObject("cp2A");
      cp2A[PSTR("cp2A1")] = (uint64_t)mySettings.cp2A[0] * 1000;
      cp2A[PSTR("cp2A2")] = (uint64_t)mySettings.cp2A[1] * 1000;
      cp2A[PSTR("cp2A3")] = (uint64_t)mySettings.cp2A[2] * 1000;
      cp2A[PSTR("cp2A4")] = (uint64_t)mySettings.cp2A[3] * 1000;
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
      doc.clear();
      JsonObject cp2B = doc.createNestedObject("cp2B");
      cp2B[PSTR("cp2B1")] = mySettings.cp2B[0];
      cp2B[PSTR("cp2B2")] = mySettings.cp2B[1];
      cp2B[PSTR("cp2B3")] = mySettings.cp2B[2];
      cp2B[PSTR("cp2B4")] = mySettings.cp2B[3];
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
      doc.clear();
      JsonObject cp4A = doc.createNestedObject("cp4A");
      cp4A[PSTR("cp4A1")] = mySettings.cp4A[0];
      cp4A[PSTR("cp4A2")] = mySettings.cp4A[1];
      cp4A[PSTR("cp4A3")] = mySettings.cp4A[2];
      cp4A[PSTR("cp4A4")] = mySettings.cp4A[3];
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
      doc.clear();
      JsonObject cp4B = doc.createNestedObject("cp4B");
      cp4B[PSTR("cp4B1")] = mySettings.cp4B[0];
      cp4B[PSTR("cp4B2")] = mySettings.cp4B[1];
      cp4B[PSTR("cp4B3")] = mySettings.cp4B[2];
      cp4B[PSTR("cp4B4")] = mySettings.cp4B[3];
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
      doc.clear();
      JsonObject cp3A = doc.createNestedObject("cp3A");
      cp3A[PSTR("cp3A1")] = mySettings.cp3A[0];
      cp3A[PSTR("cp3A2")] = mySettings.cp3A[1];
      cp3A[PSTR("cp3A3")] = mySettings.cp3A[2];
      cp3A[PSTR("cp3A4")] = mySettings.cp3A[3];
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
      doc.clear();
      JsonObject lbl = doc.createNestedObject("lbl");
      lbl[PSTR("lbl1")] = mySettings.lbl[0];
      lbl[PSTR("lbl2")] = mySettings.lbl[1];
      lbl[PSTR("lbl3")] = mySettings.lbl[2];
      lbl[PSTR("lbl4")] = mySettings.lbl[3];
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
      doc.clear();
      JsonObject stg = doc.createNestedObject("stg");
      stg[PSTR("itP")] = mySettings.itP;
      stg[PSTR("itW")] = mySettings.itW;
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
      doc.clear();
      JsonObject cpM = doc.createNestedObject("cpM");
      cpM[PSTR("cpM1")] = mySettings.cpM[0];
      cpM[PSTR("cpM2")] = mySettings.cpM[1];
      cpM[PSTR("cpM3")] = mySettings.cpM[2];
      cpM[PSTR("cpM4")] = mySettings.cpM[3];
      doc[PSTR("ch1")] = (int)myStates.dutyState[0] == mySettings.ON ? 1 : 0;
      doc[PSTR("ch2")] = (int)myStates.dutyState[1] == mySettings.ON ? 1 : 0;
      doc[PSTR("ch3")] = (int)myStates.dutyState[2] == mySettings.ON ? 1 : 0;
      doc[PSTR("ch4")] = (int)myStates.dutyState[3] == mySettings.ON ? 1 : 0;
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
    }
    #endif
    
    log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

#ifdef USE_WEB_IFACE
void onWsEvent(const JsonObject &doc){
  if(doc["evType"] == nullptr){
    log_manager->debug(PSTR(__func__), PSTR("Event type not found.\n"));
    return;
  }
  int evType = doc["evType"].as<int>();


  #ifdef USE_ASYNC_WEB
  if(evType == (int)WS_EVT_CONNECT){
  #endif
  #ifndef USE_ASYNC_WEB
  if(evType == (int)WStype_CONNECTED){
  #endif
    FLAG_SYNC_CLIENT_ATTR_2 = true;
  }
  #ifdef USE_ASYNC_WEB
  if(evType == (int)WS_EVT_DISCONNECT){
  #endif
  #ifndef USE_ASYNC_WEB
  else if(evType == (int)WStype_DISCONNECTED){
  #endif
      if(config.wsCount < 1){
          log_manager->debug(PSTR(__func__),PSTR("No WS client is active. \n"));
      }
  }
  #ifdef USE_ASYNC_WEB
  if(evType == (int)WS_EVT_DATA){
  #endif
  #ifndef USE_ASYNC_WEB
  else if(evType == (int)WStype_TEXT){
  #endif
    if(doc["cmd"] == nullptr){
        log_manager->debug(PSTR(__func__), "Command not found.\n");
        return;
    }
    const char* cmd = doc["cmd"].as<const char*>();
    if(strcmp(cmd, (const char*) "attr") == 0){
      processSharedAttributeUpdate(doc);
      FLAG_SYNC_CLIENT_ATTR_1 = true;
    }
    else if(strcmp(cmd, (const char*) "saveSettings") == 0){
      FLAG_SAVE_SETTINGS = true;
    }
    else if(strcmp(cmd, (const char*) "configSave") == 0){
      FLAG_SAVE_CONFIG = true;
    }
    else if(strcmp(cmd, (const char*) "saveState") == 0){
      FLAG_SAVE_STATES = true;
      log_manager->debug(PSTR(__func__), PSTR("FLAG_SAVE_STATES set to TRUE\n"));
    }
    else if(strcmp(cmd, (const char*) "setPanic") == 0){
      doc[PSTR("st")] = configcomcu.fP ? "OFF" : "ON";
      processSetPanic(doc);
    }
    else if(strcmp(cmd, (const char*) "reboot") == 0){
      reboot();
    }
    else if(strcmp(cmd, (const char*) "setSwitch") == 0){
      setSwitch(doc["ch"].as<String>(), doc["state"].as<int>() == 1 ? "ON" : "OFF");
    }
    #ifdef USE_DISK_LOG
    else if(strcmp(cmd, (const char*) PSTR("wsStreamCardLogger")) == 0){
      GLOBAL_TARGET_CLIENT_ID = doc[PSTR("num")].as<uint32_t>(); 
      GLOBAL_LOG_FILE_NAME = doc[PSTR("fileName")].as<String>();
      FLAG_WS_STREAM_SDCARD = true;
    }
    #endif
  }
}

void wsSendTelemetryTR(void *arg){
  while(true){
    if(config.fIface && config.wsCount > 0){
      char buffer[128];
      StaticJsonDocument<128> doc;
      JsonObject devTel = doc.createNestedObject("devTel");
      devTel[PSTR("heap")] = heap_caps_get_free_size(MALLOC_CAP_8BIT);
      devTel[PSTR("rssi")] = WiFi.RSSI();
      devTel[PSTR("uptime")] = millis()/1000;
      devTel[PSTR("dt")] = rtc.getEpoch();
      devTel[PSTR("dts")] = rtc.getDateTime();
      serializeJson(doc, buffer);
      wsBroadcastTXT(buffer);
      doc.clear();
  
      if( xQueueWsPayloadPowerSensor != NULL ){
        WSPayloadPowerSensor payload;
        if( xQueueReceive( xQueueWsPayloadPowerSensor,  &( payload ), ( TickType_t ) 0 ) == pdPASS )
        {
          JsonObject powerSensor = doc.createNestedObject("powerSensor");
          powerSensor[PSTR("volt")] = payload.volt;
          powerSensor[PSTR("amp")] = payload.amp;
          powerSensor[PSTR("watt")] = payload.watt;
          powerSensor[PSTR("freq")] = payload.freq;
          powerSensor[PSTR("pf")] = payload.pf;
          powerSensor[PSTR("ener")] = payload.ener;
          serializeJson(doc, buffer);
          wsBroadcastTXT(buffer);
          doc.clear();
        }
      }

      if( xQueueWsPayloadWeatherSensor != NULL ){
        WSPayloadWeatherSensor payload;
        if( xQueueReceive( xQueueWsPayloadWeatherSensor,  &( payload ), ( TickType_t ) 0 ) == pdPASS )
        {
          JsonObject weatherSensor = doc.createNestedObject("weatherSensor");
          weatherSensor[PSTR("celc")] = payload.celc;
          weatherSensor[PSTR("rh")] = payload.rh;
          weatherSensor[PSTR("alt")] = payload.alt;
          weatherSensor[PSTR("hpa")] = payload.hpa;
          serializeJson(doc, buffer);
          wsBroadcastTXT(buffer);
          doc.clear();
        }
      }

      #ifdef USE_DISK_LOG
      if(FLAG_WS_STREAM_SDCARD)
      {
        wsStreamCardLogger(GLOBAL_TARGET_CLIENT_ID, GLOBAL_LOG_FILE_NAME);
        FLAG_WS_STREAM_SDCARD = false;
        GLOBAL_TARGET_CLIENT_ID = 0;
        GLOBAL_LOG_FILE_NAME = "";
      }
      #endif
    }
    vTaskDelay((const TickType_t) 1000 / portTICK_PERIOD_MS);
  }
}

void onMQTTUpdateStart(){
  if(xHandleWeatherSensor != NULL){vTaskSuspend(xHandleWeatherSensor);}
  if(xHandlePowerSensor != NULL){vTaskSuspend(xHandlePowerSensor);}
  if(xHandleRelayControl != NULL){vTaskSuspend(xHandleRelayControl);}
  #ifdef USE_WEB_IFACE
  if(xHandleWsSendTelemetry != NULL){vTaskSuspend(xHandleWsSendTelemetry);}
  if(xHandleIface != NULL){vTaskSuspend(xHandleIface);}
  #endif
  if(xHandlePublishDevTel != NULL){vTaskSuspend(xHandlePublishDevTel);}
}

void onMQTTUpdateEnd(){
  
}
#endif