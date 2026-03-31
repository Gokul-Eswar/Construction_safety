#pragma once
#include "camera_source.hpp"
#include "inference/inference_engine.hpp"
#include "spatial_mapper.hpp"
#include "utils/visualizer.hpp"
#include "utils/mqtt_client.hpp"
#include "utils/config_loader.hpp"
#include "utils/violation_logger.hpp"
#include "utils/alert_throttler.hpp"
#include "sort_tracker.hpp"
#include "utils/mjpeg_streamer.hpp"
#include <memory>
#include <mutex>
#include <vector>
#include <map>
#include <atomic>
#include <ctime>

struct StreamContext {
    std::string id;
    std::string name;
    std::unique_ptr<CameraSource> source;
    std::unique_ptr<SortTracker> tracker;
    std::unique_ptr<SpatialMapper> spatial_mapper;
    std::vector<ZoneConfig> zones;
    cv::Mat last_processed_frame;
    std::mutex frame_mutex;
    uint64_t frame_count = 0;
};

class PipelineManager {
public:
    PipelineManager(const AppConfig& config);
    ~PipelineManager();

    bool init();
    void start();
    void stop();

protected:
    void checkAlerts(const std::string& stream_id, const std::vector<Detection>& detections, const std::vector<ZoneConfig>& zones);

private:
    void onFrameReceived(const std::string& stream_id, GstSample* sample);
    void updateTiledView();

    AppConfig config_;

    std::map<std::string, std::unique_ptr<StreamContext>> streams_;
    
    std::unique_ptr<InferenceEngine> engine_;
    std::unique_ptr<Visualizer> visualizer_;
    std::unique_ptr<MQTTClient> mqtt_client_;
    std::unique_ptr<safety::ViolationLogger> violation_logger_;
    std::unique_ptr<safety::AlertThrottler> alert_throttler_;
    std::unique_ptr<MJPEGStreamer> streamer_;

    bool running_;
    std::mutex mutex_;
    std::atomic<std::time_t> last_activity_;
    
    std::thread tiling_thread_;
};
