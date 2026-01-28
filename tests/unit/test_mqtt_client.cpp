#include <gtest/gtest.h>
#include "utils/mqtt_client.hpp"

TEST(MQTTClientTest, ConnectionFlow) {
    MQTTClient client("test_client");
    EXPECT_FALSE(client.isConnected());
    
    EXPECT_TRUE(client.connect("localhost", 1883));
    EXPECT_TRUE(client.isConnected());
    
    client.disconnect();
    EXPECT_FALSE(client.isConnected());
}

TEST(MQTTClientTest, PublishFlow) {
    MQTTClient client("test_client");
    
    // Should fail if not connected
    EXPECT_FALSE(client.publish("test/topic", "hello"));
    
    ASSERT_TRUE(client.connect("localhost", 1883));
    EXPECT_TRUE(client.publish("test/topic", "hello"));
}
