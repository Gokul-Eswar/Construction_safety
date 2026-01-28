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
        // Create cv::Mat from buffer data
        // For simplicity assuming BGR or RGB based on caps. 
        // Our test source uses I420, real pipeline uses hardware decoder output.
        // For the mock integration, we just verify we can map the buffer.
        
        cv::Mat frame;
        if (GST_VIDEO_INFO_FORMAT(&info) == GST_VIDEO_FORMAT_I420) {
            cv::Mat yuv(info.height + info.height / 2, info.width, CV_8UC1, map.data);
            cv::cvtColor(yuv, frame, cv::COLOR_YUV2BGR_I420);
        } else if (GST_VIDEO_INFO_FORMAT(&info) == GST_VIDEO_FORMAT_BGR) {
            frame = cv::Mat(info.height, info.width, CV_8UC3, map.data).clone();
        }

        if (!frame.empty()) {
            // 2. runInference
            auto detections = engine_->runInference(frame);
            
            // 3. Visualize
            visualizer_->drawDetections(frame, detections);
            
            // 4. Spatial Mapping (example: map center of box)
            for (auto& det : detections) {
                cv::Point2f center(det.box.x + det.box.width / 2.0f, det.box.y + det.box.height / 2.0f);
                cv::Point2f world_pos = spatial_mapper_->mapToWorld(center);
                // Log or use world_pos for zone checks in Phase 3
            }

            // In real app, we might display or stream this back
            // cv::imshow("Debug View", frame);
            // cv::waitKey(1);
        }

        gst_buffer_unmap(buffer, &map);
    }
}
