#include "mqtt_client.hpp"
#include <iostream>
#include <mqtt/async_client.h>

MQTTClient::MQTTClient(const std::string& client_id) : client_id_(client_id) {
}

MQTTClient::~MQTTClient() {
    disconnect();
}

bool MQTTClient::connect(const std::string& host, int port) {
    std::string server_address = "tcp://" + host + ":" + std::to_string(port);
    try {
        client_ = std::make_unique<paho::mqtt::async_client>(server_address, client_id_);
        
        paho::mqtt::connect_options connOpts;
        connOpts.set_clean_session(true);

        std::cout << "Connecting to MQTT broker at " << server_address << "..." << std::endl;
        client_->connect(connOpts)->wait();
        std::cout << "Connected to MQTT broker." << std::endl;
        return true;
    } catch (const paho::mqtt::exception& exc) {
        std::cerr << "MQTT Connection Error: " << exc.what() << std::endl;
        return false;
    }
}

bool MQTTClient::publish(const std::string& topic, const std::string& payload) {
    if (!client_ || !client_->is_connected()) return false;
    
    try {
        client_->publish(topic, payload, 1, false);
        return true;
    } catch (const paho::mqtt::exception& exc) {
        std::cerr << "MQTT Publish Error: " << exc.what() << std::endl;
        return false;
    }
}

void MQTTClient::disconnect() {
    if (client_ && client_->is_connected()) {
        try {
            std::cout << "Disconnecting from MQTT broker..." << std::endl;
            client_->disconnect()->wait();
        } catch (const paho::mqtt::exception& exc) {
            std::cerr << "MQTT Disconnection Error: " << exc.what() << std::endl;
        }
    }
}

bool MQTTClient::isConnected() const {
    return client_ && client_->is_connected();
}
