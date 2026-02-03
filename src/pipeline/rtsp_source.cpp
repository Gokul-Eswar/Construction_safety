#include "rtsp_source.hpp"
#include <iostream>
#include <gst/app/gstappsink.h>
#include "../utils/latency_logger.hpp"

RTSPSource::RTSPSource(const std::string& id, const std::string& uri) 
    : id_(id), uri_(uri), pipeline_(nullptr), bus_(nullptr), bus_watch_id_(0),
      frame_callback_(nullptr), frame_count_(0), current_fps_(0.0),
      is_running_(false), should_reconnect_(false) {
}

RTSPSource::~RTSPSource() {
    stop();
}

std::string RTSPSource::getPipelineString() const {
    if (uri_ == "test") {
        return "videotestsrc ! video/x-raw,format=I420,framerate=30/1 ! appsink name=mysink emit-signals=true max-buffers=1 drop=true";
    }
    // Optimized RTSP pipeline with rtspsrc specific retry/timeout properties
    // udp-reconnect=1: reconnect on UDP timeout
    // timeout=5000000: 5 second timeout (in microseconds)
    return "rtspsrc location=" + uri_ + " latency=0 drop-on-latency=true udp-reconnect=1 timeout=5000000 ! "
           "rtph264depay ! h264parse ! decodebin ! "
           "queue max-size-buffers=1 leaky=2 ! "
           "appsink name=mysink emit-signals=true max-buffers=1 drop=true";
}

void RTSPSource::setFrameCallback(FrameCallback callback) {
    frame_callback_ = callback;
}

bool RTSPSource::start() {
    if (is_running_) return true;

    // First attempt to start pipeline
    std::lock_guard<std::mutex> lock(pipeline_mutex_);
    
    GError* error = nullptr;
    std::string pipeline_str = getPipelineString();
    
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
    
    if (!pipeline_) {
        std::cerr << "[" << id_ << "] Failed to create pipeline: " << (error ? error->message : "Unknown error") << std::endl;
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
        std::cerr << "[" << id_ << "] Failed to find appsink 'mysink'" << std::endl;
        // Don't return false here, let reconnection handle it
        should_reconnect_ = true;
    }
    
    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "[" << id_ << "] Failed to set pipeline to PLAYING state" << std::endl;
        should_reconnect_ = true;
    }
    
    start_time_ = std::chrono::steady_clock::now();
    last_fps_check_time_ = start_time_;
    frame_count_ = 0;
    last_frame_count_ = 0;
    
    is_running_ = true;
    should_reconnect_ = false;

    // Start background monitor thread
    reconnection_thread_ = std::thread(&RTSPSource::reconnectionLoop, this);
    
    return true;
}

void RTSPSource::stop() {
    is_running_ = false;
    
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

SourceStats RTSPSource::getStats() const {
    const_cast<RTSPSource*>(this)->updateStats();
    // Active if running AND not currently waiting to reconnect
    bool is_active = is_running_ && !should_reconnect_;
    return {frame_count_.load(), current_fps_, is_active};
}

void RTSPSource::updateStats() {
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

GstFlowReturn RTSPSource::on_new_sample(GstElement* sink, gpointer user_data) {
    RTSPSource* self = static_cast<RTSPSource*>(user_data);
    
    GstSample* sample = nullptr;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    
    if (sample) {
        self->frame_count_++;
        
        // Success means we are definitely connected
        self->should_reconnect_ = false;

        LatencyLogger::getInstance().startTimer(self->id_ + "_e2e", self->frame_count_);

        if (self->frame_callback_) {
            self->frame_callback_(sample);
        }
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }
    
    return GST_FLOW_ERROR;
}

gboolean RTSPSource::on_bus_message(GstBus* bus, GstMessage* msg, gpointer data) {
    RTSPSource* self = static_cast<RTSPSource*>(data);
    self->handleMessage(msg);
    return TRUE; // Keep watching
}

void RTSPSource::handleMessage(GstMessage* msg) {
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug_info;
            gst_message_parse_error(msg, &err, &debug_info);
            std::cerr << "[" << id_ << "] GStreamer Error: " << err->message << std::endl;
            std::cerr << "[" << id_ << "] Debug info: " << (debug_info ? debug_info : "none") << std::endl;
            g_clear_error(&err);
            g_free(debug_info);
            
            // Trigger reconnection
            should_reconnect_ = true;
            break;
        }
        case GST_MESSAGE_EOS:
            std::cerr << "[" << id_ << "] End of Stream (EOS) received." << std::endl;
            should_reconnect_ = true;
            break;
        default:
            break;
    }
}

void RTSPSource::reconnectionLoop() {
    while (is_running_) {
        if (should_reconnect_) {
            std::cout << "[" << id_ << "] Connection lost. Attempting to reconnect in 5s..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            if (!is_running_) break; // Check if stopped during sleep

            std::cout << "[" << id_ << "] Reconnecting..." << std::endl;
            
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
                    
                    gst_element_set_state(pipeline_, GST_STATE_PLAYING);
                    should_reconnect_ = false;
                    std::cout << "[" << id_ << "] Reconnection attempt finished." << std::endl;
                } else {
                     std::cerr << "[" << id_ << "] Rebuild failed: " << (error ? error->message : "Unknown") << std::endl;
                     if (error) g_error_free(error);
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
