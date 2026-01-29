#include "pipeline_manager.hpp"
#include <iostream>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>

PipelineManager::PipelineManager(const AppConfig& config)
    : config_(config), running_(false) {
}

PipelineManager::~PipelineManager() {
    stop();
}

bool PipelineManager::init() {
    // 1. Init Engine
    InferenceConfig inf_config;
    inf_config.model_path = config_.model_path;
    engine_ = std::make_unique<InferenceEngine>(inf_config);
    if (!engine_->init()) return false;

    // 2. Init Source
    source_ = std::make_unique<RTSPSource>(config_.rtsp_uri);
    source_->setFrameCallback([this](GstSample* sample) {
        this->onFrameReceived(sample);
    });

    // 3. Init Spatial Mapper
    spatial_mapper_ = std::make_unique<SpatialMapper>();
    // Default calibration or loaded from config could go here

    // 4. Init Visualizer
    visualizer_ = std::make_unique<Visualizer>();

    // 5. Init Persistence & Alerting
    violation_logger_ = std::make_unique<safety::ViolationLogger>();
    if (!violation_logger_->init(config_.database_path)) {
        std::cerr << "Failed to initialize violation logger at " << config_.database_path << std::endl;
        return false;
    }

    alert_throttler_ = std::make_unique<safety::AlertThrottler>();
    alert_throttler_->set_cooldown(config_.alert_cooldown);

    // 6. Init MQTT
    if (!config_.mqtt.host.empty()) {
        mqtt_client_ = std::make_unique<MQTTClient>(config_.mqtt.client_id);
        mqtt_client_->connect(config_.mqtt.host, config_.mqtt.port);
    }

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
                std::vector<std::vector<cv::Point>> all_zones;
                for (const auto& z : config_.zones) all_zones.push_back(z.points);
                visualizer_->drawZones(frame, all_zones);
            }

            // Check alerts
            checkAlerts(detections);
        }

        gst_buffer_unmap(buffer, &map);
    }
}

void PipelineManager::checkAlerts(const std::vector<Detection>& detections) {
    if (config_.zones.empty()) return;

    for (const auto& det : detections) {
        // Use center of bottom of box for ground-plane check
        cv::Point feet(det.box.x + det.box.width / 2, det.box.y + det.box.height);
        
        for (const auto& zone : config_.zones) {
            double dist = cv::pointPolygonTest(zone.points, feet, false);
            if (dist >= 0) { // Inside or on edge
                
                // 1. Log to Database (Persistent Record)
                // We use -1 for object_id if tracking is not yet implemented
                if (violation_logger_) {
                    violation_logger_->log_violation(zone.id, det.confidence, -1);
                }

                // 2. Check Throttler before Alerting
                // We use -1 for object_id for now, meaning "any person in this zone" throttles alerts
                // If tracking were enabled, we would pass det.track_id
                if (alert_throttler_ && alert_throttler_->should_alert(zone.id, -1)) {
                    if (mqtt_client_ && mqtt_client_->isConnected()) {
                        std::string alert = "{\"alert\": \"zone_violation\", \"zone_name\": \"" + zone.name + 
                                        "\", \"class_id\": " + std::to_string(det.class_id) + "}";
                        mqtt_client_->publish(config_.mqtt.topic, alert);
                    }
                }
            }
        }
    }
}
