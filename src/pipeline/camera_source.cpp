#include "camera_source.hpp"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <gst/app/gstappsink.h>

#include "../utils/latency_logger.hpp"

CameraSource::CameraSource(const std::string& id, const std::string& type, const std::string& uri)
    : id_(id),
      type_(type),
      uri_(uri),
      pipeline_(nullptr),
      bus_(nullptr),
      bus_watch_id_(0),
      frame_callback_(nullptr),
      frame_count_(0),
      current_fps_(0.0),
      is_running_(false),
      should_reconnect_(false),
      stream_state_(StreamState::Failed) {
}

CameraSource::~CameraSource() {
    stop();
}

std::string CameraSource::getPipelineString() const {
    if (type_ != "rtsp" && type_ != "usb") {
        std::cerr << "[" << id_ << "] Unsupported camera type: " << type_ << " (supported: rtsp, usb)" << "\n";
        return "";
    }

    const char* use_nv = std::getenv("USE_NVIDIA_HW");
    bool nvidia_hw = (use_nv && std::string(use_nv) == "1");

    if (type_ == "usb") {
        std::string src_element;
#ifdef _WIN32
        src_element = "ksvideosrc";
#else
        src_element = "v4l2src";
#endif
        std::string device_prop = (uri_.length() > 0 && std::isdigit(uri_[0])) ? "device-index=" : "device=";

        return src_element + " " + device_prop + uri_ + " ! "
               "videoconvert ! videostab ! video/x-raw,format=BGR ! "
               "queue max-size-buffers=1 leaky=2 ! "
               "appsink name=mysink emit-signals=true max-buffers=1 drop=true";
    }

    if (nvidia_hw) {
        return "rtspsrc location=" + uri_ + " latency=0 drop-on-latency=true ! "
               "rtph264depay ! h264parse ! nvv4l2decoder ! "
               "nvvideoconvert ! videostab ! video/x-raw,format=BGR ! "
               "queue max-size-buffers=1 leaky=2 ! "
               "appsink name=mysink emit-signals=true max-buffers=1 drop=true";
    }

    return "rtspsrc location=" + uri_ + " latency=0 drop-on-latency=true ! "
           "rtph264depay ! h264parse ! decodebin ! "
           "videoconvert ! videostab ! video/x-raw,format=BGR ! "
           "queue max-size-buffers=1 leaky=2 ! "
           "appsink name=mysink emit-signals=true max-buffers=1 drop=true";
}

void CameraSource::setFrameCallback(FrameCallback callback) {
    frame_callback_ = callback;
}

void CameraSource::transitionTo(StreamState state, const std::string& reason) {
    StreamState previous = stream_state_.exchange(state);
    {
        std::lock_guard<std::mutex> guard(state_mutex_);
        last_error_ = reason;
    }

    if (previous != state) {
        std::cout << "[" << id_ << "] State " << static_cast<int>(previous)
                  << " -> " << static_cast<int>(state) << ": " << reason << "\n";
    }
}

bool CameraSource::teardownPipelineWithTimeout(std::chrono::milliseconds timeout) {
    if (!pipeline_) {
        return true;
    }

    gst_element_set_state(pipeline_, GST_STATE_NULL);
    GstState current = GST_STATE_NULL;
    GstState pending = GST_STATE_NULL;
    GstStateChangeReturn ret = gst_element_get_state(pipeline_, &current, &pending, timeout.count() * GST_MSECOND);

    if (ret == GST_STATE_CHANGE_FAILURE || ret == GST_STATE_CHANGE_ASYNC) {
        ++teardown_timeout_count_;
        transitionTo(StreamState::Degraded, "teardown timeout/failure");
    }

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

    return ret == GST_STATE_CHANGE_SUCCESS || ret == GST_STATE_CHANGE_NO_PREROLL;
}

bool CameraSource::startPipelineWithTimeout(std::chrono::milliseconds timeout) {
    GError* error = nullptr;
    std::string pipeline_str = getPipelineString();
    if (pipeline_str.empty()) {
        transitionTo(StreamState::Failed, "empty pipeline string");
        return false;
    }

    pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
    if (!pipeline_) {
        std::string err = error ? error->message : "unknown parse launch error";
        if (error) {
            g_error_free(error);
        }
        ++error_count_;
        transitionTo(StreamState::Failed, "pipeline creation failed: " + err);
        return false;
    }

    bus_ = gst_element_get_bus(pipeline_);
    if (bus_) {
        // Keep existing watch path for compatibility with GLib loop deployments.
        bus_watch_id_ = gst_bus_add_watch(bus_, on_bus_message, this);
    }

    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline_), "mysink");
    if (!sink) {
        ++error_count_;
        transitionTo(StreamState::Degraded, "appsink 'mysink' not found");
        return false;
    }

    g_signal_connect(sink, "new-sample", G_CALLBACK(on_new_sample), this);
    gst_object_unref(sink);

    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        ++error_count_;
        ++restart_timeout_count_;
        transitionTo(StreamState::Degraded, "PLAYING state transition failed");
        return false;
    }

    GstState current = GST_STATE_NULL;
    GstState pending = GST_STATE_NULL;
    GstStateChangeReturn wait_ret = gst_element_get_state(pipeline_, &current, &pending, timeout.count() * GST_MSECOND);
    if (wait_ret == GST_STATE_CHANGE_FAILURE || wait_ret == GST_STATE_CHANGE_ASYNC) {
        ++restart_timeout_count_;
        transitionTo(StreamState::Degraded, "PLAYING transition timeout");
        return false;
    }

    transitionTo(StreamState::Running, "pipeline started");
    return true;
}

bool CameraSource::start() {
    if (is_running_) {
        return true;
    }

    std::lock_guard<std::mutex> lock(pipeline_mutex_);

    if (!startPipelineWithTimeout(std::chrono::milliseconds(3000))) {
        teardownPipelineWithTimeout(std::chrono::milliseconds(1000));
        return false;
    }

    start_time_ = std::chrono::steady_clock::now();
    last_fps_check_time_ = start_time_;
    last_frame_received_time_ = start_time_;
    frame_count_ = 0;
    last_frame_count_ = 0;
    reconnect_attempt_ = 0;

    is_running_ = true;
    should_reconnect_ = false;

    reconnection_thread_ = std::thread(&CameraSource::reconnectionLoop, this);
    return true;
}

void CameraSource::stop() {
    if (!is_running_) {
        return;
    }

    is_running_ = false;
    should_reconnect_ = false;

    if (reconnection_thread_.joinable()) {
        reconnection_thread_.join();
    }

    std::lock_guard<std::mutex> lock(pipeline_mutex_);
    teardownPipelineWithTimeout(std::chrono::milliseconds(1500));
    transitionTo(StreamState::Failed, "source stopped");
}

SourceStats CameraSource::getStats() const {
    const_cast<CameraSource*>(this)->updateStats();
    bool is_active = is_running_ && !should_reconnect_;

    SourceStats stats;
    stats.frame_count = frame_count_.load();
    stats.fps = current_fps_;
    stats.active = is_active;
    stats.is_running = is_running_;
    stats.state = stream_state_.load();
    stats.reconnect_attempt = reconnect_attempt_;
    stats.reconnect_count = reconnect_count_;
    stats.error_count = error_count_;
    stats.stale_timeout_count = stale_timeout_count_;
    stats.restart_timeout_count = restart_timeout_count_;
    stats.teardown_timeout_count = teardown_timeout_count_;

    {
        std::lock_guard<std::mutex> guard(state_mutex_);
        stats.last_error = last_error_;
    }

    return stats;
}

void CameraSource::updateStats() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps_check_time_).count();

    if (elapsed >= 1000) {
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
        self->last_frame_received_time_ = std::chrono::steady_clock::now();
        self->should_reconnect_ = false;
        self->reconnect_attempt_ = 0;

        if (self->stream_state_.load() != StreamState::Running) {
            self->transitionTo(StreamState::Running, "frame flow resumed");
        }

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
    return TRUE;
}

void CameraSource::handleMessage(GstMessage* msg) {
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError* err = nullptr;
            gchar* debug_info = nullptr;
            gst_message_parse_error(msg, &err, &debug_info);

            std::string err_msg = err ? err->message : "unknown bus error";
            std::cerr << "[" << id_ << "] GStreamer Error: " << err_msg << "\n";
            std::cerr << "[" << id_ << "] Debug info: " << (debug_info ? debug_info : "none") << "\n";

            if (err) {
                g_clear_error(&err);
            }
            g_free(debug_info);

            ++error_count_;
            should_reconnect_ = true;
            transitionTo(StreamState::Reconnecting, "bus error: " + err_msg);
            break;
        }
        case GST_MESSAGE_EOS:
            std::cerr << "[" << id_ << "] End of Stream (EOS) received." << "\n";
            ++error_count_;
            should_reconnect_ = true;
            transitionTo(StreamState::Reconnecting, "EOS received");
            break;
        default:
            break;
    }
}

void CameraSource::pumpBusMessages() {
    std::lock_guard<std::mutex> lock(pipeline_mutex_);
    if (!bus_) {
        return;
    }

    while (true) {
        GstMessage* msg = gst_bus_timed_pop_filtered(
            bus_,
            0,
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_WARNING));

        if (!msg) {
            break;
        }

        handleMessage(msg);
        gst_message_unref(msg);
    }
}

void CameraSource::reconnectionLoop() {
    while (is_running_) {
        // Explicit polling keeps bus/error handling live even without GLib main loop.
        pumpBusMessages();

        if (!should_reconnect_) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_frame_received_time_).count();
            if (elapsed > 10) {
                ++stale_timeout_count_;
                should_reconnect_ = true;
                transitionTo(StreamState::Degraded, "stale stream: no frames for 10s");
            }
        }

        if (should_reconnect_) {
            int delay_sec = 5 * (1 << reconnect_attempt_);
            if (delay_sec > 30) {
                delay_sec = 30;
            }

            transitionTo(StreamState::Reconnecting, "attempt " + std::to_string(reconnect_attempt_ + 1));
            std::cout << "[" << id_ << "] Reconnecting in " << delay_sec << "s (attempt "
                      << (reconnect_attempt_ + 1) << ")" << "\n";

            for (int i = 0; i < delay_sec * 2 && is_running_; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            if (!is_running_) {
                break;
            }

            ++reconnect_attempt_;
            ++reconnect_count_;

            bool start_ok = false;
            {
                std::lock_guard<std::mutex> lock(pipeline_mutex_);
                teardownPipelineWithTimeout(std::chrono::milliseconds(1200));
                start_ok = startPipelineWithTimeout(std::chrono::milliseconds(3000));
            }

            if (start_ok) {
                should_reconnect_ = false;
                reconnect_attempt_ = 0;
                last_frame_received_time_ = std::chrono::steady_clock::now();
                transitionTo(StreamState::Running, "reconnection successful");
            } else {
                transitionTo(StreamState::Failed, "reconnection failed");
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
