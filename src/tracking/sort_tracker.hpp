#pragma once
#include "kalman_box_tracker.hpp"
#include <vector>
#include <memory>
#include "inference/inference_engine.hpp"

// ================================================================================
// TRACKING CONFIGURATION (from AppConfig::tracking)
// ================================================================================
struct TrackingConfig {
    int max_age = 45;
    int min_hits = 3;
    float iou_threshold = 0.3f;
    float feature_threshold = 0.5f;
    int occlusion_extension_frames = 15;
};

class SortTracker {
public:
    SortTracker(int maxAge = 45, int minHits = 3, float iouThreshold = 0.3f, float featureThreshold = 0.5f, int occlusionExtensionFrames = 15);
    SortTracker(const TrackingConfig& config);
    ~SortTracker() = default;

    /**
     * @brief Update the tracker with new detections.
     * @param detections Detections from current frame.
     * @return Tracked objects with IDs assigned.
     */
    std::vector<Detection> update(const std::vector<Detection>& detections);

private:
    int max_age_;
    int min_hits_;
    float iou_threshold_;
    float feature_threshold_;
    int occlusion_extension_frames_;
    int next_id_;

    std::vector<std::unique_ptr<KalmanBoxTracker>> trackers_;
    std::vector<cv::Rect2f> previous_boxes_;  // Track previous detection boxes for occlusion detection
    std::vector<int> occlusion_frames_;       // Count frames each object is likely occluded

    [[nodiscard]] float calculateIou(cv::Rect2f bbTest, cv::Rect2f bbGt) const;
    [[nodiscard]] float calculateFeatureDist(const cv::Mat& f1, const cv::Mat& f2) const;
    
    // Distance-based matching to prevent ID swaps
    [[nodiscard]] float calculateSpatialDistance(cv::Rect2f box1, cv::Rect2f box2) const;
    
    // Occlusion detection heuristic
    [[nodiscard]] bool isLikelyOccluded(const cv::Rect2f& prev_box, const cv::Rect2f& current_box) const;
};

