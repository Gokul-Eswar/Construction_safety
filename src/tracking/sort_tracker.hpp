#pragma once
#include "kalman_box_tracker.hpp"
#include <vector>
#include <memory>
#include "inference/inference_engine.hpp"

class SortTracker {
public:
    SortTracker(int maxAge = 15, int minHits = 3, float iouThreshold = 0.3f, float featureThreshold = 0.5f);
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
    int next_id_;

    std::vector<std::unique_ptr<KalmanBoxTracker>> trackers_;

    [[nodiscard]] float calculateIou(cv::Rect2f bbTest, cv::Rect2f bbGt) const;
    [[nodiscard]] float calculateFeatureDist(const cv::Mat& f1, const cv::Mat& f2) const;
};
