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
    // 1. Init Engine (Shared)
    InferenceConfig inf_config;
    inf_config.model_path = config_.model_path;
    engine_ = std::make_unique<InferenceEngine>(inf_config);
    if (!engine_->init()) return false;

    // 2. Init Shared Utilities
    violation_logger_ = std::make_unique<safety::ViolationLogger>();
    if (!violation_logger_->init(config_.database_path)) {
        std::cerr << "Failed to initialize violation logger." << std::endl;
        return false;
    }

    alert_throttler_ = std::make_unique<safety::AlertThrottler>();
    alert_throttler_->set_cooldown(config_.alert_cooldown);

    visualizer_ = std::make_unique<Visualizer>();
    
    streamer_ = std::make_unique<MJPEGStreamer>();
    streamer_->start(config_.stream_port);

    if (!config_.mqtt.host.empty()) {
        mqtt_client_ = std::make_unique<MQTTClient>(config_.mqtt.client_id);
        mqtt_client_->connect(config_.mqtt.host, config_.mqtt.port);
    }

    // 3. Init Streams
    for (const auto& sc : config_.streams) {
        auto ctx = std::make_unique<StreamContext>();
        ctx->id = sc.id;
        ctx->name = sc.name;
        ctx->zones = sc.zones;
        ctx->tracker = std::make_unique<SortTracker>();
        ctx->source = std::make_unique<RTSPSource>(sc.rtsp_uri);
        
        ctx->source->setFrameCallback([this, id = sc.id](GstSample* sample) {
            this->onFrameReceived(id, sample);
        });

        streams_[sc.id] = std::move(ctx);
    }

    return true;
}

void PipelineManager::start() {
    running_ = true;
    for (auto& pair : streams_) {
        if (!pair.second->source->start()) {
            std::cerr << "Failed to start stream: " << pair.first << std::endl;
        }
    }
    
    tiling_thread_ = std::thread(&PipelineManager::updateTiledView, this);
    std::cout << "All pipelines started." << std::endl;
}

void PipelineManager::stop() {
    running_ = false;
    if (tiling_thread_.joinable()) tiling_thread_.join();

    for (auto& pair : streams_) {
        pair.second->source->stop();
    }
    if (mqtt_client_) {
        mqtt_client_->disconnect();
    }
    std::cout << "All pipelines stopped." << std::endl;
}

void PipelineManager::onFrameReceived(const std::string& stream_id, GstSample* sample) {
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
            auto it = streams_.find(stream_id);
            if (it != streams_.end()) {
                auto& ctx = it->second;

                // 1. Inference (Shared engine is thread-safe if it uses separate execution contexts, 
                // but currently we might need a mutex or multiple TRT contexts if running in parallel.
                // For simplicity in prototype, we'll mutex the inference forward pass.)
                std::vector<Detection> raw_detections;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    raw_detections = engine_->runInference(frame);
                }
                
                // 2. Tracking (Per stream)
                auto detections = ctx->tracker->update(raw_detections);
                
                // 3. Visualization
                visualizer_->drawDetections(frame, detections);
                std::vector<std::vector<cv::Point>> zone_points;
                for (const auto& z : ctx->zones) zone_points.push_back(z.points);
                visualizer_->drawZones(frame, zone_points);
                
                // Add stream name
                cv::putText(frame, ctx->name, cv::Point(20, 40), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 255), 2);

                // 4. Alerting
                checkAlerts(stream_id, detections, ctx->zones);

                // 5. Update last frame for tiling
                {
                    std::lock_guard<std::mutex> lock(ctx->frame_mutex);
                    ctx->last_processed_frame = frame.clone();
                }
            }
        }

        gst_buffer_unmap(buffer, &map);
    }
}

void PipelineManager::checkAlerts(const std::string& stream_id, const std::vector<Detection>& detections, const std::vector<ZoneConfig>& zones) {
    for (const auto& det : detections) {
        cv::Point feet(det.box.x + det.box.width / 2, det.box.y + det.box.height);
        
        for (const auto& zone : zones) {
            double dist = cv::pointPolygonTest(zone.points, feet, false);
            if (dist >= 0) {
                if (violation_logger_) {
                    violation_logger_->log_violation(zone.id, det.confidence, det.track_id);
                }

                if (alert_throttler_ && alert_throttler_->should_alert(zone.id, det.track_id)) {
                    if (mqtt_client_ && mqtt_client_->isConnected()) {
                        std::string alert = "{\"alert\": \"zone_violation\", \"stream_id\": \"" + stream_id + 
                                        "\", \"zone_name\": \"" + zone.name + 
                                        "\", \"track_id\": " + std::to_string(det.track_id) + "}";
                        mqtt_client_->publish(config_.mqtt.topic, alert);
                    }
                }
            }
        }
    }
}

void PipelineManager::updateTiledView() {
    while (running_) {
        std::vector<cv::Mat> current_frames;
        for (auto& pair : streams_) {
            std::lock_guard<std::mutex> lock(pair.second->frame_mutex);
            if (!pair.second->last_processed_frame.empty()) {
                current_frames.push_back(pair.second->last_processed_frame.clone());
            }
        }

        if (!current_frames.empty()) {
            if (current_frames.size() == 1) {
                streamer_->publish(current_frames[0]);
            } else {
                int target_w = 640;
                int target_h = 360;
                for (auto& f : current_frames) cv::resize(f, f, cv::Size(target_w, target_h));

                cv::Mat combined;
                if (current_frames.size() == 2) {
                    cv::vconcat(current_frames[0], current_frames[1], combined);
                } else if (current_frames.size() <= 4) {
                    // 2x2 grid
                    cv::Mat top, bottom;
                    cv::hconcat(current_frames[0], current_frames[1], top);
                    
                    if (current_frames.size() == 3) {
                        cv::Mat black = cv::Mat::zeros(target_h, target_w, CV_8UC3);
                        cv::hconcat(current_frames[2], black, bottom);
                    } else {
                        cv::hconcat(current_frames[2], current_frames[3], bottom);
                    }
                    cv::vconcat(top, bottom, combined);
                } else {
                    // Fallback for many streams: just first 4 or implement paging
                    combined = current_frames[0];
                }
                streamer_->publish(combined);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
