#ifndef UDAWALOGGER_H
#define UDAWALOGGER_H

#include <Arduino.h>
#include <stdarg.h>
#include <list>

enum class LogLevel
{
    NONE,
    ERROR,
    WARN,
    INFO,
    DEBUG,
    VERBOSE,
};

class ILogHandler
{
    public:
        ILogHandler();
        ILogHandler(uint32_t baudRate);
        virtual void write(const char *tag, const LogLevel level, const char *fmt, va_list args) = 0;
};

class UdawaLogger
{
    public:
        void operator = (const UdawaLogger &) = delete;
        static UdawaLogger *getInstance(const LogLevel logLevel = LogLevel::VERBOSE);

        void addLogger(ILogHandler *logHandler);
        void removeLogger(ILogHandler *logHandler);
        void verbose(const char *tag, const char *fmt, ...);
        void debug(const char *tag, const char *fmt, ...);
        void info(const char *tag, const char *fmt, ...);
        void warn(const char *tag, const char *fmt, ...);
        void error(const char *tag, const char *fmt, ...);
        void setLogLevel(LogLevel level);
        
    private:
        UdawaLogger(const LogLevel logLevel) : _logLevel(logLevel){}
        void dispatchMessage(const char *tag, const LogLevel level, const char *fmt, va_list args);
        std::list<ILogHandler*> _logHandlers;
        static UdawaLogger *_logManager;
        LogLevel _logLevel;
};

class UdawaThingsboardLogger{
    public:
        static void log(const char *error){
            UdawaLogger *_logger = UdawaLogger::getInstance(LogLevel::VERBOSE);
            _logger->debug(PSTR(__func__), PSTR("%s\n"), error);
        }
        template<typename ...Args>
        static int printfln(char const * const format, Args const &... args){
            UdawaLogger *_logger = UdawaLogger::getInstance(LogLevel::VERBOSE);
            size_t len = strlen(format);
            char newFormat[len + 2];  // +2 for '\n' and null terminator
            strcpy(newFormat, format);
            strcat(newFormat, "\n");

            _logger->debug(PSTR(__func__), newFormat, args...);
            return 1U;
        }
        static int println(char const * const message){
            UdawaLogger *_logger = UdawaLogger::getInstance(LogLevel::VERBOSE);
            _logger->debug(PSTR(__func__), PSTR("%s\n"), message);
            return 1U;
        }
};

#endif