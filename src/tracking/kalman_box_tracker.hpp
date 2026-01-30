#pragma once
#include <opencv2/video/tracking.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

/**
 * @brief This class represents the internal state of individual tracked objects observed as bbox.
 */
class KalmanBoxTracker {
public:
    KalmanBoxTracker(cv::Rect2f initial_rect, int id);
    ~KalmanBoxTracker() = default;

    /**
     * @brief Predict the next state using Kalman Filter.
     * @return Predicted bounding box.
     */
    cv::Rect2f predict();

    /**
     * @brief Update the state with observed bounding box.
     * @param rect Observed bounding box.
     */
    void update(cv::Rect2f rect);

    /**
     * @brief Get the current bounding box estimate.
     */
    cv::Rect2f get_state() const;

    int get_id() const { return id_; }
    int get_time_since_update() const { return time_since_update_; }
    int get_hit_streak() const { return hit_streak_; }

private:
    cv::KalmanFilter kf_;
    int id_;
    int time_since_update_;
    int hit_streak_;
    int age_;

    // Utility to convert Rect to state vector [x, y, s, r]
    // x,y: center coords, s: area, r: aspect ratio
    cv::Mat rect_to_state(cv::Rect2f rect);
    cv::Rect2f state_to_rect(cv::Mat state) const;
};
