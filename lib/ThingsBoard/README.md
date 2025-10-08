# Client SDK to connect with ThingsBoard IoT Platform from various IoT devices (Arduino, Espressif, etc.)

[![MIT license](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](https://lbesson.mit-license.org/)
[![ESP32](https://img.shields.io/badge/ESP-32-green.svg?style=flat-square)](https://www.espressif.com/en/products/socs/esp32)
[![ESP8266](https://img.shields.io/badge/ESP-8266-blue.svg?style=flat-square)](https://www.espressif.com/en/products/socs/esp8266)
[![GitHub release](https://img.shields.io/github/release/thingsboard/thingsboard-client-sdk/all.svg?style=flat-square)](https://github.com/thingsboard/thingsboard-client-sdk/releases/)
[![GitHub downloads](https://img.shields.io/github/downloads/thingsboard/thingsboard-client-sdk/all.svg?style=flat-square)](https://github.com/thingsboard/thingsboard-client-sdk/releases/)
[![Arduino actions status](https://github.com/thingsboard/thingsboard-client-sdk/actions/workflows/arduino-compile.yml/badge.svg)](https://github.com/thingsboard/thingsboard-client-sdk/actions/workflows/arduino-compile.yml)
[![Espressif IDF v4.4 actions status](https://github.com/thingsboard/thingsboard-client-sdk/actions/workflows/espidf-compile-v4.4.yml/badge.svg)](https://github.com/thingsboard/thingsboard-client-sdk/actions/workflows/espidf-compile-v4.4.yml)
[![Espressif IDF v5.1 actions status](https://github.com/thingsboard/thingsboard-client-sdk/actions/workflows/espidf-compile-v5.1.yml/badge.svg)](https://github.com/thingsboard/thingsboard-client-sdk/actions/workflows/espidf-compile-v5.1.yml)
[![ESP32 actions status](https://github.com/thingsboard/thingsboard-client-sdk/actions/workflows/esp32-compile.yml/badge.svg)](https://github.com/thingsboard/thingsboard-client-sdk/actions/workflows/esp32-compile.yml)
[![ESP8266 actions status](https://github.com/thingsboard/thingsboard-client-sdk/actions/workflows/esp8266-compile.yml/badge.svg)](https://github.com/thingsboard/thingsboard-client-sdk/actions/workflows/esp8266-compile.yml)
![GitHub stars](https://img.shields.io/github/stars/thingsboard/thingsboard-client-sdk?style=social)

This library provides access to the ThingsBoard platform over the `MQTT` protocol.

## Examples

The SDK comes with a number of example sketches. See **Files --> Examples --> ThingsBoard** within the Arduino application.
Please review the complete guide for `ESP32` Pico Kit `GPIO` control and `DHT22` sensor monitoring available [here](https://thingsboard.io/docs/samples/esp32/gpio-control-pico-kit-dht22-sensor/).

## Supported Frameworks

`ThingsBoard` does not directly depend on any specific `MQTT Client` implementation, instead any implementation of the `IMQTT_Client` can be used. Because there are no further dependencies on `Arduino`, besides the client that communicates it allows us to use this library with `Arduino`, when using the `Arduino_MQTT_Client` or with `Espressif IDF` when using the `Espressif_MQTT_Client`.

Example usage for `Espressif` can be found in the `examples/0014-espressif_esp32_send_data` folder, all other code portions can be implemented the same way only initialization of the needed dependencies is slightly different. Meaning internal call to `ThingsBoard` works the same on both `Espressif` and `Arduino`.

This is also the case, because the only always used dependency that is remaining, is [`ArduinoJson`](https://arduinojson.org/), which despite its name does not require any `Arduino` component. _Please Note:_ you must use `v7.x.x` of this library as `v6.x.x` is not supported anymore.

## Installation

This project can be built with either [PlatformIO](https://platformio.org/), [`ESP IDF Extension`](https://www.espressif.com/) or [Arduino IDE](https://www.arduino.cc/en/software).

The project can be found in the [PlatformIO Registry](https://registry.platformio.org/libraries/thingsboard/ThingsBoard), [ESP Component registry](https://components.espressif.com/components/thingsboard/thingsboard) or the [Arduino libraries](https://www.arduino.cc/reference/en/libraries/thingsboard/).

A description on how to include the library in you project can be found below for each of the aforementioned possible methods of integrating the project.

#### PlatformIO

To add an external library, the most important portion is the [`lib_deps`](https://docs.platformio.org/en/latest/projectconf/sections/env/options/library/lib_deps.html) specification, simply add `thingsboard/ThingsBoard`.
There are multiple ways to define the version that should be fetched, but the most basic is simply getting the last released version, with the aforementioned line, to learn more see [Package Specifications](https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_install.html#package-specifications).

```
lib_deps=
    thingsboard/ThingsBoard
```

#### ESP IDF Extension

To add an external library, what needs to be done differs between versions. If an [ESP-IDF](https://github.com/espressif/esp-idf) version after and including `v3.2.0` is used
then the project can simply be added over the [Component Manager](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-component-manager.html).

To do that we can simply call `idf.py add-dependency <DEPENDENCY>`, with the name of the dependency as an argument. Similar to `PlatformIO` there are a multiple way to define the version that should be fetched, but the method below is the most basic to simply get the last released version, to learn more see [Using Component Manager with a Project](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-component-manager.html#using-with-a-project).

```
idf.py add-dependency "thingsboard/ThingsBoard"
```

If an [ESP-IDF](https://github.com/espressif/esp-idf) version prior to `v3.2.0` is used then the component has to be added as a `git submodule`.
Meaning the repository has to first be a `git` project, if that is not the case already simply install `git` and call `git init` in the folder containing your project.

Similar to the other call there are a multiple way to define the version that should be fetched, but the method below is the most basic to simply get the last released version, to learn more see [Git Submodule Help page](https://git-scm.com/docs/git-submodule).

```
git submodule add https://github.com/thingsboard/thingsboard-client-sdk.git components/ThingsBoard
```

#### Arduino IDE

To add an external library, we simply have to open `Tools` -> `Manage Libraries` and then search for `ThingsBoard` then press the `install` button for the wanted version. See [how to install library on Arduino IDE](https://arduinogetstarted.com/faq/how-to-install-library-on-arduino-ide) for more detailed information and some troubleshooting if the aforementioned method does not work.


## Dependencies

Following dependencies are installed automatically or must be installed, too:

**Installed automatically:**
 - [ArduinoJSON](https://github.com/bblanchon/ArduinoJson) — needed for dealing with the `JSON` payload that is sent to and received by `ThingsBoard`. _Please Note:_ you must use `v7.x.x` of this library as `v6.x.x` is not supported anymore.
 - [MQTT PubSub Client](https://github.com/thingsboard/pubsubclient) — for interacting with `MQTT`, when using the `Arduino_MQTT_Client` instance as an argument to `ThingsBoard`. Only installed if this library is used over `Arduino IDE` or `PlatformIO` with the Arduino framework.

**Needs to be installed manually:**
 - [MbedTLS Library](https://github.com/Seeed-Studio/Seeed_Arduino_mbedtls) — needed to create hashes for the OTA update for non `Espressif` boards.
 - [Arduino Timer](https://github.com/contrem/arduino-timer) - needed to create non-blocking callback timers for non `Espressif` boards.
 - [WiFiEsp Client](https://github.com/bportaluri/WiFiEsp) — needed when using a `Arduino Uno` with a `ESP8266`.
 - [StreamUtils](https://github.com/bblanchon/StreamUtils) — needed when sending arbitrary amount of payload even if the buffer size is too small to hold that complete payload is wanted, aforementioned feature is automatically enabled if the library is installed.

## Supported ThingsBoard Features

Example implementations for all base features, mentioned above, can be found in the `examples` folder. See the `README.md` in each example folder, to see which boards are supported and which functionality the example shows.

### Over `MQTT`:

All possible features are implemented over `MQTT` over a specific `IAPI_Implementation` instance:

 - [Telemetry data upload](https://thingsboard.io/docs/reference/mqtt-api/#telemetry-upload-api) / `ThingsBoard`
 - [Device attribute publish](https://thingsboard.io/docs/reference/mqtt-api/#publish-attribute-update-to-the-server) / `ThingsBoard`
 - [Server-side RPC](https://thingsboard.io/docs/reference/mqtt-api/#server-side-rpc) / `Server_Side_RPC`
 - [Client-side RPC](https://thingsboard.io/docs/reference/mqtt-api/#client-side-rpc) / `Client_Side_RPC`
 - [Request attribute values](https://thingsboard.io/docs/reference/mqtt-api/#request-attribute-values-from-the-server) / `Attribute_Request`
 - [Attribute update subscription](https://thingsboard.io/docs/reference/mqtt-api/#subscribe-to-attribute-updates-from-the-server) / `Shared_Attribute_Update`
 - [Device provisioning](https://thingsboard.io/docs/reference/mqtt-api/#device-provisioning) / `Provision`
 - [Device claiming](https://thingsboard.io/docs/reference/mqtt-api/#claiming-devices) / `ThingsBoard`
 - [Firmware OTA update](https://thingsboard.io/docs/reference/mqtt-api/#firmware-api) / `OTA_Firmware_Update`

## Troubleshooting

This troubleshooting guide contains common issues that are well known and can occur if the library is used wrongly. Ensure to read this section before creating a new `GitHub Issue`.

### Enabling internal debug messages

If the device is causing problems that are not already described in more detail below, it might be useful to enable internal debug messages, which will allow the library to print more information about sent and received messages as well as internal processes. This is disabled per default to decrease the amount of logs and memory for the log strings on the flash.

```cpp
// If not set the value is 0 per default, meaning it will only print internal error messages,
// set to 1 to also print debugging messages which might help to discern the exact place where a method fails
#define THINGSBOARD_ENABLE_DEBUG 1
#include <ThingsBoard.h>
```

### Not enough space for JSON serialization

The buffer size for the underlying MQTT client might be too small to send or receive data. If that is the case the SDK will not send data if the size of it is bigger than the configured internal buffer size. Respective logs in the `"Serial Monitor"` window will indicate the condition:

```
[TB] Send buffer size (256) to small for the given payloads size (342), increase with Set_Buffer_Size accordingly or install the StreamUtils library
```

If that's the case, the buffer size for serialization should be increased. To do so, `Set_Buffer_Size()` method can be used on the `ThingsBoard` instance, which will then forward that call to the underlying client implementation.

```cpp
// Initialize underlying client, used to establish a connection
WiFiClient espClient;

// Initalize the Mqtt client instance
Arduino_MQTT_Client mqttClient(espClient);

// The SDK setup
ThingsBoard tb(mqttClient);

void setup() {
  // Increase internal buffer size after inital creation.
  tb.Set_Buffer_Size(512, 512);
}
```

Alternatively, it is possible to enable the mentioned `THINGSBOARD_ENABLE_STREAM_UTILS` option, which sends messages that are bigger than the given buffer size with a method that skips the internal buffer, be aware tough this only works for sent messages. The internal buffer size still has to be big enough to receive the biggest possible message received by the client that is sent by the server.

For that the only thing that needs to be done is to install the required `StreamUtils` library, see the [Dependencies](https://github.com/thingsboard/thingsboard-client-sdk?tab=readme-ov-file#dependencies) section.

## Tips and Tricks

### Custom API Implementation Instance

The `ThingsBoard` class instance only supports a minimal subset of the actual API, see the [Supported ThingsBoard Features](https://github.com/thingsboard/thingsboard-client-sdk?tab=readme-ov-file#supported-thingsboard-features) section. But with the usage of the `IAPI_Implementation` base class, it is possible to write an own implementation that implements an additional API implementation or changes the behavior for an already existing API implementation.

For that a `class` needs to inherit the `API_Implemenatation` class and `override` the needed methods shown below:

```cpp
#ifndef Custom_API_Implementation_h
#define Custom_API_Implementation_h

// Local includes.
#include "IAPI_Implementation.h"


class Custom_API_Implementation : public IAPI_Implementation {
  public:
    ~Custom_API_Implementation() override = default;

    API_Process_Type Get_Process_Type() const override {
        return API_Process_Type::JSON;
    }

    void Process_Response(char const * topic, uint8_t * payload, uint32_t length) override {
        // Nothing to do
    }

    void Process_Json_Response(char const * topic, JsonDocument const & data) override {
        // Nothing to do
    }

    bool Is_Response_Topic_Matching(char const * topic) const override {
        return true;
    }

    bool Unsubscribe() override {
        return true;
    }

    bool Resubscribe_Permanent_Subscriptions() override {
        return true;
    }

#if !THINGSBOARD_USE_ESP_TIMER
    void loop() override {
        // Nothing to do
    }
#endif // !THINGSBOARD_USE_ESP_TIMER

    void Initialize() override {
        // Nothing to do
    }

    void Set_Client_Callbacks(Callback<void, IAPI_Implementation &>::function subscribe_api_callback, Callback<bool, char const * const, JsonDocument const &, Deserialization_Options>::function send_json_callback, Callback<bool, char const * const, char const * const>::function send_json_string_callback, Callback<bool, char const * const>::function subscribe_topic_callback, Callback<bool, char const * const>::function unsubscribe_topic_callback, Callback<uint16_t>::function get_receive_size_callback, Callback<uint16_t>::function get_send_size_callback, Callback<bool, uint16_t, uint16_t>::function set_buffer_size_callback, Callback<size_t *>::function get_request_id_callback) override {
        // Nothing to do
    }
};

#endif // Custom_API_Implementation_h
```

Once that has been done it can simply be passed to the `ThingsBoard` instance constructor.

```cpp
// Initialize underlying client, used to establish a connection
WiFiClient espClient;

// Initalize the Mqtt client instance
Arduino_MQTT_Client mqttClient(espClient);

// Initialize used apis with Custom API
Custom_API_Implementation custom_api;

// The SDK setup with the custom API implementation
ThingsBoard tb(mqttClient, &custom_api);
```

### Custom Updater Instance

When using the `ThingsBoard` class instance, the class used to flash the binary data onto the device is not hard coded,
but instead the `OTA_Update_Callback` class expects an argument, the `IUpdater` implementation.

Thanks to it being an interface it allows an arbitrary implementation,
meaning as long as the device can flash binary data and supports the C++ STL it supports OTA updates, with the `ThingsBoard` library.

Currently, implemented in the library itself are the `Arduino_ESP32_Updater`, which is used for flashing the binary data when using a `ESP32` and `Arduino`, the `Arduino_ESP8266_Updater` which is used with the `ESP8266` and `Arduino`, the `Espressif_Updater` which is used with the `ESP32` and the `Espressif IDF` tool chain and lastly the `SDCard_Updater` which is used for both `Arduino` and the `Espressif IDF` to flash binary data onto an already initialized SD card.

If another device or feature wants to be supported, a custom interface implementation needs to be created.
For that a `class` needs to inherit the `IUpdater` interface and `override` the needed methods shown below:

```cpp
#include <IUpdater.h>

class Custom_Updater : public IUpdater {
  public:
    ~Custom_Updater() override {
        reset();
    }

    bool begin(size_t const & firmware_size) override {
        return true;
    }
  
    size_t write(uint8_t * payload, size_t const & total_bytes) override {
        return total_bytes;
    }
  
    void reset() override {
        // Nothing to do
    }
  
    bool end() override {
        return true;
    }
};
```

Once that has been done it can simply be passed instead of the `Espressif_Updater`, `Arduino_ESP8266_Updater`, `Arduino_ESP32_Updater` or `SDCard_Updater` instance.

```cpp
// Initalize the Updater client instance used to flash binary to flash memory
Custom_Updater updater;

const OTA_Update_Callback callback(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, &finished_callback, &progress_callback, &update_starting_callback, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
```

### Custom MQTT Instance

When using the `ThingsBoard` class instance, the protocol used to send the data to the MQTT broker is not hard coded,
but instead the `ThingsBoard` class expects the argument to a `IMQTT_Client` implementation.

Thanks to it being an interface it allows an arbitrary implementation,
meaning the underlying MQTT client can be whatever the user decides, so it can for example be used to support platforms using `Arduino` or even `Espressif IDF`.

Currently, implemented in the library itself is the `Arduino_MQTT_Client`, which is simply a wrapper around the [`PubSubClient`](https://github.com/thingsboard/pubsubclient), see [compatible Hardware](https://github.com/thingsboard/pubsubclient?tab=readme-ov-file#compatible-hardware) for whether the board you are using is supported or not, useful when using `Arduino`. As well as the `Espressif_MQTT_Client`, which is a simple wrapper around the [`esp-mqtt`](https://github.com/espressif/esp-mqtt), useful when using `Espressif IDF` with a `ESP32`.

If another device or feature wants to be supported, a custom interface implementation needs to be created.
For that a `class` needs to inherit the `IMQTT_Client` interface and `override` the needed methods shown below:

```cpp
#include <IMQTT_Client.h>

class Custom_MQTT_Client : public IMQTT_Client {
  public:
    ~Custom_MQTT_Client() override = default;

    void set_data_callback(Callback<void, char *, uint8_t *, unsigned int>::function callback) override {
        // Nothing to do
    }

    void set_connect_callback(Callback<void>::function callback) override {
        // Nothing to do
    }

    bool set_buffer_size(uint16_t receive_buffer_size, uint16_t send_buffer_size) override {
        return true;
    }

    uint16_t get_receive_buffer_size() override {
        return 0U;
    }

    uint16_t get_send_buffer_size() override {
        return 0U;
    }

    void set_server(char const * domain, uint16_t port) override {
        // Nothing to do
    }

    bool connect(char const * client_id, char const * user_name, char const * password) override {
        return true;
    }

    void disconnect() override {
        // Nothing to do
    }

    bool loop() override {
        return true;
    }

    bool publish(char const * topic, uint8_t const * payload, size_t const & length) override {
        return true;
    }

    bool subscribe(char const * topic) override {
        return true;
    }

    bool unsubscribe(char const * topic) override {
        return true;
    }

    bool connected() override {
        return true;
    }

    MQTT_Connection_State get_connection_state() override {
        return MQTT_Connection_State::DISCONNECTED;
    }

    MQTT_Connection_Error get_last_connection_error() override {
        return MQTT_Connection_Error::NONE;
    }

    void subscribe_connection_state_changed_callback(Callback<void, MQTT_Connection_State, MQTT_Connection_Error>::function callback) override {
        // Nothing to do
    }

#if THINGSBOARD_ENABLE_STREAM_UTILS

    bool begin_publish(char const * topic, size_t const & length) override {
        return true;
    }

    bool end_publish() override {
        return true;
    }

    //----------------------------------------------------------------------------
    // Print interface
    //----------------------------------------------------------------------------

    size_t write(uint8_t payload_byte) override {
        return 1U;
    }

    size_t write(uint8_t const * buffer, size_t const & size) override {
        return size;
    }

#endif // THINGSBOARD_ENABLE_STREAM_UTILS
};
```

Once that has been done it can simply be passed instead of the `Arduino_MQTT_Client` or the `Espressif_MQTT_Client` instance.

```cpp
// Initalize the Mqtt client instance
Custom_MQTT_Client mqttClient;

// The SDK setup
ThingsBoard tb(mqttClient);
```

## Have a question or proposal?

You are welcome in our [issues](https://github.com/thingsboard/thingsboard-client-sdk/issues) and [Q&A forum](https://groups.google.com/forum/#!forum/thingsboard).

## License

This code is released under the MIT License.