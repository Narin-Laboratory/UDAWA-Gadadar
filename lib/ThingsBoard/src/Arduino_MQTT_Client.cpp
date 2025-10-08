// Header include.
#include "Arduino_MQTT_Client.h"

#ifdef ARDUINO

Arduino_MQTT_Client::Arduino_MQTT_Client(Client & transport_client) :
    m_connected_callback(),
    m_mqtt_client(transport_client)
{
    // Nothing to do
}

void Arduino_MQTT_Client::set_client(Client & transport_client) {
    m_mqtt_client.setClient(transport_client);
}

void Arduino_MQTT_Client::set_data_callback(Callback<void, char *, uint8_t *, unsigned int>::function callback) {
    m_mqtt_client.setCallback(callback);
}

void Arduino_MQTT_Client::set_connect_callback(Callback<void>::function callback) {
    m_connected_callback.Set_Callback(callback);
}

bool Arduino_MQTT_Client::set_buffer_size(uint16_t receive_buffer_size, uint16_t send_buffer_size) {
    return m_mqtt_client.setBufferSize(receive_buffer_size, send_buffer_size);
}

uint16_t Arduino_MQTT_Client::get_receive_buffer_size() {
    return m_mqtt_client.getReceiveBufferSize();
}

uint16_t Arduino_MQTT_Client::get_send_buffer_size() {
    return m_mqtt_client.getSendBufferSize();
}

void Arduino_MQTT_Client::set_server(char const * domain, uint16_t port) {
    m_mqtt_client.setServer(domain, port);
}

bool Arduino_MQTT_Client::connect(char const * client_id, char const * user_name, char const * password) {
    update_connection_state(MQTT_Connection_State::CONNECTING);
    MQTT_Connection_Error const connection_error = connect_mqtt_client(client_id, user_name, password);
    bool const result = connection_error == MQTT_Connection_Error::NONE;
    if (result) {
        m_connected_callback.Call_Callback();
        update_connection_state(MQTT_Connection_State::CONNECTED);
        return result;
    }
    m_last_connection_error = connection_error;
    update_connection_state(MQTT_Connection_State::ERROR);
    return result;
}

void Arduino_MQTT_Client::disconnect() {
    update_connection_state(MQTT_Connection_State::DISCONNECTING);
    m_mqtt_client.disconnect();
    update_connection_state(MQTT_Connection_State::DISCONNECTED);
}

bool Arduino_MQTT_Client::loop() {
    return m_mqtt_client.loop();
}

bool Arduino_MQTT_Client::publish(char const * topic, uint8_t const * payload, size_t const & length) {
    return m_mqtt_client.publish(topic, payload, length, false);
}

bool Arduino_MQTT_Client::subscribe(char const * topic) {
    return m_mqtt_client.subscribe(topic);
}

bool Arduino_MQTT_Client::unsubscribe(char const * topic) {
    return m_mqtt_client.unsubscribe(topic);
}

bool Arduino_MQTT_Client::connected() {
    return m_mqtt_client.connected();
}

MQTT_Connection_State Arduino_MQTT_Client::get_connection_state() const {
    return m_connection_state;
}

MQTT_Connection_Error Arduino_MQTT_Client::get_last_connection_error() const {
    return m_last_connection_error;
}

void Arduino_MQTT_Client::subscribe_connection_state_changed_callback(Callback<void, MQTT_Connection_State, MQTT_Connection_Error>::function callback) {
    m_connection_state_changed_callback.Set_Callback(callback);
}

#if THINGSBOARD_ENABLE_STREAM_UTILS

bool Arduino_MQTT_Client::begin_publish(char const * topic, size_t const & length) {
    return m_mqtt_client.beginPublish(topic, length, false);
}

bool Arduino_MQTT_Client::end_publish() {
    return m_mqtt_client.endPublish();
}

size_t Arduino_MQTT_Client::write(uint8_t payload_byte) {
    return m_mqtt_client.write(payload_byte);
}

size_t Arduino_MQTT_Client::write(uint8_t const * buffer, size_t const & size) {
    return m_mqtt_client.write(buffer, size);
}

#endif // THINGSBOARD_ENABLE_STREAM_UTILS

MQTT_Connection_Error Arduino_MQTT_Client::connect_mqtt_client(char const * client_id, char const * user_name, char const * password) {
    m_mqtt_client.connect(client_id, user_name, password);
    int const current_state = m_mqtt_client.state();
    return current_state < 0 ?  MQTT_Connection_Error::REFUSE_SERVER_UNAVAILABLE : static_cast<MQTT_Connection_Error>(current_state);
}

void Arduino_MQTT_Client::update_connection_state(MQTT_Connection_State new_state) {
  m_connection_state = new_state;
  m_connection_state_changed_callback.Call_Callback(get_connection_state(), get_last_connection_error());
}

#endif // ARDUINO
