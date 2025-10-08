#ifndef Arduino_MQTT_Client_h
#define Arduino_MQTT_Client_h

#ifdef ARDUINO

// Local includes.
#include "IMQTT_Client.h"

// Library include
#include <PubSubClient.h>


/// @brief MQTT Client interface implementation that uses the PubSubClient forked by ThingsBoard (https://github.com/thingsboard/pubsubclient),
/// under the hood to establish and communicate over a MQTT connection
///
/// @note The fork is recommended, because it includes multiple fixes and features:
/// - Solve issues with using std::function callbacks for non ESP boards
/// - Seperats the underlying Heap buffer previously used for both outgoing and incoming messages into two seperate buffers
/// - Multiple other minor bufixes and features
/// Since v0.15.0 only the ThingsBoard fork of the PubSubClient can be used, because splitting the Heap buffer into a input and output buffer resulted in breaking API changes
class Arduino_MQTT_Client : public IMQTT_Client {
  public:
    /// @brief Constructs a IMQTT_Client implementation without a network client, meaning it has to be added later with the set_client() method
    Arduino_MQTT_Client() = default;

    /// @brief Constructs a IMQTT_Client implementation with the given network client
    /// @param transport_client Client that is used to send the actual payload via. MQTT, needs to implement the client interface,
    /// but the actual type of connection does not matter (Ethernet or WiFi)
    Arduino_MQTT_Client(Client & transport_client);

    ~Arduino_MQTT_Client() override = default;

    /// @brief Sets the client has to be used if the empty constructor was used initally
    /// @param transport_client Client that is used to send the actual payload via. MQTT, needs to implement the client interface,
    /// but the actual type of connection does not matter (Ethernet or WiFi)
    void set_client(Client & transport_client);

    void set_data_callback(Callback<void, char *, uint8_t *, unsigned int>::function callback) override;

    void set_connect_callback(Callback<void>::function callback) override;

    bool set_buffer_size(uint16_t receive_buffer_size, uint16_t send_buffer_size) override;

    uint16_t get_receive_buffer_size() override;

    uint16_t get_send_buffer_size() override;

    void set_server(char const * domain, uint16_t port) override;

    bool connect(char const * client_id, char const * user_name, char const * password) override;

    void disconnect() override;

    bool loop() override;

    bool publish(char const * topic, uint8_t const * payload, size_t const & length) override;

    bool subscribe(char const * topic) override;

    bool unsubscribe(char const * topic) override;

    bool connected() override;

    MQTT_Connection_State get_connection_state() const override;

    MQTT_Connection_Error get_last_connection_error() const override;

    void subscribe_connection_state_changed_callback(Callback<void, MQTT_Connection_State, MQTT_Connection_Error>::function callback) override;

#if THINGSBOARD_ENABLE_STREAM_UTILS

    bool begin_publish(char const * topic, size_t const & length) override;

    bool end_publish() override;

    //----------------------------------------------------------------------------
    // Print interface
    //----------------------------------------------------------------------------

    size_t write(uint8_t payload_byte) override;

    size_t write(uint8_t const * buffer, size_t const & size) override;

#endif // THINGSBOARD_ENABLE_STREAM_UTILS

  private:
    MQTT_Connection_Error connect_mqtt_client(char const * client_id, char const * user_name, char const * password);

    /// @brief Updates the interal connection state and informs the subscribed subject, about changes to the internal state
    /// @param new_state New state the connection to the MQTT broker is in now and the subject should be informed about
    void update_connection_state(MQTT_Connection_State new_state);

    MQTT_Connection_State                                        m_connection_state = {};                  // Current connection state to the MQTT broker
    MQTT_Connection_Error                                        m_last_connection_error = {};             // Last error that occured while trying to establish a connection to the MQTT broker
    Callback<void, MQTT_Connection_State, MQTT_Connection_Error> m_connection_state_changed_callback = {}; // Callback that will be called as soon as the mqtt client connection changes
    Callback<void>                                               m_connected_callback = {};                // Callback that will be called as soon as the mqtt client has connected
    PubSubClient                                                 m_mqtt_client = {};                       // Underlying MQTT client instance used to send data
};

#endif // ARDUINO

#endif // Arduino_MQTT_Client_h
