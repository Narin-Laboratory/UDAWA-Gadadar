// Header include.
#include "Arduino_ESP32_Updater.h"

#if defined(ESP32) && defined(ARDUINO)

// Library include.
#include <Update.h>

Arduino_ESP32_Updater::~Arduino_ESP32_Updater() {
    reset();
}

bool Arduino_ESP32_Updater::begin(size_t const & firmware_size) {
    return Update.begin(firmware_size);
}

size_t Arduino_ESP32_Updater::write(uint8_t * payload, size_t const & total_bytes) {
    return Update.write(payload, total_bytes);
}

void Arduino_ESP32_Updater::reset() {
    Update.abort();
}

bool Arduino_ESP32_Updater::end() {
    return Update.end();
}

#endif // defined(ESP32) && defined(ARDUINO)
