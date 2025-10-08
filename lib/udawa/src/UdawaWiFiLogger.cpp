#include "UdawaWiFiLogger.h"

UdawaWiFiLogger* UdawaWiFiLogger::_instance = nullptr;

UdawaWiFiLogger::UdawaWiFiLogger(String hostIP, uint16_t port, uint16_t bufferSize) 
    : _hostIP(hostIP), _port(port), _bufferSize(bufferSize) {

}

UdawaWiFiLogger* UdawaWiFiLogger::getInstance(String hostIP, uint16_t port, uint16_t bufferSize) {
    if (_instance == nullptr) {
        _instance = new UdawaWiFiLogger(hostIP, port, bufferSize);
    }
    return _instance;
}

void UdawaWiFiLogger::write(const char* tag, const LogLevel level, const char* fmt, va_list args) {
    char buffer[_bufferSize];  // Adjust buffer size as needed
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    String message = String(esp_log_timestamp()) + " [" + String(tag) + "] " + buffer;
    udp.beginPacket(_hostIP.c_str(), _port);
    udp.write((const uint8_t*)message.c_str(), message.length());
    udp.endPacket();
}

void UdawaWiFiLogger::setConfig(String hostIP, uint16_t port, uint16_t bufferSize){
    _hostIP = hostIP;
    _port = port;
    _bufferSize = bufferSize;
}