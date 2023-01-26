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
HardwareSerial PZEMSerial(1);
PZEM004Tv30 PZEM(PZEMSerial, S1_RX, S1_TX);

const size_t callbacksSize = 15;
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
  { "bridge", processBridge}
};

void setup()
{
  startup();
  loadSettings();
  configCoMCULoad();
  syncConfigCoMCU();
  if(String(config.model) == String("Generic")){
    strlcpy(config.model, "Gadadar", sizeof(config.model));
  }

  mySettings.flag_bme280 = bme.begin(0x76);
  if(!mySettings.flag_bme280){
    log_manager->error(PSTR(__func__),PSTR("BME weather sensor failed to initialize!\n"));
  }

  networkInit();
  tb.setBufferSize(DOCSIZE);

  if(mySettings.intvDevTel != 0){
    taskid_t taskPublishDeviceTelemetry = taskManager.scheduleFixedRate(mySettings.intvDevTel * 1000, [] {
      publishDeviceTelemetry();
    });
  }

  if(mySettings.intvRecPwrUsg != 0){
    taskid_t taskRecPowerUsage = taskManager.scheduleFixedRate(mySettings.intvRecPwrUsg * 1000, [] {
      recPowerUsage();
    });
  }

  if(mySettings.intvRecWthr != 0){
    taskid_t taskRecWeather = taskManager.scheduleFixedRate(mySettings.intvRecWthr * 1000, [] {
      recWeatherData();
    });
  }

}

void loop()
{
  udawa();
  relayControlByDateTime();
  relayControlBydtCyc();
  relayControlByIntrvl();
  publishSwitch();

  if(tb.connected() && FLAG_IOT_SUBSCRIBE)
  {
    if(tb.callbackSubscribe(callbacks, callbacksSize))
    {
      log_manager->info(PSTR(__func__),PSTR("Callbacks subscribed successfuly!\n"));
      FLAG_IOT_SUBSCRIBE = false;
    }
    if (tb.Firmware_Update(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION))
    {
      log_manager->info(PSTR(__func__),PSTR("OTA Update finished, rebooting...\n"));
      reboot();
    }
    else
    {
      log_manager->info(PSTR(__func__),PSTR("Firmware up-to-date.\n"));
    }

    syncClientAttributes();
  }
}

const uint32_t MAX_INT = 0xFFFFFFFF;
uint32_t micro2milli(uint32_t hi, uint32_t lo)
{
  if (hi >= 1000)
  {
    log_manager->error(PSTR(__func__),"Cannot store milliseconds in uint32!\n");
  }

  uint32_t r = (lo >> 16) + (hi << 16);
  uint32_t ans = r / 1000;
  r = ((r % 1000) << 16) + (lo & 0xFFFF);
  ans = (ans << 16) + r / 1000;

  return ans;
}


void recPowerUsage(){
  if(tb.connected()){
    StaticJsonDocument<DOCSIZE> doc;
    if(!isnan(PZEM.voltage())){
      doc["volt"] = PZEM.voltage();
      doc["amp"] = PZEM.current();
      doc["watt"] = PZEM.power();
      doc["ener"] = PZEM.energy();
      doc["freq"] = PZEM.frequency();
      doc["pf"] = PZEM.pf();

      tb.sendTelemetryDoc(doc);
      //log_manager->debug(PSTR(__func__), PSTR("Volt: %.2f - Amp: %.2f \n"), PZEM.voltage(), PZEM.current());
    }
  }
}

void recWeatherData(){
  if(tb.connected() && mySettings.flag_bme280){
    float celc = bme.readTemperature();
    float rh = bme.readHumidity();
    float hpa = bme.readPressure() / 100.0F;
    float alt = bme.readAltitude(mySettings.seaHpa);

    StaticJsonDocument<DOCSIZE> doc;
    doc["celc"] = celc;
    doc["rh"] = rh;
    doc["hpa"] = hpa;
    doc["alt"] = alt;
    tb.sendTelemetryDoc(doc);
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
    mySettings.ON = doc["ON"].as<bool>();
  }
  else
  {
    mySettings.ON = 1;
  }

  if(doc["intvRecPwrUsg"] != nullptr){mySettings.intvRecPwrUsg = doc["intvRecPwrUsg"].as<uint16_t>();}
  else{mySettings.intvRecPwrUsg = 0;}

  if(doc["intvRecWthr"] != nullptr){mySettings.intvRecWthr = doc["intvRecWthr"].as<uint16_t>();}
  else{mySettings.intvRecWthr = 0;}

  if(doc["intvDevTel"] != nullptr){mySettings.intvDevTel = doc["intvDevTel"].as<uint16_t>();}
  else{mySettings.intvDevTel = 0;}

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
  syncClientAttributes();
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

  StaticJsonDocument<DOCSIZE> doc;
  doc["method"] = "sCfg";
  String state = data["params"]["state"].as<String>();
  if(state == String("ON")){
    doc["fPanic"] = 1;
  }
  else{
    doc["fPanic"] = 0;
  }
  serialWriteToCoMcu(doc, 0);
  return callbackResponse("setPanic", data["params"]["state"].as<int>());
}

callbackResponse processBridge(const callbackData &data)
{
  StaticJsonDocument<DOCSIZE> doc;
  doc["method"] = data["params"]["method"];
  doc["params"] = data["params"];
  if(data["params"]["isRpc"] != nullptr){
    serialWriteToCoMcu(doc, 1);
    String result;
    serializeJson(doc, result);
    return callbackResponse("bridge", result.c_str());
  }else{
    serialWriteToCoMcu(doc, 0);
    String result;
    serializeJson(doc, result);
    return callbackResponse("bridge", result.c_str());
  }
}

void setSwitch(String ch, String state)
{
  bool fState = 0;
  uint8_t pin = 0;

  if(ch == String("ch1")){pin = mySettings.pin[0]; mySettings.dutyState[0] = (state == String("ON")) ? mySettings.ON : !mySettings.ON; mySettings.publishSwitch[0] = true;}
  else if(ch == String("ch2")){pin = mySettings.pin[1]; mySettings.dutyState[1] = (state == String("ON")) ? mySettings.ON : !mySettings.ON; mySettings.publishSwitch[1] = true;}
  else if(ch == String("ch3")){pin = mySettings.pin[2]; mySettings.dutyState[2] = (state == String("ON")) ? mySettings.ON : !mySettings.ON; mySettings.publishSwitch[2] = true;}
  else if(ch == String("ch4")){pin = mySettings.pin[3]; mySettings.dutyState[3] = (state == String("ON")) ? mySettings.ON : !mySettings.ON; mySettings.publishSwitch[3] = true;}

  if(state == String("ON"))
  {
    fState = mySettings.ON;
  }
  else
  {
    fState = !mySettings.ON;
  }

  setCoMCUPin(pin, 1, OUTPUT, 0, fState);
  log_manager->warn(PSTR(__func__), "Relay %s was set to %s / %d.\n", ch, state, (int)fState);
}

void relayControlByDateTime(){
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
          mySettings.dutyState[i] = !mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "OFF");
          log_manager->debug(PSTR(__func__),PSTR("Relay Ch%d changed to %d - ts:%d - tr:%d - exp:%d\n"), i+1,
            mySettings.dutyState[i], mySettings.rlyActDT[i], mySettings.rlyActDr[i],
            mySettings.rlyActDr[i] - (rtc.getEpoch() - mySettings.rlyActDT[i]));
      }
    }
    else if(mySettings.rlyActDr[i] > 0 && mySettings.rlyCtrlMd[i] == 3){
      uint32_t currHour = rtc.getHour(true);
      uint16_t currMinute = rtc.getMinute();
      uint8_t currSecond = rtc.getSecond();
      String currDT = rtc.getDateTime();
      uint16_t currentTimeInSec = (currHour * 60 * 60) + (currMinute * 60) + currSecond;

      uint32_t rlyActDT = mySettings.rlyActDT[i] + config.gmtOffset;
      uint32_t targetHour = hour(rlyActDT);
      uint16_t targetMinute = minute(rlyActDT);
      uint8_t targetSecond = second(rlyActDT);
      char targetDT[32];
      sprintf(targetDT, "%02d.%02d.%02d %02d:%02d:%02d", day(rlyActDT), month(rlyActDT),
        year(rlyActDT), hour(rlyActDT), minute(rlyActDT), second(rlyActDT));
      uint16_t targetTimeInSec = (targetHour * 60 * 60) + (targetMinute * 60) + targetSecond;

      if(targetTimeInSec <= currentTimeInSec && (mySettings.rlyActDr[i]) >=
        (currentTimeInSec - targetTimeInSec) && mySettings.dutyState[i] != mySettings.ON){
          mySettings.dutyState[i] = mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "ON");
          log_manager->debug(PSTR(__func__),PSTR("Relay Ch%d changed to %d \n currentTimeInSec:%d (%d:%d:%d - %s - %d) targetTimeInSec:%d (%d:%d:%d - %s - %d) - rlyActDr:%d - exp:%d\n"), i+1,
            mySettings.dutyState[i], currentTimeInSec, currHour, currMinute, currSecond, currDT.c_str(), rtc.getEpoch(),
            targetTimeInSec, targetHour, targetMinute, targetSecond, targetDT, rlyActDT,
            mySettings.rlyActDr[i], mySettings.rlyActDr[i] - (currentTimeInSec - targetTimeInSec));
      }
      else if(mySettings.dutyState[i] == mySettings.ON && (mySettings.rlyActDr[i]) <=
        (currentTimeInSec - targetTimeInSec)){
          mySettings.dutyState[i] = !mySettings.ON;
          String ch = "ch" + String(i+1);
          setSwitch(ch, "OFF");
          log_manager->debug(PSTR(__func__),PSTR("Relay Ch%d changed to %d \n currentTimeInSec:%d (%d:%d:%d - %s - %d) targetTimeInSec:%d (%d:%d:%d - %s - %d) - rlyActDr:%d - exp:%d\n"), i+1,
            mySettings.dutyState[i], currentTimeInSec, currHour, currMinute, currSecond, currDT.c_str(), rtc.getEpoch(),
            targetTimeInSec, targetHour, targetMinute, targetSecond, targetDT, rlyActDT,
            mySettings.rlyActDr[i], mySettings.rlyActDr[i] - (currentTimeInSec - targetTimeInSec));
      }
    }
  }
}

void relayControlBydtCyc()
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
          mySettings.dutyState[i] = !mySettings.ON;
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

void relayControlByIntrvl(){
  for(uint8_t i = 0; i < countof(mySettings.pin); i++)
  {
    if(mySettings.rlyActIT[i] != 0 && mySettings.rlyActITOn[i] != 0 && mySettings.rlyCtrlMd[i] == 4)
    {
      if( mySettings.dutyState[i] == !mySettings.ON && (millis() - mySettings.rlyActITOnTs[i]) > (mySettings.rlyActIT[i] * 1000) ){
        mySettings.dutyState[i] = mySettings.ON;
        String ch = "ch" + String(i+1);
        setSwitch(ch, "ON");
        mySettings.rlyActITOnTs[i] = millis();
        log_manager->debug(PSTR(__func__), PSTR("Relay Ch%d has changed to %d - rlyActIT:%d - rlyActITOn:%d\n"), i+1, mySettings.dutyState[i],
          mySettings.rlyActIT[i], mySettings.rlyActITOn[i]);
      }
      if( mySettings.dutyState[i] == mySettings.ON && (millis() - mySettings.rlyActITOnTs[i]) > (mySettings.rlyActITOn[i] * 1000) ){
        mySettings.dutyState[i] = !mySettings.ON;
        String ch = "ch" + String(i+1);
        setSwitch(ch, "OFF");
        log_manager->debug(PSTR(__func__), PSTR("Relay Ch%d has changed to %d - rlyActIT:%d - rlyActITOn:%d\n"), i+1, mySettings.dutyState[i],
          mySettings.rlyActIT[i], mySettings.rlyActITOn[i]);
      }
    }
  }
}

callbackResponse processSharedAttributesUpdate(const callbackData &data)
{
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
  if(data["provisionDeviceKey"] != nullptr){strlcpy(config.provisionDeviceKey, data["provisionDeviceKey"].as<const char*>(), sizeof(config.provisionDeviceKey));}
  if(data["provisionDeviceSecret"] != nullptr){strlcpy(config.provisionDeviceSecret, data["provisionDeviceSecret"].as<const char*>(), sizeof(config.provisionDeviceSecret));}
  if(data["logLev"] != nullptr){config.logLev = data["logLev"].as<uint8_t>();}
  if(data["gmtOffset"] != nullptr){config.gmtOffset = data["gmtOffset"].as<int>();}


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

  if(data["seaHpa"] != nullptr){mySettings.seaHpa = data["seaHpa"].as<float>();}

  if(data["fPanic"] != nullptr){configcomcu.fPanic = data["fPanic"].as<bool>();}
  if(data["bfreq"] != nullptr){configcomcu.bfreq = data["bfreq"].as<uint16_t>();}
  if(data["fBuzz"] != nullptr){configcomcu.fBuzz = data["fBuzz"].as<bool>();}
  if(data["pinBuzzer"] != nullptr){configcomcu.pinBuzzer = data["pinBuzzer"].as<uint8_t>();}
  if(data["pinLedR"] != nullptr){configcomcu.pinLedR = data["pinLedR"].as<uint8_t>();}
  if(data["pinLedG"] != nullptr){configcomcu.pinLedG = data["pinLedG"].as<uint8_t>();}
  if(data["pinLedB"] != nullptr){configcomcu.pinLedB = data["pinLedB"].as<uint8_t>();}


  mySettings.lastUpdated = millis();
  return callbackResponse("sharedAttributesUpdate", 1);
}

void syncClientAttributes()
{
  StaticJsonDocument<DOCSIZE> doc;

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
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["wpass"] = config.wpass;
  doc["dssid"] = config.dssid;
  doc["dpass"] = config.dpass;
  doc["upass"] = config.upass;
  doc["accessToken"] = config.accessToken;
  doc["provisionDeviceKey"] = config.provisionDeviceKey;
  doc["provisionDeviceSecret"] = config.provisionDeviceSecret;
  doc["logLev"] = config.logLev;
  doc["gmtOffset"] = config.gmtOffset;
  tb.sendAttributeDoc(doc);
  doc.clear();
  doc["dtCycCh1"] = mySettings.dtCyc[0];
  doc["dtCycCh2"] = mySettings.dtCyc[1];
  doc["dtCycCh3"] = mySettings.dtCyc[2];
  doc["dtCycCh4"] = mySettings.dtCyc[3];
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
  doc["intvRecPwrUsg"] = mySettings.intvRecPwrUsg;
  doc["intvRecWthr"] = mySettings.intvRecWthr;
  doc["intvDevTel"] = mySettings.intvDevTel;
  doc["rlyCtrlMdCh1"] = mySettings.rlyCtrlMd[0];
  doc["rlyCtrlMdCh2"] = mySettings.rlyCtrlMd[1];
  doc["rlyCtrlMdCh3"] = mySettings.rlyCtrlMd[2];
  doc["rlyCtrlMdCh4"] = mySettings.rlyCtrlMd[3];
  doc["seaHpa"] = mySettings.seaHpa;
  tb.sendAttributeDoc(doc);
  doc.clear();
}

void publishDeviceTelemetry()
{
  if(tb.connected()){
    StaticJsonDocument<DOCSIZE> doc;

    doc["heap"] = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = millis()/1000;
    doc["dt"] = rtc.getEpoch();
    doc["dts"] = rtc.getDateTime();
    tb.sendTelemetryDoc(doc);
    doc.clear();
  }
}

void publishSwitch(){
  if(tb.connected()){
    for (uint8_t i = 0; i < 4; i++){
      if(mySettings.publishSwitch[i]){
        StaticJsonDocument<DOCSIZE> doc;
        String chName = "ch" + String(i+1);
        doc[chName.c_str()] = (int)mySettings.dutyState[i];
        tb.sendTelemetryDoc(doc);
        doc.clear();

        mySettings.publishSwitch[i] = false;
      }
    }
  }
}