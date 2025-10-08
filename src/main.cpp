#include <ArduinoJson.h>
#include "main.h"
#include "set.h"

void setup(){
    loggingSetup();
    storageSetup();
    networkingSetup();
    coreroutineSetup();

    setCredentials();
    
    logger->debug(PSTR(__func__), PSTR("Hello world!\n"));
}

void loop(){
    coreroutineLoop();
}