#ifndef Constants_h
#define Constants_h

// Local includes.
#include "Configuration.h"

// Library includes.
#include <ArduinoJson.h>
#include <stdint.h>


uint16_t constexpr DEFAULT_MQTT_PORT = 1883U;
char constexpr PROV_ACCESS_TOKEN[] = "provision";
// Log messages.
char constexpr UNABLE_TO_SERIALIZE[] = "Unable to serialize key-value json";
char constexpr UNABLE_TO_SERIALIZE_JSON[] = "Unable to serialize json data";
char constexpr UNABLE_TO_DE_SERIALIZE_JSON[] = "Unable to de-serialize received json data with error (%s)";
char constexpr UNABLE_TO_ALLOCATE_JSON[] = "Allocating memory for the JsonDocument failed, passed JsonDocument is NULL";
char constexpr INVALID_BUFFER_SIZE[] = "Send buffer size (%u) to small for the given payloads size (%u), increase with Set_Buffer_Size accordingly or install the StreamUtils library";
char constexpr UNABLE_TO_ALLOCATE_BUFFER[] = "Allocating memory for the internal MQTT buffer failed";
char constexpr MAXIMUM_RESPONSE_EXCEEDED[] = "Prevented allocation on the heap (%u) for JsonDocument. Discarding message that is bigger than maximum response size (%u)";
char constexpr HEAP_ALLOCATION_FAILED[] = "Failed allocating required size (%u) for JsonDocument. Ensure there is enough heap memory left";
char constexpr CONNECT_FAILED[] = "Connecting to server failed";
#if THINGSBOARD_ENABLE_DEBUG
char constexpr RECEIVE_MESSAGE[] = "Received (%u) bytes of data from server over topic (%s)";
char constexpr ALLOCATING_JSON[] = "Allocated internal JsonDocument for MQTT server response with size (%u)";
char constexpr SEND_MESSAGE[] = "Sending data to server over topic (%s) with data (%s)";
char constexpr SEND_SERIALIZED[] = "Hidden, because json data is bigger than buffer, therefore showing in console is skipped";
#endif // THINGSBOARD_ENABLE_DEBUG
// Claim topics.
char constexpr CLAIM_TOPIC[] = "v1/devices/me/claim";
// Claim data keys.
char constexpr SECRET_KEY[] = "secretKey";
char constexpr DURATION_KEY[] = "durationMs";
// RPC topics.
char constexpr RPC_SUBSCRIBE_TOPIC[] = "v1/devices/me/rpc/request/+";
char constexpr RPC_RESPONSE_TOPIC[] = "v1/devices/me/rpc/response";
char constexpr RPC_SEND_REQUEST_TOPIC[] = "v1/devices/me/rpc/request";
// RPC data keys.
char constexpr METHOD_KEY[] = "method";
char constexpr PARAMS_KEY[] = "params";
// Shared attribute topics.
char constexpr ATTRIBUTE_TOPIC[] = "v1/devices/me/attributes";
char constexpr ATTRIBUTE_REQUEST_TOPIC[] = "v1/devices/me/attributes/request";
char constexpr ATTRIBUTE_RESPONSE_TOPIC[] = "v1/devices/me/attributes/response";
// Shared attribute data keys.
char constexpr SHARED_KEY[] = "shared";
// Telemetry topics.
char constexpr TELEMETRY_TOPIC[] = "v1/devices/me/telemetry";
// Provision topics.
char constexpr PROV_REQUEST_TOPIC[] = "/provision/request";
char constexpr PROV_RESPONSE_TOPIC[] = "/provision/response";
// Provision data keys.
char constexpr PROV_DEVICE_NAME[] = "deviceName";
char constexpr PROV_PROV_TYPE[] = "provisionType";
char constexpr PROV_PROV_DEVICE_KEY[] = "provisionDeviceKey";
char constexpr PROV_PROV_DEVICE_SECRET[] = "provisionDeviceSecret";
char constexpr PROV_CRED_TYPE[] = "credentialsType";
char constexpr PROV_CRED_USERNAME[] = "username";
char constexpr PROV_CRED_PASSWORD[] = "password";
char constexpr PROV_CRED_CLIENT_ID[] = "clientId";
char constexpr PROV_CRED_HASH[] = "hash";
char constexpr PROV_STATUS[] = "status";
char constexpr PROV_ACCESS_TOKEN_KEY[] = "accessToken";
// Firmware topics.
char constexpr FIRMWARE_RESPONSE_TOPIC[] = "v2/fw/response";
// Firmware data keys.
char constexpr CURR_FW_TITLE_KEY[] = "current_fw_title";
char constexpr CURR_FW_VER_KEY[] = "current_fw_version";
char constexpr FW_ERROR_KEY[] = "fw_error";
char constexpr FW_STATE_KEY[] = "fw_state";
char constexpr FW_CHKS_KEY[] = "fw_checksum";
char constexpr FW_CHKS_ALGO_KEY[] = "fw_checksum_algorithm";
char constexpr FW_SIZE_KEY[] = "fw_size";
char constexpr FW_TITLE_KEY[] = "fw_title";
char constexpr FW_VER_KEY[] = "fw_version";
// Firmware states.
char constexpr FW_STATE_DOWNLOADING[] = "DOWNLOADING";
char constexpr FW_STATE_DOWNLOADED[] = "DOWNLOADED";
char constexpr FW_STATE_VERIFIED[] = "VERIFIED";
char constexpr FW_STATE_UPDATING[] = "UPDATING";
char constexpr FW_STATE_UPDATED[] = "UPDATED";
char constexpr FW_STATE_FAILED[] = "FAILED";
// Hashing algorithm names.
char constexpr MD5[] = "MD5";
char constexpr SHA256[] = "SHA256";
char constexpr SHA384[] = "SHA384";
char constexpr SHA512[] = "SHA512";
// General data keys.
char constexpr CLIENT_SCOPE[] = "client";
char constexpr KEY[] = "key";
char constexpr VALUE_KEY[] = "value";

/// @brief Options for deserializing JSON content, allows to use PROGMEM
/// as a string buffer for the serialized json, which will then be sent over MQTT
enum Deserialization_Options {
  NONE,
  PROGMEM
};

#endif // Constants_h