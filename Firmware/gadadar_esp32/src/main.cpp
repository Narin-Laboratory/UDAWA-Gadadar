/**
 * UDAWA - Universal Digital Agriculture Watering Assistant
 * Firmware for Actuator 4Ch UDAWA Board (Gadadar)
 * Licensed under aGPLv3
 * Researched and developed by PRITA Research Group & Narin Laboratory
 * prita.undiknas.ac.id | narin.co.id
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
Stream_Stats<float> _ener;
Stream_Stats<float> _freq;
Stream_Stats<float> _pf;

Stream_Stats<float> _celc;
Stream_Stats<float> _rh;
Stream_Stats<float> _hpa;
Stream_Stats<float> _alt;

Task syncClientAttr(TASK_IMMEDIATE, TASK_ONCE, &syncClientAttrCb, &r, 0, NULL, NULL, 0);
Task publishSwitchLoop(1 * TASK_SECOND, TASK_FOREVER, &publishSwitchCb, &r, 0, NULL, NULL, 0);
Task publishDeviceTelemetryLoop(mySettings.intvDevTel * TASK_SECOND, TASK_FOREVER, &publishDeviceTelemetryCb, &r, 0, NULL, NULL, 0);
Task calcWeatherDataLoop(30 * TASK_SECOND, TASK_FOREVER, &calcWeatherDataCb, &r, 0, NULL, NULL, 0);
Task recWeatherDataLoop(mySettings.intvRecWthr * TASK_SECOND, TASK_FOREVER, &recWeatherDataCb, &r, 0, NULL, NULL, 0);
Task calcPowerUsageLoop(1 * TASK_SECOND, TASK_FOREVER, &calcPowerUsageCb, &r, 0, NULL, NULL, 0);
Task recPowerUsageLoop(mySettings.intvRecPwrUsg * TASK_SECOND, TASK_FOREVER, &calcPowerUsageCb, &r, 0, NULL, NULL, 0);
Task wsSendTelemetryLoop(1 * TASK_SECOND, TASK_FOREVER, &wsSendTelemetryCb, &r, 0, &wsSendEnable, NULL, 0);
Task wsSendSensorsLoop(1 * TASK_SECOND, TASK_FOREVER, &wsSendSensorsCb, &r, 0, &wsSendEnable, NULL, 0);
Task relayControlBydtCycLoop(1 * TASK_SECOND, TASK_FOREVER, &relayControlBydtCycCb, &r, 0, NULL, NULL, 0);
Task relayControlByDateTimeLoop(1 * TASK_SECOND, TASK_FOREVER, &relayControlByDateTimeCb, &r, 0, NULL, NULL, 0);
Task relayControlByIntrvlLoop(1 * TASK_SECOND, TASK_FOREVER, &relayControlByIntrvlCb, &r, 0, NULL, NULL, 0);
Task relayControlByMultiTimeLoop(1 * TASK_SECOND, TASK_FOREVER, &relayControlByMultiTimeCb, &r, 0, NULL, NULL, 0);
Task selfDiagnosticShortLoop(120 * TASK_SECOND, TASK_FOREVER, &selfDiagnosticShortCb, &r, 0, NULL, NULL, 0);
Task selfDiagnosticLongLoop(120 * TASK_SECOND, TASK_FOREVER, &selfDiagnosticLongCb, &r, 0, NULL, NULL, 0);

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
  { "getSwitchCh1", processGetSwitchCh1},
  { "getSwitchCh2", processGetSwitchCh2},
  { "getSwitchCh3", processGetSwitchCh3},
  { "getSwitchCh4", processGetSwitchCh4},
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

  relayControlBydtCycLoop.enable();
  relayControlByDateTimeLoop.enable();
  relayControlByIntrvlLoop.enable();
  relayControlByMultiTimeLoop.enable();
  publishSwitchLoop.enable();
  selfDiagnosticShortLoop.enable();
  selfDiagnosticLongLoop.enable();
}

void loop()
{
  udawa();
}

void syncClientAttrCb(){
  log_manager->debug(PSTR(__func__),PSTR("Executed.\n"));
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

  //log_manager->debug(PSTR(__func__), PSTR("Volt: %.2f Amp: %.2f Watt %.2f Ener: %.2f Freq. %.2f PF %.2f\n"),
  //  mySettings.volt, mySettings.amp, mySettings.watt, mySettings.ener, mySettings.freq, mySettings.pf);
}

void recPowerUsageCb(){
  if(tb.connected()){
    StaticJsonDocument<DOCSIZE_MIN> doc;
    doc["volt"] = _volt.Get_Average();
    doc["amp"] = _amp.Get_Average();
    doc["watt"] = _watt.Get_Average();
    doc["freq"] = _freq.Get_Average();
    doc["pf"] = _pf.Get_Average();

    HardwareSerial PZEMSerial(1);
    PZEM004Tv30 PZEM(PZEMSerial, S1_RX, S1_TX);
    float ener = PZEM.energy();
    if(mySettings.lastEner == -1){
      mySettings.lastEner = ener;
    }
    else{
      doc["ener"] = ener - mySettings.lastEner;
      mySettings.lastEner = ener;
    }

    tb.sendTelemetryDoc(doc);
    _volt.Clear(); _amp.Clear(); _watt.Clear(); _freq.Clear(); _pf.Clear();

    //log_manager->debug(PSTR(__func__), PSTR("Volt: %.2f Amp: %.2f Watt %.2f Ener: %.2f Freq. %.2f PF %.2f\n"),
    //mySettings.volt, mySettings._amp, mySettings._watt, mySettings._ener, mySettings._freq, mySettings._pf);
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
  }
}

void recWeatherDataCb(){
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

  if(doc["dtCyc"] != nullptr)
  {
    uint8_t index = 0;
    for(JsonVariant v : doc["dtCyc"].as<JsonArray>())
    {
        mySettings.dtCyc[index] = v.as<uint8_t>();
        index++;
    }
  }
  else
  {
    for(uint8_t i = 0; i < countof(mySettings.dtCyc); i++)
    {
        mySettings.dtCyc[i] = 0;
    }
  }

  if(doc["dtRng"] != nullptr)
  {
    uint8_t index = 0;
    for(JsonVariant v : doc["dtRng"].as<JsonArray>())
    {
        mySettings.dtRng[index] = v.as<unsigned long>();
        index++;
    }
  }
  else
  {
    for(uint8_t i = 0; i < countof(mySettings.dtRng); i++)
    {
        mySettings.dtRng[i] = 0;
    }
  }

  if(doc["dtCycFS"] != nullptr)
  {
    uint8_t index = 0;
    for(JsonVariant v : doc["dtCycFS"].as<JsonArray>())
    {
        mySettings.dtCycFS[index] = v.as<uint8_t>();
        index++;
    }
  }
  else
  {
    for(uint8_t i = 0; i < countof(mySettings.dtCycFS); i++)
    {
        mySettings.dtCycFS[i] = 0;
    }
  }

  if(doc["dtRngFS"] != nullptr)
  {
    uint8_t index = 0;
    for(JsonVariant v : doc["dtRngFS"].as<JsonArray>())
    {
        mySettings.dtRngFS[index] = v.as<unsigned long>();
        index++;
    }
  }
  else
  {
    for(uint8_t i = 0; i < countof(mySettings.dtRngFS); i++)
    {
        mySettings.dtRngFS[i] = 0;
    }
  }

  if(doc["rlyActDT"] != nullptr)
  {
    uint8_t index = 0;
    for(JsonVariant v : doc["rlyActDT"].as<JsonArray>())
    {
        mySettings.rlyActDT[index] = v.as<unsigned long>();
        index++;
    }
  }
  else
  {
    for(uint8_t i = 0; i < countof(mySettings.rlyActDT); i++)
    {
        mySettings.rlyActDT[i] = 0;
    }
  }

  if(doc["rlyActDr"] != nullptr)
  {
    uint8_t index = 0;
    for(JsonVariant v : doc["rlyActDr"].as<JsonArray>())
    {
        mySettings.rlyActDr[index] = v.as<unsigned long>();
        index++;
    }
  }
  else
  {
    for(uint8_t i = 0; i < countof(mySettings.rlyActDr); i++)
    {
        mySettings.rlyActDr[i] = 0;
    }
  }

if(doc["rlyActIT"] != nullptr)
  {
    uint8_t index = 0;
    for(JsonVariant v : doc["rlyActIT"].as<JsonArray>())
    {
        mySettings.rlyActIT[index] = v.as<unsigned long>();
        index++;
    }
  }
  else
  {
    for(uint8_t i = 0; i < countof(mySettings.rlyActIT); i++)
    {
        mySettings.rlyActIT[i] = 0;
    }
  }

  if(doc["rlyActITOn"] != nullptr)
  {
    uint8_t index = 0;
    for(JsonVariant v : doc["rlyActITOn"].as<JsonArray>())
    {
        mySettings.rlyActITOn[index] = v.as<unsigned long>();
        index++;
    }
  }
  else
  {
    for(uint8_t i = 0; i < countof(mySettings.rlyActITOn); i++)
    {
        mySettings.rlyActITOn[i] = 0;
    }
  }

  if(doc["pin"] != nullptr)
  {
    uint8_t index = 0;
    for(JsonVariant v : doc["pin"].as<JsonArray>())
    {
        mySettings.pin[index] = v.as<uint8_t>();
        index++;
    }
  }
  else
  {
    for(uint8_t i = 0; i < countof(mySettings.pin); i++)
    {
        mySettings.pin[i] = 0;
    }
  }

  if(doc["ON"] != nullptr)
  {
    mySettings.ON = doc["ON"].as<uint8_t>();
  }
  else
  {
    mySettings.ON = 1;
  }

  if(doc["intvRecPwrUsg"] != nullptr){mySettings.intvRecPwrUsg = doc["intvRecPwrUsg"].as<uint16_t>();}
  else{mySettings.intvRecPwrUsg = 900;}

  if(doc["intvRecWthr"] != nullptr){mySettings.intvRecWthr = doc["intvRecWthr"].as<uint16_t>();}
  else{mySettings.intvRecWthr = 300;}

  if(doc["intvDevTel"] != nullptr){mySettings.intvDevTel = doc["intvDevTel"].as<uint16_t>();}
  else{mySettings.intvDevTel = 60;}

  if(doc["rlyCtrlMd"] != nullptr)
  {
    uint8_t index = 0;
    for(JsonVariant v : doc["rlyCtrlMd"].as<JsonArray>())
    {
        mySettings.rlyCtrlMd[index] = v.as<uint8_t>();
        index++;
    }
  }
  else
  {
    for(uint8_t i = 0; i < countof(mySettings.rlyCtrlMd); i++)
    {
        mySettings.rlyCtrlMd[i] = 0;
    }
  }

  for(uint8_t i = 0; i < countof(mySettings.dutyCounter); i++)
  {
    mySettings.dutyCounter[i] = 86400;
  }

  if(doc["seaHpa"] != nullptr){mySettings.seaHpa = doc["seaHpa"].as<float>();}
  else{mySettings.seaHpa = 1019.00;}

  if(doc["rlyActMT"] != nullptr)
  {
    uint8_t index = 0;
    for(JsonVariant v : doc["rlyActMT"].as<JsonArray>())
    {
        mySettings.rlyActMT[index] = v.as<String>();
        index++;
    }
  }
  else
  {
    for(uint8_t i = 0; i < countof(mySettings.rlyActMT); i++)
    {
        mySettings.rlyActMT[i] = "[{}]";
    }
  }

  if(doc["label"] != nullptr)
  {
    uint8_t index = 0;
    for(JsonVariant v : doc["label"].as<JsonArray>())
    {
        strlcpy(mySettings.label[index], v.as<const char*>(), sizeof(mySettings.label[index]));
        index++;
    }
  }
  else
  {
    for(uint8_t i = 0; i < countof(mySettings.label); i++)
    {
        strlcpy(mySettings.label[i], "unnamed", sizeof(mySettings.label[i]));
    }
  }

  String tmp;
  if(config.logLev >= 4){serializeJsonPretty(doc, tmp);}
  log_manager->debug(PSTR(__func__), "Loaded settings:\n %s \n", tmp.c_str());
}

void saveSettings()
{
  StaticJsonDocument<DOCSIZE> doc;

  JsonArray dtCyc = doc.createNestedArray("dtCyc");
  for(uint8_t i=0; i<countof(mySettings.dtCyc); i++)
  {
    dtCyc.add(mySettings.dtCyc[i]);
  }

  JsonArray dtRng = doc.createNestedArray("dtRng");
  for(uint8_t i=0; i<countof(mySettings.dtRng); i++)
  {
    dtRng.add(mySettings.dtRng[i]);
  }

  JsonArray dtCycFS = doc.createNestedArray("dtCycFS");
  for(uint8_t i=0; i<countof(mySettings.dtCycFS); i++)
  {
    dtCycFS.add(mySettings.dtCycFS[i]);
  }

  JsonArray dtRngFS = doc.createNestedArray("dtRngFS");
  for(uint8_t i=0; i<countof(mySettings.dtRngFS); i++)
  {
    dtRngFS.add(mySettings.dtRngFS[i]);
  }

  JsonArray rlyActDT = doc.createNestedArray("rlyActDT");
  for(uint8_t i=0; i<countof(mySettings.rlyActDT); i++)
  {
    rlyActDT.add(mySettings.rlyActDT[i]);
  }

  JsonArray rlyActDr = doc.createNestedArray("rlyActDr");
  for(uint8_t i=0; i<countof(mySettings.rlyActDr); i++)
  {
    rlyActDr.add(mySettings.rlyActDr[i]);
  }

  JsonArray rlyActIT = doc.createNestedArray("rlyActIT");
  for(uint8_t i=0; i<countof(mySettings.rlyActIT); i++)
  {
    rlyActIT.add(mySettings.rlyActIT[i]);
  }

  JsonArray rlyActITOn = doc.createNestedArray("rlyActITOn");
  for(uint8_t i=0; i<countof(mySettings.rlyActITOn); i++)
  {
    rlyActITOn.add(mySettings.rlyActITOn[i]);
  }

  JsonArray pin = doc.createNestedArray("pin");
  for(uint8_t i=0; i<countof(mySettings.pin); i++)
  {
    pin.add(mySettings.pin[i]);
  }

  JsonArray rlyActMT = doc.createNestedArray("rlyActMT");
  for(uint8_t i=0; i<countof(mySettings.rlyActMT); i++)
  {
    rlyActMT.add(mySettings.rlyActMT[i]);
  }

  JsonArray label = doc.createNestedArray("label");
  for(uint8_t i=0; i<countof(mySettings.label); i++)
  {
    label.add(mySettings.label[i]);
  }

  doc["ON"] = mySettings.ON;
  doc["intvRecPwrUsg"] = mySettings.intvRecPwrUsg;
  doc["intvRecWthr"] = mySettings.intvRecWthr;
  doc["intvDevTel"] = mySettings.intvDevTel;

  JsonArray rlyCtrlMd = doc.createNestedArray("rlyCtrlMd");
  for(uint8_t i=0; i<countof(mySettings.rlyCtrlMd); i++)
  {
    rlyCtrlMd.add(mySettings.rlyCtrlMd[i]);
  }

  doc["seaHpa"] = mySettings.seaHpa;

  writeSettings(doc, settingsPath);
  String tmp;
  if(config.logLev >= 4){serializeJsonPretty(doc, tmp);}
  log_manager->debug(PSTR(__func__), "Written settings:\n %s \n", tmp.c_str());
}

JsonObject processOnUpdateFinished(const JsonObject &data){
  saveSettings();
  return data;
}

JsonObject processWsEvent(const JsonObject &doc){
  if(doc["evType"] == nullptr){
    log_manager->debug(PSTR(__func__), "Event type not found.\n");
    return doc;
  }
  int evType = doc["evType"].as<int>();


  if(evType == (int)WS_EVT_CONNECT){
    syncClientAttr.setInterval(TASK_IMMEDIATE);
    syncClientAttr.setIterations(TASK_ONCE);
    syncClientAttr.enable();

    wsSendTelemetryLoop.enableIfNot();
    wsSendSensorsLoop.enableIfNot();

  }
  if(evType == (int)WS_EVT_DISCONNECT){
    if(ws.count() < 1){
      wsSendTelemetryLoop.disable();
      wsSendSensorsLoop.disable();
      log_manager->debug(PSTR(__func__),PSTR("No WS client is active. \n"));
    }
  }
  else if(evType == (int)WS_EVT_DATA){
    if(doc["cmd"] == nullptr){
      log_manager->debug(PSTR(__func__), "Command not found.\n");
      return doc;
    }
    const char* cmd = doc["cmd"].as<const char*>();
    if(strcmp(cmd, (const char*) "attr") == 0){
      processSharedAttributesUpdate(doc);
      if(tb.connected()){
        doc.remove("cmd");
        doc.remove("evType");
        StaticJsonDocument<DOCSIZE_MIN> tele;
        tele = doc;
        tb.sendAttributeDoc(tele);
      }
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

JsonObject processOnTbConnected(const JsonObject &data){
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
  publishDeviceTelemetryLoop.enable();
  recPowerUsageLoop.enable();
  recWeatherDataLoop.enable();
  return data;
}

JsonObject processOnTbDisconnected(const JsonObject &data){
  publishDeviceTelemetryLoop.disable();
  recPowerUsageLoop.disable();
  recWeatherDataLoop.disable();
  return data;
}

JsonObject processEmitAlarmWs(const JsonObject &data){
  if(ws.count() < 1){
    return data;
  }
  StaticJsonDocument<DOCSIZE_MIN> doc;
  doc = data;
  wsSend(doc);
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
    doc["fPanic"] = 1;
    configcomcu.fPanic = 1;

    mySettings.rlyCtrlMd[0] = 0;
    mySettings.rlyCtrlMd[1] = 0;
    mySettings.rlyCtrlMd[2] = 0;
    mySettings.rlyCtrlMd[3] = 0;
    setSwitch("ch1", "OFF");
    setSwitch("ch2", "OFF");
    setSwitch("ch3", "OFF");
    setSwitch("ch4", "OFF");
  }
  else{
    doc["fPanic"] = 0;
    configcomcu.fPanic = 0;
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
  uint8_t pin = 0;
  uint8_t id = 0;

  if(ch == String("ch1")){id = 0; pin = mySettings.pin[0]; mySettings.dutyState[0] = (state == String("ON")) ? mySettings.ON : 1 - mySettings.ON; mySettings.publishSwitch[0] = true;}
  else if(ch == String("ch2")){id = 1; pin = mySettings.pin[1]; mySettings.dutyState[1] = (state == String("ON")) ? mySettings.ON : 1 - mySettings.ON; mySettings.publishSwitch[1] = true;}
  else if(ch == String("ch3")){id = 2; pin = mySettings.pin[2]; mySettings.dutyState[2] = (state == String("ON")) ? mySettings.ON : 1 - mySettings.ON; mySettings.publishSwitch[2] = true;}
  else if(ch == String("ch4")){id = 3; pin = mySettings.pin[3]; mySettings.dutyState[3] = (state == String("ON")) ? mySettings.ON : 1 - mySettings.ON; mySettings.publishSwitch[3] = true;}

  if(state == String("ON"))
  {
    fState = mySettings.ON;
    mySettings.stateOnTs[id] = millis();
  }
  else
  {
    fState = 1 - mySettings.ON;
    mySettings.stateOnTs[id] = 0;
  }

  setCoMCUPin(pin, 1, OUTPUT, 0, fState);
  log_manager->warn(PSTR(__func__), "Relay %s was set to %s / %d.\n", ch, state, (int)fState);
}

void relayControlByDateTimeCb(){
  for(uint8_t i = 0; i < countof(mySettings.pin); i++)
  {
    if(mySettings.rlyActDr[i] > 0 && mySettings.rlyCtrlMd[i] == 2){
      if(mySettings.rlyActDT[i] <= (rtc.getEpoch()) && (mySettings.rlyActDr[i]) >=
        (rtc.getEpoch() - mySettings.rlyActDT[i]) && mySettings.dutyState[i] != mySettings.ON){
          mySettings.dutyState[i] = mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "ON");
          log_manager->debug(PSTR(__func__),PSTR("Relay Ch%d changed to %d - ts:%d - tr:%d - exp:%d\n"), i+1,
            mySettings.dutyState[i], mySettings.rlyActDT[i], mySettings.rlyActDr[i],
            mySettings.rlyActDr[i] - (rtc.getEpoch() - mySettings.rlyActDT[i]));
      }
      else if(mySettings.dutyState[i] == mySettings.ON && (mySettings.rlyActDr[i]) <=
        (rtc.getEpoch() - mySettings.rlyActDT[i])){
          mySettings.dutyState[i] = 1 - mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "OFF");
          log_manager->debug(PSTR(__func__),PSTR("Relay Ch%d changed to %d - ts:%d - tr:%d - exp:%d\n"), i+1,
            mySettings.dutyState[i], mySettings.rlyActDT[i], mySettings.rlyActDr[i],
            mySettings.rlyActDr[i] - (rtc.getEpoch() - mySettings.rlyActDT[i]));
      }
    }
    else if(mySettings.rlyActDr[i] > 0 && mySettings.rlyCtrlMd[i] == 3){
      int currHour = rtc.getHour(true);
      int currMinute = rtc.getMinute();
      int currSecond = rtc.getSecond();
      String currDT = rtc.getDateTime();
      int currentTimeInSec = (currHour * 60 * 60) + (currMinute * 60) + currSecond;

      long rlyActDT = mySettings.rlyActDT[i] + config.gmtOffset;
      int targetHour = hour(rlyActDT);
      int targetMinute = minute(rlyActDT);
      int targetSecond = second(rlyActDT);
      char targetDT[32];
      sprintf(targetDT, "%02d.%02d.%02d %02d:%02d:%02d", day(rlyActDT), month(rlyActDT),
        year(rlyActDT), hour(rlyActDT), minute(rlyActDT), second(rlyActDT));
      int targetTimeInSec = (targetHour * 60 * 60) + (targetMinute * 60) + targetSecond;

      if(targetTimeInSec <= currentTimeInSec && (mySettings.rlyActDr[i]) >=
        (currentTimeInSec - targetTimeInSec) && mySettings.dutyState[i] != mySettings.ON){
          mySettings.dutyState[i] = mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "ON");
          log_manager->debug(PSTR(__func__),PSTR("currentTimeInSec:%d (%d:%d:%d - %s - %d) targetTimeInSec:%d (%d:%d:%d - %s - %d) - rlyActDr:%d - exp:%d\n"), i+1,
            mySettings.dutyState[i], currentTimeInSec, currHour, currMinute, currSecond, currDT.c_str(), rtc.getEpoch(),
            targetTimeInSec, targetHour, targetMinute, targetSecond, targetDT, rlyActDT,
            mySettings.rlyActDr[i], mySettings.rlyActDr[i] - (currentTimeInSec - targetTimeInSec));
      }
      else if(mySettings.dutyState[i] == mySettings.ON && (mySettings.rlyActDr[i]) <=
        (currentTimeInSec - targetTimeInSec)){
          mySettings.dutyState[i] = 1 - mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "OFF");
          log_manager->debug(PSTR(__func__),PSTR("currentTimeInSec:%d (%d:%d:%d - %s - %d) targetTimeInSec:%d (%d:%d:%d - %s - %d) - rlyActDr:%d - exp:%d\n"), i+1,
            mySettings.dutyState[i], currentTimeInSec, currHour, currMinute, currSecond, currDT.c_str(), rtc.getEpoch(),
            targetTimeInSec, targetHour, targetMinute, targetSecond, targetDT, rlyActDT,
            mySettings.rlyActDr[i], mySettings.rlyActDr[i] - (currentTimeInSec - targetTimeInSec));
      }
    }
  }
}

void relayControlBydtCycCb()
{
  for(uint8_t i = 0; i < countof(mySettings.pin); i++)
  {
    if (mySettings.dtRng[i] < 2){mySettings.dtRng[i] = 2;} //safenet
    if(mySettings.dtCyc[i] != 0 && mySettings.rlyCtrlMd[i] == 1)
    {
      if( mySettings.dutyState[i] == mySettings.ON )
      {
        if( mySettings.dtCyc[i] != 100 && (millis() - mySettings.dutyCounter[i] ) >= (float)(( ((float)mySettings.dtCyc[i] / 100) * (float)mySettings.dtRng[i]) * 1000))
        {
          mySettings.dutyState[i] = 1 - mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "OFF");
          mySettings.dutyCounter[i] = millis();
          log_manager->debug(PSTR(__func__),PSTR("Relay Ch%d has changed to %d - dtCyc:%d - dtRng:%ld\n"), i+1, mySettings.dutyState[i], mySettings.dtCyc[i], mySettings.dtRng[i]);
        }
      }
      else
      {
        if( mySettings.dtCyc[i] != 0 && (millis() - mySettings.dutyCounter[i] ) >= (float) ( ((100 - (float) mySettings.dtCyc[i]) / 100) * (float) mySettings.dtRng[i]) * 1000)
        {
          mySettings.dutyState[i] = mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "ON");
          mySettings.dutyCounter[i] = millis();
          log_manager->debug(PSTR(__func__),PSTR("Relay Ch%d has changed to %d - dtCyc:%d - dtRng:%ld\n"), i+1, mySettings.dutyState[i], mySettings.dtCyc[i], mySettings.dtRng[i]);
        }
      }
    }
  }
}

void relayControlByIntrvlCb(){
  for(uint8_t i = 0; i < countof(mySettings.pin); i++)
  {
    if(mySettings.rlyActIT[i] != 0 && mySettings.rlyActITOn[i] != 0 && mySettings.rlyCtrlMd[i] == 4)
    {
      if( mySettings.dutyState[i] == 1 - mySettings.ON && (millis() - mySettings.rlyActITOnTs[i]) > (mySettings.rlyActIT[i] * 1000) ){
        mySettings.dutyState[i] = mySettings.ON;
        String ch = "ch" + String(i+1);
        setSwitch(ch, "ON");
        mySettings.rlyActITOnTs[i] = millis();
        log_manager->debug(PSTR(__func__), PSTR("Relay Ch%d has changed to %d - rlyActIT:%d - rlyActITOn:%d\n"), i+1, mySettings.dutyState[i],
          mySettings.rlyActIT[i], mySettings.rlyActITOn[i]);
      }
      if( mySettings.dutyState[i] == mySettings.ON && (millis() - mySettings.rlyActITOnTs[i]) > (mySettings.rlyActITOn[i] * 1000) ){
        mySettings.dutyState[i] = 1 - mySettings.ON;
        String ch = "ch" + String(i+1);
        setSwitch(ch, "OFF");
        log_manager->debug(PSTR(__func__), PSTR("Relay Ch%d has changed to %d - rlyActIT:%d - rlyActITOn:%d\n"), i+1, mySettings.dutyState[i],
          mySettings.rlyActIT[i], mySettings.rlyActITOn[i]);
      }
    }
  }
}

void relayControlByMultiTimeCb(){
  for(uint8_t i = 0; i < countof(mySettings.pin); i++){
    if(mySettings.rlyCtrlMd[i] == 6){
      StaticJsonDocument<DOCSIZE_MIN> doc;
      DeserializationError error = deserializeJson(doc, mySettings.rlyActMT[i]);
      if(error != DeserializationError::Ok){
        log_manager->warn(PSTR(__func__),PSTR("Failed to parse JSON for CH%d: %s\n"),i+1, mySettings.rlyActMT[i].c_str());
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

        int rlyActDT = v["d"].as<int>();
        int targetHour = v["h"].as<int>();
        int targetHourToSec = targetHour * 3600;
        int targetMinute = v["i"].as<int>();
        int targetMinuteToSec =targetMinute * 60;
        int targetSecond =v["s"].as<int>();
        int targetTimeInSec = targetHourToSec + targetMinuteToSec + targetSecond;

        int activationOffset = targetTimeInSec - currentTimeInSec;
        int deactivationOffset = activationOffset + rlyActDT;
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
        log_manager->info(PSTR(__func__), PSTR("%d ActiveTimeWindow found, turning ON switch: %s\n"), activeTimeWindowCounter, ch.c_str());
      }else if(mySettings.dutyState[i] == mySettings.ON && activeTimeWindowCounter < 1) {
        mySettings.dutyState[i] = 1 - mySettings.ON;
        String ch = "ch" + String(i+1);
        setSwitch(ch, "OFF");
        log_manager->info(PSTR(__func__), PSTR("No ActiveTimeWindow found, turning OFF switch: %s\n"), ch.c_str());
      }
    }
  }
}

callbackResponse processSharedAttributesUpdate(const callbackData &data)
{
  log_manager->debug(PSTR(__func__), PSTR("Received attributes update request:\n"));
  if(config.logLev >= 4){serializeJsonPretty(data, Serial);}
  Serial.println();

  if(data["model"] != nullptr){strlcpy(config.model, data["model"].as<const char*>(), sizeof(config.model));}
  if(data["group"] != nullptr){strlcpy(config.group, data["group"].as<const char*>(), sizeof(config.group));}
  if(data["broker"] != nullptr){strlcpy(config.broker, data["broker"].as<const char*>(), sizeof(config.broker));}
  if(data["port"] != nullptr){data["port"].as<uint16_t>();}
  if(data["wssid"] != nullptr){strlcpy(config.wssid, data["wssid"].as<const char*>(), sizeof(config.wssid));}
  if(data["wpass"] != nullptr){strlcpy(config.wpass, data["wpass"].as<const char*>(), sizeof(config.wpass));}
  if(data["dssid"] != nullptr){strlcpy(config.dssid, data["dssid"].as<const char*>(), sizeof(config.dssid));}
  if(data["dpass"] != nullptr){strlcpy(config.dpass, data["dpass"].as<const char*>(), sizeof(config.dpass));}
  if(data["upass"] != nullptr){strlcpy(config.upass, data["upass"].as<const char*>(), sizeof(config.upass));}
  if(data["accessToken"] != nullptr){strlcpy(config.accessToken, data["accessToken"].as<const char*>(), sizeof(config.accessToken));}
  if(data["provisionDeviceKey"] != nullptr){strlcpy(config.provisionDeviceKey, data["provisionDeviceKey"].as<const char*>(), sizeof(config.provisionDeviceKey));}
  if(data["provisionDeviceSecret"] != nullptr){strlcpy(config.provisionDeviceSecret, data["provisionDeviceSecret"].as<const char*>(), sizeof(config.provisionDeviceSecret));}
  if(data["logLev"] != nullptr){config.logLev = data["logLev"].as<uint8_t>(); log_manager->set_log_level(PSTR("*"), (LogLevel) config.logLev);;}
  if(data["gmtOffset"] != nullptr){config.gmtOffset = data["gmtOffset"].as<int>();}
  if(data["useCloud"] != nullptr){config.useCloud = data["useCloud"].as<int>();}
  if(data["httpUname"] != nullptr){strlcpy(config.httpUname, data["httpUname"].as<const char*>(), sizeof(config.httpUname));}
  if(data["httpPass"] != nullptr){strlcpy(config.httpPass, data["httpPass"].as<const char*>(), sizeof(config.httpPass));}
  if(data["useWiFiOta"] != nullptr){config.useWiFiOta = data["useWiFiOta"].as<bool>();}
  if(data["useWebIface"] != nullptr){config.useWebIface = data["useWebIface"].as<bool>();}
  if(data["hostname"] != nullptr){strlcpy(config.hostname, data["hostname"].as<const char*>(), sizeof(config.hostname));}


  if(data["dtCycCh1"] != nullptr)
  {
    mySettings.dtCyc[0] = data["dtCycCh1"].as<uint8_t>();
    if(data["dtCycCh1"].as<uint8_t>() == 0)
    {
      setSwitch(String("ch1"), String("OFF"));
    }
  }
  if(data["dtCycCh2"] != nullptr)
  {
    mySettings.dtCyc[1] = data["dtCycCh2"].as<uint8_t>();
    if(data["dtCycCh2"].as<uint8_t>() == 0)
    {
      setSwitch(String("ch2"), String("OFF"));
    }
  }
  if(data["dtCycCh3"] != nullptr)
  {
    mySettings.dtCyc[2] = data["dtCycCh3"].as<uint8_t>();
    if(data["dtCycCh3"].as<uint8_t>() == 0)
    {
      setSwitch(String("ch3"), String("OFF"));
    }
  }
  if(data["dtCycCh4"] != nullptr)
  {
    mySettings.dtCyc[3] = data["dtCycCh4"].as<uint8_t>();
    if(data["dtCycCh4"].as<uint8_t>() == 0)
    {
      setSwitch(String("ch4"), String("OFF"));
    }
  }

  if(data["dtRngCh1"] != nullptr){mySettings.dtRng[0] = data["dtRngCh1"].as<unsigned long>();}
  if(data["dtRngCh2"] != nullptr){mySettings.dtRng[1] = data["dtRngCh2"].as<unsigned long>();}
  if(data["dtRngCh3"] != nullptr){mySettings.dtRng[2] = data["dtRngCh3"].as<unsigned long>();}
  if(data["dtRngCh4"] != nullptr){mySettings.dtRng[3] = data["dtRngCh4"].as<unsigned long>();}

  if(data["dtCycFSCh1"] != nullptr){mySettings.dtCycFS[0] = data["dtCycFSCh1"].as<uint8_t>();}
  if(data["dtCycFSCh2"] != nullptr){mySettings.dtCycFS[1] = data["dtCycFSCh2"].as<uint8_t>();}
  if(data["dtCycFSCh3"] != nullptr){mySettings.dtCycFS[2] = data["dtCycFSCh3"].as<uint8_t>();}
  if(data["dtCycFSCh4"] != nullptr){mySettings.dtCycFS[3] = data["dtCycFSCh4"].as<uint8_t>();}

  if(data["dtRngFSCh1"] != nullptr){mySettings.dtRngFS[0] = data["dtRngFSCh1"].as<unsigned long>();}
  if(data["dtRngFSCh2"] != nullptr){mySettings.dtRngFS[1] = data["dtRngFSCh2"].as<unsigned long>();}
  if(data["dtRngFSCh3"] != nullptr){mySettings.dtRngFS[2] = data["dtRngFSCh3"].as<unsigned long>();}
  if(data["dtRngFSCh4"] != nullptr){mySettings.dtRngFS[3] = data["dtRngFSCh4"].as<unsigned long>();}

  if(data["rlyActMTCh1"] != nullptr){mySettings.rlyActMT[0] = data["rlyActMTCh1"].as<String>();}
  if(data["rlyActMTCh2"] != nullptr){mySettings.rlyActMT[1] = data["rlyActMTCh2"].as<String>();}
  if(data["rlyActMTCh3"] != nullptr){mySettings.rlyActMT[2] = data["rlyActMTCh3"].as<String>();}
  if(data["rlyActMTCh4"] != nullptr){mySettings.rlyActMT[3] = data["rlyActMTCh4"].as<String>();}

  if(data["rlyActDTCh1"] != nullptr){
    uint64_t micro = data["rlyActDTCh1"].as<uint64_t>();
    uint32_t micro_high = micro >> 32;
    uint32_t micro_low = micro & MAX_INT;
    mySettings.rlyActDT[0] = micro2milli(micro_high, micro_low);
  }
  if(data["rlyActDTCh2"] != nullptr){
    uint64_t micro = data["rlyActDTCh2"].as<uint64_t>();
    uint32_t micro_high = micro >> 32;
    uint32_t micro_low = micro & MAX_INT;
    mySettings.rlyActDT[1] = micro2milli(micro_high, micro_low);
  }
  if(data["rlyActDTCh3"] != nullptr){
    uint64_t micro = data["rlyActDTCh3"].as<uint64_t>();
    uint32_t micro_high = micro >> 32;
    uint32_t micro_low = micro & MAX_INT;
    mySettings.rlyActDT[2] = micro2milli(micro_high, micro_low);
  }
  if(data["rlyActDTCh4"] != nullptr){
    uint64_t micro = data["rlyActDTCh4"].as<uint64_t>();
    uint32_t micro_high = micro >> 32;
    uint32_t micro_low = micro & MAX_INT;
    mySettings.rlyActDT[3] = micro2milli(micro_high, micro_low);
  }

  if(data["rlyActDrCh1"] != nullptr){mySettings.rlyActDr[0] = data["rlyActDrCh1"].as<unsigned long>();}
  if(data["rlyActDrCh2"] != nullptr){mySettings.rlyActDr[1] = data["rlyActDrCh2"].as<unsigned long>();}
  if(data["rlyActDrCh3"] != nullptr){mySettings.rlyActDr[2] = data["rlyActDrCh3"].as<unsigned long>();}
  if(data["rlyActDrCh4"] != nullptr){mySettings.rlyActDr[3] = data["rlyActDrCh4"].as<unsigned long>();}

  if(data["rlyActITCh1"] != nullptr){mySettings.rlyActIT[0] = data["rlyActITCh1"].as<unsigned long>();}
  if(data["rlyActITCh2"] != nullptr){mySettings.rlyActIT[1] = data["rlyActITCh2"].as<unsigned long>();}
  if(data["rlyActITCh3"] != nullptr){mySettings.rlyActIT[2] = data["rlyActITCh3"].as<unsigned long>();}
  if(data["rlyActITCh4"] != nullptr){mySettings.rlyActIT[3] = data["rlyActITCh4"].as<unsigned long>();}

  if(data["rlyActITOnCh1"] != nullptr){mySettings.rlyActITOn[0] = data["rlyActITOnCh1"].as<unsigned long>();}
  if(data["rlyActITOnCh2"] != nullptr){mySettings.rlyActITOn[1] = data["rlyActITOnCh2"].as<unsigned long>();}
  if(data["rlyActITOnCh3"] != nullptr){mySettings.rlyActITOn[2] = data["rlyActITOnCh3"].as<unsigned long>();}
  if(data["rlyActITOnCh4"] != nullptr){mySettings.rlyActITOn[3] = data["rlyActITOnCh4"].as<unsigned long>();}

  if(data["pinCh1"] != nullptr){mySettings.pin[0] = data["pinCh1"].as<uint8_t>();}
  if(data["pinCh2"] != nullptr){mySettings.pin[1] = data["pinCh2"].as<uint8_t>();}
  if(data["pinCh3"] != nullptr){mySettings.pin[2] = data["pinCh3"].as<uint8_t>();}
  if(data["pinCh4"] != nullptr){mySettings.pin[3] = data["pinCh4"].as<uint8_t>();}

  if(data["ON"] != nullptr){mySettings.ON = data["ON"].as<bool>();}
  if(data["intvRecPwrUsg"] != nullptr){mySettings.intvRecPwrUsg = data["intvRecPwrUsg"].as<uint16_t>();}
  if(data["intvRecWthr"] != nullptr){mySettings.intvRecWthr = data["intvRecWthr"].as<uint16_t>();}
  if(data["intvDevTel"] != nullptr){mySettings.intvDevTel = data["intvDevTel"].as<uint16_t>();}

  if(data["rlyCtrlMdCh1"] != nullptr){mySettings.rlyCtrlMd[0] = data["rlyCtrlMdCh1"].as<uint8_t>();}
  if(data["rlyCtrlMdCh2"] != nullptr){mySettings.rlyCtrlMd[1] = data["rlyCtrlMdCh2"].as<uint8_t>();}
  if(data["rlyCtrlMdCh3"] != nullptr){mySettings.rlyCtrlMd[2] = data["rlyCtrlMdCh3"].as<uint8_t>();}
  if(data["rlyCtrlMdCh4"] != nullptr){mySettings.rlyCtrlMd[3] = data["rlyCtrlMdCh4"].as<uint8_t>();}

  if(data["labelCh1"] != nullptr){strlcpy(mySettings.label[0], data["labelCh1"].as<const char*>(), sizeof(mySettings.label[0]));}
  if(data["labelCh2"] != nullptr){strlcpy(mySettings.label[1], data["labelCh2"].as<const char*>(), sizeof(mySettings.label[1]));}
  if(data["labelCh3"] != nullptr){strlcpy(mySettings.label[2], data["labelCh3"].as<const char*>(), sizeof(mySettings.label[2]));}
  if(data["labelCh4"] != nullptr){strlcpy(mySettings.label[3], data["labelCh4"].as<const char*>(), sizeof(mySettings.label[3]));}

  if(data["seaHpa"] != nullptr){mySettings.seaHpa = data["seaHpa"].as<float>();}

  if(data["fPanic"] != nullptr){configcomcu.fPanic = data["fPanic"].as<bool>();}
  if(data["bfreq"] != nullptr){configcomcu.bfreq = data["bfreq"].as<uint16_t>();}
  if(data["fBuzz"] != nullptr){configcomcu.fBuzz = data["fBuzz"].as<bool>();}
  if(data["pinBuzzer"] != nullptr){configcomcu.pinBuzzer = data["pinBuzzer"].as<uint8_t>();}
  if(data["pinLedR"] != nullptr){configcomcu.pinLedR = data["pinLedR"].as<uint8_t>();}
  if(data["pinLedG"] != nullptr){configcomcu.pinLedG = data["pinLedG"].as<uint8_t>();}
  if(data["pinLedB"] != nullptr){configcomcu.pinLedB = data["pinLedB"].as<uint8_t>();}
  if(data["ledON"] != nullptr){configcomcu.ledON = data["ledON"].as<uint8_t>();}


  if(data["cmd"] == nullptr){
    wsSendAttributes();
  }
  mySettings.lastUpdated = millis();
  return callbackResponse("sharedAttributesUpdate", 1);
}

void syncClientAttributes()
{
  StaticJsonDocument<DOCSIZE_MIN> doc;

  IPAddress ip = WiFi.localIP();
  char ipa[25];
  sprintf(ipa, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  doc["ipad"] = ipa;
  doc["compdate"] = COMPILED;
  doc["fmTitle"] = CURRENT_FIRMWARE_TITLE;
  doc["fmVersion"] = CURRENT_FIRMWARE_VERSION;
  doc["stamac"] = WiFi.macAddress();
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["apmac"] = WiFi.softAPmacAddress();
  doc["flashFree"] = ESP.getFreeSketchSpace();
  doc["firmwareSize"] = ESP.getSketchSize();
  doc["flashSize"] = ESP.getFlashChipSize();
  doc["sdkVer"] = ESP.getSdkVersion();
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["model"] = config.model;
  doc["group"] = config.group;
  doc["broker"] = config.broker;
  doc["port"] = config.port;
  doc["wssid"] = config.wssid;
  doc["ap"] = WiFi.SSID();
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["wpass"] = config.wpass;
  doc["dssid"] = config.dssid;
  doc["dpass"] = config.dpass;
  doc["upass"] = config.upass;
  doc["accessToken"] = config.accessToken;
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["provisionDeviceKey"] = config.provisionDeviceKey;
  doc["provisionDeviceSecret"] = config.provisionDeviceSecret;
  doc["logLev"] = config.logLev;
  doc["gmtOffset"] = config.gmtOffset;
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["useCloud"] = (int)config.useCloud;
  doc["dtCycCh1"] = mySettings.dtCyc[0];
  doc["dtCycCh2"] = mySettings.dtCyc[1];
  doc["dtCycCh3"] = mySettings.dtCyc[2];
  doc["dtCycCh4"] = mySettings.dtCyc[3];
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["dtRngCh1"] = mySettings.dtRng[0];
  doc["dtRngCh2"] = mySettings.dtRng[1];
  doc["dtRngCh3"] = mySettings.dtRng[2];
  doc["dtRngCh4"] = mySettings.dtRng[3];
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["dtCycFSCh1"] = mySettings.dtCycFS[0];
  doc["dtCycFSCh2"] = mySettings.dtCycFS[1];
  doc["dtCycFSCh3"] = mySettings.dtCycFS[2];
  doc["dtCycFSCh4"] = mySettings.dtCycFS[3];
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["dtRngFSCh1"] = mySettings.dtRngFS[0];
  doc["dtRngFSCh2"] = mySettings.dtRngFS[1];
  doc["dtRngFSCh3"] = mySettings.dtRngFS[2];
  doc["dtRngFSCh4"] = mySettings.dtRngFS[3];
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["rlyActDTCh1"] = (uint64_t)mySettings.rlyActDT[0] * 1000;
  doc["rlyActDTCh2"] = (uint64_t)mySettings.rlyActDT[1] * 1000;
  doc["rlyActDTCh3"] = (uint64_t)mySettings.rlyActDT[2] * 1000;
  doc["rlyActDTCh4"] = (uint64_t)mySettings.rlyActDT[3] * 1000;
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["rlyActDrCh1"] = mySettings.rlyActDr[0];
  doc["rlyActDrCh2"] = mySettings.rlyActDr[1];
  doc["rlyActDrCh3"] = mySettings.rlyActDr[2];
  doc["rlyActDrCh4"] = mySettings.rlyActDr[3];
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["rlyActITCh1"] = mySettings.rlyActIT[0];
  doc["rlyActITCh2"] = mySettings.rlyActIT[1];
  doc["rlyActITCh3"] = mySettings.rlyActIT[2];
  doc["rlyActITCh4"] = mySettings.rlyActIT[3];
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["rlyActITOnCh1"] = mySettings.rlyActITOn[0];
  doc["rlyActITOnCh2"] = mySettings.rlyActITOn[1];
  doc["rlyActITOnCh3"] = mySettings.rlyActITOn[2];
  doc["rlyActITOnCh4"] = mySettings.rlyActITOn[3];
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["pinCh1"] = mySettings.pin[0];
  doc["pinCh2"] = mySettings.pin[1];
  doc["pinCh3"] = mySettings.pin[2];
  doc["pinCh4"] = mySettings.pin[3];
  doc["ON"] = mySettings.ON;
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["rlyActMTCh1"] = mySettings.rlyActMT[0];
  doc["rlyActMTCh2"] = mySettings.rlyActMT[1];
  doc["rlyActMTCh3"] = mySettings.rlyActMT[2];
  doc["rlyActMTCh4"] = mySettings.rlyActMT[3];
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["labelCh1"] = mySettings.label[0];
  doc["labelCh2"] = mySettings.label[1];
  doc["labelCh3"] = mySettings.label[2];
  doc["labelCh4"] = mySettings.label[3];
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["intvRecPwrUsg"] = mySettings.intvRecPwrUsg;
  doc["intvRecWthr"] = mySettings.intvRecWthr;
  doc["intvDevTel"] = mySettings.intvDevTel;
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["rlyCtrlMdCh1"] = mySettings.rlyCtrlMd[0];
  doc["rlyCtrlMdCh2"] = mySettings.rlyCtrlMd[1];
  doc["rlyCtrlMdCh3"] = mySettings.rlyCtrlMd[2];
  doc["rlyCtrlMdCh4"] = mySettings.rlyCtrlMd[3];
  doc["seaHpa"] = mySettings.seaHpa;
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["pinBuzzer"] = configcomcu.pinBuzzer;
  doc["pinLedR"] = configcomcu.pinLedR;
  doc["pinLedG"] = configcomcu.pinLedG;
  doc["pinLedB"] = configcomcu.pinLedB;
  doc["httpUname"] = config.httpUname;
  doc["httpPass"] = config.httpPass;
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["ledON"] = configcomcu.ledON;
  doc["bfreq"] = configcomcu.bfreq;
  doc["fPanic"] = configcomcu.fPanic;
  doc["fBuzz"] = configcomcu.fBuzz;
  doc["bfreq"] = configcomcu.bfreq;
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["useWiFiOta"] = (int)config.useWiFiOta;
  doc["useWebIface"] = (int)config.useWebIface;
  doc["hostname"] = config.hostname;
  tb.sendAttributeDoc(doc);
  doc.clear();
}

void publishDeviceTelemetryCb()
{
  if(tb.connected()){
    StaticJsonDocument<DOCSIZE_MIN> doc;

    doc["heap"] = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = millis()/1000;
    doc["dt"] = rtc.getEpoch();
    doc["dts"] = rtc.getDateTime();
    tb.sendTelemetryDoc(doc);
    doc.clear();
  }
}

void publishSwitchCb(){
  for (uint8_t i = 0; i < 4; i++){
    if(mySettings.publishSwitch[i]){
      StaticJsonDocument<DOCSIZE_MIN> doc;
      String chName = "ch" + String(i+1);
      doc[chName.c_str()] = (int)mySettings.dutyState[i] == mySettings.ON ? 1 : 0;
      if(tb.connected()){ tb.sendTelemetryDoc(doc); }
      if(ws.count() > 0){ wsSend(doc); }
      mySettings.publishSwitch[i] = false;
    }
  }
}

void wsSend(StaticJsonDocument<DOCSIZE_MIN> &doc){
  String buffer;
  serializeJson(doc, buffer);
  ws.textAll(buffer.c_str());
}
void wsSend(StaticJsonDocument<DOCSIZE_MIN> &doc, AsyncWebSocketClient * client){
  String buffer;
  serializeJson(doc, buffer);
  ws.text(client->id(), buffer.c_str());
}

bool wsSendEnable(){
  if(config.useWebIface && (ws.count() > 0)){
    return true;
  }
  else{
    return false;
  }
}

void wsSendTelemetryCb(){
  if(ws.count() > 0){
    StaticJsonDocument<DOCSIZE_MIN> doc;
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
  if(ws.count() > 0){
    HardwareSerial PZEMSerial(1);
    PZEM004Tv30 PZEM(PZEMSerial, S1_RX, S1_TX);

    StaticJsonDocument<DOCSIZE_MIN> doc;
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
  StaticJsonDocument<DOCSIZE_MIN> doc;

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
  attr["flashFree"] = ESP.getFreeSketchSpace();
  attr["firmwareSize"] = ESP.getSketchSize();
  attr["flashSize"] = ESP.getFlashChipSize();
  attr["sdkVer"] = ESP.getSdkVersion();
  wsSend(doc);
  doc.clear();
  JsonObject cfg = doc.createNestedObject("cfg");
  cfg["model"] = config.model;
  cfg["group"] = config.group;
  cfg["broker"] = config.broker;
  cfg["port"] = config.port;
  cfg["wssid"] = config.wssid;
  cfg["ap"] = WiFi.SSID();
  cfg["wpass"] = config.wpass;
  cfg["gmtOffset"] = config.gmtOffset;
  cfg["httpUname"] = config.httpUname;
  cfg["httpPass"] = config.httpPass;
  cfg["name"] = config.name;
  cfg["intvRecPwrUsg"] = mySettings.intvRecPwrUsg;
  cfg["intvRecWthr"] = mySettings.intvRecWthr;
  cfg["intvDevTel"] = mySettings.intvDevTel;
  cfg["useCloud"] = (int)config.useCloud;
  wsSend(doc);
  doc.clear();
  JsonObject dtCyc = doc.createNestedObject("dtCyc");
  dtCyc["dtCycCh1"] = mySettings.dtCyc[0];
  dtCyc["dtCycCh2"] = mySettings.dtCyc[1];
  dtCyc["dtCycCh3"] = mySettings.dtCyc[2];
  dtCyc["dtCycCh4"] = mySettings.dtCyc[3];
  wsSend(doc);
  doc.clear();
  JsonObject dtRng = doc.createNestedObject("dtRng");
  dtRng["dtRngCh1"] = mySettings.dtRng[0];
  dtRng["dtRngCh2"] = mySettings.dtRng[1];
  dtRng["dtRngCh3"] = mySettings.dtRng[2];
  dtRng["dtRngCh4"] = mySettings.dtRng[3];
  wsSend(doc);
  doc.clear();
  JsonObject dtCycFS = doc.createNestedObject("dtCycFS");
  dtCycFS["dtCycFSCh1"] = mySettings.dtCycFS[0];
  dtCycFS["dtCycFSCh2"] = mySettings.dtCycFS[1];
  dtCycFS["dtCycFSCh3"] = mySettings.dtCycFS[2];
  dtCycFS["dtCycFSCh4"] = mySettings.dtCycFS[3];
  wsSend(doc);
  doc.clear();
  JsonObject dtRngFS = doc.createNestedObject("dtRngFS");
  dtRngFS["dtRngFSCh1"] = mySettings.dtRngFS[0];
  dtRngFS["dtRngFSCh2"] = mySettings.dtRngFS[1];
  dtRngFS["dtRngFSCh3"] = mySettings.dtRngFS[2];
  dtRngFS["dtRngFSCh4"] = mySettings.dtRngFS[3];
  wsSend(doc);
  doc.clear();
  JsonObject rlyActDT = doc.createNestedObject("rlyActDT");
  rlyActDT["rlyActDTCh1"] = (uint64_t)mySettings.rlyActDT[0] * 1000;
  rlyActDT["rlyActDTCh2"] = (uint64_t)mySettings.rlyActDT[1] * 1000;
  rlyActDT["rlyActDTCh3"] = (uint64_t)mySettings.rlyActDT[2] * 1000;
  rlyActDT["rlyActDTCh4"] = (uint64_t)mySettings.rlyActDT[3] * 1000;
  wsSend(doc);
  doc.clear();
  JsonObject rlyActDr = doc.createNestedObject("rlyActDr");
  rlyActDr["rlyActDrCh1"] = mySettings.rlyActDr[0];
  rlyActDr["rlyActDrCh2"] = mySettings.rlyActDr[1];
  rlyActDr["rlyActDrCh3"] = mySettings.rlyActDr[2];
  rlyActDr["rlyActDrCh4"] = mySettings.rlyActDr[3];
  wsSend(doc);
  doc.clear();
  JsonObject rlyActIT = doc.createNestedObject("rlyActIT");
  rlyActIT["rlyActITCh1"] = mySettings.rlyActIT[0];
  rlyActIT["rlyActITCh2"] = mySettings.rlyActIT[1];
  rlyActIT["rlyActITCh3"] = mySettings.rlyActIT[2];
  rlyActIT["rlyActITCh4"] = mySettings.rlyActIT[3];
  wsSend(doc);
  doc.clear();
  JsonObject rlyActITOn = doc.createNestedObject("rlyActITOn");
  rlyActITOn["rlyActITOnCh1"] = mySettings.rlyActITOn[0];
  rlyActITOn["rlyActITOnCh2"] = mySettings.rlyActITOn[1];
  rlyActITOn["rlyActITOnCh3"] = mySettings.rlyActITOn[2];
  rlyActITOn["rlyActITOnCh4"] = mySettings.rlyActITOn[3];
  wsSend(doc);
  doc.clear();
  JsonObject rlyCtrlMd = doc.createNestedObject("rlyCtrlMd");
  rlyCtrlMd["rlyCtrlMdCh1"] = mySettings.rlyCtrlMd[0];
  rlyCtrlMd["rlyCtrlMdCh2"] = mySettings.rlyCtrlMd[1];
  rlyCtrlMd["rlyCtrlMdCh3"] = mySettings.rlyCtrlMd[2];
  rlyCtrlMd["rlyCtrlMdCh4"] = mySettings.rlyCtrlMd[3];
  wsSend(doc);
  doc.clear();
  JsonObject rlyActMT = doc.createNestedObject("rlyActMT");
  rlyActMT["rlyActMTCh1"] = mySettings.rlyActMT[0];
  rlyActMT["rlyActMTCh2"] = mySettings.rlyActMT[1];
  rlyActMT["rlyActMTCh3"] = mySettings.rlyActMT[2];
  rlyActMT["rlyActMTCh4"] = mySettings.rlyActMT[3];
  wsSend(doc);
  doc.clear();
  doc["ch1"] = mySettings.dutyState[0] == mySettings.ON ? 1 : 0;
  doc["ch2"] = mySettings.dutyState[1] == mySettings.ON ? 1 : 0;
  doc["ch3"] = mySettings.dutyState[2] == mySettings.ON ? 1 : 0;
  doc["ch4"] = mySettings.dutyState[3] == mySettings.ON ? 1 : 0;
  wsSend(doc);
  doc.clear();
  JsonObject label = doc.createNestedObject("label");
  label["labelCh1"] = mySettings.label[0];
  label["labelCh2"] = mySettings.label[1];
  label["labelCh3"] = mySettings.label[2];
  label["labelCh4"] = mySettings.label[3];
  wsSend(doc);
  doc.clear();
  doc["useWiFiOta"] = (int)config.useWiFiOta;
  doc["useWebIface"] = (int)config.useWebIface;
  doc["hostname"] = config.hostname;
  wsSend(doc);
  doc.clear();
}

