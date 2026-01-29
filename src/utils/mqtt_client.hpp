#pragma once
#include <string>
#include <memory>

namespace mqtt {
    class async_client;
}

class IMQTTClient {
public:
    virtual ~IMQTTClient() = default;
    virtual bool connect(const std::string& host, int port) = 0;
    virtual bool publish(const std::string& topic, const std::string& payload) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
};

class MQTTClient : public IMQTTClient {
public:
    MQTTClient(const std::string& client_id);
    ~MQTTClient() override;

    bool connect(const std::string& host, int port) override;
    bool publish(const std::string& topic, const std::string& payload) override;
    void disconnect() override;
    bool isConnected() const override;

private:
    std::string client_id_;
    std::unique_ptr<mqtt::async_client> client_;
};
