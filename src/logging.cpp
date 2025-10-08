#include "logging.h"

// Define the variables here (only once)
UdawaLogger *logger = UdawaLogger::getInstance(LogLevel::VERBOSE);
UdawaSerialLogger *serialLogger = UdawaSerialLogger::getInstance(SERIAL_BAUD_RATE);
#ifdef USE_WIFI_LOGGER
UdawaWiFiLogger *wiFiLogger = UdawaWiFiLogger::getInstance("255.255.255.255", 29514, 1024);
#endif

void loggingSetup(){
    logger->addLogger(serialLogger);
    logger->setLogLevel(LogLevel::VERBOSE);

    logger->info(PSTR(__func__), PSTR("\n\n--- UDAWA Smart System ---\r\nby PSTI Undiknas | udawa.or.id\r\n--------------------------\r\nFirmware: %s v%s\r\nCompiled: %s\r\n--------------------------\n\n"), CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, COMPILED);
}