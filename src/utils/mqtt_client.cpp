#include "mqtt_client.hpp"
#include <iostream>

MQTTClient::MQTTClient(const std::string& client_id) : client_id_(client_id), connected_(false) {
}

MQTTClient::~MQTTClient() {
    disconnect();
}

bool MQTTClient::connect(const std::string& host, int port) {
    std::cout << "[Mock MQTT] Connecting to " << host << ":" << port << " as " << client_id_ << "..." << std::endl;
    // Real implementation would use paho library here
    connected_ = true;
    return true;
}

bool MQTTClient::publish(const std::string& topic, const std::string& payload) {
    if (!connected_) return false;
    std::cout << "[Mock MQTT] Publishing to " << topic << ": " << payload << std::endl;
    return true;
}

void MQTTClient::disconnect() {
    if (connected_) {
        std::cout << "[Mock MQTT] Disconnecting..." << std::endl;
        connected_ = false;
    }
}

bool MQTTClient::isConnected() const {
    return connected_;
}
