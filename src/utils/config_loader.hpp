#pragma once
#include <string>
#include <vector>
#include <opencv2/core.hpp>

struct MQTTConfig {
    std::string host = "localhost";
    int port = 1883;
    std::string topic = "safety/alerts";
    std::string client_id = "safety_system_p1";
};

struct ZoneConfig {
    int id;
    std::string name;
    std::vector<cv::Point> points;
};

struct AppConfig {
    std::string rtsp_uri;
    std::string model_path;
    MQTTConfig mqtt;
    std::vector<ZoneConfig> zones;
    
    // Default constructor
    AppConfig() = default;
};

class ConfigLoader {
public:
    static AppConfig load(const std::string& path);
    static bool save(const std::string& path, const AppConfig& config);
};
