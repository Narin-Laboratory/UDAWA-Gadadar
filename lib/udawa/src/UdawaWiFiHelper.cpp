#include "UdawaWiFiHelper.h"

UdawaWiFiHelper::UdawaWiFiHelper() {

}

void UdawaWiFiHelper::modeSTA(){
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.softAPdisconnect();
    WiFi.disconnect(true, true);
    WiFi.enableAP(false);
    WiFi.enableSTA(true);
    WiFi.mode(WIFI_MODE_STA);
    _logger->debug(PSTR(__func__), PSTR("STA %s started, trying to connect to AP: %s using password %s\n"), _hname, _wssid, _wpass);
    WiFi.setHostname(_hname);
    WiFi.setAutoReconnect(true);
    connectToStrongestAP();
}

void UdawaWiFiHelper::connectToStrongestAP() {
    _logger->debug(PSTR(__func__), PSTR("Scanning for strongest AP with SSID: %s\n"), _wssid);

    int n = WiFi.scanNetworks();
    if (n == 0) {
        _logger->warn(PSTR(__func__), PSTR("No networks found.\n"));
        _logger->debug(PSTR(__func__), PSTR("Falling back to default WiFi.begin()\n"));
        WiFi.begin(_wssid, _wpass);
        return;
    }

    _logger->debug(PSTR(__func__), PSTR("Found %d networks.\n"), n);

    int bestRSSI = -100;
    int bestNetworkIndex = -1;

    for (int i = 0; i < n; ++i) {
        if (WiFi.SSID(i) == _wssid) {
            _logger->debug(PSTR(__func__), PSTR("Found matching network %s with RSSI: %d\n"), WiFi.SSID(i).c_str(), WiFi.RSSI(i));
            if (WiFi.RSSI(i) > bestRSSI) {
                bestRSSI = WiFi.RSSI(i);
                bestNetworkIndex = i;
            }
        }
    }

    if (bestNetworkIndex != -1) {
        _logger->debug(PSTR(__func__), PSTR("Connecting to strongest AP: %s (BSSID: %s, RSSI: %d)\n"), WiFi.SSID(bestNetworkIndex).c_str(), WiFi.BSSIDstr(bestNetworkIndex).c_str(), WiFi.RSSI(bestNetworkIndex));
        WiFi.begin(_wssid, _wpass, 0, WiFi.BSSID(bestNetworkIndex));
    } else {
        _logger->warn(PSTR(__func__), PSTR("No AP with matching SSID found.\n"));
        _logger->debug(PSTR(__func__), PSTR("Falling back to default WiFi.begin()\n"));
        WiFi.begin(_wssid, _wpass);
    }
}

void UdawaWiFiHelper::modeAP(bool open){
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.disconnect(true, true);
    WiFi.enableSTA(false);
    WiFi.enableAP(true);
    WiFi.mode(WIFI_MODE_AP);
    if(open){
        WiFi.softAP(_hname);
    }else{  
        WiFi.softAP(_hname, _htP);
    }
    _logger->debug(PSTR(__func__), PSTR("SoftAP %s started with IP address: %s\n"), _hname, WiFi.softAPIP().toString().c_str());
    _state.softAPstartTime = millis();
    _state.softAPClientAvailCheckstartTime = millis();
    
    if(!_fInit){
        arduino_event_info_t info;
        onWiFiEvent(ARDUINO_EVENT_WIFI_AP_START, info);
    }
}

void UdawaWiFiHelper::setInitState (bool fInit){
    _fInit = fInit;
}

void UdawaWiFiHelper::begin(const char* wssid, const char* wpass,
    const char* dssid, const char* dpass, const char* hname, const char* htP){
    _wssid = wssid; 
    _wpass = wpass; 
    _dssid = dssid;
    _dpass = dpass;
    _hname = hname;
    _htP = htP;

    if(!_fInit){
        _logger->debug(PSTR(__func__), PSTR("Initializing WiFi network for the first time (AP Mode)...\n"));
        modeAP(true);
    }else{
        _logger->debug(PSTR(__func__), PSTR("Initializing WiFi network...\n"));
        modeSTA();
    }

    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info){
        this->onWiFiEvent(event, info);
    });
}

void UdawaWiFiHelper::addOnConnectedCallback(WiFiConnectedCallback callback) {

    _onConnectedCallbacks.push_back(callback);
}

void UdawaWiFiHelper::addOnDisconnectedCallback(WiFiDisconnectedCallback callback) {
    _onDisconnectedCallbacks.push_back(callback);
}

void UdawaWiFiHelper::addOnGotIPCallback(WiFiGotIPCallback callback) {
    _onGotIPCallbacks.push_back(callback);
}

void UdawaWiFiHelper::addOnAPNewClientIP(WiFiAPNewClientIPCallback callback){
    _onAPNewClientIPCallbacks.push_back(callback);
}

void UdawaWiFiHelper::addOnAPClientDisconnected(WiFiAPClientDisconnectedCallback callback){
    _onAPClientDisconnectedCallbacks.push_back(callback);
}

void UdawaWiFiHelper::addOnAPStart(WiFiAPStartCallback callback){
    _onAPStartCallbacks.push_back(callback);
}

int UdawaWiFiHelper::rssiToPercent(int rssi) {
  // Map RSSI to a percentage range
  // Adjust these values based on your environment and observations
  const int minRSSI = -120;  // Minimum expected RSSI
  const int maxRSSI = 0;   // Maximum expected RSSI

  // Clamp the RSSI value to the defined range
  rssi = constrain(rssi, minRSSI, maxRSSI);

  // Calculate the percentage
  int percentage = map(rssi, minRSSI, maxRSSI, 0, 100);
  return percentage;
}

void UdawaWiFiHelper::getAvailableWiFi(JsonDocument &doc){
    _logger->debug(PSTR(__func__), PSTR("Starting WiFi scanner...\n"));
    int num = WiFi.scanNetworks();
    _logger->debug(PSTR(__func__), PSTR("Scan finished. Found %d networks.\n"), num);

    JsonArray arr = doc.to<JsonArray>();
    for(int i = 0; i < num; i++){
        JsonObject obj = arr.add<JsonObject>();
        obj["ssid"] = WiFi.SSID(i);
        obj["rssi"] = rssiToPercent(WiFi.RSSI(i));
        _logger->debug(PSTR(__func__), PSTR("Found %s with signal strength %i\n"), WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }
}

void UdawaWiFiHelper::run(){
    unsigned long now = millis();
    if(now - _lastRun > 1000){
        if(_fInit && WiFi.getMode() == WIFI_MODE_AP){
            if(millis() - _state.softAPClientAvailCheckstartTime > _state.softAPClientAvailCheckTimeout * 1000){
                uint8_t connectedWiFiClientNum = WiFi.softAPgetStationNum();
                _logger->debug(PSTR(__func__), PSTR("Checking any connected WiFi client, found %d connected.\n"), connectedWiFiClientNum);
                if(connectedWiFiClientNum > 0){
                    _state.softAPstartTime = millis();
                }

                _state.softAPClientAvailCheckstartTime = millis();
            }
            

            if (millis() - _state.softAPstartTime > _state.softAPTimeout * 1000) {
                _logger->warn(PSTR(__func__), PSTR("SoftAP timedout. Switch back to STA mode.\n"));
                modeSTA();
                _state.softAPstartTime = millis();
            }
        }

        if(_state.STADisconnectCounter >= _state.STAMaximumDisconnectCount && WiFi.getMode() == WIFI_MODE_STA){
            _logger->warn(PSTR(__func__), PSTR("STA unable to connect %d times. Switch back to AP mode.\n"), _state.STADisconnectCounter);
            _state.STADisconnectCounter = 0;
            modeAP(false);
        }  

        if(_fInit && WiFi.getMode() == WIFI_MODE_STA && !_state.fSTAGotIP && _state.STADHCPFailedCounter > _state.STADHCPFailedTimedout){
            _state.STADHCPFailedCounter = 0;
            _logger->error(PSTR(__func__), PSTR("DHCP timedout! Trying to reconnect... \n"));
            WiFi.disconnect(true, true);
            modeSTA();
        }

        if(_fInit && WiFi.getMode() == WIFI_MODE_STA && !_state.fSTAGotIP){
            _state.STADHCPFailedCounter++;
            _logger->warn(PSTR(__func__), PSTR("Waiting for DHCP... %d/%d\n"), _state.STADHCPFailedCounter, _state.STADHCPFailedTimedout);
        }

        _lastRun = now;
    }  
}

void UdawaWiFiHelper::onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  // Implement your event handling logic here
  // For example, you can print event details
  if(event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED){
    if(WiFi.getMode() == WIFI_MODE_STA){
        _state.fSTAGotIP = false;
        _state.STADisconnectCounter++;
        _logger->debug(PSTR(__func__), PSTR("WiFi network %s disconnected %d/%d times!\n"), info.wifi_sta_connected.ssid, 
            _state.STADisconnectCounter, _state.STAMaximumDisconnectCount);
    }
    for (auto callback : _onDisconnectedCallbacks) { 
        callback(); // Call each callback
    }
  }
  else if(event == ARDUINO_EVENT_WIFI_STA_CONNECTED){
    _logger->debug(PSTR(__func__), PSTR("WiFi network %s connected!\n"), info.wifi_sta_connected.ssid);
    _state.STADisconnectCounter = 0;
    for (auto callback : _onConnectedCallbacks) { 
        callback(); // Call each callback
    }
  }
  else if(event == ARDUINO_EVENT_WIFI_STA_GOT_IP){
    _state.fSTAGotIP = true;
    _logger->debug(PSTR(__func__), PSTR("WiFi network got IP: %s.\n"), WiFi.localIP().toString().c_str());
    for (auto callback : _onGotIPCallbacks) { 
        callback(); // Call each callback
    }
  }
  else if(event == ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED){
    _logger->debug(PSTR(__func__), PSTR("WiFi AP new client IP released.\n"));
    for (auto callback : _onAPNewClientIPCallbacks) { 
        callback(); // Call each callback
    }
  }
  else if(event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED){
    _logger->debug(PSTR(__func__), PSTR("WiFi AP client disconnected.\n"));
    for (auto callback : _onAPClientDisconnectedCallbacks) { 
        callback(); // Call each callback
    }
  }
  else if(event == ARDUINO_EVENT_WIFI_AP_START){
    _logger->debug(PSTR(__func__), PSTR("WiFi AP started \n"));
    for (auto callback : _onAPStartCallbacks) { 
        callback(); // Call each callback
    }
  }
}