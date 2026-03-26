#pragma once
#include <opencv2/video/tracking.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

/**
 * @brief This class represents the internal state of individual tracked objects observed as bbox.
 */
class KalmanBoxTracker {
public:
    KalmanBoxTracker(cv::Rect2f initialRect, int id);
    ~KalmanBoxTracker() = default;

    /**
     * @brief Predict the next state using Kalman Filter.
     * @return Predicted bounding box.
     */
    [[nodiscard]] cv::Rect2f predict();

    /**
     * @brief Update the state with observed bounding box and visual feature.
     * @param rect Observed bounding box.
     * @param feature Visual feature embedding.
     */
    void update(cv::Rect2f rect, const cv::Mat& feature = cv::Mat());

    /**
     * @brief Get the current bounding box estimate.
     */
    [[nodiscard]] cv::Rect2f getState() const;
    [[nodiscard]] cv::Mat getFeature() const { return feature_; }

    [[nodiscard]] int getId() const { return id_; }
    [[nodiscard]] int getTimeSinceUpdate() const { return time_since_update_; }
    [[nodiscard]] int getHitStreak() const { return hit_streak_; }

private:
    cv::KalmanFilter kf_;
    int id_;
    int time_since_update_;
    int hit_streak_;
    int age_;
    cv::Mat feature_; // EMA of visual features

    // Utility to convert Rect to state vector [x, y, s, r]
    // x,y: center coords, s: area, r: aspect ratio
    [[nodiscard]] cv::Mat rectToState(cv::Rect2f rect);
    [[nodiscard]] cv::Rect2f stateToRect(cv::Mat state) const;
};
