#include "UdawaLogger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

UdawaLogger *UdawaLogger::_logManager = nullptr;
ILogHandler::ILogHandler(){}
ILogHandler::ILogHandler(uint32_t baudRate){}

UdawaLogger *UdawaLogger::getInstance(const LogLevel log_level)
{
    if(_logManager == nullptr)
    {
        _logManager = new UdawaLogger(log_level);
    }
    return _logManager;
}

void UdawaLogger::dispatchMessage(const char *tag, const LogLevel level, const char *fmt, va_list args)
{
    if(_logLevel >= level)
    {
        for(auto& log_handler: _logHandlers)
        {
            log_handler->write(tag, level, fmt, args);
        }
    }
}

void UdawaLogger::addLogger(ILogHandler *log_handler)
{
    _logHandlers.push_back(log_handler);
}

void UdawaLogger::removeLogger(ILogHandler *log_handler)
{
    _logHandlers.remove(log_handler);
}

void UdawaLogger::verbose(const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    dispatchMessage(tag, LogLevel::VERBOSE, fmt, args);
    va_end(args);
}

void UdawaLogger::debug(const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    dispatchMessage(tag, LogLevel::DEBUG, fmt, args);
    va_end(args);
}

void UdawaLogger::info(const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    dispatchMessage(tag, LogLevel::INFO, fmt, args);
    va_end(args);
}

void UdawaLogger::warn(const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    dispatchMessage(tag, LogLevel::WARN, fmt, args);
    va_end(args);
}

void UdawaLogger::error(const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    dispatchMessage(tag, LogLevel::ERROR, fmt, args);
    va_end(args);
}

void UdawaLogger::setLogLevel(LogLevel level)
{
    _logLevel = level;
}