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
Stream_Stats<float> _volt;
Stream_Stats<float> _amp;
Stream_Stats<float> _watt;
Stream_Stats<float> _freq;
Stream_Stats<float> _pf;

Stream_Stats<float> _celc;
Stream_Stats<float> _rh;
Stream_Stats<float> _hpa;
Stream_Stats<float> _alt;

#define INTV_publishSwitchLoop  1 * TASK_SECOND
#define INTV_publishDeviceTelemetryLoop TASK_SECOND
#define INTV_calcWeatherDataLoop 1 * TASK_SECOND
#define INTV_calcPowerUsageLoop 1 * TASK_SECOND
#define INTV_wsSendTelemetryLoop 1 * TASK_SECOND
#define INTV_wsSendSensorsLoop 1 * TASK_SECOND
#define INTV_relayControlCP1Loop 1 * TASK_SECOND
#define INTV_relayControlCP2Loop 1 * TASK_SECOND
#define INTV_relayControlCP3Loop 1 * TASK_SECOND
#define INTV_relayControlCP4Loop 1 * TASK_SECOND
#define INTV_selfDiagnosticShortLoop 120 * TASK_SECOND
#define INTV_selfDiagnosticLongLoop 1800 * TASK_SECOND

Task syncClientAttr(TASK_IMMEDIATE, TASK_ONCE, &syncClientAttrCb, &r, 0, NULL, NULL, 0);
Task publishSwitchLoop(INTV_publishSwitchLoop, TASK_FOREVER, &publishSwitchCb, &r, 0, NULL, NULL, 0);
Task publishDeviceTelemetryLoop(mySettings.itD * TASK_SECOND, TASK_FOREVER, &publishDeviceTelemetryCb, &r, 0, NULL, NULL, 0);
Task calcWeatherDataLoop(INTV_calcWeatherDataLoop, TASK_FOREVER, &calcWeatherDataCb, &r, 0, NULL, NULL, 0);
Task recWeatherDataLoop(mySettings.itW * TASK_SECOND, TASK_FOREVER, &recWeatherDataCb, &r, 0, NULL, NULL, 0);
Task calcPowerUsageLoop(INTV_calcPowerUsageLoop, TASK_FOREVER, &calcPowerUsageCb, &r, 0, NULL, NULL, 0);
Task recPowerUsageLoop(mySettings.itW * TASK_SECOND, TASK_FOREVER, &recPowerUsageCb, &r, 0, NULL, NULL, 0);
Task wsSendTelemetryLoop(INTV_wsSendTelemetryLoop, TASK_FOREVER, &wsSendTelemetryCb, &r, 0, &wsSendEnable, NULL, 0);
Task wsSendSensorsLoop(INTV_wsSendSensorsLoop, TASK_FOREVER, &wsSendSensorsCb, &r, 0, &wsSendEnable, NULL, 0);
Task relayControlCP1Loop(INTV_relayControlCP1Loop, TASK_FOREVER, &relayControlCP1Cb, &r, 0, NULL, NULL, 0);
Task relayControlCP2Loop(INTV_relayControlCP2Loop, TASK_FOREVER, &relayControlCP2Cb, &r, 0, NULL, NULL, 0);
Task relayControlCP3Loop(INTV_relayControlCP3Loop, TASK_FOREVER, &relayControlCP3Cb, &r, 0, NULL, NULL, 0);
Task relayControlCP4Loop(INTV_relayControlCP4Loop, TASK_FOREVER, &relayControlCP4Cb, &r, 0, NULL, NULL, 0);
Task selfDiagnosticShortLoop(INTV_selfDiagnosticShortLoop, TASK_FOREVER, &selfDiagnosticShortCb, &r, 0, NULL, NULL, 0);
Task selfDiagnosticLongLoop(INTV_selfDiagnosticLongLoop, TASK_FOREVER, &selfDiagnosticLongCb, &r, 0, NULL, NULL, 0);

const size_t cbSize = 5;
GCB cb[cbSize] = {
  { "emitAlarmWs", processEmitAlarmWs },
  { "wsEvent", processWsEvent },
  { "onTbConnected", processOnTbConnected },
  { "onTbDisconnected", processOnTbDisconnected },
  { "onUpdateFinished", processOnUpdateFinished }
};

const size_t callbacksSize = 16;
GenericCallback callbacks[callbacksSize] = {
  { "sharedAttributesUpdate", processSharedAttributesUpdate },
  { "provisionResponse", processProvisionResponse },
  { "saveConfig", processSaveConfig },
  { "saveSettings", processSaveSettings },
  { "saveConfigCoMCU", processSaveConfigCoMCU },
  { "syncClientAttributes", processSyncClientAttributes },
  { "reboot", processReboot },
  { "setSwitch", processSetSwitch },
  { "getCh1", processGetSwitchCh1},
  { "getCh2", processGetSwitchCh2},
  { "getCh3", processGetSwitchCh3},
  { "getCh4", processGetSwitchCh4},
  { "setPanic", processSetPanic},
  { "bridge", processBridge},
  { "resetConfig",  processResetConfig},
  { "updateSpiffs", processUpdateSpiffs}
};

void setup()
{
  startup();
  loadSettings();
  syncConfigCoMCU();
  if(String(config.model) == String("Generic")){
    strlcpy(config.model, "Gadadar", sizeof(config.model));
  }
  cbSubscribe(cb, cbSize);
  setAlarm(999, 1, 1, 3000);

  setSwitch("ch1", "OFF");
  setSwitch("ch2", "OFF");
  setSwitch("ch3", "OFF");
  setSwitch("ch4", "OFF");

  mySettings.flag_bme280 = bme.begin(0x76);
  if(!mySettings.flag_bme280){
    log_manager->warn(PSTR(__func__),PSTR("BME weather sensor failed to initialize!\n"));
  }

  calcPowerUsageLoop.enable();  
  calcWeatherDataLoop.enable();

  relayControlCP1Loop.enable();
  relayControlCP2Loop.enable();
  relayControlCP3Loop.enable();
  relayControlCP4Loop.enable();
  publishSwitchLoop.enable();
  selfDiagnosticShortLoop.enable();
  selfDiagnosticLongLoop.enable();
}

void loop()
{
  udawa();
}

void syncClientAttrCb(){
  syncClientAttributes();
  wsSendAttributes();
  mySettings.publishSwitch[0] = true;
  mySettings.publishSwitch[1] = true;
  mySettings.publishSwitch[2] = true;
  mySettings.publishSwitch[3] = true;
}

void selfDiagnosticShortCb(){
  HardwareSerial PZEMSerial(1);
  PZEM004Tv30 PZEM(PZEMSerial, S1_RX, S1_TX);

  if(!mySettings.flag_bme280){
    setAlarm(111, 1, 5, 1000);
  }
  if(isnan(PZEM.voltage()) || PZEM.voltage() <= 0){
    setAlarm(121, 1, 5, 1000);
  }
  if(rtc.getYear() < 2023){
    setAlarm(131, 1, 5, 1000);
  }

  uint8_t ACTIVE_CH_COUNTER;
  for(uint8_t i = 0; i < 4; i++){
    if(mySettings.dutyState[i] == mySettings.ON){
      ACTIVE_CH_COUNTER++;
    }

    if(mySettings.stateOnTs[i] > 0){
      if(millis() - mySettings.stateOnTs[i] > 3000000){
        if(i = 0){setAlarm(211, 1, 10, 1000);}
        else if(i = 0){setAlarm(212, 1, 10, 1000);}
        else if(i = 0){setAlarm(213, 1, 10, 1000);}
        else if(i = 0){setAlarm(214, 1, 10, 1000);}
      }
    }

  }

  if(!isnan(PZEM.voltage()) && ACTIVE_CH_COUNTER > 0 && (int)PZEM.power() < 5){
    setAlarm(221, 1, 10, 1000);
  }

  if(!isnan(PZEM.voltage()) && ACTIVE_CH_COUNTER == 0 && (int)PZEM.power() > 5){
    setAlarm(222, 1, 10, 1000);
  }
}

void selfDiagnosticLongCb(){ 
  
}

void calcPowerUsageCb(){
  HardwareSerial PZEMSerial(1);
  PZEM004Tv30 PZEM(PZEMSerial, S1_RX, S1_TX);

  if(!isnan(PZEM.voltage())){
    _volt.Add(PZEM.voltage());
  }

  if(!isnan(PZEM.current())){
    _amp.Add(PZEM.current());
  }

  if(!isnan(PZEM.power())){
    _watt.Add(PZEM.power());
  }

  if(!isnan(PZEM.frequency())){
    _freq.Add(PZEM.frequency());
  }

  if(!isnan(PZEM.pf())){
    _pf.Add(PZEM.pf());
  }

  StaticJsonDocument<DOCSIZE_MIN> doc;
  doc["_volt"] = PZEM.voltage();
  doc["_amp"] = PZEM.current();
  doc["_watt"] = PZEM.power();
  doc["_freq"] = PZEM.frequency();
  doc["_pf"] = PZEM.pf();
  doc["_ener"] = PZEM.energy();

  tb.sendAttributeDoc(doc);
}

void recPowerUsageCb(){
  //log_manager->verbose(PSTR(__func__), PSTR("Overrun: %d, start delayed by: %d\n"), wifiKeeperLoop.getOverrun(), wifiKeeperLoop.getStartDelay());
  if(tb.connected()){
    StaticJsonDocument<DOCSIZE_MIN> doc;
    doc["volt"] = _volt.Get_Average();
    doc["amp"] = _amp.Get_Average();
    doc["watt"] = _watt.Get_Average();
    doc["freq"] = _freq.Get_Average();
    doc["pf"] = _pf.Get_Average();

    HardwareSerial PZEMSerial(1);
    PZEM004Tv30 PZEM(PZEMSerial, S1_RX, S1_TX);
    doc["ener"] = PZEM.energy();

    tb.sendTelemetryDoc(doc);
    _volt.Clear(); _amp.Clear(); _watt.Clear(); _freq.Clear(); _pf.Clear();
  }
}

void calcWeatherDataCb(){
  if(mySettings.flag_bme280){
    mySettings.celc = bme.readTemperature();
    mySettings.rh = bme.readHumidity();
    mySettings.hpa = bme.readPressure() / 100.0F;
    mySettings.alt = bme.readAltitude(mySettings.seaHpa);

    _celc.Add(mySettings.celc);
    _rh.Add(mySettings.rh);
    _hpa.Add(mySettings.hpa);
    _alt.Add(mySettings.alt);

    StaticJsonDocument<DOCSIZE_MIN> doc;
    doc["_celc"] = mySettings.celc;
    doc["_rh"] = mySettings.rh;
    doc["_hpa"] = mySettings.hpa;
    doc["_alt"] = mySettings.alt;
    tb.sendAttributeDoc(doc);
  }
}

void recWeatherDataCb(){
  //log_manager->verbose(PSTR(__func__), PSTR("Overrun: %d, start delayed by: %d\n"), wifiKeeperLoop.getOverrun(), wifiKeeperLoop.getStartDelay());
  if(tb.connected()){
    mySettings._celc = _celc.Get_Average();
    mySettings._rh = _rh.Get_Average();
    mySettings._hpa = _hpa.Get_Average();
    mySettings._alt = _alt.Get_Average();

    StaticJsonDocument<DOCSIZE_MIN> doc;
    doc["celc"] = mySettings._celc;
    doc["rh"] = mySettings._rh;
    doc["hpa"] = mySettings._hpa;
    doc["alt"] = mySettings._alt;
    tb.sendTelemetryDoc(doc);
    _celc.Clear(); _rh.Clear(); _hpa.Clear(); _alt.Clear();
  }
}

void loadSettings()
{
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
  else{mySettings.itD = 60;}

  for(uint8_t i = 0; i < countof(mySettings.dutyCounter); i++) { mySettings.dutyCounter[i] = 86400; }

  if(doc["seaHpa"] != nullptr){mySettings.seaHpa = doc["seaHpa"].as<float>();}
  else{mySettings.seaHpa = 1019.00;}

  if(doc["lbl"] != nullptr) { uint8_t index = 0; for(JsonVariant v : doc["lbl"].as<JsonArray>()) { strlcpy(mySettings.lbl[index], v.as<const char*>(), sizeof(mySettings.lbl[index])); index++; } } 
  else { for(uint8_t i = 0; i < countof(mySettings.lbl); i++) { strlcpy(mySettings.lbl[i], "unnamed", sizeof(mySettings.lbl[i])); } }

  String tmp;
  if(config.logLev >= 4){serializeJson(doc, tmp);}
  log_manager->debug(PSTR(__func__), "Loaded settings:\n %s \n", tmp.c_str());
}

void saveSettings()
{
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
}

JsonObject processOnUpdateFinished(JsonObject &data){
  saveSettings();
  return data;
}

JsonObject processWsEvent(JsonObject &doc){
  if(doc["evType"] == nullptr){
    log_manager->debug(PSTR(__func__), "Event type not found.\n");
    return doc;
  }
  int evType = doc["evType"].as<int>();


  if(evType == (int)WStype_CONNECTED){
    syncClientAttr.setInterval(TASK_IMMEDIATE);
    syncClientAttr.setIterations(TASK_ONCE);
    syncClientAttr.enableIfNot();

    wsSendTelemetryLoop.setInterval(INTV_wsSendTelemetryLoop);
    wsSendTelemetryLoop.setIterations(TASK_FOREVER);
    wsSendTelemetryLoop.enableIfNot();

    wsSendSensorsLoop.setInterval(INTV_wsSendSensorsLoop);
    wsSendSensorsLoop.setIterations(TASK_FOREVER);
    wsSendSensorsLoop.enableIfNot();

  }
  if(evType == (int)WStype_DISCONNECTED){
    if(config.wsCount < 1){
      wsSendTelemetryLoop.disable();
      wsSendSensorsLoop.disable();
      log_manager->debug(PSTR(__func__),PSTR("No WS client is active. \n"));
    }
  }
  else if(evType == (int)WStype_TEXT){
    if(doc["cmd"] == nullptr){
      log_manager->debug(PSTR(__func__), "Command not found.\n");
      return doc;
    }
    const char* cmd = doc["cmd"].as<const char*>();
    if(strcmp(cmd, (const char*) "attr") == 0){
      processSharedAttributesUpdate(doc);
      syncClientAttr.setInterval(TASK_IMMEDIATE);
      syncClientAttr.setIterations(TASK_ONCE);
      syncClientAttr.enable();
    }
    else if(strcmp(cmd, (const char*) "saveSettings") == 0){
      saveSettings();
    }
    else if(strcmp(cmd, (const char*) "saveConfig") == 0){
      configSave();
    }
    else if(strcmp(cmd, (const char*) "setPanic") == 0){
      JsonObject params = doc.createNestedObject("params");
      params["state"]= "ON";
      processSetPanic(doc);
    }
    else if(strcmp(cmd, (const char*) "reboot") == 0){
      reboot();
    }
    else if(strcmp(cmd, (const char*) "setSwitch") == 0){
      setSwitch(doc["ch"].as<String>(), doc["state"].as<int>() == 1 ? "ON" : "OFF");
    }
  }

  return doc;
}

JsonObject processOnTbConnected(JsonObject &data){
  StaticJsonDocument<DOCSIZE_MIN> doc;
  doc["method"] = "sharedAttributesUpdate";
  JsonObject params = doc.createNestedObject("params");
  params["name"] = config.name;
  JsonObject payload = doc.template as<JsonObject>();
  tb.server_rpc_call(payload);

  if(tb.callbackSubscribe(callbacks, callbacksSize))
  {
    log_manager->info(PSTR(__func__),PSTR("Callbacks subscribed successfuly!\n"));
  }
  if (tb.Firmware_Update(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION))
  {
    log_manager->info(PSTR(__func__),PSTR("OTA Update finished, rebooting...\n"));
    reboot();
  }

  syncClientAttr.setInterval(TASK_IMMEDIATE);
  syncClientAttr.setIterations(TASK_ONCE);
  syncClientAttr.enable();

  publishDeviceTelemetryLoop.setInterval(mySettings.itD * TASK_SECOND);
  publishDeviceTelemetryLoop.setIterations(TASK_FOREVER);
  publishDeviceTelemetryLoop.setCallback(&publishDeviceTelemetryCb);
  publishDeviceTelemetryLoop.enable();

  recPowerUsageLoop.setInterval(mySettings.itP * TASK_SECOND);
  recPowerUsageLoop.setIterations(TASK_FOREVER);
  recPowerUsageLoop.setCallback(&recPowerUsageCb);
  recPowerUsageLoop.enable();

  recWeatherDataLoop.setInterval(mySettings.itW * TASK_SECOND);
  recWeatherDataLoop.setIterations(TASK_FOREVER);
  recWeatherDataLoop.setCallback(&recWeatherDataCb);
  recWeatherDataLoop.enable();
  return data;
}

JsonObject processOnTbDisconnected(JsonObject &data){
  publishDeviceTelemetryLoop.disable();
  recPowerUsageLoop.disable();
  recWeatherDataLoop.disable();
  return data;
}

JsonObject processEmitAlarmWs(JsonObject &data){
  if(!config.fIface || config.wsCount < 1){
    return data;
  }
  wsSend(data);
  return data;
}

callbackResponse processUpdateSpiffs(const callbackData &data){
  updateSpiffs.enableDelayed(10);
  return callbackResponse("updateSpiffs", 1);
}

callbackResponse processSaveConfig(const callbackData &data)
{
  configSave();
  return callbackResponse("saveConfig", 1);
}

callbackResponse processSaveSettings(const callbackData &data)
{
  saveSettings();
  loadSettings();

  mySettings.lastUpdated = millis();
  return callbackResponse("saveSettings", 1);
}

callbackResponse processSaveConfigCoMCU(const callbackData &data)
{
  configCoMCUSave();
  configCoMCULoad();
  //syncConfigCoMCU();
  return callbackResponse("saveConfigCoMCU", 1);
}

callbackResponse processReboot(const callbackData &data)
{
  reboot();
  return callbackResponse("reboot", 1);
}

callbackResponse processSyncClientAttributes(const callbackData &data)
{
  syncClientAttr.setInterval(TASK_IMMEDIATE);
  syncClientAttr.setIterations(TASK_ONCE);
  syncClientAttr.enable();
  return callbackResponse("syncClientAttributes", 1);
}

callbackResponse processSetSwitch(const callbackData &data)
{
  if(data["params"]["ch"] != nullptr && data["params"]["state"] != nullptr)
  {
    String ch = data["params"]["ch"].as<String>();
    String state = data["params"]["state"].as<String>();
    log_manager->debug(PSTR(__func__),"Calling setSwitch (%s - %s)...\n", ch, state);
    setSwitch(ch, state);
  }
  return callbackResponse(data["params"]["ch"].as<const char*>(), data["params"]["state"].as<const char*>());
}

callbackResponse processGetSwitchCh1(const callbackData &data)
{
  return callbackResponse("ch1", mySettings.dutyState[0] == mySettings.ON ? "ON" : "OFF");
}

callbackResponse processGetSwitchCh2(const callbackData &data)
{
  return callbackResponse("ch2", mySettings.dutyState[1] == mySettings.ON ? "ON" : "OFF");
}

callbackResponse processGetSwitchCh3(const callbackData &data)
{
  return callbackResponse("ch3", mySettings.dutyState[2] == mySettings.ON ? "ON" : "OFF");
}

callbackResponse processGetSwitchCh4(const callbackData &data)
{
  return callbackResponse("ch4", mySettings.dutyState[3] == mySettings.ON ? "ON" : "OFF");
}

callbackResponse processSetPanic(const callbackData &data)
{

  StaticJsonDocument<DOCSIZE_MIN> doc;
  doc["method"] = "sCfg";
  String state = data["params"]["state"].as<String>();
  if(state == String("ON")){
    doc["fP"] = 1;
    configcomcu.fP = 1;

    mySettings.cpM[0] = 0;
    mySettings.cpM[1] = 0;
    mySettings.cpM[2] = 0;
    mySettings.cpM[3] = 0;
    setSwitch("ch1", "OFF");
    setSwitch("ch2", "OFF");
    setSwitch("ch3", "OFF");
    setSwitch("ch4", "OFF");
  }
  else{
    doc["fP"] = 0;
    configcomcu.fP = 0;
  }
  serialWriteToCoMcu(doc, 0);
  syncClientAttr.setInterval(TASK_IMMEDIATE);
  syncClientAttr.setIterations(TASK_ONCE);
  syncClientAttr.enable();
  return callbackResponse("setPanic", data["params"]["state"].as<int>());
}

callbackResponse processBridge(const callbackData &data)
{
  StaticJsonDocument<DOCSIZE_MIN> doc;
  doc["method"] = data["params"]["method"];
  doc["params"] = data["params"];
  if(data["params"]["isRpc"] != nullptr){
    serialWriteToCoMcu(doc, 1);
    String result;
    serializeJson(doc, result);
    return callbackResponse("bridge", result.c_str());
  }
  else if(doc["method"] == "resetPZEM"){
    HardwareSerial PZEMSerial(1);
    PZEM004Tv30 PZEM(PZEMSerial, S1_RX, S1_TX);
    PZEM.resetEnergy();
    return callbackResponse("resetPZEM", 1);
  }
  else{
    serialWriteToCoMcu(doc, 0);
    String result;
    serializeJson(doc, result);
    return callbackResponse("bridge", result.c_str());
  }
}

callbackResponse processResetConfig(const callbackData &data){
  if(data["params"]["format"] != nullptr){
    bool formatted = SPIFFS.format();
    if(formatted)
    {
      log_manager->info(PSTR(__func__),PSTR("SPIFFS formatting success.\n"));
    }
    else
    {
      log_manager->warn(PSTR(__func__),PSTR("SPIFFS formatting failed.\n"));
    }
  }
  configReset();
  reboot();
  return callbackResponse("resetConfig", 1);
}

void setSwitch(String ch, String state)
{
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
}

void relayControlCP2Cb(){
  //log_manager->verbose(PSTR(__func__), PSTR("Overrun: %d, start delayed by: %d\n"), wifiKeeperLoop.getOverrun(), wifiKeeperLoop.getStartDelay());
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

void relayControlCP1Cb()
{
  //log_manager->verbose(PSTR(__func__), PSTR("Overrun: %d, start delayed by: %d\n"), wifiKeeperLoop.getOverrun(), wifiKeeperLoop.getStartDelay());
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

void relayControlCP4Cb(){
  //log_manager->verbose(PSTR(__func__), PSTR("Overrun: %d, start delayed by: %d\n"), wifiKeeperLoop.getOverrun(), wifiKeeperLoop.getStartDelay());
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

void relayControlCP3Cb(){
  //log_manager->verbose(PSTR(__func__), PSTR("Overrun: %d, start delayed by: %d\n"), wifiKeeperLoop.getOverrun(), wifiKeeperLoop.getStartDelay());
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

callbackResponse processSharedAttributesUpdate(const callbackData &data)
{
  String s;
  serializeJson(data, s);
  log_manager->debug(PSTR(__func__), PSTR("Received attributes update request: %s\n"), s.c_str());

  if(data["model"] != nullptr){strlcpy(config.model, data["model"].as<const char*>(), sizeof(config.model));}
  if(data["group"] != nullptr){strlcpy(config.group, data["group"].as<const char*>(), sizeof(config.group));}
  if(data["broker"] != nullptr){strlcpy(config.broker, data["broker"].as<const char*>(), sizeof(config.broker));}
  if(data["port"] != nullptr){config.port = data["port"].as<uint16_t>();}
  if(data["wssid"] != nullptr){strlcpy(config.wssid, data["wssid"].as<const char*>(), sizeof(config.wssid));}
  if(data["wpass"] != nullptr){strlcpy(config.wpass, data["wpass"].as<const char*>(), sizeof(config.wpass));}
  if(data["dssid"] != nullptr){strlcpy(config.dssid, data["dssid"].as<const char*>(), sizeof(config.dssid));}
  if(data["dpass"] != nullptr){strlcpy(config.dpass, data["dpass"].as<const char*>(), sizeof(config.dpass));}
  if(data["upass"] != nullptr){strlcpy(config.upass, data["upass"].as<const char*>(), sizeof(config.upass));}
  if(data["accTkn"] != nullptr){strlcpy(config.accTkn, data["accTkn"].as<const char*>(), sizeof(config.accTkn));}
  if(data["provDK"] != nullptr){strlcpy(config.provDK, data["provDK"].as<const char*>(), sizeof(config.provDK));}
  if(data["provDS"] != nullptr){strlcpy(config.provDS, data["provDS"].as<const char*>(), sizeof(config.provDS));}
  if(data["logLev"] != nullptr){config.logLev = data["logLev"].as<uint8_t>(); log_manager->set_log_level(PSTR("*"), (LogLevel) config.logLev);;}
  if(data["gmtOff"] != nullptr){config.gmtOff = data["gmtOff"].as<int>();}
  if(data["fIoT"] != nullptr){config.fIoT = data["fIoT"].as<int>();}
  if(data["htU"] != nullptr){strlcpy(config.htU, data["htU"].as<const char*>(), sizeof(config.htU));}
  if(data["htP"] != nullptr){strlcpy(config.htP, data["htP"].as<const char*>(), sizeof(config.htP));}
  if(data["fWOTA"] != nullptr){config.fWOTA = data["fWOTA"].as<bool>();}
  if(data["fIface"] != nullptr){config.fIface = data["fIface"].as<bool>();}
  if(data["hname"] != nullptr){strlcpy(config.hname, data["hname"].as<const char*>(), sizeof(config.hname));}
  if(data["logIP"] != nullptr){strlcpy(config.logIP, data["logIP"].as<const char*>(), sizeof(config.logIP));}
  if(data["logPrt"] != nullptr){config.logPrt = data["logPrt"].as<uint16_t>();}


  if(data["cp1A1"] != nullptr)
  {
    mySettings.cp1A[0] = data["cp1A1"].as<uint8_t>();
    if(data["cp1A1"].as<uint8_t>() == 0)
    {
      setSwitch(String("ch1"), String("OFF"));
    }
  }
  if(data["cp1A2"] != nullptr)
  {
    mySettings.cp1A[1] = data["cp1A2"].as<uint8_t>();
    if(data["cp1A2"].as<uint8_t>() == 0)
    {
      setSwitch(String("ch2"), String("OFF"));
    }
  }
  if(data["cp1A3"] != nullptr)
  {
    mySettings.cp1A[2] = data["cp1A3"].as<uint8_t>();
    if(data["cp1A3"].as<uint8_t>() == 0)
    {
      setSwitch(String("ch3"), String("OFF"));
    }
  }
  if(data["cp1A4"] != nullptr)
  {
    mySettings.cp1A[3] = data["cp1A4"].as<uint8_t>();
    if(data["cp1A4"].as<uint8_t>() == 0)
    {
      setSwitch(String("ch4"), String("OFF"));
    }
  }

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

  if(data["fP"] != nullptr){configcomcu.fP = data["fP"].as<bool>();}
  if(data["bFr"] != nullptr){configcomcu.bFr = data["bFr"].as<uint16_t>();}
  if(data["fB"] != nullptr){configcomcu.fB = data["fB"].as<bool>();}
  if(data["pBz"] != nullptr){configcomcu.pBz = data["pBz"].as<uint8_t>();}
  if(data["pLR"] != nullptr){configcomcu.pLR = data["pLR"].as<uint8_t>();}
  if(data["pLG"] != nullptr){configcomcu.pLG = data["pLG"].as<uint8_t>();}
  if(data["pLB"] != nullptr){configcomcu.pLB = data["pLB"].as<uint8_t>();}
  if(data["lON"] != nullptr){configcomcu.lON = data["lON"].as<uint8_t>();}


  if(data["cmd"] == nullptr){
    wsSendAttributes();
  }
  mySettings.lastUpdated = millis();
  return callbackResponse("sharedAttributesUpdate", 1);
}

void syncClientAttributes()
{
  StaticJsonDocument<1024> root;
  JsonObject doc = root.to<JsonObject>();

  IPAddress ip = WiFi.localIP();
  char ipa[25];
  sprintf(ipa, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  doc["ipad"] = ipa;
  doc["compdate"] = COMPILED;
  doc["fmTitle"] = CURRENT_FIRMWARE_TITLE;
  doc["fmVersion"] = CURRENT_FIRMWARE_VERSION;
  doc["stamac"] = WiFi.macAddress();
  tb.sendAttributeObj(doc);
  root.clear();
  doc["apmac"] = WiFi.softAPmacAddress();
  doc["flFree"] = ESP.getFreeSketchSpace();
  doc["fwSize"] = ESP.getSketchSize();
  doc["flSize"] = ESP.getFlashChipSize();
  doc["sdkVer"] = ESP.getSdkVersion();
  tb.sendAttributeObj(doc);
  root.clear();
  doc["model"] = config.model;
  doc["group"] = config.group;
  doc["broker"] = config.broker;
  doc["port"] = config.port;
  doc["wssid"] = config.wssid;
  doc["ap"] = WiFi.SSID();
  tb.sendAttributeObj(doc);
  root.clear();
  doc["wpass"] = config.wpass;
  doc["dssid"] = config.dssid;
  doc["dpass"] = config.dpass;
  doc["upass"] = config.upass;
  doc["accTkn"] = config.accTkn;
  tb.sendAttributeObj(doc);
  root.clear();
  doc["provDK"] = config.provDK;
  doc["provDS"] = config.provDS;
  doc["logLev"] = config.logLev;
  doc["gmtOff"] = config.gmtOff;
  tb.sendAttributeObj(doc);
  root.clear();
  doc["fIoT"] = (int)config.fIoT;
  doc["cp1A1"] = mySettings.cp1A[0];
  doc["cp1A2"] = mySettings.cp1A[1];
  doc["cp1A3"] = mySettings.cp1A[2];
  doc["cp1A4"] = mySettings.cp1A[3];
  tb.sendAttributeObj(doc);
  root.clear();
  doc["cp1B1"] = mySettings.cp1B[0];
  doc["cp1B2"] = mySettings.cp1B[1];
  doc["cp1B3"] = mySettings.cp1B[2];
  doc["cp1B4"] = mySettings.cp1B[3];
  tb.sendAttributeObj(doc);
  root.clear();
  doc["cp2A1"] = (uint64_t)mySettings.cp2A[0] * 1000;
  doc["cp2A2"] = (uint64_t)mySettings.cp2A[1] * 1000;
  doc["cp2A3"] = (uint64_t)mySettings.cp2A[2] * 1000;
  doc["cp2A4"] = (uint64_t)mySettings.cp2A[3] * 1000;
  tb.sendAttributeObj(doc);
  root.clear();
  doc["cp2B1"] = mySettings.cp2B[0];
  doc["cp2B2"] = mySettings.cp2B[1];
  doc["cp2B3"] = mySettings.cp2B[2];
  doc["cp2B4"] = mySettings.cp2B[3];
  tb.sendAttributeObj(doc);
  root.clear();
  doc["cp4A1"] = mySettings.cp4A[0];
  doc["cp4A2"] = mySettings.cp4A[1];
  doc["cp4A3"] = mySettings.cp4A[2];
  doc["cp4A4"] = mySettings.cp4A[3];
  tb.sendAttributeObj(doc);
  root.clear();
  doc["cp4B1"] = mySettings.cp4B[0];
  doc["cp4B2"] = mySettings.cp4B[1];
  doc["cp4B3"] = mySettings.cp4B[2];
  doc["cp4B4"] = mySettings.cp4B[3];
  tb.sendAttributeObj(doc);
  root.clear();
  doc["pR1"] = mySettings.pR[0];
  doc["pR2"] = mySettings.pR[1];
  doc["pR3"] = mySettings.pR[2];
  doc["pR4"] = mySettings.pR[3];
  doc["ON"] = mySettings.ON;
  tb.sendAttributeObj(doc);
  root.clear();
  doc["cp3A1"] = mySettings.cp3A[0];
  doc["cp3A2"] = mySettings.cp3A[1];
  doc["cp3A3"] = mySettings.cp3A[2];
  doc["cp3A4"] = mySettings.cp3A[3];
  tb.sendAttributeObj(doc);
  root.clear();
  doc["lbl1"] = mySettings.lbl[0];
  doc["lbl2"] = mySettings.lbl[1];
  doc["lbl3"] = mySettings.lbl[2];
  doc["lbl4"] = mySettings.lbl[3];
  tb.sendAttributeObj(doc);
  root.clear();
  doc["itP"] = mySettings.itP;
  doc["itW"] = mySettings.itW;
  doc["itD"] = mySettings.itD;
  tb.sendAttributeObj(doc);
  root.clear();
  doc["cpM1"] = mySettings.cpM[0];
  doc["cpM2"] = mySettings.cpM[1];
  doc["cpM3"] = mySettings.cpM[2];
  doc["cpM4"] = mySettings.cpM[3];
  doc["seaHpa"] = mySettings.seaHpa;
  tb.sendAttributeObj(doc);
  root.clear();
  doc["pBz"] = configcomcu.pBz;
  doc["pLR"] = configcomcu.pLR;
  doc["pLG"] = configcomcu.pLG;
  doc["pLB"] = configcomcu.pLB;
  doc["htU"] = config.htU;
  doc["htP"] = config.htP;
  tb.sendAttributeObj(doc);
  root.clear();
  doc["lON"] = configcomcu.lON;
  doc["bFr"] = configcomcu.bFr;
  doc["fP"] = configcomcu.fP;
  doc["fB"] = configcomcu.fB;
  doc["bFr"] = configcomcu.bFr;
  tb.sendAttributeObj(doc);
  root.clear();
  doc["fWOTA"] = (int)config.fWOTA;
  doc["fIface"] = (int)config.fIface;
  doc["hname"] = config.hname;
  doc["logIP"] = config.logIP;
  doc["logPrt"] = config.logPrt;
  tb.sendAttributeObj(doc);
  root.clear();
}

void publishDeviceTelemetryCb()
{
    StaticJsonDocument<DOCSIZE_MIN> root;
    JsonObject doc = root.to<JsonObject>();

    doc["heap"] = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = millis()/1000;
    doc["dt"] = rtc.getEpoch();
    doc["dts"] = rtc.getDateTime();
    tb.sendTelemetryObj(doc);
}

void publishSwitchCb(){
  for (uint8_t i = 0; i < 4; i++){
    if(mySettings.publishSwitch[i]){
      StaticJsonDocument<DOCSIZE_MIN> doc;
      JsonObject data = doc.to<JsonObject>();

      String chName = "ch" + String(i+1);
      data[chName.c_str()] = (int)mySettings.dutyState[i] == mySettings.ON ? 1 : 0;
      
      if(tb.connected()){ tb.sendTelemetryObj(data); }
      if(config.fIface){ wsSend(data); }
      mySettings.publishSwitch[i] = false;
    }
  }
}

void wsSend(JsonObject &doc){
  String buffer;
  serializeJson(doc, buffer);
  ws.broadcastTXT(buffer);
}
void wsSend(JsonObject &doc, uint8_t num){
  String buffer;
  serializeJson(doc, buffer);
  ws.sendTXT(num, buffer);
}

bool wsSendEnable(){
  if(config.fIface){
    log_manager->verbose(PSTR(__func__),PSTR("Enabled.\n"));
    return true;
  }
  else{
    return false;
  }
}

void wsSendTelemetryCb(){
  if(config.fIface){
    StaticJsonDocument<DOCSIZE_MIN> root;
    JsonObject doc = root.to<JsonObject>();
    JsonObject devTel = doc.createNestedObject("devTel");
    devTel["heap"] = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    devTel["rssi"] = WiFi.RSSI();
    devTel["uptime"] = millis()/1000;
    devTel["dt"] = rtc.getEpoch();
    devTel["dts"] = rtc.getDateTime();
    wsSend(doc);
  }
}

void wsSendSensorsCb(){
  if(config.fIface){
    HardwareSerial PZEMSerial(1);
    PZEM004Tv30 PZEM(PZEMSerial, S1_RX, S1_TX);

    StaticJsonDocument<DOCSIZE_MIN> root;
    JsonObject doc = root.to<JsonObject>();
    JsonObject pzem = doc.createNestedObject("pzem");
    if(!isnan(PZEM.voltage())){
      pzem["volt"] = round2(PZEM.voltage());
      pzem["amp"] = round2(PZEM.current());
      pzem["watt"] = round2(PZEM.power());
      pzem["ener"] = round2(PZEM.energy());
      pzem["freq"] = round2(PZEM.frequency());
      pzem["pf"] = round2(PZEM.pf())*100;
      wsSend(doc);
      doc.clear();
    }

    JsonObject bme280 = doc.createNestedObject("bme280");
    bme280["celc"] = round2(mySettings.celc);
    bme280["rh"] = round2(mySettings.rh);
    bme280["hpa"] = round2(mySettings.hpa);
    bme280["alt"] = round2(mySettings.alt);
    wsSend(doc);
    doc.clear();
  }
}

void wsSendAttributes(){
  StaticJsonDocument<DOCSIZE_MIN> root;
  JsonObject doc = root.to<JsonObject>();
  IPAddress ip = WiFi.localIP();
  char ipa[25];
  sprintf(ipa, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  JsonObject attr = doc.createNestedObject("attr");
  attr["ipad"] = ipa;
  attr["compdate"] = COMPILED;
  attr["fmTitle"] = CURRENT_FIRMWARE_TITLE;
  attr["fmVersion"] = CURRENT_FIRMWARE_VERSION;
  attr["stamac"] = WiFi.macAddress();
  attr["apmac"] = WiFi.softAPmacAddress();
  attr["flFree"] = ESP.getFreeSketchSpace();
  attr["fwSize"] = ESP.getSketchSize();
  attr["flSize"] = ESP.getFlashChipSize();
  attr["sdkVer"] = ESP.getSdkVersion();
  wsSend(doc);
  root.clear();
  JsonObject cfg = doc.createNestedObject("cfg");
  cfg["model"] = config.model;
  cfg["group"] = config.group;
  cfg["broker"] = config.broker;
  cfg["port"] = config.port;
  cfg["wssid"] = config.wssid;
  cfg["ap"] = WiFi.SSID();
  cfg["wpass"] = config.wpass;
  cfg["gmtOff"] = config.gmtOff;
  cfg["htU"] = config.htU;
  cfg["htP"] = config.htP;
  cfg["name"] = config.name;
  cfg["itP"] = mySettings.itP;
  cfg["itW"] = mySettings.itW;
  cfg["itD"] = mySettings.itD;
  cfg["fIoT"] = (int)config.fIoT;
  cfg["fWOTA"] = (int)config.fWOTA;
  cfg["fIface"] = (int)config.fIface;
  cfg["hname"] = config.hname;
  cfg["logIP"] = config.logIP;
  cfg["logPrt"] = config.logPrt;
  wsSend(doc);
  root.clear();
  JsonObject cp1A = doc.createNestedObject("cp1A");
  cp1A["cp1A1"] = mySettings.cp1A[0];
  cp1A["cp1A2"] = mySettings.cp1A[1];
  cp1A["cp1A3"] = mySettings.cp1A[2];
  cp1A["cp1A4"] = mySettings.cp1A[3];
  wsSend(doc);
  root.clear();
  JsonObject cp1B = doc.createNestedObject("cp1B");
  cp1B["cp1B1"] = mySettings.cp1B[0];
  cp1B["cp1B2"] = mySettings.cp1B[1];
  cp1B["cp1B3"] = mySettings.cp1B[2];
  cp1B["cp1B4"] = mySettings.cp1B[3];
  wsSend(doc);
  root.clear();
  JsonObject cp2A = doc.createNestedObject("cp2A");
  cp2A["cp2A1"] = (uint64_t)mySettings.cp2A[0] * 1000;
  cp2A["cp2A2"] = (uint64_t)mySettings.cp2A[1] * 1000;
  cp2A["cp2A3"] = (uint64_t)mySettings.cp2A[2] * 1000;
  cp2A["cp2A4"] = (uint64_t)mySettings.cp2A[3] * 1000;
  wsSend(doc);
  root.clear();
  JsonObject cp2B = doc.createNestedObject("cp2B");
  cp2B["cp2B1"] = mySettings.cp2B[0];
  cp2B["cp2B2"] = mySettings.cp2B[1];
  cp2B["cp2B3"] = mySettings.cp2B[2];
  cp2B["cp2B4"] = mySettings.cp2B[3];
  wsSend(doc);
  root.clear();
  JsonObject cp4A = doc.createNestedObject("cp4A");
  cp4A["cp4A1"] = mySettings.cp4A[0];
  cp4A["cp4A2"] = mySettings.cp4A[1];
  cp4A["cp4A3"] = mySettings.cp4A[2];
  cp4A["cp4A4"] = mySettings.cp4A[3];
  wsSend(doc);
  root.clear();
  JsonObject cp4B = doc.createNestedObject("cp4B");
  cp4B["cp4B1"] = mySettings.cp4B[0];
  cp4B["cp4B2"] = mySettings.cp4B[1];
  cp4B["cp4B3"] = mySettings.cp4B[2];
  cp4B["cp4B4"] = mySettings.cp4B[3];
  wsSend(doc);
  root.clear();
  JsonObject cpM = doc.createNestedObject("cpM");
  cpM["cpM1"] = mySettings.cpM[0];
  cpM["cpM2"] = mySettings.cpM[1];
  cpM["cpM3"] = mySettings.cpM[2];
  cpM["cpM4"] = mySettings.cpM[3];
  wsSend(doc);
  root.clear();
  JsonObject cp3A = doc.createNestedObject("cp3A");
  cp3A["cp3A1"] = mySettings.cp3A[0];
  cp3A["cp3A2"] = mySettings.cp3A[1];
  cp3A["cp3A3"] = mySettings.cp3A[2];
  cp3A["cp3A4"] = mySettings.cp3A[3];
  wsSend(doc);
  root.clear();
  doc["ch1"] = mySettings.dutyState[0] == mySettings.ON ? 1 : 0;
  doc["ch2"] = mySettings.dutyState[1] == mySettings.ON ? 1 : 0;
  doc["ch3"] = mySettings.dutyState[2] == mySettings.ON ? 1 : 0;
  doc["ch4"] = mySettings.dutyState[3] == mySettings.ON ? 1 : 0;
  wsSend(doc);
  root.clear();
  JsonObject lbl = doc.createNestedObject("lbl");
  lbl["lbl1"] = mySettings.lbl[0];
  lbl["lbl2"] = mySettings.lbl[1];
  lbl["lbl3"] = mySettings.lbl[2];
  lbl["lbl4"] = mySettings.lbl[3];
  wsSend(doc);
  root.clear();
}