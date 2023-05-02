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

Task relayControlCP1Loop(TASK_SECOND, TASK_FOREVER, &relayControlCP1Cb, &r, 1, NULL, NULL);
Task relayControlCP2Loop(TASK_SECOND, TASK_FOREVER, &relayControlCP2Cb, &r, 1, NULL, NULL);
Task relayControlCP3Loop(TASK_SECOND, TASK_FOREVER, &relayControlCP3Cb, &r, 1, NULL, NULL);
Task relayControlCP4Loop(TASK_SECOND, TASK_FOREVER, &relayControlCP4Cb, &r, 1, NULL, NULL);
Task selfDiagnosticLoop(120 * TASK_SECOND, TASK_FOREVER, &selfDiagnosticCb, &r, 1, NULL, NULL);
Task taskPublishSwitch(TASK_IMMEDIATE, TASK_ONCE, &publishSwitchCb, &r, 0, NULL, NULL);

Task calcPowerUsageLoop(TASK_SECOND, TASK_FOREVER, &calcPowerUsageCb, &r, 1, NULL, NULL);
Task calcWeatherDataLoop(60 * TASK_SECOND, TASK_FOREVER, &calcWeatherDataCb, &r, 1, NULL, NULL);
Task recPowerUsageLoop(mySettings.itP * TASK_SECOND, TASK_FOREVER, &recPowerUsageCb, &r, 0, NULL, NULL);
Task recWeatherDataLoop((mySettings.itW + 120) * TASK_SECOND, TASK_FOREVER, &recWeatherDataCb, &r, 0, NULL, NULL);
Task deviceTelemetryLoop(mySettings.itD * TASK_SECOND, TASK_FOREVER, &deviceTelemetryLoopCb, &r, 0, NULL, NULL);

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
  startup();
  loadSettings();
  syncConfigCoMCU();
  if(String(config.model) == String("Generic")){
    strlcpy(config.model, "Gadadar", sizeof(config.model));
  }
  setAlarm(999, 1, 1, 3000);
  stateReset(0);

  mySettings.flag_bme280 = bme.begin(0x76);
  if(!mySettings.flag_bme280){
    log_manager->warn(PSTR(__func__),PSTR("BME weather sensor failed to initialize!\n"));
  }

  if(mySettings.itD > 0){
    deviceTelemetryLoop.setInterval(mySettings.itD * TASK_SECOND);
    deviceTelemetryLoop.setIterations(TASK_FOREVER);
    deviceTelemetryLoop.enable();
  }

  if(mySettings.itP > 0){
    recPowerUsageLoop.setInterval(mySettings.itP * TASK_SECOND);
    recPowerUsageLoop.setIterations(TASK_FOREVER);
    recPowerUsageLoop.enable();
  }
  if(mySettings.itW > 0){
    recWeatherDataLoop.setInterval((mySettings.itW + 120) * TASK_SECOND);
    recWeatherDataLoop.setIterations(TASK_FOREVER);
    recWeatherDataLoop.enable();
  }

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void loop()
{
  udawa();
}

void selfDiagnosticCb(){
  long startMillis = millis();

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

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void calcPowerUsageCb(){
  long startMillis = millis();

  HardwareSerial PZEMSerial(1);
  PZEM004Tv30 PZEM(PZEMSerial, S1_RX, S1_TX);
  
  if(isnan(PZEM.voltage())){return;}
  
  if(!isnan(PZEM.voltage())){
    _volt.Add(PZEM.voltage());
    tb.sendAttributeFloat("_volt", PZEM.voltage());
    tb.sendAttributeFloat("_ener", PZEM.energy());
  }

  if(!isnan(PZEM.current())){
    _amp.Add(PZEM.current());
    tb.sendAttributeFloat("_amp", PZEM.current());
  }

  if(!isnan(PZEM.power())){
    _watt.Add(PZEM.power());
    tb.sendAttributeFloat("_watt", PZEM.power());
  }

  if(!isnan(PZEM.frequency())){
    _freq.Add(PZEM.frequency());
    tb.sendAttributeFloat("_freq", PZEM.frequency());
  }

  if(!isnan(PZEM.pf())){
    _pf.Add(PZEM.pf());
    tb.sendAttributeFloat("_pf", PZEM.pf());
  }

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void recPowerUsageCb(){
  long startMillis = millis();

  if(tb.connected()){
    tb.sendTelemetryFloat("volt", _volt.Get_Average());
    tb.sendTelemetryFloat("amp", _amp.Get_Average());
    tb.sendTelemetryFloat("watt", _watt.Get_Average());
    tb.sendTelemetryFloat("freq",  _freq.Get_Average());
    tb.sendTelemetryFloat("pf",  _pf.Get_Average());

    HardwareSerial PZEMSerial(1);
    PZEM004Tv30 PZEM(PZEMSerial, S1_RX, S1_TX);
    tb.sendTelemetryFloat("ener", PZEM.energy());

   
    _volt.Clear(); _amp.Clear(); _watt.Clear(); _freq.Clear(); _pf.Clear();
  }

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void calcWeatherDataCb(){
  long startMillis = millis();

  if(mySettings.flag_bme280){
    mySettings.celc = bme.readTemperature();
    mySettings.rh = bme.readHumidity();
    mySettings.hpa = bme.readPressure() / 100.0F;
    mySettings.alt = bme.readAltitude(mySettings.seaHpa);

    _celc.Add(mySettings.celc);
    _rh.Add(mySettings.rh);
    _hpa.Add(mySettings.hpa);
    _alt.Add(mySettings.alt);

    tb.sendAttributeFloat("_celc",mySettings.celc);
    tb.sendAttributeFloat("_rh", mySettings.rh);
    tb.sendAttributeFloat("_hpa", mySettings.hpa);
    tb.sendAttributeFloat("_alt", mySettings.alt);
  }

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void recWeatherDataCb(){
  long startMillis = millis();

  if(tb.connected()){
    mySettings._celc = _celc.Get_Average();
    mySettings._rh = _rh.Get_Average();
    mySettings._hpa = _hpa.Get_Average();
    mySettings._alt = _alt.Get_Average();

    tb.sendTelemetryFloat("celc",mySettings._celc);
    tb.sendTelemetryFloat("rh", mySettings._rh);
    tb.sendTelemetryFloat("hpa", mySettings._hpa);
    tb.sendTelemetryFloat("alt", mySettings._alt);
    _celc.Clear(); _rh.Clear(); _hpa.Clear(); _alt.Clear();
  }

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
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
  else{mySettings.itD = 60;}

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
  long startMillis = millis();

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
  taskPublishSwitch.setInterval(TASK_IMMEDIATE);
  taskPublishSwitch.setIterations(TASK_ONCE);
  taskPublishSwitch.enableDelayed(1 * TASK_SECOND);

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
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

void attUpdateCb(const Shared_Attribute_Data &data)
{
  long startMillis = millis();

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

  log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void onTbConnected(){
    long startMillis = millis();

    mySettings.publishSwitch[0] = 1;
    mySettings.publishSwitch[1] = 1;
    mySettings.publishSwitch[2] = 1;
    mySettings.publishSwitch[3] = 1;
    taskPublishSwitch.setInterval(TASK_IMMEDIATE);
    taskPublishSwitch.setIterations(TASK_ONCE);
    taskPublishSwitch.enableDelayed(3 * TASK_SECOND);

    if(mySettings.itD > 0){
        deviceTelemetryLoop.setInterval(mySettings.itD * TASK_SECOND);
        deviceTelemetryLoop.setIterations(TASK_FOREVER);
        deviceTelemetryLoop.enableDelayed(10 * TASK_SECOND);
    }
    log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void onTbDisconnected(){
    long startMillis = millis();
    deviceTelemetryLoop.disable();
    log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
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
            setAlarm(666, 1, 1, 10000);
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
        log_manager->verbose(PSTR(__func__), PSTR("Relay operations are changed to manual.\n."));
    }
    setSwitch("ch1", "OFF");
    setSwitch("ch2", "OFF");
    setSwitch("ch3", "OFF");
    setSwitch("ch4", "OFF");
    log_manager->verbose(PSTR(__func__), PSTR("Relay operations are disabled.\n."));
}

void deviceTelemetryLoopCb(){
    tb.sendAttributeInt(PSTR("uptime"), millis());
    tb.sendAttributeInt(PSTR("heap"), heap_caps_get_free_size(MALLOC_CAP_8BIT));
    tb.sendAttributeInt(PSTR("rssi"), WiFi.RSSI());
    tb.sendAttributeInt(PSTR("dt"), rtc.getEpoch());
}

void onAlarm(int code){
    long startMillis = millis();

    log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}

void publishSwitchCb(){
    long startMillis = millis();

    for (uint8_t i = 0; i < sizeof(mySettings.dutyState); i++){
        if(mySettings.publishSwitch[i]){
            String chName = "ch" + String(i+1);
            int state = (int)mySettings.dutyState[i] == mySettings.ON ? 1 : 0;
            
            if(config.fIoT){
                tb.sendTelemetryInt(chName.c_str(), state);
            }

            mySettings.publishSwitch[i] = false;
        }
    }

    log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);   
}

void onSyncClientAttr(){
    long startMillis = millis();

    StaticJsonDocument<DOCSIZE_MIN> doc;
    char buffer[DOCSIZE_MIN];

    doc[PSTR("cp1A1")] = mySettings.cp1A[0];
    doc[PSTR("cp1A2")] = mySettings.cp1A[1];
    doc[PSTR("cp1A3")] = mySettings.cp1A[2];
    doc[PSTR("cp1A4")] = mySettings.cp1A[3];
    doc[PSTR("cp1B1")] = mySettings.cp1B[0];
    doc[PSTR("cp1B2")] = mySettings.cp1B[1];
    doc[PSTR("cp1B3")] = mySettings.cp1B[2];
    doc[PSTR("cp1B4")] = mySettings.cp1B[3];
    serializeJson(doc, buffer);
    tb.sendAttributeJSON(buffer);
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
    tb.sendAttributeJSON(buffer);
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
    tb.sendAttributeJSON(buffer);
    doc.clear();
    doc[PSTR("pR1")] = mySettings.pR[0];
    doc[PSTR("pR2")] = mySettings.pR[1];
    doc[PSTR("pR3")] = mySettings.pR[2];
    doc[PSTR("pR4")] = mySettings.pR[3];
    doc[PSTR("ON")] = mySettings.ON;
    serializeJson(doc, buffer);
    tb.sendAttributeJSON(buffer);
    doc.clear();
    doc[PSTR("cp3A1")] = mySettings.cp3A[0].c_str();
    serializeJson(doc, buffer);
    tb.sendAttributeJSON(buffer);
    doc.clear();
    doc[PSTR("cp3A2")] = mySettings.cp3A[1].c_str();
    serializeJson(doc, buffer);
    tb.sendAttributeJSON(buffer);
    doc.clear();
    doc[PSTR("cp3A3")] = mySettings.cp3A[2].c_str();
    serializeJson(doc, buffer);
    tb.sendAttributeJSON(buffer);
    doc.clear();
    doc[PSTR("cp3A4")] = mySettings.cp3A[3].c_str();
    serializeJson(doc, buffer);
    tb.sendAttributeJSON(buffer);
    doc.clear();
    doc[PSTR("lbl1")] = mySettings.lbl[0];
    doc[PSTR("lbl2")] = mySettings.lbl[1];
    doc[PSTR("lbl3")] = mySettings.lbl[2];
    doc[PSTR("lbl4")] = mySettings.lbl[3];
    doc[PSTR("itP")] = mySettings.itP;
    doc[PSTR("itW")] = mySettings.itW;
    doc[PSTR("itD")] = mySettings.itD;
    serializeJson(doc, buffer);
    tb.sendAttributeJSON(buffer);
    doc.clear();
    doc[PSTR("cpM1")] = mySettings.cpM[0];
    doc[PSTR("cpM2")] = mySettings.cpM[1];
    doc[PSTR("cpM3")] = mySettings.cpM[2];
    doc[PSTR("cpM4")] = mySettings.cpM[3];
    doc[PSTR("seaHpa")] = mySettings.seaHpa;
    serializeJson(doc, buffer);
    tb.sendAttributeJSON(buffer);
    doc.clear();
    
    log_manager->verbose(PSTR(__func__), PSTR("Executed (%dms).\n"), millis() - startMillis);
}