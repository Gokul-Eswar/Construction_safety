#pragma once

#include <string>
#include <gst/gst.h>
#include <functional>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>

struct SourceStats {
    uint64_t frame_count;
    double fps;
    bool active;
    bool is_running;
};

class RTSPSource {
public:
    using FrameCallback = std::function<void(GstSample*)>;

    RTSPSource(const std::string& id, const std::string& uri);
    ~RTSPSource();

    bool start();
    void stop();
    void setFrameCallback(FrameCallback callback);
    SourceStats getStats() const;
    std::string getPipelineString() const;

private:
    static GstFlowReturn on_new_sample(GstElement* sink, gpointer user_data);
    static gboolean on_bus_message(GstBus* bus, GstMessage* msg, gpointer data);
    
    void updateStats();
    void handleMessage(GstMessage* msg);
    void reconnectionLoop();

    std::string id_;
    std::string uri_;
    GstElement* pipeline_;
    GstBus* bus_;
    guint bus_watch_id_;
    
    FrameCallback frame_callback_;
    
    // Stats
    std::atomic<uint64_t> frame_count_;
    uint64_t last_frame_count_;
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point last_fps_check_time_;
    double current_fps_;

    // Reconnection & State
    std::atomic<bool> is_running_;
    std::atomic<bool> should_reconnect_;
    std::thread reconnection_thread_;
    std::mutex pipeline_mutex_;
    
    int reconnect_attempt_ = 0;
};
