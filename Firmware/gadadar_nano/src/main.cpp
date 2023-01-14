#include "main.h"

libudawaatmega328 nano;
SoftwareSerial pzemSWSerial(S1_RX, S1_TX);
PZEM004Tv30 PZEM(pzemSWSerial);
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
    if(strcmp(method, (const char*) "setSettings") == 0)
    {

    }
    else if(strcmp(method, (const char*) "getPowerUsage") == 0){
        getPowerUsage(doc);
    }
  }
  doc.clear();
}

void getPowerUsage(StaticJsonDocument<DOCSIZE> &doc){
  float volt = PZEM.voltage();
  if(volt > 0){
    doc["volt"] = PZEM.voltage();
    doc["amp"] = PZEM.current();
    doc["watt"] = PZEM.power();
    doc["ener"] = PZEM.energy();
    doc["freq"] = PZEM.frequency();
    doc["pf"] = PZEM.pf();

    nano.serialWriteToESP32(doc);
  }
  else{
    doc["err"] = 1;
  }
}