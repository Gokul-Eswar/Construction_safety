#pragma once
#include "rtsp_source.hpp"
#include "inference/inference_engine.hpp"
#include "spatial/spatial_mapper.hpp"
#include "utils/visualizer.hpp"
#include "utils/mqtt_client.hpp"
#include "utils/config_loader.hpp"
#include "utils/violation_logger.hpp"
#include "utils/alert_throttler.hpp"
#include "tracking/sort_tracker.hpp"
#include "utils/mjpeg_streamer.hpp"
#include <memory>
#include <mutex>

class PipelineManager {
...
    std::unique_ptr<safety::AlertThrottler> alert_throttler_;
    std::unique_ptr<SortTracker> tracker_;
    std::unique_ptr<MJPEGStreamer> streamer_;

    bool running_;
    std::mutex mutex_;
};
