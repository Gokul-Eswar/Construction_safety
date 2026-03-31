#include "kalman_box_tracker.hpp"
#include <cmath>

KalmanBoxTracker::KalmanBoxTracker(cv::Rect2f initialRect, int id) 
    : id_(id), time_since_update_(0), hit_streak_(0), age_(0), 
      is_stationary_(false), stationary_frame_count_(0), last_velocity_magnitude_(0.0f) {
    
    // State: [x, y, s, r, dx, dy, ds]
    // Obs: [x, y, s, r]
    kf_ = cv::KalmanFilter(7, 4, 0);

    // Transition Matrix (Constant Velocity Model)
    kf_.transitionMatrix = (cv::Mat_<float>(7, 7) << 
        1,0,0,0,1,0,0,
        0,1,0,0,0,1,0,
        0,0,1,0,0,0,1,
        0,0,0,1,0,0,0,
        0,0,0,0,1,0,0,
        0,0,0,0,0,1,0,
        0,0,0,0,0,0,1);

    // Measurement Matrix
    kf_.measurementMatrix = (cv::Mat_<float>(4, 7) << 
        1,0,0,0,0,0,0,
        0,1,0,0,0,0,0,
        0,0,1,0,0,0,0,
        0,0,0,1,0,0,0);

    // Covariances
    setIdentity(kf_.processNoiseCov, cv::Scalar::all(1e-2));
    kf_.processNoiseCov.at<float>(4,4) = 1e-1; // v_x
    kf_.processNoiseCov.at<float>(5,5) = 1e-1; // v_y
    kf_.processNoiseCov.at<float>(6,6) = 1e-1; // v_s

    setIdentity(kf_.measurementNoiseCov, cv::Scalar::all(1e-1));
    setIdentity(kf_.errorCovPost, cv::Scalar::all(1));

    // Initialize state
    cv::Mat state = rectToState(initialRect);
    kf_.statePost.at<float>(0) = state.at<float>(0);
    kf_.statePost.at<float>(1) = state.at<float>(1);
    kf_.statePost.at<float>(2) = state.at<float>(2);
    kf_.statePost.at<float>(3) = state.at<float>(3);
}

cv::Rect2f KalmanBoxTracker::predict() {
    cv::Mat p = kf_.predict();
    age_++;
    if (time_since_update_ > 0) hit_streak_ = 0;
    time_since_update_++;
    return stateToRect(p);
}

void KalmanBoxTracker::update(cv::Rect2f rect, const cv::Mat& feature) {
    time_since_update_ = 0;
    hit_streak_++;
    
    // Calculate velocity magnitude for stationary detection
    cv::Mat state_before = kf_.statePost.clone();
    float prev_x = state_before.at<float>(0);
    float prev_y = state_before.at<float>(1);
    
    cv::Mat measurement = rectToState(rect);
    float curr_x = measurement.at<float>(0);
    float curr_y = measurement.at<float>(1);
    
    float dx = curr_x - prev_x;
    float dy = curr_y - prev_y;
    last_velocity_magnitude_ = std::sqrt(dx * dx + dy * dy);
    
    kf_.correct(measurement);

    // Track stationary state
    if (last_velocity_magnitude_ < VELOCITY_THRESHOLD) {
        stationary_frame_count_++;
        if (stationary_frame_count_ >= STATIONARY_CONFIRM_FRAMES) {
            is_stationary_ = true;
        }
    } else {
        stationary_frame_count_ = 0;
        is_stationary_ = false;
    }

    // Update Visual Feature (EMA)
    if (!feature.empty()) {
        if (feature_.empty()) {
            feature_ = feature.clone();
        } else {
            // Smoothly update the feature vector (90% history, 10% new)
            const float alpha = 0.1f;
            cv::addWeighted(feature_, 1.0f - alpha, feature, alpha, 0, feature_);
            cv::normalize(feature_, feature_);
        }
    }
}

cv::Rect2f KalmanBoxTracker::getState() const {
    return stateToRect(kf_.statePost);
}

cv::Mat KalmanBoxTracker::rectToState(cv::Rect2f rect) {
    float w = rect.width;
    float h = rect.height;
    float x = rect.x + w / 2.0f;
    float y = rect.y + h / 2.0f;
    float s = w * h; // area
    float r = w / (h + 1e-6f); // aspect ratio
    return (cv::Mat_<float>(4, 1) << x, y, s, r);
}

cv::Rect2f KalmanBoxTracker::stateToRect(cv::Mat state) const {
    float x = state.at<float>(0);
    float y = state.at<float>(1);
    float s = state.at<float>(2);
    float r = state.at<float>(3);
    
    float w = std::sqrt(std::max(0.0f, s * r));
    float h = s / (w + 1e-6f);
    
    return cv::Rect2f(x - w / 2.0f, y - h / 2.0f, w, h);
}
