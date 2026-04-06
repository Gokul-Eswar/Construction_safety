#pragma once

#include <string>
#include <gst/gst.h>
#include <functional>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>

enum class StreamState {
    Running,
    Degraded,
    Reconnecting,
    Failed
};

struct SourceStats {
    uint64_t frame_count;
    double fps;
    bool active;
    bool is_running;
    StreamState state;
    int reconnect_attempt;
    uint64_t reconnect_count;
    uint64_t error_count;
    uint64_t stale_timeout_count;
    uint64_t restart_timeout_count;
    uint64_t teardown_timeout_count;
    std::string last_error;
};

class CameraSource {
public:
    using FrameCallback = std::function<void(GstSample*)>;

    CameraSource(const std::string& id, const std::string& type, const std::string& uri);
    ~CameraSource();

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
    void pumpBusMessages();
    bool teardownPipelineWithTimeout(std::chrono::milliseconds timeout);
    bool startPipelineWithTimeout(std::chrono::milliseconds timeout);
    void transitionTo(StreamState state, const std::string& reason);

    std::string id_;
    std::string type_;
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
    std::atomic<StreamState> stream_state_;
    std::thread reconnection_thread_;
    std::mutex pipeline_mutex_;
    mutable std::mutex state_mutex_;
    
    int reconnect_attempt_ = 0;
    uint64_t reconnect_count_ = 0;
    uint64_t error_count_ = 0;
    uint64_t stale_timeout_count_ = 0;
    uint64_t restart_timeout_count_ = 0;
    uint64_t teardown_timeout_count_ = 0;
    std::string last_error_;
    std::chrono::steady_clock::time_point last_frame_received_time_;
};
