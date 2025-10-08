#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "UdawaSerialLogger.h"
#include <Arduino.h>

UdawaSerialLogger *UdawaSerialLogger::_serialLogger = nullptr;

UdawaSerialLogger::UdawaSerialLogger(u_int32_t baudRate) : _baudRate(baudRate) {
    Serial.begin(_baudRate);
}

UdawaSerialLogger *UdawaSerialLogger::getInstance(const uint32_t baudRate)
{
    if(_serialLogger == nullptr)
    {
        _serialLogger = new UdawaSerialLogger(baudRate);
    }
    return _serialLogger;
}

char UdawaSerialLogger::_getErrorChar(const LogLevel level)
{
    switch(level)
    {
        case LogLevel::ERROR:
            return 'E';
        case LogLevel::WARN:
            return 'W';
        case LogLevel::INFO:
            return 'I';
        case LogLevel::DEBUG:
            return 'D';
        case LogLevel::VERBOSE:
            return 'V';
        default:
            return 'X';
    }
}

int UdawaSerialLogger::_getConsoleColorCode(const LogLevel level)
{
    switch(level)
    {
        case LogLevel::ERROR:
            return RED_COLOR_CODE;
        case LogLevel::WARN:
            return YELLOW_COLOR_CODE;
        case LogLevel::INFO:
            return GREEN_COLOR_CODE;
        case LogLevel::DEBUG:
            return CYAN_COLOR_CODE;
        case LogLevel::VERBOSE:
            return MAGENTA_COLOR_CODE;
        default:
            return GREEN_COLOR_CODE;
    }
}

int UdawaSerialLogger::_mapLogLevel(const LogLevel level)
{
    switch(level)
    {
        case LogLevel::NONE:
            return ESP_LOG_NONE;
        case LogLevel::ERROR:
            return ESP_LOG_ERROR;
        case LogLevel::WARN:
            return ESP_LOG_WARN;
        case LogLevel::INFO:
            return ESP_LOG_INFO;
        case LogLevel::DEBUG:
            return ESP_LOG_DEBUG;
        case LogLevel::VERBOSE:
            return ESP_LOG_VERBOSE;
        default:
            return ESP_LOG_VERBOSE;
    }
}

void UdawaSerialLogger::write(const char *tag, LogLevel level, const char *fmt, va_list args)
{
    if(xSemaphoreUdawaSerialLogger == NULL)
    {
        xSemaphoreUdawaSerialLogger = xSemaphoreCreateMutex();
    }

    if(xSemaphoreUdawaSerialLogger != NULL && xSemaphoreTake(xSemaphoreUdawaSerialLogger, (TickType_t) 20))
    {
        // 1. Create a sufficiently large buffer on the stack.
        //    1024 bytes should be safe for most log messages.
        char log_buffer[1024];

        // 2. Format the log message prefix into the buffer.
        //    snprintf is safe and prevents overflows.
        int prefix_len = snprintf(log_buffer, sizeof(log_buffer), "\033[0;%dm%c (%lu) %s: ", 
                                  _getConsoleColorCode(level), 
                                  _getErrorChar(level), 
                                  (unsigned long)esp_log_timestamp(), 
                                  tag);

        // 3. Format the actual message content onto the end of the prefix.
        //    Check if there's space before writing.
        if (prefix_len < sizeof(log_buffer)) {
            vsnprintf(log_buffer + prefix_len, sizeof(log_buffer) - prefix_len, fmt, args);
        }

        // 4. Print the complete, formatted buffer to the Serial port.
        Serial.print(log_buffer);
        
        // 5. Print the color reset code.
        Serial.print("\033[0m");

        xSemaphoreGive(xSemaphoreUdawaSerialLogger);
    }
}