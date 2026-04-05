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
    std::string type = "rtsp"; // Default to rtsp
    std::string uri; // Renamed from rtsp_uri for generality
    std::vector<ZoneConfig> zones;
    std::vector<CalibrationPoint> calibration;
};

// ================================================================================
// Detection Tuning Configuration (YOLO parameters)
// ================================================================================
struct DetectionTuning {
    float confidence_threshold = 0.20f;  // Min confidence for detections
    float nms_threshold = 0.50f;          // Non-Maximum Suppression threshold
    int inference_interval = 1;           // Process every Nth frame (1 = all)
};

// ================================================================================
// Tracking Tuning Configuration (SORT + Kalman filter parameters)
// ================================================================================
struct TrackingTuning {
    int max_age = 45;                     // Frames to keep track alive without detection (occlusion tolerance)
    int min_hits = 3;                     // Minimum detections before track confirmed
    float iou_threshold = 0.3f;           // IOU threshold for detection-to-track association
    float feature_threshold = 0.5f;       // Color histogram Re-ID matching threshold
    int occlusion_extension_frames = 15;  // Extra frames for occluded objects
};

// ================================================================================
// Preprocessing Configuration (CLAHE for extreme lighting)
// ================================================================================
struct PreprocessingConfig {
    bool clahe_enabled = true;            // Enable CLAHE preprocessing
    float clahe_clip_limit = 2.0f;        // Contrast amplification (1.0-4.0)
    int clahe_tile_size = 8;              // Tile grid size (8x8 typical)
    int clahe_blur_kernel = 3;            // Gaussian blur kernel for artifacts
};

// ================================================================================
// Zone Detection Configuration (boundary perspective error handling)
// ================================================================================
struct ZoneDetectionConfig {
    enum class Mode { POINT, FOOTPRINT, CALIBRATED };
    
    Mode mode = Mode::FOOTPRINT;          // Detection mode (point/footprint/calibrated)
    float boundary_margin = 5.0f;         // Pixel margin at zone boundary
    enum class VotingStrategy { ALL, MAJORITY, ANY };
    VotingStrategy footprint_voting = VotingStrategy::MAJORITY;  // Voting strategy for footprint
};

struct AppConfig {
    std::vector<StreamConfig> streams;
    std::string model_path;
    std::string database_path = "safety_violations.db";
    int alert_cooldown = 5000;
    int log_retention_days = 30; // Default to 30 days
    int stream_port = 8081;
    MQTTConfig mqtt;
    
    // ================================================================================
    // Computer Vision Edge Case Tuning Parameters (NEW)
    // ================================================================================
    DetectionTuning detection;
    TrackingTuning tracking;
    PreprocessingConfig preprocessing;
    ZoneDetectionConfig zone_detection;
    
    // Default constructor
    AppConfig() = default;
};

class ConfigLoader {
public:
    static AppConfig load(const std::string& path);
    static bool save(const std::string& path, const AppConfig& config);
};

