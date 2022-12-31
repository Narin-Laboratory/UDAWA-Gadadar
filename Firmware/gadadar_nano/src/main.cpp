#include "main.h"

libudawaatmega328 nano;

Settings mySettings;
RunningStatistics inputStats;

void setup() {
  // put your setup code here, to run once:
  nano.begin();
  inputStats.setWindowSecs( mySettings.windowLength / mySettings.testFreq);
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
      if(doc["pinACS"] != nullptr){mySettings.pinACS = doc["pinACS"].as<uint8_t>();}
      if(doc["testFreq"] != nullptr){mySettings.testFreq = doc["testFreq"].as<float>();}
      if(doc["windowLength"] != nullptr){mySettings.windowLength = doc["windowLength"].as<float>();}
      if(doc["intercept"] != nullptr){mySettings.intercept = doc["intercept"].as<float>();}
      if(doc["slope"] != nullptr){mySettings.slope = doc["slope"].as<float>();}
    }
    else if(strcmp(method, (const char*) "getPowerUsage") == 0)
    {
      getPowerUsage(doc);
    }
  }
  doc.clear();

  // create statistics to look at the raw test signal
  mySettings.ACSValue = analogRead(mySettings.pinACS);
  // read the analog in value:
  // log to Stats function
  inputStats.input(mySettings.ACSValue);
}

void getPowerUsage(StaticJsonDocument<DOCSIZE> &doc)
{
  pinMode(mySettings.pinACS, INPUT);
  mySettings.ampsTRMS = mySettings.intercept + mySettings.slope * inputStats.sigma();
  doc["ampsTRMS"] = mySettings.ampsTRMS;
  doc["ACSValue"] = mySettings.ACSValue;
  nano.serialWriteToESP32(doc);
}