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

struct CalibrationPoint {
    cv::Point2f image;
    cv::Point2f world;
};

struct StreamConfig {
    std::string id;
    std::string name;
    std::string rtsp_uri;
    std::vector<ZoneConfig> zones;
    std::vector<CalibrationPoint> calibration;
};

struct AppConfig {
    std::vector<StreamConfig> streams;
    std::string model_path;
    std::string database_path = "safety_violations.db";
    int alert_cooldown = 5000;
    int inference_interval = 1; // Process every Nth frame (1 = all)
    int stream_port = 8081;
    MQTTConfig mqtt;
    
    // Default constructor
    AppConfig() = default;
};

class ConfigLoader {
public:
    static AppConfig load(const std::string& path);
    static bool save(const std::string& path, const AppConfig& config);
};