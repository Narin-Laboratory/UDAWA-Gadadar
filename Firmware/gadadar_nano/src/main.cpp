#include "main.h"

libudawaatmega328 nano;
Settings mySettings;

void setup() {
  // put your setup code here, to run once:
  nano.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  nano.execute();

  StaticJsonDocument<DOCSIZE> doc;
  doc = nano.serialReadFromESP32();
  if(doc["err"] == nullptr && doc["err"].as<int>() != 1)
  {
    const char* method = doc["method"].as<const char*>();
    if(strcmp(method, (const char*) "sStng") == 0)
    {

    }
  }
  doc.clear();
}