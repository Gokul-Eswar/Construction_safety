#pragma once
#include "kalman_box_tracker.hpp"
#include <vector>
#include <memory>
#include "inference/inference_engine.hpp"

class SortTracker {
public:
    SortTracker(int max_age = 15, int min_hits = 3, float iou_threshold = 0.3f);
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
    int next_id_;

    std::vector<std::unique_ptr<KalmanBoxTracker>> trackers_;

    float calculate_iou(cv::Rect2f bb_test, cv::Rect2f bb_gt);
};
