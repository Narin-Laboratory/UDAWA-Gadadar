#ifndef ThingsBoard_h
#define ThingsBoard_h

// Local includes.
#include "Constants.h"
#include "IAPI_Implementation.h"
#include "IMQTT_Client.h"
#include "DefaultLogger.h"
#include "Telemetry.h"

// Library includes.
#if THINGSBOARD_ENABLE_STREAM_UTILS
#include <StreamUtils.h>
#endif // THINGSBOARD_ENABLE_STREAM_UTILS

/// @brief Wrapper around any arbitrary MQTT Client implementing the IMQTT_Client interface, to allow connecting and sending / retrieving data from ThingsBoard over the MQTT or MQTT with TLS/SSL protocol
/// @note BufferSize of the underlying data buffer can be changed during the runtime and the maximum amount of data points that can ever be sent or received are automatically deduced at runtime.
/// Additionally, there are internal vectors that hold all subscriptions and requests and dynamically allocate memory on the heap, depending on how much space we currently require.
/// Furthermore, there are internal vectors in the @ref Shared_Attribute_Callback and the @ref Attribute_Request_Callback, which hold the amount of keys we want to request or subscribe to updates too.
/// Dynamically increasing the internal size, allows to adjust how much space we require depending on the amount of subscribed or requested keys.
class ThingsBoard {
  public:
    /// @brief Constructs a instance with the given network client that should be used to establish the connection to ThingsBoard
    /// @note Directly forwards the given arguments to the overloaded Container constructor,
    /// meaning all combinatons of arguments that would initalize an std::vector can be used to call this constructor.
    /// See possible std::vector constructors here https://en.cppreference.com/w/cpp/container/vector/vector, for the possible passable parameters.
    /// The possibilites mainly consist out of the fill constructor, where a number n and a value is given and then the value is copied into that many elements,
    /// alternatively if no value is given the default constructed value is copied n times instead,
    /// or the range constructor where we can pass an interator to the start and to the end of the data container (last element + 1)
    /// to copy every element in between thoose iterators, in the same order as in the original data container.
    /// The last option is a copy constructor where we pass another container and all the values of that container will be copied into our buffer
    /// @tparam ...Args Holds the multiple arguments that will simply be forwarded to the container constructor and therefore allow to use every overloaded constructor without having to explicitly implement them
    /// @param client MQTT Client implementation that should be used to establish the connection to ThingsBoard
    /// @param max_stack_size Maximum amount of bytes we want to allocate on the stack.
    /// Is used when sending a lot of data at once over MQTT, because to actually send the JsonDocument data it first has to be serialized into a json string payload.
    /// To achieve this the data contained in the JsonDocument is copied for the scope of the @ref Send_Json method and is then copied into the outgoing MQTT buffer.
    /// This variable therefore decides the threshold where the JsonDocument is copied into the heap instead of a object on the stack.
    /// This is created to ensure no StackOverflow occurs because most supported boards run the actual sending code in a seperate FreeRTOS Task with limited stack space where even a stack allocation of 4 KiB might already cause a crash
    /// To circumvent this copy the alternative mentioned in the send_buffer_size argument can also be used because it skips the internal copy alltogether, because the JsonDocument is instead directly copied into the outgoing MQTT buffer, default = 1024
    /// @param max_response_size Maximum amount of bytes allocated for the interal JsonDocument structure that holds the received payload.
    /// Size is calculated automatically from certain characters in the received payload (',', '{', '[') but if we receive a malicious payload that contains these symbols in a string {"example":",,,,,,..."}.
    /// It is possible to cause huge allocations, but because the memory only lives for as long as the subscribed callback methods it should not be a problem,
    /// especially because attempting to allocate too much memory, will cause the allocation to fail, which is checked. But if the failure of that heap allocation is subscribed for example with the heap_caps_register_failed_alloc_callback method on the ESP32,
    /// then that subscribed callback will be called and could theoretically restart the device. To circumvent that we can simply set the size of this variable to a value that should never be exceeded by a non malicious json payload.
    /// If this safety feature is not required, because the heap allocation failure callback is not subscribed, then the value of the variable can simply be kept as 0, which means we will not check the received payload for its size before the allocation happens, default = 0
    /// @param buffering_size Amount of bytes allocated to speed up serialization.
    /// Used when THINGSBOARD_ENABLE_STREAM_UTILS is enabled by importing the ArduinoStreamUtils (https://github.com/bblanchon/ArduinoStreamUtils) in the project.
    /// This feature allows to improve the underlying data streams by directly writing the data into the MQTT Client instead of into an output buffer,
    /// but writing each byte one by one, would be too slow, therefore the ArduinoStreamUtils (https://github.com/bblanchon/ArduinoStreamUtils) library is used to buffer those calls into bigger packets.
    /// The variable therefore decides the exact buffering size for these packets, where bigger packets cause faster serialization but in exchange require more memory, default = 64
    /// @param ...args APIs that should be connected to ThingsBoard and therefore be able to send and receive data over MQTT, that will be forwarded into the overloaded Container constructor see https://en.cppreference.com/w/cpp/container/vector/vector for more information.
    /// Ensure the actual API implementations are kept alive as long as the instance of this class. Because the values are not copied, but a non owning pointers to the values are inserted into the local container member variable instead
    template<typename... Args>
#if THINGSBOARD_ENABLE_STREAM_UTILS
    ThingsBoard(IMQTT_Client & client, size_t const & max_stack_size = 1024U, size_t const & buffering_size = 64U, size_t const & max_response_size = 0U, Args const &... args)
#else
    ThingsBoard(IMQTT_Client & client, size_t const & max_stack_size = 1024U, size_t const & max_response_size = 0U, Args const &... args)
#endif // THINGSBOARD_ENABLE_STREAM_UTILS
        : m_client(client)
        , m_max_stack(max_stack_size)
#if THINGSBOARD_ENABLE_STREAM_UTILS
        , m_buffering_size(buffering_size)
#endif // THINGSBOARD_ENABLE_STREAM_UTILS
        , m_max_response_size(max_response_size)
        , m_api_implementations(args...)
    {
            for (auto & api : m_api_implementations) {
                    if (api == nullptr) {
                            continue;
                    }
                    api->Set_Client_Callbacks(std::bind(&ThingsBoard::Subscribe_API_Implementation, this, std::placeholders::_1), [this](char const * topic, JsonDocument const & doc, Deserialization_Options opt) { return this->Send_Json(topic, doc, opt); }, std::bind(&ThingsBoard::Send_Json_String, this, std::placeholders::_1, std::placeholders::_2), std::bind(&ThingsBoard::Subscribe_Topic, this, std::placeholders::_1), std::bind(&ThingsBoard::Unsubscribe_Topic, this, std::placeholders::_1), std::bind(&ThingsBoard::Get_Receive_Buffer_Size, this), std::bind(&ThingsBoard::Get_Send_Buffer_Size, this), std::bind(&ThingsBoard::Set_Buffer_Size, this, std::placeholders::_1, std::placeholders::_2), std::bind(&ThingsBoard::Get_Last_Request_ID, this));
                    api->Initialize();
            }
            // Initialize callback.
            m_client.set_data_callback(std::bind(&ThingsBoard::On_MQTT_Message, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            m_client.set_connect_callback(std::bind(&ThingsBoard::Resubscribe_Permanent_Subscriptions, this));
    }

    /// @brief Gets the registered underlying MQTT Client implementation
    /// @note Allows for calling method directly on the client itself, not advised in normal use cases,
    // as it might cause problems if the library expects the client to be sending / receiving data
    // but it can not do that anymore, because it has been disconnected or certain settings were changed
    /// @return Reference to the registered underlying MQTT Client implementation
    IMQTT_Client & Get_Client() {
            return m_client;
    }

    /// @brief Sets the maximum amount of bytes that we want to allocate on the stack, before the memory is allocated on the heap instead
    /// @note Is used when sending a lot of data at once over MQTT, because to actually send the JsonDocument data it first has to be serialized into a json string payload.
    /// To achieve this the data contained in the JsonDocument is copied for the scope of the @ref Send_Json method and is then copied into the outgoing MQTT buffer.
    /// This variable therefore decides the threshold where the JsonDocument is copied into the heap instead of a object on the stack.
    /// This is created to ensure no StackOverflow occurs because most supported boards run the actual sending code in a seperate FreeRTOS Task with limited stack space where even a stack allocation of 4 KiB might already cause a crash
    /// To circumvent this copy the alternative mentioned in the send_buffer_size argument of the constructor can also be used because it skips the internal copy alltogether, because the JsonDocument is instead directly copied into the outgoing MQTT buffer
    /// @param max_stack_size Maximum amount of bytes we want to allocate on the stack
    void Set_Maximum_Stack_Size(size_t const & max_stack_size) {
            m_max_stack = max_stack_size;
    }

    /// @brief Returns the maximum amount of bytes that we want to allocate on the stack, before the memory is allocated on the heap instead
    /// @return Maximum amount of bytes we want to allocate on the stack
    size_t const & Get_Maximum_Stack_Size() const {
            return m_max_stack;
    }

#if THINGSBOARD_ENABLE_STREAM_UTILS
    /// @brief Returns the amount of bytes that can be allocated to speed up fall back serialization with the StreamUtils class
    /// See https://github.com/bblanchon/ArduinoStreamUtils for more information on the underlying class used
    /// @return Amount of bytes allocated to speed up serialization
    size_t const & Get_Buffering_Size() const {
        return m_buffering_size;
    }

    /// @brief Sets the amount of bytes allocated to speed up serialization
    /// @note Used when THINGSBOARD_ENABLE_STREAM_UTILS is enabled by importing the ArduinoStreamUtils (https://github.com/bblanchon/ArduinoStreamUtils) in the project.
    /// This feature allows to improve the underlying data streams by directly writing the data into the MQTT Client instead of into an output buffer,
    /// but writing each byte one by one, would be too slow, therefore the ArduinoStreamUtils (https://github.com/bblanchon/ArduinoStreamUtils) library is used to buffer those calls into bigger packets.
    /// The variable therefore decides the exact buffering size for these packets, where bigger packets cause faster serialization but in exchange require more memory
    /// @param buffering_size Amount of bytes allocated to speed up serialization
    void Set_Buffering_Size(size_t const & buffering_size) {
            m_buffering_size = buffering_size;
    }
#endif // THINGSBOARD_ENABLE_STREAM_UTILS

    /// @brief Sets the Maximum amount of bytes allocated for the interal JsonDocument structure that holds the received payload
    /// @note Size is calculated automatically from certain characters in the received payload (',', '{', '[') but if we receive a malicious payload that contains these symbols in a string {"example":",,,,,,..."}.
    /// It is possible to cause huge allocations, but because the memory only lives for as long as the subscribed callback methods it should not be a problem,
    /// especially because attempting to allocate too much memory, will cause the allocation to fail, which is checked. But if the failure of that heap allocation is subscribed for example with the heap_caps_register_failed_alloc_callback method on the ESP32,
    /// then that subscribed callback will be called and could theoretically restart the device. To circumvent that we can simply set the size of this variable to a value that should never be exceeded by a non malicious json payload.
    /// If this safety feature is not required, because the heap allocation failure callback is not subscribed, then the value of the variable can simply be kept as 0, which means we will not check the received payload for its size before the allocation happens
    /// @param max_response_size Maximum amount of bytes allocated for the interal JsonDocument structure that holds the received payload
    void Set_Max_Response_Size(size_t const & max_response_size) {
            m_max_response_size = max_response_size;
    }

    /// @brief Gets the Maximum amount of bytes allocated for the interal JsonDocument structure that holds the received payload
    /// @note Size is calculated automatically from certain characters in the received payload (',', '{', '[') but if we receive a malicious payload that contains these symbols in a string {"example":",,,,,,..."}.
    /// It is possible to cause huge allocations, but because the memory only lives for as long as a subscribed callback methods it should not be a problem,
    /// especially because attempting to allocate too much memory, will cause the allocation to fail, which is checked. But if the failure of that heap allocation is subscribed for example with the heap_caps_register_failed_alloc_callback method on the ESP32,
    /// then that subscribed callback will be called and could theoretically restart the device. To circumvent that we can simply set the size of this variable to a value that should never be exceeded by a non malicious json payload.
    /// If this safety feature is not required, because the heap allocation failure callback is not subscribed, then the value of the variable can simply be kept as 0, which means we will not check the received payload for its size before the allocation happens
    /// @return Maximum amount of bytes allocated for the interal JsonDocument structure that holds the received payload
    size_t const & Get_Max_Response_Size() {
            return m_max_response_size;
    }

    /// @copydoc IMQTT_Client::set_buffer_size
    bool Set_Buffer_Size(uint16_t receive_buffer_size, uint16_t send_buffer_size) {
            bool const result = m_client.set_buffer_size(receive_buffer_size, send_buffer_size);
            if (!result) {
                    DefaultLogger::printfln(UNABLE_TO_ALLOCATE_BUFFER);
            }
            return result;
    }

    /// @copydoc IMQTT_Client::get_receive_buffer_size
    uint16_t Get_Receive_Buffer_Size() {
            return m_client.get_receive_buffer_size();
    }

    /// @copydoc IMQTT_Client::get_send_buffer_size
    uint16_t Get_Send_Buffer_Size() {
            return m_client.get_send_buffer_size();
    }

    /// @brief Clears all currently subscribed callbacks and unsubscribed from all currently subscribed MQTT topics
    /// @note Any response that will stil be received is discarded
    /// and any ongoing firmware update is aborted and will not be finished.
    /// Was done automatically in the connect() method in previous versions of the library, but is not done anymore,
    /// because connect() method now reconencts to all previously subscribed MQTT topics instead,
    /// therefore there is no need anymore to discard all previously subscribed callbacks and letting the user resubscribe by hand themselves,
    /// because that resubscription is now handled automatically. But to allow for the use case of complete unsubscriptions this method still exists
    void Cleanup_Subscriptions() {
            for (auto & api : m_api_implementations) {
                    if (api == nullptr) {
                            continue;
                    }
                    // Results are ignored, because the important part of clearing internal data structures always succeeds
                    (void)api->Unsubscribe();
            }
    }

    /// @brief Connects to the given server instance and port with the given credentials
    /// @note Additionally internal Containers are not deleted or changed when reconnecting so any permanent subscriptions already previously subscribed, does not need to be resubscribed.
    /// Furthermore if there are still any active permanent subscriptions (server-side RPC or shared attribute update subscriptions), the aforementioned topics will be resubscribed automatically as soon as the device has successfully connected.
    /// This means that any permanent subscriptions will immediately be notified of any changes again once the device connects. This removes the need to register these callbacks every time the device disconnects
    /// and instead can be done once at startup. They can even be subscribed before the device has even connected to the cloud
    /// @param host Non owning pointer to server instance name the client should connect too.
    /// Additionally it has to be kept alive by the user for the runtime of the MQTT client connection
    /// @param access_token Non owning pointer to access token, that allows to differentiate which MQTT device is sending the traffic to the MQTT broker.
    /// Can be "provision", if the device creates itself instead. See https://thingsboard.io/docs/user-guide/device-provisioning/?mqttprovisioning=without#provision-device-apis for more information.
    /// Additionally it has to be kept alive by the user for the runtime of the MQTT client connection, default = PROV_ACCESS_TOKEN ("provision")
    /// @param port Port that will be used to establish a connection and send / receive data.
    /// Should be either 1883 for unencrypted MQTT or 8883 for MQTT with TLS/SSL encryption.
    /// The latter is recommended if relevant data is sent or if the client receives and handles Remote Procedure Calls or Shared Attribute Update Callbacks from the server,
    /// because using an unencrpyted connection, will allow 3rd parties to listen to the communication and impersonate the server sending payloads which might influence the device in unexpected ways.
    /// However if Over the Air udpates are enabled secure communication should definetly be enabled, because if that is not done a 3rd party might impersonate the server sending a malicious payload,
    /// which is then flashed onto the device instead of the real firmware. Which depeding on the payload might even be able to destroy the device or make it otherwise unusable.
    /// See https://stackoverflow.blog/2020/12/14/security-considerations-for-ota-software-updates-for-iot-gateway-devices/ for more information on the aforementioned security risk, default = DEFAULT_MQTT_PORT (1883)
    /// @param user_name Non owning pointer to client username that is used to authenticate, who is connecting over MQTT.
    /// Additionally it has to be kept alive by the user for the runtime of the MQTT client connection, default = nullptr
    /// @param password Non owning pointer to client password that is used to authenticate, who is connecting over MQTT.
    /// Additionally it has to be kept alive by the user for the runtime of the MQTT client connection, default = nullptr
    /// @return Whether connecting to ThingsBoard was successful or not
    bool connect(char const * host, char const * access_token = PROV_ACCESS_TOKEN, uint16_t port = DEFAULT_MQTT_PORT, char const * client_id = nullptr, char const * password = nullptr) {
            if (host == nullptr) {
                    return false;
            }
            m_client.set_server(host, port);
            return Connect_To_Host(access_token, Helper::String_IsNull_Or_Empty(client_id) ? access_token : client_id, Helper::String_IsNull_Or_Empty(password) ? nullptr : password);
    }

    /// @copydoc IMQTT_Client::disconnect
    void disconnect() {
            m_client.disconnect();
    }

    /// @copydoc IMQTT_Client::connected
    bool connected() {
            return m_client.connected();
    }

    /// @copydoc IMQTT_Client::get_connection_state
    MQTT_Connection_State Get_Connection_State() {
            return m_client.get_connection_state();
    }

    /// @copydoc IMQTT_Client::get_last_connection_error
    MQTT_Connection_Error Get_Last_Connection_Error() {
            return m_client.get_last_connection_error();
    }

    /// @copydoc IMQTT_Client::subscribe_connection_state_changed_callback
    void Subscribe_Connection_State_Changed_Callback(Callback<void, MQTT_Connection_State, MQTT_Connection_Error>::function callback) {
            m_client.subscribe_connection_state_changed_callback(callback);
    }

    /// @copydoc IMQTT_Client::loop
    bool loop() {
#if !THINGSBOARD_USE_ESP_TIMER
            for (auto & api : m_api_implementations) {
                    if (api == nullptr) {
                            continue;
                    }
                    api->loop();
            }
#endif // !THINGSBOARD_USE_ESP_TIMER
            return m_client.loop();
    }

    /// @brief Sends key-value pairs from the given JsonDocument over the given topic
    /// @note The passed JsonDocument data first has to be serialized into a json string payload to be then copied into the outgoing MQTT buffer.
    /// To circumvent this copy the alternative mentioned in the send_buffer_size argument of the constructor can also be used because it skips the internal copy alltogether,
    /// because the JsonDocument is instead directly copied into the outgoing MQTT buffer
    /// @param topic Non owning pointer to topic that the message is sent over, where different MQTT topics expect a different kind of payload.
    /// Does not need to kept alive as the function copies the data into the outgoing MQTT buffer to publish the given payload
    /// @param source JsonDocument containing our json key-value pairs,
    /// is checked before usage for any possible occuring internal errors. See https://arduinojson.org/v7/api/jsondocument/ for more information
    /// @param deserialize_option Option that decides how the given JsonDocument will be deserialized,
    /// allows to use PROGMEM as a string buffer for the serialized json, which will then be sent over MQTT, default = Deserialization_Options::NONE
    /// @return Whether copying the payload contained in the source into the outgoing MQTT buffer, was successful or not
    bool Send_Json(char const * topic, JsonDocument const & source, Deserialization_Options deserialize_option = Deserialization_Options::NONE) {
            // Check if allocating needed memory failed when trying to create the JsonDocument,
            // if it did the isNull() method will return true. See https://arduinojson.org/v7/api/jsonvariant/isnull/ for more information
            if (source.isNull()) {
                    DefaultLogger::printfln(UNABLE_TO_ALLOCATE_JSON);
                    return false;
            }
            bool result = false;

            size_t const json_size = Helper::Measure_Json(source);
#if THINGSBOARD_ENABLE_STREAM_UTILS
            // Check if the size of the given message would be too big for the actual client,
            // if it is utilize the serialize json work around, so that the internal client buffer can be circumvented
            if (json_size > m_client.get_send_buffer_size())  {
#if THINGSBOARD_ENABLE_DEBUG
                    DefaultLogger::printfln(SEND_MESSAGE, topic, SEND_SERIALIZED);
#endif // THINGSBOARD_ENABLE_DEBUG
                    result = Serialize_Json(topic, source);
            }
            else
#endif // THINGSBOARD_ENABLE_STREAM_UTILS
            if (json_size > Get_Maximum_Stack_Size()) {
                    char* json = new char[json_size]();
                    if (serializeJson(source, json, json_size) == 0) {
                            DefaultLogger::printfln(UNABLE_TO_SERIALIZE_JSON);
                    }
                    else {
                            result = Send_Json_String(topic, json);
                    }
                    delete[] json;
                    json = nullptr;
            }
            else {
                    char json[json_size] = {};
                    if (serializeJson(source, json, json_size) == 0) {
                            DefaultLogger::printfln(UNABLE_TO_SERIALIZE_JSON);
                            return result;
                    }
                    result = Send_Json_String(topic, json);
            }

            return result;
    }

    /// @brief Sends key-value pairs from the given json string over the given topic
    /// @param topic Non owning pointer to topic that the message is sent over, where different MQTT topics expect a different kind of payload.
    /// Does not need to kept alive as the function copies the data into the outgoing MQTT buffer to publish the given payload
    /// @param json Non owning pointer to the string containing serialized json key-value pairs that should be copied into the outgoing MQTT buffer.
    /// Does not need to kept alive as the function copies the data into the outgoing MQTT buffer to publish the given payload
    /// @return Whether copying the payload contained in the json string into the outgoing MQTT buffer, was successful or not
    bool Send_Json_String(char const * topic, char const * json) {
            if (json == nullptr) {
                    return false;
            }

            uint16_t current_send_buffer_size = m_client.get_send_buffer_size();
            auto const json_size = strlen(json);

            if (current_send_buffer_size < json_size) {
                    DefaultLogger::printfln(INVALID_BUFFER_SIZE, current_send_buffer_size, json_size);
                    return false;
            }

#if THINGSBOARD_ENABLE_DEBUG
            DefaultLogger::printfln(SEND_MESSAGE, topic, json);
#endif // THINGSBOARD_ENABLE_DEBUG
            return m_client.publish(topic, reinterpret_cast<uint8_t const *>(json), json_size);
    }

    /// @brief Subscribes the given API implementation
    /// @note Ensure the actual API implementation is kept alive as long as the instance of this class. Because the value is not copied,
    /// but a non owning pointer to the value is inserted into the local container member variable instead
    /// @param api Additional API that should be connected to ThingsBoard and therefore be able to send and receive data over MQTT
    void Subscribe_API_Implementation(IAPI_Implementation & api) {
            api.Set_Client_Callbacks(std::bind(&ThingsBoard::Subscribe_API_Implementation, this, std::placeholders::_1), [this](char const * topic, JsonDocument const & doc, Deserialization_Options opt) { return this->Send_Json(topic, doc, opt); }, std::bind(&ThingsBoard::Send_Json_String, this, std::placeholders::_1, std::placeholders::_2), std::bind(&ThingsBoard::Subscribe_Topic, this, std::placeholders::_1), std::bind(&ThingsBoard::Unsubscribe_Topic, this, std::placeholders::_1), std::bind(&ThingsBoard::Get_Receive_Buffer_Size, this), std::bind(&ThingsBoard::Get_Send_Buffer_Size, this), std::bind(&ThingsBoard::Set_Buffer_Size, this, std::placeholders::_1, std::placeholders::_2), std::bind(&ThingsBoard::Get_Last_Request_ID, this));
            api.Initialize();
            m_api_implementations.push_back(&api);
    }

    /// @brief Subscribes the given API implementation
    /// @note Ensure the actual API implementations that should be connected to ThingsBoard and therefore be able to send and receive data over MQTT.
    /// Ensure the actual API implementations are kept alive as long as the instance of this class.
    /// Because the values are not copied, but a non owning pointers to the values are inserted into the local container member variable instead
    /// @tparam InputIterator Class that allows for forward incrementable access to data
    /// of the given data container, allows for using / passing either std::vector or std::array.
    /// See https://en.cppreference.com/w/cpp/iterator/input_iterator for more information on the requirements of the iterator
    /// @param first Iterator pointing to the first element in the data container
    /// @param last Iterator pointing to the end of the data container (last element + 1)
    template <typename InputIterator>
    void Subscribe_API_Implementations(InputIterator const & first, InputIterator const & last) {
            for (auto it = first; it != last; ++it) {
                    auto & api = *it;
                    if (api == nullptr) {
                            continue;
                    }
                    api->Set_Client_Callbacks(std::bind(&ThingsBoard::Subscribe_API_Implementation, this, std::placeholders::_1), [this](char const * topic, JsonDocument const & doc, Deserialization_Options opt) { return this->Send_Json(topic, doc, opt); }, std::bind(&ThingsBoard::Send_Json_String, this, std::placeholders::_1, std::placeholders::_2), std::bind(&ThingsBoard::Subscribe_Topic, this, std::placeholders::_1), std::bind(&ThingsBoard::Unsubscribe_Topic, this, std::placeholders::_1), std::bind(&ThingsBoard::Get_Receive_Buffer_Size, this), std::bind(&ThingsBoard::Get_Send_Buffer_Size, this), std::bind(&ThingsBoard::Set_Buffer_Size, this, std::placeholders::_1, std::placeholders::_2), std::bind(&ThingsBoard::Get_Last_Request_ID, this));
                    api->Initialize();
            }
            m_api_implementations.insert(m_api_implementations.end(), first, last);
    }

    //----------------------------------------------------------------------------
    // Claiming API

    /// @brief Send a claiming request for this device
    /// @note Allows any user registered on the cloud to assign the device as their own (claim),
    /// as long as they enter the given corresponding device name and secret key in the given amount of time.
    /// Optionally the secret key can be left empty, results in the cloud allowing any user to claim the device without the need to enter a secret key.
    /// See https://thingsboard.io/docs/user-guide/claiming-devices/ for more information
    /// @param duration_ms Total time in milliseconds that the device can be claimed for.
    /// If the device is not claimed in the given timeframe then another claiming request would need to be sent to allow for the device to be claimed again
    /// @param secret_key Non owning pointer to the password that needs to be entered to claim the device.
    /// Functions as an additional security mechanism to only allow the actual inteded user to claim the device.
    /// If this feature is not needed and claiming without the password is wanted simply pass a nullptr or empty string as the argument instead.
    /// Does not need to kept alive as the function copies the data into the outgoing MQTT buffer to publish the claiming request, default = nullptr
    /// @return Whether copying the created claiming request into the outgoing MQTT buffer, was successful or not
    bool Claim_Request(size_t const & duration_ms, char const * secret_key = nullptr) {
            JsonDocument request_buffer;

            if (!Helper::String_IsNull_Or_Empty(secret_key)) {
                    request_buffer[SECRET_KEY] = secret_key;
            }
            request_buffer[DURATION_KEY] = duration_ms;
            return Send_Json(CLAIM_TOPIC, request_buffer);
    }

    //----------------------------------------------------------------------------
    // Telemetry API

    /// @brief Sends the given key-value pair as telemetry data.
    /// See https://thingsboard.io/docs/user-guide/telemetry/ for more information
    /// @tparam T Type of the passed value
    /// @param key Non owning pointer to the key of the key-value pair.
    /// Does not need to kept alive as the function copies the data into the outgoing MQTT buffer to publish the key-value pair
    /// @param value Value of the key-value pair
    /// @return Whether copying the key-value pair into the outgoing MQTT buffer, was successful or not
    template<typename T>
    bool Send_Telemetry_Data(char const * key, T const & value) {
            return Send_Key_Value_Pair(key, value);
    }

    /// @brief Send aggregated key-value pair as telemetry data
    /// @note Expects iterators to a container containing Telemetry class instances.
    /// See https://thingsboard.io/docs/user-guide/telemetry/ for more information
    /// @tparam InputIterator Class that allows for forward incrementable access to data
    /// of the given data container, allows for using / passing either std::vector or std::array.
    /// See https://en.cppreference.com/w/cpp/iterator/input_iterator for more information on the requirements of the iterator
    /// @param first Iterator pointing to the first element in the data container
    /// @param last Iterator pointing to the end of the data container (last element + 1)
    /// @return Whether copying the key-value pairs into the outgoing MQTT buffer, was successful or not
    template<typename InputIterator>
    bool Send_Telemetry(InputIterator const & first, InputIterator const & last) {
            return Send_Data_Array(first, last, true);
    }

    /// @brief Send string containing json as telemetry data.
    /// See https://thingsboard.io/docs/user-guide/telemetry/ for more information
    /// @param json Non owning pointer to the string containing our json key-value pairs
    /// Does not need to kept alive as the function copies the data into the outgoing MQTT buffer to publish the key-value pairs
    /// @return Whether copying the key-value pairs into the outgoing MQTT buffer, was successful or not
    bool Send_Telemetry_String(char const * json) {
            return Send_Json_String(TELEMETRY_TOPIC, json);
    }

    /// @brief Send key-value pairs as telemetry data.
    /// See https://thingsboard.io/docs/user-guide/telemetry/ for more information
    /// @param source JsonDocument containing our json key-value pairs,
    // is checked before usage for any possible occuring internal errors. See https://arduinojson.org/v7/api/jsondocument/ for more information
    /// @return Whether copying the key-value pairs into the outgoing MQTT buffer, was successful or not
    bool Send_Telemetry_Json(JsonDocument const & source) {
            return Send_Json(TELEMETRY_TOPIC, source);
    }

    //----------------------------------------------------------------------------
    // Attribute API

    /// @brief Sends the given key-value pair as attribute data.
    /// See https://thingsboard.io/docs/user-guide/attributes/ for more information
    /// @tparam T Type of the passed value
    /// @param key Non owning pointer to the key of the key-value pair.
    /// Does not need to kept alive as the function copies the data into the outgoing MQTT buffer to publish the key-value pair
    /// @param value Value of the key-value pair
    /// @return Whether copying the key-value pair into the outgoing MQTT buffer, was successful or not
    template<typename T>
    bool Send_Attribute_Data(char const * key, T const & value) {
            return Send_Key_Value_Pair(key, value, false);
    }

    /// @brief Send aggregated key-value pair as attribute data
    /// @note Expects iterators to a container containing Attribute class instances.
    /// See https://thingsboard.io/docs/user-guide/attribute/ for more information
    /// @tparam InputIterator Class that allows for forward incrementable access to data
    /// of the given data container, allows for using / passing either std::vector or std::array.
    /// See https://en.cppreference.com/w/cpp/iterator/input_iterator for more information on the requirements of the iterator
    /// @param first Iterator pointing to the first element in the data container
    /// @param last Iterator pointing to the end of the data container (last element + 1)
    /// @return Whether copying the key-value pairs into the outgoing MQTT buffer, was successful or not
    template<typename InputIterator>
    bool Send_Attributes(InputIterator const & first, InputIterator const & last) {
            return Send_Data_Array(first, last, false);
    }

    /// @brief Send string containing json as attribute data.
    /// See https://thingsboard.io/docs/user-guide/attribute/ for more information
    /// @param json Non owning pointer to the string containing our json key-value pairs
    /// Does not need to kept alive as the function copies the data into the outgoing MQTT buffer to publish the key-value pairs
    /// @return Whether copying the key-value pairs into the outgoing MQTT buffer, was successful or not
    bool Send_Attribute_String(char const * json) {
            return Send_Json_String(ATTRIBUTE_TOPIC, json);
    }

    /// @brief Send key-value pairs as attribute data.
    /// See https://thingsboard.io/docs/user-guide/attribute/ for more information
    /// @param source JsonDocument containing our json key-value pairs,
    // is checked before usage for any possible occuring internal errors. See https://arduinojson.org/v7/api/jsondocument/ for more information
    /// @return Whether copying the key-value pairs into the outgoing MQTT buffer, was successful or not
    bool Send_Attribute_Json(JsonDocument const & source) {
            return Send_Json(ATTRIBUTE_TOPIC, source);
    }

  private:
    using IAPI_Container = Container<IAPI_Implementation *>;

#if THINGSBOARD_ENABLE_STREAM_UTILS
    /// @brief Serializes key-value pairs from the given JsonDocument over the given topic directly into the underlying client
    /// @note The passed JsonDocument data circumvents the copy usually required and instead directly serializes the data into the outgoing MQTT buffer.
    /// This reduces the memory footprint of sending data over MQTT but in exchange increases send times, because the data is sent in smaller packets and not as one big packet.
    /// To increase the serialization speed the buffering_size argument in the constructor can be increased or with the @ref Set_Buffering_Size method
    /// @param topic Non owning pointer to topic that the message is sent over, where different MQTT topics expect a different kind of payload.
    /// Does not need to kept alive as the function copies the data into the outgoing MQTT buffer to publish the given payload
    /// @param source JsonDocument containing our json key-value pairs. See https://arduinojson.org/v7/api/jsondocument/ for more information
    /// @return Whether seriaizing the payload contained in the source directly into the outgoing MQTT buffer, was successful or not
    bool Serialize_Json(char const * topic, JsonDocument const & source) {
            size_t const json_size = Helper::Measure_Json(source);
            if (!m_client.begin_publish(topic, json_size - 1)) {
                    DefaultLogger::printfln(UNABLE_TO_SERIALIZE_JSON);
                    return false;
            }
            BufferingPrint buffered_print(m_client, Get_Buffering_Size());
            auto const bytes_serialized = serializeJson(source, buffered_print);
            if (bytes_serialized == 0) {
                    DefaultLogger::printfln(UNABLE_TO_SERIALIZE_JSON);
                    return false;
            }
            buffered_print.flush();
            return m_client.end_publish();
    }
#endif // THINGSBOARD_ENABLE_STREAM_UTILS

    /// @copydoc IMQTT_Client::subscribe
    bool Subscribe_Topic(char const * topic) {
            return m_client.subscribe(topic);
    }

    /// @copydoc IMQTT_Client::unsubscribe
    bool Unsubscribe_Topic(char const * topic) {
            return m_client.unsubscribe(topic);
    }

    /// @brief Gets a mutable pointer to the request id, the current value is the id of the last sent request
    /// @note Is used because each request to the cloud of the same type (attribute request, rpc request, over the air firmware update), has to use a different id to differentiate which request should receive which response.
    /// To therefore ensure that behaviour across the API implementations we simply provide a global request id that can be used and incremented by all API implementations that require a request ID
    /// @return Mutable pointer to the identifier of the last request
    size_t * Get_Last_Request_ID() {
            return &m_request_id;
    }

    /// @brief Connects to the previously set server, with the given credentials
    /// @param access_token Non owning pointer to access token, that allows to differentiate which MQTT device is sending the traffic to the MQTT broker.
    /// Can be "provision", if the device creates itself instead. See https://thingsboard.io/docs/user-guide/device-provisioning/?mqttprovisioning=without#provision-device-apis for more information.
    /// Additionally it has to be kept alive by the user for the runtime of the MQTT client connection
    /// @param client_id Non owning pointer to client username that is used to authenticate, who is connecting over MQTT.
    /// Additionally it has to be kept alive by the user for the runtime of the MQTT client connection
    /// @param password Non owning pointer to client password that is used to authenticate, who is connecting over MQTT.
    /// Additionally it has to be kept alive by the user for the runtime of the MQTT client connection
    /// @return Whether connecting to ThingsBoard was successful or not
    bool Connect_To_Host(char const * access_token, char const * client_id, char const * password) {
            bool const connection_result = m_client.connect(client_id, access_token, password);
            if (!connection_result) {
                    DefaultLogger::printfln(CONNECT_FAILED);
            }
            return connection_result;
    }

    /// @brief Resubscribes to all permanent subscriptions (RPC, Shared Attribute Update)
    /// @note Permanent subscriptions may receive more than one response over their lifetime,
    /// whereas other events that are only ever called once (single-event subscriptions) and then deleted after they have been handled are not resubscribed.
    /// Only the topics that establish a permanent connection are resubscribed, because all not yet received data is discarded on the MQTT broker,
    /// once a connection has been established again. This is the case because internally the device connects with the MQTT cleanSession attribute set to true.
    /// Therefore we can also clear the buffer of all single-event subscriptions, because they would never receive an answer anyway
    void Resubscribe_Permanent_Subscriptions() {
            for (auto & api : m_api_implementations) {
                    if (api == nullptr) {
                            continue;
                    }
                    // Results are ignored, because the important part of clearing internal data structures always succeeds
                    (void)api->Resubscribe_Permanent_Subscriptions();
            }
    }

    /// @brief Sends the given key-value pair as telemtry or attribute data
    /// @tparam T Type of the passed value
    /// @param key Non owning pointer to the key of the key-value pair.
    /// Does not need to kept alive as the function copies the data into the outgoing MQTT buffer to publish the key-value pair
    /// @param value Value of the key-value pair
    /// @return Whether copying the key-value pair into the outgoing MQTT buffer, was successful or not
    template<typename T>
    bool Send_Key_Value_Pair(char const * key, T const & value, bool telemetry = true) {
            const Telemetry t(key, value);
            if (t.IsEmpty()) {
                    return false;
            }

            JsonDocument json_buffer;
            if (!t.SerializeKeyValue(json_buffer)) {
                    DefaultLogger::printfln(UNABLE_TO_SERIALIZE);
                    return false;
            }
            return telemetry ? Send_Telemetry_Json(json_buffer) : Send_Attribute_Json(json_buffer);
    }

    /// @brief Send aggregated key-value pair as telemetry or attribute data
    /// @note Expects iterators to a container containing Telemetry class instances.
    /// See https://thingsboard.io/docs/user-guide/telemetry/ for more information
    /// @tparam InputIterator Class that allows for forward incrementable access to data
    /// of the given data container, allows for using / passing either std::vector or std::array.
    /// See https://en.cppreference.com/w/cpp/iterator/input_iterator for more information on the requirements of the iterator
    /// @param first Iterator pointing to the first element in the data container
    /// @param last Iterator pointing to the end of the data container (last element + 1)
    /// @return Whether copying the key-value pairs into the outgoing MQTT buffer, was successful or not
    template<typename InputIterator>
    bool Send_Data_Array(InputIterator const & first, InputIterator const & last, bool telemetry) {
            JsonDocument json_buffer;

            if (std::any_of(first, last, [&json_buffer](Telemetry const & data) { return !data.SerializeKeyValue(json_buffer); })) {
                    DefaultLogger::printfln(UNABLE_TO_SERIALIZE);
                    return false;
            }
            return telemetry ? Send_Telemetry_Json(json_buffer) : Send_Attribute_Json(json_buffer);
    }

    /// @brief Internal callback for received MQTT responses
    /// @note Payload contains data from the internal incoming buffer of the MQTT client,
    /// therefore the buffer and the specific memory region the payload points too and the following length bytes need to live on for as long as this method has not finished.
    /// This could be a problem if the system uses FreeRTOS or another tasking system and the processing of the data is interrupted.
    /// Because if this happens and we then send data it is possible for the system to overwrite the memory region that contained the previous response.
    /// Therefore we simply assume that either the used MQTT client, has seperate input and output buffers or that the receiving of data is not executed on a seperate FreeRTOS tasks to other sends.
    /// The first option of seperate input and ouput buffers is the case for all directly in the library implemented MQTT client implementations being @ref Espressif_MQTT_Client and @ref Arduino_MQTT_Client
    /// @param topic Non owning pointer to topic that the message was received over, where different MQTT topics expect a different kind of payload.
    /// Needs to be kept alive for the runtime of the method. Owned by the MQTT client implementation that called this callback method
    /// @param json Non owning pointer to the received payload.
    /// Needs to be kept alive for the runtime of the method. Owned by the MQTT client implementation that called this callback method
    /// @param length Total length of the received payload
    void On_MQTT_Message(char * topic, uint8_t * payload, unsigned int length) {
#if THINGSBOARD_ENABLE_DEBUG
            DefaultLogger::printfln(RECEIVE_MESSAGE, length, topic);
#endif // THINGSBOARD_ENABLE_DEBUG

#if THINGSBOARD_ENABLE_CXX20
            auto filtered_raw_api_implementations = m_api_implementations | std::views::filter([&topic](IAPI_Implementation const * api) {
#else
            IAPI_Container filtered_raw_api_implementations = {};
            std::copy_if(m_api_implementations.begin(), m_api_implementations.end(), std::back_inserter(filtered_raw_api_implementations), [&topic](IAPI_Implementation const * api) {
#endif // THINGSBOARD_ENABLE_CXX20
                    return (api != nullptr && api->Get_Process_Type() == API_Process_Type::RAW && api->Is_Response_Topic_Matching(topic));
            });

            for (auto & api : filtered_raw_api_implementations) {
                    api->Process_Response(topic, payload, length);
            }

            // If the filtered api implementations was not emtpy it means the response was processed as its raw bytes representation atleast once,
            // and because we interpreted it as raw bytes instead of json, we skip the further processing of those raw bytes as json.
            // We do that because the received response is in that case not even valid json in the first place and would therefore simply fail deserialization
            if (!filtered_raw_api_implementations.empty()) {
                    return;
            }

            JsonDocument json_buffer;

            // The deserializeJson method we use, can use the zero copy mode because a writeable input was passed,
            // if that were not the case the needed allocated memory would drastically increase, because the keys would need to be copied as well.
            // See https://arduinojson.org/v7/doc/deserialization/ for more info on ArduinoJson deserialization
            DeserializationError const error = deserializeJson(json_buffer, payload, length);
            if (error) {
                    DefaultLogger::printfln(UNABLE_TO_DE_SERIALIZE_JSON, error.c_str());
                    return;
            }

#if THINGSBOARD_ENABLE_CXX20
            auto filtered_json_api_implementations = m_api_implementations | std::views::filter([&topic](IAPI_Implementation const * api) {
#else
            IAPI_Container filtered_json_api_implementations = {};
            std::copy_if(m_api_implementations.begin(), m_api_implementations.end(), std::back_inserter(filtered_json_api_implementations), [&topic](IAPI_Implementation const * api) {
#endif // THINGSBOARD_ENABLE_CXX20
                    return (api != nullptr && api->Get_Process_Type() == API_Process_Type::JSON && api->Is_Response_Topic_Matching(topic));
            });

            for (auto & api : filtered_json_api_implementations) {
                    api->Process_Json_Response(topic, json_buffer);
            }
    }


    IMQTT_Client&  m_client;              // MQTT client instance.
    size_t         m_max_stack;           // Maximum stack size we allocate at once.
    size_t         m_request_id = {};          // Internal id used to differentiate which request should receive which response for certain API calls. Can send 4'294'967'296 requests before wrapping back to 0
#if THINGSBOARD_ENABLE_STREAM_UTILS
    size_t         m_buffering_size;      // Buffering size used to serialize directly into client.
#endif // THINGSBOARD_ENABLE_STREAM_UTILS
    size_t         m_max_response_size;   // Maximum size allocated on the heap to hold the Json data structure for received cloud response payload, prevents possible malicious payload allocaitng a lot of memory
    IAPI_Container m_api_implementations; // Can hold a pointer to all  possible API implementations (Server side RPC, Client side RPC, Shared attribute update, Client-side or shared attribute request, Provision)
};

#endif // ThingsBoard_h