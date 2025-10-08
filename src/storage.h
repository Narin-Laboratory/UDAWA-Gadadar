#ifndef STORAGE_H
#define STORAGE_H

#include "main.h"

struct CrashState{
    unsigned long rtcp = 0;
    int crashCnt = 0;
    bool fSafeMode = false;
    unsigned long crashStateCheckTimer = millis();
    bool crashStateCheckedFlag = false;
    unsigned long plannedRebootTimer = millis();
    unsigned int plannedRebootCountDown = 0;
    bool fPlannedReboot = false;
    bool fRTCHwDetected = false;
    unsigned long lastRecordedDatetime = 0;
    unsigned long lastRecordedDatetimeSavedTimer = 0;
    bool fFSDownloading = false;
    bool fStartServices = false;
    bool fStopServices = false;
    bool fDoInit = false;
};

// New structs based on prompt and old.h
struct TimerConfig {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    unsigned long duration; // Duration in seconds

    TimerConfig() : hour(0), minute(0), second(0), duration(0) {}
    TimerConfig(uint8_t h, uint8_t m, uint8_t s, uint16_t d) : hour(h), minute(m), second(s), duration(d) {}
};

struct Relay {
    uint8_t pin;
    uint8_t mode;
    uint16_t wattage;
    unsigned long lastActive;
    unsigned long lastChanged;

    uint8_t dutyCycle;
    unsigned long dutyRange;
    unsigned long autoOff;
    bool state;

    String label;
    uint16_t overrunInSec;

    TimerConfig timers[maxTimers];

    unsigned long datetime;
    unsigned long duration;

    // Default constructor
    Relay() : pin(0), mode(0), wattage(0), lastActive(0), lastChanged(0),
              dutyCycle(0), dutyRange(0), autoOff(0), state(false), label("No label"),
              overrunInSec(3600), datetime(0), duration(0) {
        for (uint8_t i = 0; i < maxTimers; i++) {
            timers[i] = TimerConfig();
        }
    }

    // Parameterized constructor
    Relay(uint8_t pin, uint8_t mode, uint16_t wattage,  unsigned long lastActive, unsigned long lastChanged,
        uint8_t dutyCycle, unsigned long dutyRange, unsigned long autoOff, bool state, String label,
        uint16_t overrunInSec, unsigned long datetime, unsigned long duration) : pin(pin),
        mode(mode), wattage(wattage), lastActive(lastActive), lastChanged(lastChanged), dutyCycle(dutyCycle),
        dutyRange(dutyRange), autoOff(autoOff), state(state), label(label), overrunInSec(overrunInSec),
        datetime(datetime), duration(duration) {
            for (uint8_t i = 0; i < maxTimers; i++) {
                timers[i] = TimerConfig();
            }
        }
};

struct AppConfig {
    uint8_t s1tx;
    uint8_t s1rx;
    unsigned long intvWeb;
    unsigned long intvAttr;
    unsigned long intvTele;
    int maxWatt;
    bool relayON;
};

struct AppState {
    bool fPowerSensor = false;
    bool fResetPowerSensor = false;
    bool fIOExtender = false;
    TaskHandle_t xHandlePowerSensor = NULL;
    BaseType_t xReturnedPowerSensor;
    unsigned long powerSensorTaskRoutineLastActivity = 0;
    TaskHandle_t xHandleRelayControl = NULL;
    BaseType_t xReturnedRelayControl;
    unsigned long relayControlTaskRoutineLastActivity = 0;
    bool fPanic = false;
    bool fsaveAppRelay = false;
    bool fSaveAppState = false;
    bool fsyncClientAttributes = false;

    // Transient state variables, moved from coreroutineLoop
    bool panic_action_taken = false;
    unsigned long lastWebBcast = 0;
    unsigned long lastAttrBcast = 0;
};

extern UdawaConfig config;
extern CrashState crashState;
extern GenericConfig crashStateConfig;

// New extern declarations
extern AppConfig appConfig;
extern AppState appState;
extern Relay relays[4];

extern GenericConfig appConfigGC;
extern GenericConfig appStateGC;
extern GenericConfig appRelaysGC;

void storageSetup();
void storageConvertAppConfig(JsonDocument &doc, bool direction, bool load_defaults = false);
void storageConvertAppState(JsonDocument &doc, bool direction, bool load_defaults = false);
void storageConvertAppRelay(JsonDocument &doc, bool direction, bool load_defaults = false);
void storageConvertUdawaConfig(JsonDocument &doc, bool direction, bool load_defaults = false);

#endif