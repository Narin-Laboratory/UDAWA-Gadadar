#include "storage.h"

UdawaConfig config(PSTR("/config.json"));
CrashState crashState;
GenericConfig crashStateConfig(PSTR("/crash.json"));

// New definitions
AppConfig appConfig;
AppState appState;
Relay relays[4] = {
    Relay(0, 0, 0, 0, 0, 0, 0, 0, false, "No label", 3600, 0, 0),
    Relay(1, 0, 0, 0, 0, 0, 0, 0, false, "No label", 3600, 0, 0),
    Relay(2, 0, 0, 0, 0, 0, 0, 0, false, "No label", 3600, 0, 0),
    Relay(3, 0, 0, 0, 0, 0, 0, 0, false, "No label", 3600, 0, 0)
};

GenericConfig appConfigGC(PSTR("/appConfig.json"));
GenericConfig appStateGC(PSTR("/appState.json"));
GenericConfig appRelaysGC(PSTR("/appRelays.json"));


void storageSetup(){
    logger->debug(PSTR(__func__), PSTR("Initializing LittleFS: %d\n"), config.begin());
    config.load();
    
    logger->setLogLevel((LogLevel)config.state.logLev);

    JsonDocument doc;
    appConfigGC.load(doc);
    storageConvertAppConfig(doc, true, true);
    doc.clear();
    appStateGC.load(doc);
    storageConvertAppState(doc, true, true);
    doc.clear();
    appRelaysGC.load(doc);
    storageConvertAppRelay(doc, true, true);
    doc.clear();
}

void storageConvertAppConfig(JsonDocument &doc, bool direction, bool load_defaults){
  if(direction){ // from doc to config
    if(!doc["s1tx"].isNull()) appConfig.s1tx = doc["s1tx"].as<uint8_t>(); else if(load_defaults) appConfig.s1tx = s1tx;
    if(!doc["s1rx"].isNull()) appConfig.s1rx = doc["s1rx"].as<uint8_t>(); else if(load_defaults) appConfig.s1rx = s1rx;
    if(!doc["intvWeb"].isNull()) appConfig.intvWeb = doc["intvWeb"].as<unsigned long>(); else if(load_defaults) appConfig.intvWeb = intvWeb;
    if(!doc["intvAttr"].isNull()) appConfig.intvAttr = doc["intvAttr"].as<unsigned long>(); else if(load_defaults) appConfig.intvAttr = intvAttr;
    if(!doc["intvTele"].isNull()) appConfig.intvTele = doc["intvTele"].as<unsigned long>(); else if(load_defaults) appConfig.intvTele = intvTele;
    if(!doc["maxWatt"].isNull()) appConfig.maxWatt = doc["maxWatt"].as<int>(); else if(load_defaults) appConfig.maxWatt = maxWatt;
    if(!doc["relayON"].isNull()) appConfig.relayON = doc["relayON"].as<bool>(); else if(load_defaults) appConfig.relayON = relayON;
    if(!doc["fPowerSensorDummy"].isNull()) appConfig.fPowerSensorDummy = doc["fPowerSensorDummy"].as<bool>(); else if(load_defaults) appConfig.fPowerSensorDummy = fPowerSensorDummy;
    if(!doc["powerSensorAlarmTimer"].isNull()) appConfig.powerSensorAlarmTimer = doc["powerSensorAlarmTimer"].as<unsigned long>(); else if(load_defaults) appConfig.powerSensorAlarmTimer = powerSensorAlarmTimer;
  }
  else{ // from config to doc
    doc[PSTR("s1tx")] = appConfig.s1tx;
    doc[PSTR("s1rx")] = appConfig.s1rx;
    doc[PSTR("intvWeb")] = appConfig.intvWeb;
    doc[PSTR("intvAttr")] = appConfig.intvAttr;
    doc[PSTR("intvTele")] = appConfig.intvTele;
    doc[PSTR("maxWatt")] = appConfig.maxWatt;
    doc[PSTR("relayON")] = appConfig.relayON;
    doc[PSTR("fPowerSensorDummy")] = appConfig.fPowerSensorDummy;
    doc[PSTR("powerSensorAlarmTimer")] = appConfig.powerSensorAlarmTimer;
  }
}

void storageConvertAppState(JsonDocument &doc, bool direction, bool load_defaults){
  if(direction){ // from doc to state
    if(!doc["fPanic"].isNull()) appState.fPanic = doc["fPanic"].as<bool>(); else if(load_defaults) appState.fPanic = false;
  }
  else{ // from state to doc
    doc[PSTR("fPanic")] = appState.fPanic;
  }
}

void storageConvertAppRelay(JsonDocument &doc, bool direction, bool load_defaults){
    if(direction){ // from doc to relays
        if (!doc["relays"].isNull()) {
            JsonArrayConst relays_arr = doc["relays"].as<JsonArrayConst>();
            for(uint8_t i = 0; i < countof(relays); i++){
                if (i < relays_arr.size() && !relays_arr[i].isNull()) {
                    JsonObjectConst relay_obj = relays_arr[i].as<JsonObjectConst>();
                    if(!relay_obj["pin"].isNull()) relays[i].pin = relay_obj["pin"].as<uint8_t>();
                    if(!relay_obj["mode"].isNull()) relays[i].mode = relay_obj["mode"].as<uint8_t>();
                    if(!relay_obj["wattage"].isNull()) relays[i].wattage = relay_obj["wattage"].as<uint16_t>();
                    if(!relay_obj["lastActive"].isNull()) relays[i].lastActive = relay_obj["lastActive"].as<unsigned long>();
                    if(!relay_obj["lastChanged"].isNull()) relays[i].lastChanged = relay_obj["lastChanged"].as<unsigned long>();
                    if(!relay_obj["dutyCycle"].isNull()) relays[i].dutyCycle = relay_obj["dutyCycle"].as<uint8_t>();
                    if(!relay_obj["dutyRange"].isNull()) relays[i].dutyRange = relay_obj["dutyRange"].as<unsigned long>();
                    if(!relay_obj["autoOff"].isNull()) relays[i].autoOff = relay_obj["autoOff"].as<unsigned long>();
                    if(!relay_obj["state"].isNull()) relays[i].state = relay_obj["state"].as<bool>();
                    if(!relay_obj["label"].isNull()) relays[i].label = relay_obj["label"].as<String>();
                    if(!relay_obj["overrunInSec"].isNull()) relays[i].overrunInSec = relay_obj["overrunInSec"].as<uint16_t>();
                    if(!relay_obj["duration"].isNull()) relays[i].duration = relay_obj["duration"].as<unsigned long>();
                    if(!relay_obj["datetime"].isNull()) relays[i].datetime = relay_obj["datetime"].as<unsigned long>();

                    if(!relay_obj["timers"].isNull()){
                        JsonArrayConst timers_arr = relay_obj["timers"].as<JsonArrayConst>();
                        for(uint8_t j = 0; j < maxTimers; j++){
                            if (j < timers_arr.size() && !timers_arr[j].isNull()) {
                                JsonObjectConst timer_obj = timers_arr[j].as<JsonObjectConst>();
                                if(!timer_obj["h"].isNull()) relays[i].timers[j].hour = timer_obj["h"].as<uint8_t>();
                                if(!timer_obj["i"].isNull()) relays[i].timers[j].minute = timer_obj["i"].as<uint8_t>();
                                if(!timer_obj["s"].isNull()) relays[i].timers[j].second = timer_obj["s"].as<uint8_t>();
                                if(!timer_obj["d"].isNull()) relays[i].timers[j].duration = timer_obj["d"].as<unsigned long>();
                            }
                        }
                    }
                }
            }
        }
    }
    else{ // from relays to doc
    JsonArray _relays = doc[PSTR("relays")].to<JsonArray>();
        for(uint8_t i = 0; i < countof(relays); i++){
      JsonObject relay = _relays.add<JsonObject>();
        relay[PSTR("pin")] = relays[i].pin;
        relay[PSTR("mode")] = relays[i].mode;
        relay[PSTR("wattage")] = relays[i].wattage;
        relay[PSTR("lastActive")] = relays[i].lastActive;
        relay[PSTR("lastChanged")] = relays[i].lastChanged;
        relay[PSTR("dutyCycle")] = relays[i].dutyCycle;
        relay[PSTR("dutyRange")] = relays[i].dutyRange;
        relay[PSTR("autoOff")] = relays[i].autoOff;
        relay[PSTR("state")] = relays[i].state;
        relay[PSTR("label")] = relays[i].label;
        relay[PSTR("overrunInSec")] = relays[i].overrunInSec;
        relay[PSTR("duration")] = relays[i].duration;
        relay[PSTR("datetime")] = relays[i].datetime;

      JsonArray timers = relay[PSTR("timers")].to<JsonArray>();
        for (uint8_t j = 0; j < maxTimers; j++) {
          JsonObject timer = timers.add<JsonObject>();
            timer[PSTR("h")] = relays[i].timers[j].hour;
            timer[PSTR("i")] = relays[i].timers[j].minute;
            timer[PSTR("s")] = relays[i].timers[j].second;
            timer[PSTR("d")] = relays[i].timers[j].duration;
        }
        }
    }
}

void storageConvertUdawaConfig(JsonDocument &doc, bool direction, bool load_defaults){
  if(direction){ // from doc to config
    if(!doc["fInit"].isNull()) config.state.fInit = doc["fInit"].as<bool>();
    if(!doc["hwid"].isNull()) strlcpy(config.state.hwid, doc["hwid"].as<const char*>(), sizeof(config.state.hwid));
    if(!doc["name"].isNull()) strlcpy(config.state.name, doc["name"].as<const char*>(), sizeof(config.state.name));
    if(!doc["model"].isNull()) strlcpy(config.state.model, doc["model"].as<const char*>(), sizeof(config.state.model));
    if(!doc["group"].isNull()) strlcpy(config.state.group, doc["group"].as<const char*>(), sizeof(config.state.group));
    if(!doc["logLev"].isNull()) config.state.logLev = doc["logLev"].as<uint8_t>();
    if(!doc["tbAddr"].isNull()) strlcpy(config.state.tbAddr, doc["tbAddr"].as<const char*>(), sizeof(config.state.tbAddr));
    if(!doc["tbPort"].isNull()) config.state.tbPort = doc["tbPort"].as<uint16_t>();
    if(!doc["wssid"].isNull()) strlcpy(config.state.wssid, doc["wssid"].as<const char*>(), sizeof(config.state.wssid));
    if(!doc["wpass"].isNull()) strlcpy(config.state.wpass, doc["wpass"].as<const char*>(), sizeof(config.state.wpass));
    if(!doc["dssid"].isNull()) strlcpy(config.state.dssid, doc["dssid"].as<const char*>(), sizeof(config.state.dssid));
    if(!doc["dpass"].isNull()) strlcpy(config.state.dpass, doc["dpass"].as<const char*>(), sizeof(config.state.dpass));
    if(!doc["upass"].isNull()) strlcpy(config.state.upass, doc["upass"].as<const char*>(), sizeof(config.state.upass));
    #ifdef USE_IOT
    if(!doc["fIoT"].isNull()) config.state.fIoT = doc["fIoT"].as<bool>();
    if(!doc["accTkn"].isNull()) strlcpy(config.state.accTkn, doc["accTkn"].as<const char*>(), sizeof(config.state.accTkn));
    if(!doc["provSent"].isNull()) config.state.provSent = doc["provSent"].as<bool>();
    if(!doc["provDK"].isNull()) strlcpy(config.state.provDK, doc["provDK"].as<const char*>(), sizeof(config.state.provDK));
    if(!doc["provDS"].isNull()) strlcpy(config.state.provDS, doc["provDS"].as<const char*>(), sizeof(config.state.provDS));
    #endif
    if(!doc["binURL"].isNull()) strlcpy(config.state.binURL, doc["binURL"].as<const char*>(), sizeof(config.state.binURL));
    if(!doc["gmtOff"].isNull()) config.state.gmtOff = doc["gmtOff"].as<int>();
    if(!doc["fWOTA"].isNull()) config.state.fWOTA = doc["fWOTA"].as<bool>();
    if(!doc["fWeb"].isNull()) config.state.fWeb = doc["fWeb"].as<bool>();
    if(!doc["hname"].isNull()) strlcpy(config.state.hname, doc["hname"].as<const char*>(), sizeof(config.state.hname));
    if(!doc["htU"].isNull()) strlcpy(config.state.htU, doc["htU"].as<const char*>(), sizeof(config.state.htU));
    if(!doc["htP"].isNull()) strlcpy(config.state.htP, doc["htP"].as<const char*>(), sizeof(config.state.htP));
    if(!doc["logIP"].isNull()) strlcpy(config.state.logIP, doc["logIP"].as<const char*>(), sizeof(config.state.logIP));
    if(!doc["logPort"].isNull()) config.state.logPort = doc["logPort"].as<uint16_t>();
    if(!doc["LEDOn"].isNull()) config.state.LEDOn = doc["LEDOn"].as<bool>();
    if(!doc["pinLEDR"].isNull()) config.state.pinLEDR = doc["pinLEDR"].as<uint8_t>();
    if(!doc["pinLEDG"].isNull()) config.state.pinLEDG = doc["pinLEDG"].as<uint8_t>();
    if(!doc["pinLEDB"].isNull()) config.state.pinLEDB = doc["pinLEDB"].as<uint8_t>();
    if(!doc["pinBuzz"].isNull()) config.state.pinBuzz = doc["pinBuzz"].as<uint8_t>();
  }
  else{ // from config to doc
    doc[PSTR("batt")] = 100;
    doc[PSTR("ipad")] = WiFi.localIP().toString();
    doc[PSTR("compdate")] = COMPILED;
    doc[PSTR("fmTitle")] = CURRENT_FIRMWARE_TITLE;
    doc[PSTR("fmVersion")] = CURRENT_FIRMWARE_VERSION;
    doc[PSTR("stamac")] = WiFi.macAddress();
    doc[PSTR("apmac")] = WiFi.softAPmacAddress();
    doc[PSTR("flFree")] = ESP.getFreeSketchSpace();
    doc[PSTR("fwSize")] = ESP.getSketchSize();
    doc[PSTR("flSize")] = ESP.getFlashChipSize();
    doc[PSTR("dSize")] = LittleFS.totalBytes();
    doc[PSTR("dUsed")] = LittleFS.usedBytes();
    doc[PSTR("sdkVer")] = ESP.getSdkVersion();
    doc[PSTR("fInit")] = config.state.fInit;
    doc[PSTR("hwid")] = config.state.hwid;
    doc[PSTR("name")] = config.state.name;
    doc[PSTR("model")] = config.state.model;
    doc[PSTR("group")] = config.state.group;
    doc[PSTR("logLev")] = config.state.logLev;
    doc[PSTR("tbAddr")] = config.state.tbAddr;
    doc[PSTR("tbPort")] = config.state.tbPort;
    doc[PSTR("wssid")] = config.state.wssid;
    doc[PSTR("wpass")] = config.state.wpass;
    doc[PSTR("dssid")] = config.state.dssid;
    doc[PSTR("dpass")] = config.state.dpass;
    doc[PSTR("upass")] = config.state.upass;
    #ifdef USE_IOT
    doc[PSTR("fIoT")] = config.state.fIoT;
    doc[PSTR("accTkn")] = config.state.accTkn;
    doc[PSTR("provSent")] = config.state.provSent;
    doc[PSTR("provDK")] = config.state.provDK;
    doc[PSTR("provDS")] = config.state.provDS;
    #endif
    doc[PSTR("binURL")] = config.state.binURL;
    doc[PSTR("gmtOff")] = config.state.gmtOff;
    doc[PSTR("fWOTA")] = config.state.fWOTA;
    doc[PSTR("fWeb")] = config.state.fWeb;
    doc[PSTR("hname")] = config.state.hname;
    doc[PSTR("htU")] = config.state.htU;
    doc[PSTR("htP")] = config.state.htP;
    doc[PSTR("logIP")] = config.state.logIP;
    doc[PSTR("logPort")] = config.state.logPort;
    doc[PSTR("LEDOn")] = config.state.LEDOn;
    doc[PSTR("pinLEDR")] = config.state.pinLEDR;
    doc[PSTR("pinLEDG")] = config.state.pinLEDG;
    doc[PSTR("pinLEDB")] = config.state.pinLEDB;
    doc[PSTR("pinBuzz")] = config.state.pinBuzz;
  }
}