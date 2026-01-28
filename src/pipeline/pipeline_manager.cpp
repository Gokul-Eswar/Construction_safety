#include "pipeline_manager.hpp"
#include <iostream>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>

PipelineManager::PipelineManager(const std::string& rtsp_uri, const std::string& model_path)
    : rtsp_uri_(rtsp_uri), model_path_(model_path), running_(false) {
}

PipelineManager::~PipelineManager() {
    stop();
}

bool PipelineManager::init() {
    // 1. Init Engine
    InferenceConfig config;
    config.model_path = model_path_;
    engine_ = std::make_unique<InferenceEngine>(config);
    if (!engine_->init()) return false;

    // 2. Init Source
    source_ = std::make_unique<RTSPSource>(rtsp_uri_);
    source_->setFrameCallback([this](GstSample* sample) {
        this->onFrameReceived(sample);
    });

    // 3. Init Spatial Mapper
    spatial_mapper_ = std::make_unique<SpatialMapper>();
    // Default calibration would go here or be loaded from config

    // 4. Init Visualizer
    visualizer_ = std::make_unique<Visualizer>();

    return true;
}

void PipelineManager::start() {
    if (source_ && source_->start()) {
        running_ = true;
        std::cout << "Pipeline started." << std::endl;
    }
}

void PipelineManager::stop() {
    if (source_) {
        source_->stop();
    }
    if (mqtt_client_) {
        mqtt_client_->disconnect();
    }
    running_ = false;
    std::cout << "Pipeline stopped." << std::endl;
}

void PipelineManager::setSafetyZones(const std::vector<std::vector<cv::Point>>& zones) {
    std::lock_guard<std::mutex> lock(mutex_);
    safety_zones_ = zones;
}

void PipelineManager::setMQTTConfig(const std::string& host, int port, const std::string& topic) {
    mqtt_client_ = std::make_unique<MQTTClient>("safety_system_p1");
    mqtt_client_->connect(host, port);
    mqtt_topic_ = topic;
}

void PipelineManager::onFrameReceived(GstSample* sample) {
    if (!running_) return;

    GstCaps* caps = gst_sample_get_caps(sample);
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!caps || !buffer) return;

    GstVideoInfo info;
    if (!gst_video_info_from_caps(&info, caps)) return;

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        cv::Mat frame;
        if (GST_VIDEO_INFO_FORMAT(&info) == GST_VIDEO_FORMAT_I420) {
            cv::Mat yuv(info.height + info.height / 2, info.width, CV_8UC1, map.data);
            cv::cvtColor(yuv, frame, cv::COLOR_YUV2BGR_I420);
        } else if (GST_VIDEO_INFO_FORMAT(&info) == GST_VIDEO_FORMAT_BGR) {
            frame = cv::Mat(info.height, info.width, CV_8UC3, map.data).clone();
        }

        if (!frame.empty()) {
            auto detections = engine_->runInference(frame);
            
            // Draw detections
            visualizer_->drawDetections(frame, detections);
            
            // Draw zones
            {
                std::lock_guard<std::mutex> lock(mutex_);
                visualizer_->drawZones(frame, safety_zones_);
            }

            // Check alerts
            checkAlerts(detections);
        }

        gst_buffer_unmap(buffer, &map);
    }
}

void PipelineManager::checkAlerts(const std::vector<Detection>& detections) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (safety_zones_.empty() || !mqtt_client_ || !mqtt_client_->isConnected()) return;

    for (const auto& det : detections) {
        // Use center of bottom of box for ground-plane check
        cv::Point feet(det.box.x + det.box.width / 2, det.box.y + det.box.height);
        
        for (size_t i = 0; i < safety_zones_.size(); ++i) {
            double dist = cv::pointPolygonTest(safety_zones_[i], feet, false);
            if (dist >= 0) { // Inside or on edge
                std::string alert = "{\"alert\": \"zone_violation\", \"zone_id\": " + std::to_string(i) + 
                                   ", \"class_id\": " + std::to_string(det.class_id) + "}";
                mqtt_client_->publish(mqtt_topic_, alert);
            }
        }
    }
}
