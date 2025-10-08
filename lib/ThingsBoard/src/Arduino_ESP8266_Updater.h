#ifndef Arduino_ESP8266_Updater_h
#define Arduino_ESP8266_Updater_h

// Local include.
#include "Configuration.h"

#if defined(ESP8266) && defined(ARDUINO)

// Local include.
#include "IUpdater.h"


/// @brief IUpdater implementation that uses the Arduino UpdaterClass (https://github.com/esp8266/Arduino/blob/master/cores/esp8266/Updater.h),
/// under the hood to write the given binary firmware data into flash memory so we can restart with newly received firmware
class Arduino_ESP8266_Updater : public IUpdater {
  public:
    ~Arduino_ESP8266_Updater() override;

    bool begin(size_t const & firmware_size) override;
  
    size_t write(uint8_t * payload, size_t const & total_bytes) override;

    void reset() override;
  
    bool end() override;
};

#endif // defined(ESP8266) && defined(ARDUINO)

#endif // Arduino_ESP8266_Updater_h
