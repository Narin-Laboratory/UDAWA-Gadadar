#ifndef UDAWASERIALLOGGER_H
#define UDAWASERIALLOGGER_H

#include <stdarg.h>
#include "UdawaLogger.h"
#include "UdawaSerialLogger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#define RED_COLOR_CODE 31
#define GREEN_COLOR_CODE 32
#define YELLOW_COLOR_CODE 33
#define MAGENTA_COLOR_CODE 34
#define CYAN_COLOR_CODE 35
#define WHITE_COLOR_CODE 37

static SemaphoreHandle_t xSemaphoreUdawaSerialLogger = NULL;

class UdawaSerialLogger : public ILogHandler
{
    public:
        UdawaSerialLogger(u_int32_t baudRate);
        static UdawaSerialLogger *getInstance(const uint32_t baudRate = 115200);
        void write(const char *tag, const LogLevel level, const char *fmt, va_list args) override;
    private:
        char _getErrorChar(const LogLevel level);
        int _getConsoleColorCode(const LogLevel level);
        int _mapLogLevel(const LogLevel level);
        uint32_t _baudRate;
        static UdawaSerialLogger *_serialLogger;
};



#endif