#pragma once
#include <string>
#include <functional>
#include <chrono>
#include <atomic>
#include <gst/gst.h>

using FrameCallback = std::function<void(GstSample*)>;

struct SourceStats {
    uint64_t frame_count;
    double fps;
    bool is_running;
};

class RTSPSource {
public:
    RTSPSource(const std::string& uri);
    ~RTSPSource();

    std::string getPipelineString() const;
    
    void setFrameCallback(FrameCallback callback);
    
    bool start();
    void stop();

    SourceStats getStats() const;

private:
    static GstFlowReturn on_new_sample(GstElement* sink, gpointer user_data);
    void updateStats();

    std::string uri_;
    GstElement* pipeline_;
    FrameCallback frame_callback_;

    // Stats
    mutable std::atomic<uint64_t> frame_count_{0};
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point last_fps_check_time_;
    uint64_t last_frame_count_{0};
    mutable double current_fps_{0.0};
};
