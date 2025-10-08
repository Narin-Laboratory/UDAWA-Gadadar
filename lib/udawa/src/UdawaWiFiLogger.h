#ifndef UDAWAWIFILOGGER_H
#define UDAWAWIFILOGGER_H

#include "UdawaLogger.h"
#include <WiFiUdp.h>

class UdawaWiFiLogger : public ILogHandler {
public:
    static UdawaWiFiLogger* getInstance(String hostIP, uint16_t port, uint16_t bufferSize);
    void write(const char* tag, const LogLevel level, const char* fmt, va_list args) override;
    void setConfig(String hostIP, uint16_t port, uint16_t bufferSize);

private:
    UdawaWiFiLogger(String hostIP, uint16_t port, uint16_t bufferSize); // Private constructor
    static UdawaWiFiLogger* _instance; // Singleton instance
    WiFiUDP udp;
    String _hostIP;
    uint16_t _port;
    uint16_t _bufferSize;
};

#endif