#include <libudawaatmega328.h>
#include <Filters.h>

struct Settings
{
    uint8_t pinACS = 14;

    // test signal frequency (Hz)
    float testFreq = 50;
    // how long to average the signal, for statistic
    float windowLength = 40.0;

    // to be adjusted based on calibration testing
    float intercept = 0;
    // to be adjusted based on calibration testing
    // Please check the ACS712 Tutorial video by SurtrTech to see how to get them because it depends on your sensor, or look below
    float slope = 0.0752;

    float ampsTRMS;
    float ACSValue;
};

void getPowerUsage(StaticJsonDocument<DOCSIZE> &doc);

