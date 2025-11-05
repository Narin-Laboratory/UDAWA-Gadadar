#include "networking.h"

UdawaWiFiHelper wiFiHelper;

void networkingSetup(){
    JsonDocument doc;
    wiFiHelper.getAvailableWiFi(doc);
    File file = LittleFS.open("/WiFiList.json", FILE_WRITE);
    serializeJson(doc, file);
    file.close();

    wiFiHelper.addOnConnectedCallback(networkingOnWiFiConnected);
    wiFiHelper.addOnGotIPCallback(networkingOnWiFiGotIP);
    wiFiHelper.addOnDisconnectedCallback(networkingOnWiFiDisconnected);
    wiFiHelper.addOnAPNewClientIP(networkingOnWiFiAPNewClientIP);
    wiFiHelper.addOnAPStart(networkingOnWiFiAPStart);
    wiFiHelper.setInitState(config.state.fInit);
    wiFiHelper.begin(config.state.wssid, config.state.wpass, config.state.dssid, config.state.dpass, config.state.model, config.state.htP);
}

void networkingOnWiFiConnected(){
    coreroutineSetLEDBuzzer(2, 1, 3, 50);
}
void networkingOnWiFiDisconnected(){
    coreroutineSetLEDBuzzer(1, 1, 3, 50);
}
void networkingOnWiFiGotIP(){
    crashState.fStartServices = true;
    coreroutineSetLEDBuzzer(2, 1, 3, 50);
}
void networkingOnWiFiAPNewClientIP(){}
void networkingOnWiFiAPStart(){
    crashState.fDoInit = true;
    coreroutineSetLEDBuzzer(2, 1, 3, 50);
}