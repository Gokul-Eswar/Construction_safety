#include "kalman_box_tracker.hpp"

KalmanBoxTracker::KalmanBoxTracker(cv::Rect2f initial_rect, int id) 
    : id_(id), time_since_update_(0), hit_streak_(0), age_(0) {
    
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
    cv::Mat state = rect_to_state(initial_rect);
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
    return state_to_rect(p);
}

void KalmanBoxTracker::update(cv::Rect2f rect) {
    time_since_update_ = 0;
    hit_streak_++;
    cv::Mat measurement = rect_to_state(rect);
    kf_.correct(measurement);
}

cv::Rect2f KalmanBoxTracker::get_state() const {
    return state_to_rect(kf_.statePost);
}

cv::Mat KalmanBoxTracker::rect_to_state(cv::Rect2f rect) {
    float w = rect.width;
    float h = rect.height;
    float x = rect.x + w / 2.0f;
    float y = rect.y + h / 2.0f;
    float s = w * h; // area
    float r = w / (h + 1e-6f); // aspect ratio
    return (cv::Mat_<float>(4, 1) << x, y, s, r);
}

cv::Rect2f KalmanBoxTracker::state_to_rect(cv::Mat state) const {
    float x = state.at<float>(0);
    float y = state.at<float>(1);
    float s = state.at<float>(2);
    float r = state.at<float>(3);
    
    float w = std::sqrt(std::max(0.0f, s * r));
    float h = s / (w + 1e-6f);
    
    return cv::Rect2f(x - w / 2.0f, y - h / 2.0f, w, h);
}
