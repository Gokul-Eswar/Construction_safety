#pragma once
#include "rtsp_source.hpp"
#include "inference/inference_engine.hpp"
#include "spatial/spatial_mapper.hpp"
#include "utils/visualizer.hpp"
#include "utils/mqtt_client.hpp"
#include <memory>
#include <mutex>

class PipelineManager {
public:
    PipelineManager(const std::string& rtsp_uri, const std::string& model_path);
    ~PipelineManager();

    bool init();
    void start();
    void stop();

    void setSafetyZones(const std::vector<std::vector<cv::Point>>& zones);
    void setMQTTConfig(const std::string& host, int port, const std::string& topic);

private:
    void onFrameReceived(GstSample* sample);
    void checkAlerts(const std::vector<Detection>& detections);

    std::string rtsp_uri_;
    std::string model_path_;

    std::unique_ptr<RTSPSource> source_;
    std::unique_ptr<InferenceEngine> engine_;
    std::unique_ptr<SpatialMapper> spatial_mapper_;
    std::unique_ptr<Visualizer> visualizer_;
    std::unique_ptr<MQTTClient> mqtt_client_;

    std::vector<std::vector<cv::Point>> safety_zones_;
    std::string mqtt_topic_;
    
    bool running_;
    std::mutex mutex_;
};
