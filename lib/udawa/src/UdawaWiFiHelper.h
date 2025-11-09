#ifndef UDAWAWIFIHELPER_H
#define UDAWAWIFIHELPER_H

#include <Arduino.h>
#include <WiFi.h>
#include "UdawaLogger.h"
#include "UdawaSerialLogger.h"
#include <vector>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <params.h>

struct WiFiHelperState {
    unsigned long softAPstartTime = 0;
    unsigned long softAPClientAvailCheckstartTime = 0;
    unsigned long softAPTimeout = 300;
    unsigned long softAPClientAvailCheckTimeout = 5;
    int STADisconnectCounter = 0;
    int STAMaximumDisconnectCount = 30;
    bool fSTAGotIP = false;
    int STADHCPFailedCounter = 0;
    int STADHCPFailedTimedout = 10;
};

class UdawaWiFiHelper{
    public:
        UdawaWiFiHelper();
        typedef std::function<void()> WiFiConnectedCallback;
        typedef std::function<void()> WiFiDisconnectedCallback;
        typedef std::function<void()> WiFiGotIPCallback;
        typedef std::function<void()> WiFiAPNewClientIPCallback;
        typedef std::function<void()> WiFiAPClientDisconnectedCallback;
        typedef std::function<void()> WiFiAPStartCallback;
        void addOnConnectedCallback(WiFiConnectedCallback callback);
        void addOnDisconnectedCallback(WiFiDisconnectedCallback callback);
        void addOnGotIPCallback(WiFiGotIPCallback callback);
        void addOnAPNewClientIP(WiFiAPNewClientIPCallback callback);
        void addOnAPClientDisconnected(WiFiAPClientDisconnectedCallback callback);
        void addOnAPStart(WiFiAPStartCallback callback);
        void begin(const char* wssid, const char* wpass,
            const char* dssid, const char* dpass, const char* hname, const char* htP);
        void setInitState (bool fInit);
        void run();
        void getAvailableWiFi(JsonDocument &doc);
        int rssiToPercent(int rssi);
    private:
        WiFiHelperState _state;
        UdawaLogger *_logger = UdawaLogger::getInstance(LogLevel::VERBOSE);
        const char* _wssid;
        const char* _wpass;
        const char* _dssid;
        const char* _dpass;
        const char* _hname;
        const char* _htP;
        bool _fInit;
        void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
        std::vector<WiFiConnectedCallback> _onConnectedCallbacks;
        std::vector<WiFiDisconnectedCallback> _onDisconnectedCallbacks;
        std::vector<WiFiGotIPCallback> _onGotIPCallbacks;
        std::vector<WiFiAPNewClientIPCallback> _onAPNewClientIPCallbacks;
        std::vector<WiFiAPClientDisconnectedCallback> _onAPClientDisconnectedCallbacks;
        std::vector<WiFiAPStartCallback> _onAPStartCallbacks;
        void modeSTA();
        void modeAP(bool open = false);
        void connectToStrongestAP();
        unsigned long _lastRun;
};

#endif