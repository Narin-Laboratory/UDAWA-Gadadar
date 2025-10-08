#ifndef LOGGING_H
#define LOGGING_H

#include "main.h"

extern UdawaLogger *logger;
extern UdawaSerialLogger *serialLogger;
#ifdef USE_WIFI_LOGGER
extern UdawaWiFiLogger *wiFiLogger;
#endif

void loggingSetup();

#endif