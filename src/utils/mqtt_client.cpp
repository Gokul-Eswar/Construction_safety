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
        client_ = std::make_unique<mqtt::async_client>(server_address, client_id_);
        
        mqtt::connect_options connOpts;
        connOpts.set_clean_session(true);

        std::cout << "Connecting to MQTT broker at " << server_address << "..." << "\n";
        client_->connect(connOpts)->wait();
        std::cout << "Connected to MQTT broker." << "\n";
        return true;
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT Connection Error: " << exc.what() << "\n";
        return false;
    }
}

bool MQTTClient::publish(const std::string& topic, const std::string& payload) {
    if (!client_ || !client_->is_connected()) return false;
    
    try {
        client_->publish(topic, payload, 1, false);
        return true;
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT Publish Error: " << exc.what() << "\n";
        return false;
    }
}

bool MQTTClient::subscribe(const std::string& topic, std::function<void(const std::string&, const std::string&)> callback) {
    if (!client_ || !client_->is_connected()) return false;

    try {
        std::cout << "Subscribing to topic: " << topic << "\n";
        client_->subscribe(topic, 1)->wait();
        
        client_->set_message_callback([callback](mqtt::const_message_ptr msg) {
            callback(msg->get_topic(), msg->to_string());
        });
        
        return true;
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT Subscribe Error: " << exc.what() << "\n";
        return false;
    }
}

void MQTTClient::disconnect() {
    if (client_ && client_->is_connected()) {
        try {
            std::cout << "Disconnecting from MQTT broker..." << "\n";
            client_->disconnect()->wait();
        } catch (const mqtt::exception& exc) {
            std::cerr << "MQTT Disconnection Error: " << exc.what() << "\n";
        }
    }
}

bool MQTTClient::isConnected() const {
    return client_ && client_->is_connected();
}
