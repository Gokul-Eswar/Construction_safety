#include "camera_source.hpp"
#include <iostream>
#include <gst/app/gstappsink.h>
#include "../utils/latency_logger.hpp"

CameraSource::CameraSource(const std::string& id, const std::string& type, const std::string& uri) 
    : id_(id), type_(type), uri_(uri), pipeline_(nullptr), bus_(nullptr), bus_watch_id_(0),
      frame_callback_(nullptr), frame_count_(0), current_fps_(0.0),
      is_running_(false), should_reconnect_(false) {
}

CameraSource::~CameraSource() {
    stop();
}

std::string CameraSource::getPipelineString() const {
    if (type_ == "test") {
        return "videotestsrc ! video/x-raw,format=I420,width=1280,height=720,framerate=30/1 ! appsink name=mysink emit-signals=true max-buffers=1 drop=true";
    }

    const char* use_nv = std::getenv("USE_NVIDIA_HW");
    bool nvidia_hw = (use_nv && std::string(use_nv) == "1");

    if (type_ == "usb") {
        // Wired Camera (USB)
        std::string src_element;
#ifdef _WIN32
        src_element = "ksvideosrc";
#else
        src_element = "v4l2src";
#endif
        std::string device_prop = (uri_.length() > 0 && std::isdigit(uri_[0])) ? "device-index=" : "device=";
        
        if (nvidia_hw) {
            return src_element + " " + device_prop + uri_ + " ! "
                   "videoconvert ! videostab ! video/x-raw,format=BGR ! "
                   "queue max-size-buffers=1 leaky=2 ! "
                   "appsink name=mysink emit-signals=true max-buffers=1 drop=true";
        } else {
             return src_element + " " + device_prop + uri_ + " ! "
                   "videoconvert ! videostab ! video/x-raw,format=BGR ! "
                   "queue max-size-buffers=1 leaky=2 ! "
                   "appsink name=mysink emit-signals=true max-buffers=1 drop=true";
        }
    }

    // Default: RTSP High-performance industrial pipeline
    std::string pipeline;
    if (nvidia_hw) {
        // Ultra-Low Latency Settings (latency=0)
        pipeline = "rtspsrc location=" + uri_ + " latency=0 drop-on-latency=true ! "
                   "rtph264depay ! h264parse ! nvv4l2decoder ! "
                   "nvvideoconvert ! videostab ! video/x-raw,format=BGR ! "
                   "queue max-size-buffers=1 leaky=2 ! "
                   "appsink name=mysink emit-signals=true max-buffers=1 drop=true";
    } else {
        pipeline = "rtspsrc location=" + uri_ + " latency=0 drop-on-latency=true ! "
                   "rtph264depay ! h264parse ! decodebin ! "
                   "videoconvert ! videostab ! video/x-raw,format=BGR ! "
                   "queue max-size-buffers=1 leaky=2 ! "
                   "appsink name=mysink emit-signals=true max-buffers=1 drop=true";
    }

    return pipeline;
}

void CameraSource::setFrameCallback(FrameCallback callback) {
    frame_callback_ = callback;
}

bool CameraSource::start() {
    if (is_running_) return true;

    // First attempt to start pipeline
    std::lock_guard<std::mutex> lock(pipeline_mutex_);
    
    GError* error = nullptr;
    std::string pipeline_str = getPipelineString();
    
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
    
    if (!pipeline_) {
        std::cerr << "[" << id_ << "] Failed to create pipeline: " << (error ? error->message : "Unknown error") << "\n";
        if (error) g_error_free(error);
        return false;
    }

    // Bus Watch for Error Handling
    bus_ = gst_element_get_bus(pipeline_);
    bus_watch_id_ = gst_bus_add_watch(bus_, on_bus_message, this);
    
    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline_), "mysink");
    if (sink) {
        g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample), this);
        gst_object_unref(sink);
    } else {
        std::cerr << "[" << id_ << "] Failed to find appsink 'mysink'" << "\n";
        should_reconnect_ = true;
    }
    
    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "[" << id_ << "] Failed to set pipeline to PLAYING state" << "\n";
        should_reconnect_ = true;
    }
    
    start_time_ = std::chrono::steady_clock::now();
    last_fps_check_time_ = start_time_;
    last_frame_received_time_ = start_time_; // Initialize here
    frame_count_ = 0;
    last_frame_count_ = 0;
    
    is_running_ = true;
    should_reconnect_ = false;

    // Start background monitor thread
    reconnection_thread_ = std::thread(&CameraSource::reconnectionLoop, this);
    
    return true;
}

void CameraSource::stop() {
    if (!is_running_) return;

    is_running_ = false;
    should_reconnect_ = false;

    if (reconnection_thread_.joinable()) {
        reconnection_thread_.join();
    }

    std::lock_guard<std::mutex> lock(pipeline_mutex_);
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);

        if (bus_watch_id_ > 0) {
            g_source_remove(bus_watch_id_);
            bus_watch_id_ = 0;
        }

        if (bus_) {
            gst_object_unref(bus_);
            bus_ = nullptr;
        }

        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
}

SourceStats CameraSource::getStats() const {
    const_cast<CameraSource*>(this)->updateStats();
    // Active if running AND not currently waiting to reconnect
    bool is_active = is_running_ && !should_reconnect_;
    return {frame_count_.load(), current_fps_, is_active, is_running_};
}

void CameraSource::updateStats() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps_check_time_).count();
    
    if (elapsed >= 1000) { // Update FPS every second
        uint64_t current_count = frame_count_.load();
        uint64_t frames_since_last = current_count - last_frame_count_;
        current_fps_ = (frames_since_last * 1000.0) / elapsed;
        
        last_frame_count_ = current_count;
        last_fps_check_time_ = now;
    }
}

GstFlowReturn CameraSource::on_new_sample(GstElement* sink, gpointer user_data) {
    CameraSource* self = static_cast<CameraSource*>(user_data);
    
    GstSample* sample = nullptr;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    
    if (sample) {
        self->frame_count_++;
        self->last_frame_received_time_ = std::chrono::steady_clock::now(); // Update timestamp
        
        // Success means we are definitely connected
        self->should_reconnect_ = false;
        self->reconnect_attempt_ = 0;

        LatencyLogger::getInstance().startTimer(self->id_ + "_e2e", self->frame_count_);

        if (self->frame_callback_) {
            self->frame_callback_(sample);
        }
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }
    
    return GST_FLOW_ERROR;
}

gboolean CameraSource::on_bus_message(GstBus* bus, GstMessage* msg, gpointer data) {
    (void)bus;
    CameraSource* self = static_cast<CameraSource*>(data);
    self->handleMessage(msg);
    return TRUE; // Keep watching
}

void CameraSource::handleMessage(GstMessage* msg) {
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug_info;
            gst_message_parse_error(msg, &err, &debug_info);
            std::cerr << "[" << id_ << "] GStreamer Error: " << err->message << "\n";
            std::cerr << "[" << id_ << "] Debug info: " << (debug_info ? debug_info : "none") << "\n";
            g_clear_error(&err);
            g_free(debug_info);
            
            // Trigger reconnection
            should_reconnect_ = true;
            break;
        }
        case GST_MESSAGE_EOS:
            std::cerr << "[" << id_ << "] End of Stream (EOS) received." << "\n";
            should_reconnect_ = true;
            break;
        default:
            break;
    }
}

void CameraSource::reconnectionLoop() {
    while (is_running_) {
        // Staleness check: If no frames for 10 seconds (for RTSP/USB), trigger reconnect
        if (!should_reconnect_ && type_ != "test") {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_frame_received_time_).count();
            if (elapsed > 10) {
                std::cerr << "[" << id_ << "] Stream stale (10s no frames). Forcing reconnection..." << "\n";
                should_reconnect_ = true;
            }
        }

        if (should_reconnect_) {
            // Exponential Backoff: 5s, 10s, 20s, 30s (max)
            int delay_sec = 5 * (1 << reconnect_attempt_);
            if (delay_sec > 30) delay_sec = 30;

            std::cout << "[" << id_ << "] Connection lost (Attempt " << (reconnect_attempt_ + 1) 
                      << "). Reconnecting in " << delay_sec << "s..." << "\n";
            
            // Wait while checking is_running_ frequently
            for (int i = 0; i < delay_sec * 2 && is_running_; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            
            if (!is_running_) break;

            reconnect_attempt_++;
            std::cout << "[" << id_ << "] Reconnecting..." << "\n";
            
            // Teardown existing pipeline
            {
                std::lock_guard<std::mutex> lock(pipeline_mutex_);
                if (pipeline_) {
                    gst_element_set_state(pipeline_, GST_STATE_NULL);
                    if (bus_watch_id_ > 0) {
                        g_source_remove(bus_watch_id_);
                        bus_watch_id_ = 0;
                    }
                    if (bus_) {
                        gst_object_unref(bus_);
                        bus_ = nullptr;
                    }
                    gst_object_unref(pipeline_);
                    pipeline_ = nullptr;
                }

                // Rebuild pipeline
                GError* error = nullptr;
                std::string pipeline_str = getPipelineString();
                pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);

                if (pipeline_) {
                    bus_ = gst_element_get_bus(pipeline_);
                    bus_watch_id_ = gst_bus_add_watch(bus_, on_bus_message, this);

                    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline_), "mysink");
                    if (sink) {
                        g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample), this);
                        gst_object_unref(sink);
                    }
                    
                    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE) {
                        should_reconnect_ = false;
                        last_frame_received_time_ = std::chrono::steady_clock::now(); // Reset staleness
                        std::cout << "[" << id_ << "] Reconnection attempt successful." << "\n";
                    } else {
                        std::cerr << "[" << id_ << "] Failed to set reconnected pipeline to PLAYING state." << "\n";
                    }
                } else {
                     std::cerr << "[" << id_ << "] Rebuild failed: " << (error ? error->message : "Unknown") << "\n";
                     if (error) g_error_free(error);
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
