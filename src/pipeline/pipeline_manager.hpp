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
    int dynamic_inference_interval = 1;
    double dynamic_input_scale = 1.0;
    size_t estimated_vram_bytes = 0;
    bool admitted = true;
    bool paused_for_vram = false;
    std::string admission_reason;
    uint64_t low_vram_events = 0;
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
    size_t estimatePerStreamVramBytes(int frame_width, int frame_height, int input_width, int input_height) const;
    void enforceRuntimeDegradationPolicy(StreamContext& ctx, const cv::Mat& frame);
    bool shouldRunInferenceForStream(const StreamContext& ctx) const;

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
    size_t admitted_vram_bytes_ = 0;
    size_t last_reported_free_vram_ = 0;
    
    std::thread tiling_thread_;
};
