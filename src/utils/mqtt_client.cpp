#include "mqtt_client.hpp"
#include <spdlog/spdlog.h>
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
        if (client_ && client_->is_connected()) {
            return true;
        }

        client_ = std::make_unique<mqtt::async_client>(server_address, client_id_);
        
        mqtt::connect_options connOpts;
        connOpts.set_clean_session(false);
        connOpts.set_keep_alive_interval(20);
        connOpts.set_automatic_reconnect(true);

        spdlog::info("Connecting to MQTT broker at {}...", server_address);
        client_->connect(connOpts)->wait();
        spdlog::info("Connected to MQTT broker.");
        return true;
    } catch (const mqtt::exception& exc) {
        spdlog::error("MQTT Connection Error: {}", exc.what());
        return false;
    }
}

bool MQTTClient::publish(const std::string& topic, const std::string& payload) {
    if (!client_ || !client_->is_connected()) return false;
    
    try {
        client_->publish(topic, payload, 1, false);
        return true;
    } catch (const mqtt::exception& exc) {
        spdlog::error("MQTT Publish Error: {}", exc.what());
        return false;
    }
}

bool MQTTClient::subscribe(const std::string& topic, std::function<void(const std::string&, const std::string&)> callback) {
    if (!client_ || !client_->is_connected()) return false;

    try {
        spdlog::info("Subscribing to topic: {}", topic);
        client_->subscribe(topic, 1)->wait();
        
        client_->set_message_callback([callback](mqtt::const_message_ptr msg) {
            callback(msg->get_topic(), msg->to_string());
        });
        
        return true;
    } catch (const mqtt::exception& exc) {
        spdlog::error("MQTT Subscribe Error: {}", exc.what());
        return false;
    }
}

void MQTTClient::disconnect() {
    if (client_ && client_->is_connected()) {
        try {
            spdlog::info("Disconnecting from MQTT broker...");
            client_->disconnect()->wait();
        } catch (const mqtt::exception& exc) {
            spdlog::error("MQTT Disconnection Error: {}", exc.what());
        }
    }
}

bool MQTTClient::isConnected() const {
    return client_ && client_->is_connected();
}
