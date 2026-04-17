#include "pipeline_manager.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <csignal>
#include <fstream>
#include <algorithm>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include "utils/latency_logger.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

PipelineManager::PipelineManager(const AppConfig& config)
    : config_(config), running_(false), last_activity_(std::time(nullptr)) {
}

PipelineManager::~PipelineManager() {
    stop();
}

bool PipelineManager::init() {
    // 1. Init Engine (Shared)
    InferenceConfig inf_config;
    inf_config.model_path = config_.model_path;
    inf_config.conf_threshold = config_.detection.confidence_threshold;
    inf_config.nms_threshold = config_.detection.nms_threshold;
    inf_config.clahe.enabled = config_.preprocessing.clahe_enabled;
    inf_config.clahe.clip_limit = config_.preprocessing.clahe_clip_limit;
    inf_config.clahe.tile_size = config_.preprocessing.clahe_tile_size;
    inf_config.clahe.blur_kernel = config_.preprocessing.clahe_blur_kernel;
    engine_ = std::make_unique<InferenceEngine>(inf_config);
    if (!engine_->init()) {
        spdlog::error("Inference engine initialization failed. Continuing in degraded mode (no inference).");
    }

    // 2. Init Shared Utilities
    violation_logger_ = std::make_unique<safety::ViolationLogger>();
    if (!violation_logger_->init(config_.database_path, config_.log_retention_days)) {
        spdlog::error("Failed to initialize violation logger.");
        return false;
    }

    alert_throttler_ = std::make_unique<safety::AlertThrottler>();
    alert_throttler_->set_cooldown(config_.alert_cooldown);

    visualizer_ = std::make_unique<Visualizer>();
    
    streamer_ = std::make_unique<MJPEGStreamer>();
    streamer_->start(config_.stream_port);

    // MQTT Connectivity
    if (!config_.mqtt.host.empty()) {
        mqtt_host_ = config_.mqtt.host;
        mqtt_port_ = config_.mqtt.port;
        mqtt_client_ = std::make_unique<MQTTClient>(config_.mqtt.client_id);
        ensureMQTTConnected();
    }

    // 3. Init Streams
    for (const auto& sc : config_.streams) {
        auto ctx = std::make_unique<StreamContext>();
        ctx->id = sc.id;
        ctx->name = sc.name;
        ctx->zones = sc.zones;
        TrackingConfig tracking_cfg;
        tracking_cfg.max_age = config_.tracking.max_age;
        tracking_cfg.min_hits = config_.tracking.min_hits;
        tracking_cfg.iou_threshold = config_.tracking.iou_threshold;
        tracking_cfg.feature_threshold = config_.tracking.feature_threshold;
        tracking_cfg.occlusion_extension_frames = config_.tracking.occlusion_extension_frames;
        ctx->tracker = std::make_unique<SortTracker>(tracking_cfg);
        ctx->spatial_mapper = std::make_unique<SpatialMapper>();
        
        if (!sc.calibration.empty()) {
            std::vector<cv::Point2f> img_pts, world_pts;
            for (const auto& cp : sc.calibration) {
                img_pts.push_back(cp.image);
                world_pts.push_back(cp.world);
            }
            if (ctx->spatial_mapper->setCalibration(img_pts, world_pts)) {
                spdlog::info("[{}] Spatial calibration applied with {} points.", sc.id, img_pts.size());
            }
        }

        ctx->source = std::make_unique<CameraSource>(sc.id, sc.type, sc.uri);
        
        ctx->source->setFrameCallback([this, id = sc.id](GstSample* sample) {
            this->onFrameReceived(id, sample);
        });

        ctx->dynamic_inference_interval = std::max(1, config_.detection.inference_interval);
        ctx->dynamic_input_scale = 1.0;
        ctx->estimated_vram_bytes = estimatePerStreamVramBytes(1920, 1080, inf_config.input_width, inf_config.input_height);

        streams_[sc.id] = std::move(ctx);
    }

    return true;
}

void PipelineManager::start() {
    running_ = true;
    admitted_vram_bytes_ = 0;

    for (auto& pair : streams_) {
#ifdef ENABLE_CUDA
        // Stream-aware admission control based on projected incremental VRAM.
        size_t free_vram = trt::getAvailableVRAM();
        last_reported_free_vram_ = free_vram;

        auto& ctx = pair.second;
        size_t projected_usage = admitted_vram_bytes_ + ctx->estimated_vram_bytes;
        bool can_admit = projected_usage < static_cast<size_t>(free_vram * 0.90);

        if (!can_admit) {
            ctx->admitted = false;
            ctx->admission_reason = "denied: projected VRAM " + std::to_string(projected_usage / (1024 * 1024)) +
                                    "MB exceeds 90% of free " + std::to_string(free_vram / (1024 * 1024)) + "MB";
            spdlog::error("[VRAM Admission] Denied stream '{}' - {}", pair.first, ctx->admission_reason);
            continue;
        }

        ctx->admitted = true;
        ctx->admission_reason = "admitted";
        admitted_vram_bytes_ += ctx->estimated_vram_bytes;
        spdlog::info("[VRAM Admission] Starting stream '{}' (Free VRAM: {}MB)", pair.first, (free_vram / 1024 / 1024));
#endif

        if (!pair.second->source->start()) {
            spdlog::error("Failed to start stream: {}", pair.first);
        }
    }
    
    tiling_thread_ = std::thread(&PipelineManager::updateTiledView, this);
    spdlog::info("Pipelines started (governor-controlled).");
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
    spdlog::info("All pipelines stopped.");
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
            last_activity_ = std::time(nullptr);
            auto it = streams_.find(stream_id);
            if (it != streams_.end()) {
                auto& ctx = it->second;
                ctx->frame_count++;

                // Start Processing Timer
                std::string key_proc = stream_id + "_processing";
                std::string key_inf = stream_id + "_inference";
                std::string key_track = stream_id + "_tracking";
                std::string key_render = stream_id + "_render";
                std::string key_e2e = stream_id + "_e2e";
                
                LatencyLogger::getInstance().startTimer(key_proc, ctx->frame_count);

                // Determine if we run inference
                enforceRuntimeDegradationPolicy(*ctx, frame);
                bool run_inference = shouldRunInferenceForStream(*ctx);

                std::vector<Detection> detections;
                
                if (run_inference) {
                    // 1. Inference
                    LatencyLogger::getInstance().startTimer(key_inf, ctx->frame_count);
                    std::vector<Detection> raw_detections;
                    cv::Mat inference_frame = frame;
                    if (ctx->dynamic_input_scale < 0.999) {
                        cv::resize(frame, inference_frame, cv::Size(), ctx->dynamic_input_scale, ctx->dynamic_input_scale, cv::INTER_LINEAR);
                    }
                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        raw_detections = engine_->runInference(inference_frame);
                    }

                    if (ctx->dynamic_input_scale < 0.999 && !raw_detections.empty()) {
                        const float inv_scale = static_cast<float>(1.0 / ctx->dynamic_input_scale);
                        for (auto& det : raw_detections) {
                            det.box.x = static_cast<int>(det.box.x * inv_scale);
                            det.box.y = static_cast<int>(det.box.y * inv_scale);
                            det.box.width = static_cast<int>(det.box.width * inv_scale);
                            det.box.height = static_cast<int>(det.box.height * inv_scale);
                            det.box &= cv::Rect(0, 0, frame.cols, frame.rows);
                        }
                    }
                    LatencyLogger::getInstance().stopTimer(key_inf, ctx->frame_count);
                    
                    // --- RE-ID: Feature Extraction (Color Histogram) ---
                    for (auto& det : raw_detections) {
                        // Ensure box is within frame boundaries
                        cv::Rect safe_box = det.box & cv::Rect(0, 0, frame.cols, frame.rows);
                        if (safe_box.width > 0 && safe_box.height > 0) {
                            cv::Mat crop = frame(safe_box);
                            
                            // Simple but effective color histogram for Re-ID
                            cv::Mat hsv;
                            cv::cvtColor(crop, hsv, cv::COLOR_BGR2HSV);
                            
                            int h_bins = 16, s_bins = 8;
                            int histSize[] = { h_bins, s_bins };
                            float h_ranges[] = { 0, 180 };
                            float s_ranges[] = { 0, 256 };
                            const float* ranges[] = { h_ranges, s_ranges };
                            int channels[] = { 0, 1 };

                            cv::calcHist(&hsv, 1, channels, cv::Mat(), det.feature, 2, histSize, ranges, true, false);
                            cv::normalize(det.feature, det.feature, 1.0, 0, cv::NORM_L2);
                        }
                    }

                    // 2. Tracking (Per stream)
                    LatencyLogger::getInstance().startTimer(key_track, ctx->frame_count);
                    detections = ctx->tracker->update(raw_detections);
                    LatencyLogger::getInstance().stopTimer(key_track, ctx->frame_count);
                }

                // 3. Visualization
                LatencyLogger::getInstance().startTimer(key_render, ctx->frame_count);
                if (run_inference) {
                     visualizer_->drawDetections(frame, detections);
                     // Check alerts only on inferred frames
                     checkAlerts(stream_id, detections, ctx->zones);
                }
                
                std::vector<std::vector<cv::Point>> zone_points;
                for (const auto& z : ctx->zones) zone_points.push_back(z.points);
                visualizer_->drawZones(frame, zone_points);
                
                // Add stream name
                cv::putText(frame, ctx->name, cv::Point(20, 40), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 255), 2);
                LatencyLogger::getInstance().stopTimer(key_render, ctx->frame_count);

                // 5. Update last frame for tiling
                {
                    std::lock_guard<std::mutex> lock(ctx->frame_mutex);
                    ctx->last_processed_frame = frame.clone();
                }

                // Stop Timers
                LatencyLogger::getInstance().stopTimer(key_proc, ctx->frame_count);
                LatencyLogger::getInstance().stopTimer(key_e2e, ctx->frame_count);
                LatencyLogger::getInstance().logStats();
            }
        }

        gst_buffer_unmap(buffer, &map);
    }
}

void PipelineManager::checkAlerts(const std::string& stream_id, const std::vector<Detection>& detections, const std::vector<ZoneConfig>& zones) {
    auto it = streams_.find(stream_id);
    if (it == streams_.end()) return;
    auto& ctx = it->second;

    for (const auto& det : detections) {
        // ================================================================================
        // Perspective Detection Logic (Issue 3 Implementation)
        // Detects zone violations based on bounding box positioning with multiple strategies
        // ================================================================================
        
        for (const auto& zone : zones) {
            bool is_violating = false;
            
            switch (config_.zone_detection.mode) {
                // ========================================================================
                // Mode 1: POINT - Single point detection (legacy, simple)
                // ========================================================================
                case ZoneDetectionConfig::Mode::POINT: {
                    cv::Point2f feet_img(det.box.x + det.box.width / 2.0f, det.box.y + det.box.height);
                    cv::Point2f feet_final = feet_img;
                    if (ctx->spatial_mapper) {
                        feet_final = ctx->spatial_mapper->mapToWorld(feet_img);
                    }
                    double dist = cv::pointPolygonTest(zone.points, feet_final, true);
                    is_violating = (dist >= config_.zone_detection.boundary_margin);
                    break;
                }
                
                // ========================================================================
                // Mode 2: FOOTPRINT - Multiple points on bbox bottom edge (recommended)
                // Reduces false positives from bounding box detection errors
                // ========================================================================
                case ZoneDetectionConfig::Mode::FOOTPRINT: {
                    // Project 3 footprint points: bottom-left, bottom-center, bottom-right
                    // This accounts for detection box width uncertainty
                    std::vector<cv::Point2f> footprint = {
                        {det.box.x, det.box.y + det.box.height},                           // Bottom-left
                        {det.box.x + det.box.width / 2.0f, det.box.y + det.box.height},   // Bottom-center
                        {det.box.x + det.box.width, det.box.y + det.box.height}            // Bottom-right
                    };
                    
                    // Map to world coordinates if calibrated
                    int points_in_zone = 0;
                    for (auto& pt : footprint) {
                        cv::Point2f pt_final = pt;
                        if (ctx->spatial_mapper) {
                            pt_final = ctx->spatial_mapper->mapToWorld(pt);
                        }
                        double dist = cv::pointPolygonTest(zone.points, pt_final, true);
                        if (dist >= config_.zone_detection.boundary_margin) {
                            points_in_zone++;
                        }
                    }
                    
                    // Apply voting strategy
                    switch (config_.zone_detection.footprint_voting) {
                        case ZoneDetectionConfig::VotingStrategy::ALL:
                            is_violating = (points_in_zone == 3);
                            break;
                        case ZoneDetectionConfig::VotingStrategy::MAJORITY:
                            is_violating = (points_in_zone >= 2);  // 2/3 or more
                            break;
                        case ZoneDetectionConfig::VotingStrategy::ANY:
                            is_violating = (points_in_zone >= 1);   // At least 1
                            break;
                    }
                    break;
                }
                
                // ========================================================================
                // Mode 3: CALIBRATED - Uses full perspective transform (highest accuracy)
                // Requires spatial calibration; falls back to footprint if unavailable
                // ========================================================================
                case ZoneDetectionConfig::Mode::CALIBRATED: {
                    if (ctx->spatial_mapper && ctx->spatial_mapper->isCalibrated()) {
                        // Use full homography transform for most accurate positioning
                        cv::Point2f feet_img(det.box.x + det.box.width / 2.0f, det.box.y + det.box.height);
                        cv::Point2f feet_world = ctx->spatial_mapper->mapToWorld(feet_img);
                        double dist = cv::pointPolygonTest(zone.points, feet_world, true);
                        is_violating = (dist >= config_.zone_detection.boundary_margin);
                    } else {
                        // Fallback to footprint mode if calibration unavailable
                        std::vector<cv::Point2f> footprint = {
                            {det.box.x + det.box.width / 2.0f, det.box.y + det.box.height}
                        };
                        double dist = cv::pointPolygonTest(zone.points, footprint[0], true);
                        is_violating = (dist >= config_.zone_detection.boundary_margin);
                    }
                    break;
                }
            }
            
            if (is_violating) {
                if (violation_logger_) {
                    // Capture spatial coordinates for analytics
                    std::array<float, 4> detection_box = {
                        static_cast<float>(det.box.x),
                        static_cast<float>(det.box.y),
                        static_cast<float>(det.box.width),
                        static_cast<float>(det.box.height)
                    };
                    // Get world coordinates for logging
                    cv::Point2f feet_img(det.box.x + det.box.width / 2.0f, det.box.y + det.box.height);
                    cv::Point2f feet_world = feet_img;
                    if (ctx->spatial_mapper) {
                        feet_world = ctx->spatial_mapper->mapToWorld(feet_img);
                    }
                    std::array<float, 2> world_coords = {
                        feet_world.x,
                        feet_world.y
                    };
                    violation_logger_->log_violation(zone.id, det.confidence, det.track_id,
                                                    detection_box, world_coords, stream_id);
                }
            }

            if (alert_throttler_ && alert_throttler_->should_alert(zone.id, det.track_id, is_violating)) {
                if (mqtt_client_ && mqtt_client_->isConnected()) {
                    std::string alert = "{\"alert\": \"zone_violation\", \"stream_id\": \"" + stream_id + 
                                    "\", \"zone_name\": \"" + zone.name + 
                                    "\", \"track_id\": " + std::to_string(det.track_id) + "}";
                    // Use versioned topic by default; fallback to config if empty
                    std::string topic = config_.mqtt.topic.find("/v") != std::string::npos 
                                      ? config_.mqtt.topic 
                                      : "safety/v1/violations";
                    mqtt_client_->publish(topic, alert);
                }
            }
        }
    }
}

void PipelineManager::updateTiledView() {
    while (running_) {
        ensureMQTTConnected();

        // Health check: stay healthy while active, and also when intentionally idle (no streams configured).
        auto now = std::time(nullptr);
        bool has_streams = !streams_.empty();
        bool recently_active = (now - last_activity_ < 30);
        if (!has_streams || recently_active) {
             std::ofstream health_file("/app/health");
             if (health_file.good()) health_file << "1";
        }

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

        // --- HEARTBEAT ---
        static auto last_beat_time = std::chrono::steady_clock::now();
        auto now_beat_time = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now_beat_time - last_beat_time).count() >= 2) {
            if (mqtt_client_ && mqtt_client_->isConnected()) {
                std::string beat = "{\"status\":\"online\", "
                                   "\"timestamp\":" + std::to_string(std::time(nullptr)) + "}";
                mqtt_client_->publish("safety/v1/heartbeat", beat);
            }
            last_beat_time = now_beat_time;
        }

        // --- TELEMETRY (1Hz) ---
        static auto last_telemetry = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now_beat_time - last_telemetry).count() >= 1) {
            if (mqtt_client_ && mqtt_client_->isConnected()) {
                json j;
                j["timestamp"] = std::time(nullptr);
                
                // Stream Stats
                for (auto& pair : streams_) {
                    auto stats = pair.second->source->getStats();
                    j["streams"][pair.first] = {
                        {"fps", stats.fps},
                        {"active", stats.active},
                        {"frame_count", stats.frame_count},
                        {"state", static_cast<int>(stats.state)},
                        {"reconnect_attempt", stats.reconnect_attempt},
                        {"reconnect_count", stats.reconnect_count},
                        {"error_count", stats.error_count},
                        {"stale_timeout_count", stats.stale_timeout_count},
                        {"restart_timeout_count", stats.restart_timeout_count},
                        {"teardown_timeout_count", stats.teardown_timeout_count},
                        {"last_error", stats.last_error},
                        {"dynamic_inference_interval", pair.second->dynamic_inference_interval},
                        {"dynamic_input_scale", pair.second->dynamic_input_scale},
                        {"estimated_vram_mb", pair.second->estimated_vram_bytes / (1024 * 1024)},
                        {"admitted", pair.second->admitted},
                        {"admission_reason", pair.second->admission_reason},
                        {"low_vram_events", pair.second->low_vram_events}
                    };
                }

                // Latency Stats
                auto latencies = LatencyLogger::getInstance().getAndClearStats();
                for (const auto& [key, val] : latencies) {
                    j["latency"][key] = {
                        {"avg", val.avg},
                        {"min", val.min},
                        {"max", val.max},
                        {"p99", val.p99}
                    };
                }

                // GPU Stats
                auto gpu = LatencyLogger::getInstance().getGPUStats();
                j["gpu"] = {
                    {"utilization", gpu.utilization},
                    {"temperature", gpu.temperature},
                    {"memory_used_mb", gpu.memory_used / (1024 * 1024)},
                    {"memory_total_mb", gpu.memory_total / (1024 * 1024)}
                };

                mqtt_client_->publish("safety/v1/telemetry", j.dump());
            }
            last_telemetry = now_beat_time;
        }

        // --- CLOUD SYNC (Every 10s) ---
        static auto last_sync = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now_beat_time - last_sync).count() >= 10) {
            if (violation_logger_ && mqtt_client_ && mqtt_client_->isConnected()) {
                auto pending = violation_logger_->get_pending_uploads(5);
                if (!pending.empty()) {
                    std::vector<int> uploaded_ids;
                    json sync_payload;
                    sync_payload["type"] = "cloud_sync";
                    sync_payload["records"] = json::array();
                    
                    for (const auto& rec : pending) {
                         sync_payload["records"].push_back({
                             {"id", rec.id},
                             {"timestamp", rec.timestamp},
                             {"zone_id", rec.zone_id},
                             {"confidence", rec.confidence},
                             {"object_id", rec.object_id}
                         });
                         uploaded_ids.push_back(rec.id);
                    }
                    
                    if (mqtt_client_->publish("safety/v1/cloud_sync", sync_payload.dump())) {
                        violation_logger_->mark_uploaded(uploaded_ids);
                        spdlog::info("Synced {} records to cloud.", uploaded_ids.size());
                    }
                }
            }
            last_sync = now_beat_time;
        }

        // Reduced sleep for higher visual FPS (10ms ~= 100 FPS target)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void PipelineManager::ensureMQTTConnected() {
    if (mqtt_host_.empty()) {
        return;
    }

    if (!mqtt_client_) {
        mqtt_client_ = std::make_unique<MQTTClient>(config_.mqtt.client_id);
    }

    if (mqtt_client_->isConnected()) {
        if (!control_topic_subscribed_) {
            control_topic_subscribed_ = subscribeControlTopic();
        }
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (last_mqtt_attempt_.time_since_epoch().count() > 0 && now - last_mqtt_attempt_ < mqtt_retry_interval_) {
        return;
    }

    last_mqtt_attempt_ = now;
    spdlog::warn("MQTT disconnected. Attempting reconnect to {}:{}", mqtt_host_, mqtt_port_);
    if (mqtt_client_->connect(mqtt_host_, mqtt_port_)) {
        spdlog::info("MQTT reconnected successfully.");
        control_topic_subscribed_ = subscribeControlTopic();
    } else {
        control_topic_subscribed_ = false;
    }
}

bool PipelineManager::subscribeControlTopic() {
    if (!mqtt_client_ || !mqtt_client_->isConnected()) {
        return false;
    }

    return mqtt_client_->subscribe("safety/v1/control", [this](const std::string& topic, const std::string& payload) {
        (void)topic;
        if (payload.find("restart") != std::string::npos) {
            spdlog::info("Received restart command via MQTT. Initiating shutdown...");
            std::raise(SIGTERM);
        }
    });
}

size_t PipelineManager::estimatePerStreamVramBytes(int frame_width, int frame_height, int input_width, int input_height) const {
    int w = std::max(1, frame_width);
    int h = std::max(1, frame_height);
    int iw = std::max(1, input_width);
    int ih = std::max(1, input_height);

    size_t decoder_surfaces = static_cast<size_t>(w) * static_cast<size_t>(h) * 3 / 2 * 4;
    size_t bgr_buffers = static_cast<size_t>(w) * static_cast<size_t>(h) * 3 * 2;
    size_t infer_input = static_cast<size_t>(iw) * static_cast<size_t>(ih) * 3 * sizeof(float);
    size_t infer_output = 64ULL * 1024ULL * 1024ULL;
    size_t margin = 32ULL * 1024ULL * 1024ULL;

    return decoder_surfaces + bgr_buffers + infer_input + infer_output + margin;
}

bool PipelineManager::shouldRunInferenceForStream(const StreamContext& ctx) const {
    int interval = std::max(1, ctx.dynamic_inference_interval);
    return (ctx.frame_count % static_cast<uint64_t>(interval)) == 0;
}

void PipelineManager::enforceRuntimeDegradationPolicy(StreamContext& ctx, const cv::Mat& frame) {
#ifdef ENABLE_CUDA
    size_t free_vram = trt::getAvailableVRAM();
    last_reported_free_vram_ = free_vram;

    if (!frame.empty()) {
        ctx.estimated_vram_bytes = estimatePerStreamVramBytes(frame.cols, frame.rows, 640, 640);
    }

    const size_t LOW_VRAM_WATERMARK = 600ULL * 1024ULL * 1024ULL;
    const size_t CRITICAL_VRAM_WATERMARK = 350ULL * 1024ULL * 1024ULL;

    if (free_vram < LOW_VRAM_WATERMARK) {
        ++ctx.low_vram_events;

        if (ctx.dynamic_inference_interval < 6) {
            ++ctx.dynamic_inference_interval;
            spdlog::warn("[VRAM Degrade][{}] Increased inference interval to {} (free VRAM={}MB)",
                         ctx.id, ctx.dynamic_inference_interval, (free_vram / (1024 * 1024)));
        }

        if (ctx.dynamic_input_scale > 0.65) {
            ctx.dynamic_input_scale = std::max(0.65, ctx.dynamic_input_scale - 0.10);
            spdlog::warn("[VRAM Degrade][{}] Reduced inference input scale to {} (free VRAM={}MB)",
                         ctx.id, ctx.dynamic_input_scale, (free_vram / (1024 * 1024)));
        }

        if (free_vram < CRITICAL_VRAM_WATERMARK && !ctx.paused_for_vram && ctx.source) {
            ctx.paused_for_vram = true;
            ctx.source->stop();
            spdlog::error("[VRAM Degrade][{}] Stream paused due to critical low VRAM ({}MB free)",
                          ctx.id, (free_vram / (1024 * 1024)));
        }
    } else {
        if (ctx.paused_for_vram && free_vram > (LOW_VRAM_WATERMARK + 150ULL * 1024ULL * 1024ULL) && ctx.source) {
            if (ctx.source->start()) {
                ctx.paused_for_vram = false;
                spdlog::info("[VRAM Recover][{}] Stream resumed.", ctx.id);
            }
        }

        if (ctx.dynamic_inference_interval > std::max(1, config_.detection.inference_interval)) {
            --ctx.dynamic_inference_interval;
        }
        if (ctx.dynamic_input_scale < 1.0) {
            ctx.dynamic_input_scale = std::min(1.0, ctx.dynamic_input_scale + 0.05);
        }
    }
#else
    (void)ctx;
    (void)frame;
#endif
}
