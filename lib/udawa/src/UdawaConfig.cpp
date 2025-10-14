#include "UdawaConfig.h"

SemaphoreHandle_t xSemaphoreConfig = NULL; 

UdawaConfig::UdawaConfig(const char* path) : _path(path){
    
}

bool UdawaConfig::begin(){
    if(xSemaphoreConfig == NULL){xSemaphoreConfig = xSemaphoreCreateMutex();}

    if( xSemaphoreConfig != NULL ){
      if( xSemaphoreTake( xSemaphoreConfig, ( TickType_t ) 5000 ) == pdTRUE )
      {
        if(!LittleFS.begin(true)){
            _logger->error(PSTR(__func__), PSTR("Problem with the LittleFS file system.\n"));
            
            xSemaphoreGive( xSemaphoreConfig );
            return false;
        }else{
            xSemaphoreGive( xSemaphoreConfig );
            return true;
        }
      }
      else
      {
        _logger->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
        return false;
      }
    }
    else{
        return false;
    }

    return true;
}

bool UdawaConfig::load(){
    if( xSemaphoreConfig != NULL ){
        if( xSemaphoreTake( xSemaphoreConfig, ( TickType_t ) 5000 ) == pdTRUE ){
            _logger->info(PSTR(__func__),PSTR("Loading %s.\n"), _path);
            File file = LittleFS.open(_path, FILE_READ);
            if(file.size() > 1)
            {
                _logger->info(PSTR(__func__),PSTR("%s size is normal: %d.\n"), _path, file.size());
            }
            else
            {
                _logger->warn(PSTR(__func__),PSTR("%s size is abnormal: %d!\n"), _path, file.size());
            }

            JsonDocument data;
            DeserializationError err = deserializeJson(data, file);

            if(err == DeserializationError::Ok){
                _logger->debug(PSTR(__func__), PSTR("%s is valid JSON.\n"), _path);
                
                if(data[PSTR("fInit")].is<bool>()){state.fInit = data[PSTR("fInit")].as<bool>();}
                if(data[PSTR("hwid")].is<const char*>()){strlcpy(state.hwid, data[PSTR("hwid")].as<const char*>(), sizeof(state.hwid));}
                if(data[PSTR("name")].is<const char*>()){strlcpy(state.name, data[PSTR("name")].as<const char*>(), sizeof(state.name));}
                if(data[PSTR("model")].is<const char*>()){strlcpy(state.model, data[PSTR("model")].as<const char*>(), sizeof(state.model));}
                if(data[PSTR("group")].is<const char*>()){strlcpy(state.group, data[PSTR("group")].as<const char*>(), sizeof(state.group));}
                if(data[PSTR("wssid")].is<const char*>()){strlcpy(state.wssid, data[PSTR("wssid")].as<const char*>(), sizeof(state.wssid));}
                if(data[PSTR("wpass")].is<const char*>()){strlcpy(state.wpass, data[PSTR("wpass")].as<const char*>(), sizeof(state.wpass));}
                if(data[PSTR("dssid")].is<const char*>()){strlcpy(state.dssid, data[PSTR("dssid")].as<const char*>(), sizeof(state.dssid));}
                if(data[PSTR("dpass")].is<const char*>()){strlcpy(state.dpass, data[PSTR("dpass")].as<const char*>(), sizeof(state.dpass));}
                if(data[PSTR("upass")].is<const char*>()){strlcpy(state.upass, data[PSTR("upass")].as<const char*>(), sizeof(state.upass));}
                if(data[PSTR("hname")].is<const char*>()){strlcpy(state.hname, data[PSTR("hname")].as<const char*>(), sizeof(state.hname));}
                if(data[PSTR("htU")].is<const char*>()){strlcpy(state.htU, data[PSTR("htU")].as<const char*>(), sizeof(state.htU));}
                if(data[PSTR("htP")].is<const char*>()){strlcpy(state.htP, data[PSTR("htP")].as<const char*>(), sizeof(state.htP));}
                if(data[PSTR("logIP")].is<const char*>()){strlcpy(state.logIP, data[PSTR("logIP")].as<const char*>(), sizeof(state.logIP));}
                if(data[PSTR("logLev")].is<uint8_t>()){state.logLev = data[PSTR("logLev")].as<uint8_t>();}                
                if(data[PSTR("fWOTA")].is<bool>()){state.fWOTA = data[PSTR("fWOTA")].as<bool>();}
                if(data[PSTR("fWeb")].is<bool>()){state.fWeb = data[PSTR("fWeb")].as<bool>();}
                if(data[PSTR("gmtOff")].is<int>()){state.gmtOff = data[PSTR("gmtOff")].as<int>();}
                if(data[PSTR("logPort")].is<uint16_t>()){state.logPort = data[PSTR("logPort")].as<uint16_t>();}
                if(data[PSTR("LEDOn")].is<bool>()){state.LEDOn = data[PSTR("LEDOn")].as<bool>();}
                if(data[PSTR("pinLEDR")].is<uint8_t>()){state.pinLEDR = data[PSTR("pinLEDR")].as<uint8_t>();}
                if(data[PSTR("pinLEDG")].is<uint8_t>()){state.pinLEDG = data[PSTR("pinLEDG")].as<uint8_t>();}
                if(data[PSTR("pinLEDB")].is<uint8_t>()){state.pinLEDB = data[PSTR("pinLEDB")].as<uint8_t>();}
                if(data[PSTR("pinBuzz")].is<uint8_t>()){state.pinBuzz = data[PSTR("pinBuzz")].as<uint8_t>();}
                
                #ifdef USE_IOT
                if(data[PSTR("accTkn")].is<const char*>()){strlcpy(state.accTkn, data[PSTR("accTkn")].as<const char*>(), sizeof(state.accTkn));}
                if(data[PSTR("provDK")].is<const char*>()){strlcpy(state.provDK, data[PSTR("provDK")].as<const char*>(), sizeof(state.provDK));}
                if(data[PSTR("provDS")].is<const char*>()){strlcpy(state.provDS, data[PSTR("provDS")].as<const char*>(), sizeof(state.provDS));}
                if(data[PSTR("tbPort")].is<uint16_t>()){state.tbPort = data[PSTR("tbPort")].as<uint16_t>();}
                if(data[PSTR("provSent")].is<bool>()){state.provSent = data[PSTR("provSent")].as<bool>();}
                if(data[PSTR("fIoT")].is<bool>()){state.fIoT = data[PSTR("fIoT")].as<bool>();}
                if(data[PSTR("tbAddr")].is<const char*>()){strlcpy(state.tbAddr, data[PSTR("tbAddr")].as<const char*>(), sizeof(state.tbAddr));}
                #endif
                if(data[PSTR("binURL")].is<const char*>()){strlcpy(state.binURL, data[PSTR("binURL")].as<const char*>(), sizeof(state.binURL));}

                #ifdef USE_CO_MCU
                if(data[PSTR("s2rx")].is<uint8_t>()){state.s2rx = data[PSTR("s2rx")].as<uint8_t>();}
                if(data[PSTR("s2tx")].is<uint8_t>()){state.s2tx = data[PSTR("s2tx")].as<uint8_t>();}
                if(data[PSTR("coMCUBuzzFreq")].is<uint16_t>()){state.coMCUBuzzFreq = data[PSTR("coMCUBuzzFreq")].as<uint16_t>();}
                if(data[PSTR("coMCUFBuzzer")].is<bool>()){state.coMCUFBuzzer = data[PSTR("coMCUFBuzzer")].as<bool>();}
                if(data[PSTR("coMCUPinBuzzer")].is<uint8_t>()){state.coMCUPinBuzzer = data[PSTR("coMCUPinBuzzer")].as<uint8_t>();}
                if(data[PSTR("coMCUPinLEDR")].is<uint8_t>()){state.coMCUPinLEDR = data[PSTR("coMCUPinLEDR")].as<uint8_t>();}
                if(data[PSTR("coMCUPinLEDG")].is<uint8_t>()){state.coMCUPinLEDG = data[PSTR("coMCUPinLEDG")].as<uint8_t>();}
                if(data[PSTR("coMCUPinLEDB")].is<uint8_t>()){state.coMCUPinLEDB = data[PSTR("coMCUPinLEDB")].as<uint8_t>();}
                if(data[PSTR("coMCULON")].is<uint8_t>()){state.coMCULON = data[PSTR("coMCULON")].as<uint8_t>();}
                if (data[PSTR("coMCURelayPin")].is<JsonArray>()) {
                    JsonArray arr = data[PSTR("coMCURelayPin")].as<JsonArray>();
                    for (size_t i = 0; i < arr.size() && i < 4; ++i) {
                        state.coMCURelayPin[i] = arr[i].as<uint8_t>();
                    }
                }
                #endif

            }
            else{
                _logger->error(PSTR(__func__), PSTR("%s is not valid JSON!\n"), _path);
                _logger->debug(PSTR(__func__), PSTR("Trying to load factory config from secret.h & params.h.\n"));

                char* decodedString = new char[16];
                uint64_t chipid = ESP.getEfuseMac();
                sprintf(decodedString, "%04X%08X",(uint16_t)(chipid>>32), (uint32_t)chipid);

                strlcpy(state.hwid, decodedString, sizeof(state.hwid));
                strlcpy(state.name, (String(model) + String(decodedString)).c_str(), sizeof(state.name));
                strlcpy(state.model, model, sizeof(state.model));
                strlcpy(state.group, group, sizeof(state.group));
                strlcpy(state.wssid, wssid, sizeof(state.wssid));
                strlcpy(state.wpass, wpass, sizeof(state.wpass));
                strlcpy(state.dssid, dssid, sizeof(state.dssid));
                strlcpy(state.dpass, dpass, sizeof(state.dpass));
                strlcpy(state.upass, upass, sizeof(state.upass));
                strlcpy(state.hname, hname, sizeof(state.hname));
                strlcpy(state.logIP, DEFAULT_LOG_IP, sizeof(state.logIP));
                strlcpy(state.htU, htU, sizeof(state.htU));
                strlcpy(state.htP, htP, sizeof(state.htP));
                state.logLev = logLev;
                state.fWOTA = fWOTA;
                state.fWeb = fWeb;
                state.gmtOff = gmtOff;
                state.logPort = logPort;
                state.fInit = fInit;
                state.LEDOn = LEDOn;
                state.pinLEDR = pinLEDR;
                state.pinLEDG = pinLEDG;
                state.pinLEDB = pinLEDB;
                state.pinBuzz = pinBuzz;

                #ifdef USE_IOT
                strlcpy(state.accTkn, accTkn, sizeof(accTkn));
                strlcpy(state.provDK, provDK, sizeof(provDK));
                strlcpy(state.provDS, provDS, sizeof(provDS));
                state.provSent = false;
                state.fIoT = fIoT;
                strlcpy(state.tbAddr, tbAddr, sizeof(tbAddr));
                state.tbPort = tbPort;
                #endif
                strlcpy(state.binURL, binURL, sizeof(binURL));

                #ifdef USE_CO_MCU
                state.s2rx = s2rx;
                state.s2tx = s2tx;
                state.coMCUBuzzFreq = 1600;
                state.coMCUFBuzzer = true;
                state.coMCUPinBuzzer = 2;
                state.coMCUPinLEDR = coMCUPinLEDR;
                state.coMCUPinLEDG = coMCUPinLEDG;
                state.coMCUPinLEDB = coMCUPinLEDB;
                state.coMCULON = coMCULON;
                for (int i = 0; i < 4; ++i) state.coMCURelayPin[i] = 7+i;
                #endif

                file.close();
                xSemaphoreGive( xSemaphoreConfig );
                return false;
            }
            file.close();
            xSemaphoreGive( xSemaphoreConfig );
            return true;
        }
        else{
            _logger->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
        }
    }
    return false;
}

bool UdawaConfig::save(){
    if( xSemaphoreConfig != NULL ){
      if( xSemaphoreTake( xSemaphoreConfig, ( TickType_t ) 5000 ) == pdTRUE )
      {
        if(!LittleFS.remove(_path)){
            _logger->warn(PSTR(__func__),PSTR("Failed to delete the old file: %s\n"), _path);
        }
        
        File file = LittleFS.open(_path, FILE_WRITE);
        
        if (!file){
            _logger->error(PSTR(__func__),PSTR("Failed to open the old file: %s\n"), _path);
            file.close();
            xSemaphoreGive( xSemaphoreConfig );
            return false;
        }

        JsonDocument data;
        data[PSTR("fInit")] = state.fInit;
        data[PSTR("hwid")] = state.hwid;
        data[PSTR("name")] = state.name;
        data[PSTR("model")] = state.model;
        data[PSTR("group")] = state.group;
        data[PSTR("logLev")] = state.logLev;
        data[PSTR("wssid")] = state.wssid;
        data[PSTR("wpass")] = state.wpass;
        data[PSTR("dssid")] = state.dssid;
        data[PSTR("dpass")] = state.dpass;
        data[PSTR("upass")] = state.upass;
        #ifdef USE_IOT
        data[PSTR("accTkn")] = state.accTkn;
        data[PSTR("provSent")] = state.provSent;
        data[PSTR("provDK")] = state.provDK;
        data[PSTR("provDS")] = state.provDS;
        data[PSTR("fIoT")] = state.fIoT;
        data[PSTR("tbAddr")] = state.tbAddr;
        data[PSTR("tbPort")] = state.tbPort;
        #endif
        data[PSTR("binURL")] = state.binURL;
        data[PSTR("gmtOff")] = state.gmtOff;
        data[PSTR("fWOTA")] = state.fWOTA;
        data[PSTR("fWeb")] = state.fWeb;
        data[PSTR("hname")] = state.hname;
        data[PSTR("htU")] = state.htU;
        data[PSTR("htP")] = state.htP;
        data[PSTR("logIP")] = state.logIP;
        data[PSTR("logPort")] = state.logPort;
        data[PSTR("LEDOn")] = state.LEDOn;
        data[PSTR("pinLEDR")] = state.pinLEDR;
        data[PSTR("pinLEDG")] = state.pinLEDG;
        data[PSTR("pinLEDB")] = state.pinLEDB;
        data[PSTR("pinBuzz")] = state.pinBuzz;

        #ifdef USE_CO_MCU
        data[PSTR("s2rx")] = state.s2rx;
        data[PSTR("s2tx")] = state.s2tx;
        data[PSTR("coMCUBuzzFreq")] = state.coMCUBuzzFreq;
        data[PSTR("coMCUFBuzzer")] = state.coMCUFBuzzer;
        data[PSTR("coMCUPinBuzzer")] = state.coMCUPinBuzzer;
        data[PSTR("coMCUPinLEDR")] = state.coMCUPinLEDR;
        data[PSTR("coMCUPinLEDG")] = state.coMCUPinLEDG;
        data[PSTR("coMCUPinLEDB")] = state.coMCUPinLEDB;
        data[PSTR("coMCULON")] = state.coMCULON;
        for (int i = 0; i < 4; ++i) {
            data[PSTR("coMCURelayPin")][i] = state.coMCURelayPin[i];
        }
        #endif

        serializeJson(data, file);

        _logger->debug(PSTR(__func__),PSTR("%s saved successfully.\n"), _path);
        file.close();        
        xSemaphoreGive( xSemaphoreConfig );
        return true;
      }
      else
      {
        _logger->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
      }
    }

    return false;
}

GenericConfig::GenericConfig(const char* path) : _path(path) {

}

bool GenericConfig::load(JsonDocument &data){
    if( xSemaphoreConfig != NULL ){
        if( xSemaphoreTake( xSemaphoreConfig, ( TickType_t ) 5000 ) == pdTRUE ){
            _logger->info(PSTR(__func__),PSTR("Loading %s.\n"), _path);
            File file = LittleFS.open(_path, FILE_READ);
            if(file.size() > 1)
            {
                _logger->info(PSTR(__func__),PSTR("%s size is normal: %d.\n"), _path, file.size());
            }
            else
            {
                _logger->warn(PSTR(__func__),PSTR("%s size is abnormal: %d!\n"), _path, file.size());
                file.close();
                xSemaphoreGive( xSemaphoreConfig );
                return false;
            }

            DeserializationError err = deserializeJson(data, file);

            if(err == DeserializationError::Ok){
                _logger->debug(PSTR(__func__), PSTR("%s is valid JSON.\n"), _path);                
            }
            
            file.close();
            xSemaphoreGive( xSemaphoreConfig );
            return true;
        }
        else{
            _logger->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
        }
    }
    return false;
}

bool GenericConfig::save(JsonDocument &data){
    if( xSemaphoreConfig != NULL ){
      if( xSemaphoreTake( xSemaphoreConfig, ( TickType_t ) 5000 ) == pdTRUE )
      {
        if(!LittleFS.remove(_path)){
            _logger->warn(PSTR(__func__),PSTR("Failed to delete the old file: %s\n"), _path);
        }
        
        File file = LittleFS.open(_path, FILE_WRITE);
        
        if (!file){
            _logger->error(PSTR(__func__),PSTR("Failed to open the old file: %s\n"), _path);
            file.close();
            xSemaphoreGive( xSemaphoreConfig );
            return false;
        }

        serializeJson(data, file);

        _logger->debug(PSTR(__func__),PSTR("%s saved successfully.\n"), _path);

        file.close();        
        xSemaphoreGive( xSemaphoreConfig );
        return true;
      }
      else
      {
        _logger->verbose(PSTR(__func__), PSTR("No semaphore available.\n"));
      }
    }

    return false;
}