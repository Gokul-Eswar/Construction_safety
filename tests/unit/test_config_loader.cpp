#include <gtest/gtest.h>
#include "utils/config_loader.hpp"
#include <fstream>
#include <cstdio>

TEST(ConfigLoaderTest, LoadValidConfig) {
    std::string config_path = "test_config.json";
    std::ofstream f(config_path);
    f << R"({
        "streams": [
            {
                "id": "cam1",
                "name": "Camera 1",
                "rtsp_uri": "rtsp://test",
                "zones": [
                    {
                        "id": 1,
                        "name": "ZoneA",
                        "points": [[10, 10], [100, 100]]
                    }
                ]
            }
        ],
        "model_path": "model.engine",
        "mqtt": {
            "host": "broker",
            "port": 1234
        }
    })";
    f.close();

    AppConfig config = ConfigLoader::load(config_path);
    
    ASSERT_EQ(config.streams.size(), 1);
    EXPECT_EQ(config.streams[0].rtsp_uri, "rtsp://test");
    EXPECT_EQ(config.model_path, "model.engine");
    EXPECT_EQ(config.mqtt.host, "broker");
    EXPECT_EQ(config.mqtt.port, 1234);
    ASSERT_EQ(config.streams[0].zones.size(), 1);
    EXPECT_EQ(config.streams[0].zones[0].name, "ZoneA");
    ASSERT_EQ(config.streams[0].zones[0].points.size(), 2);
    EXPECT_EQ(config.streams[0].zones[0].points[0].x, 10);

    std::remove(config_path.c_str());
}

TEST(ConfigLoaderTest, SaveConfig) {
    AppConfig config;
    StreamConfig sc;
    sc.id = "cam1";
    sc.rtsp_uri = "rtsp://saved";
    sc.zones.push_back({1, "Test", {{0,0}}});
    config.streams.push_back(sc);
    
    std::string config_path = "saved_config.json";
    EXPECT_TRUE(ConfigLoader::save(config_path, config));
    
    AppConfig loaded = ConfigLoader::load(config_path);
    ASSERT_EQ(loaded.streams.size(), 1);
    EXPECT_EQ(loaded.streams[0].rtsp_uri, "rtsp://saved");
    
    std::remove(config_path.c_str());
}
