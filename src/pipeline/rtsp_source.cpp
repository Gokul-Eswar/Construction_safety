#include "rtsp_source.hpp"
#include <iostream>
#include <gst/app/gstappsink.h>

RTSPSource::RTSPSource(const std::string& uri) 
    : uri_(uri), pipeline_(nullptr), frame_callback_(nullptr), frame_count_(0), current_fps_(0.0) {
}

RTSPSource::~RTSPSource() {
    stop();
}

std::string RTSPSource::getPipelineString() const {
    if (uri_ == "test") {
        return "videotestsrc num-buffers=100 ! video/x-raw,format=I420,framerate=30/1 ! appsink name=mysink emit-signals=true";
    }
    return "rtspsrc location=" + uri_ + " latency=0 ! rtph264depay ! h264parse ! nvv4l2decoder ! appsink name=mysink emit-signals=true";
}

void RTSPSource::setFrameCallback(FrameCallback callback) {
    frame_callback_ = callback;
}

bool RTSPSource::start() {
    GError* error = nullptr;
    std::string pipeline_str = getPipelineString();
    
    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
    
    if (!pipeline_) {
        std::cerr << "Failed to create pipeline: " << (error ? error->message : "Unknown error") << std::endl;
        if (error) g_error_free(error);
        return false;
    }
    
    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline_), "mysink");
    if (sink) {
        g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample), this);
        gst_object_unref(sink);
    } else {
        std::cerr << "Failed to find appsink 'mysink' in pipeline" << std::endl;
        stop();
        return false;
    }
    
    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Failed to set pipeline to PLAYING state" << std::endl;
        stop();
        return false;
    }
    
    start_time_ = std::chrono::steady_clock::now();
    last_fps_check_time_ = start_time_;
    frame_count_ = 0;
    last_frame_count_ = 0;
    
    return true;
}

void RTSPSource::stop() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
}

SourceStats RTSPSource::getStats() const {
    const_cast<RTSPSource*>(this)->updateStats();
    return {frame_count_.load(), current_fps_, pipeline_ != nullptr};
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
        if (self->frame_callback_) {
            self->frame_callback_(sample);
        }
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }
    
    return GST_FLOW_ERROR;
}
