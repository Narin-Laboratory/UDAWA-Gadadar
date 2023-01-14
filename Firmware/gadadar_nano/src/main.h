#include <libudawaatmega328.h>
#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>

#define S1_RX 11
#define S1_TX 12

struct Settings
{

};


void getPowerUsage(StaticJsonDocument<DOCSIZE> &doc);
