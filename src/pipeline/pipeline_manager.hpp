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

private:
    void onFrameReceived(GstSample* sample);

    std::string rtsp_uri_;
    std::string model_path_;

    std::unique_ptr<RTSPSource> source_;
    std::unique_ptr<InferenceEngine> engine_;
    
    // Phase 2 Integration
    std::unique_ptr<SpatialMapper> spatial_mapper_;
    std::unique_ptr<Visualizer> visualizer_;
    
    bool running_;
    std::mutex mutex_;
};
