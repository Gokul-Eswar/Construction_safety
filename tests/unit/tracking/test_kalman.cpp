#include <gtest/gtest.h>
#include "tracking/kalman_box_tracker.hpp"
#include <opencv2/opencv.hpp>

TEST(KalmanTrackerTest, Initialization) {
    cv::Rect2f bbox(100.0f, 100.0f, 50.0f, 50.0f);
    KalmanBoxTracker tracker(bbox, 1);
    
    // Check initial state
    auto state = tracker.getState();
    // Expected center x, y, area, ratio
    EXPECT_NEAR(state.x, 100.0f, 1.0f); 
}

TEST(KalmanTrackerTest, Prediction) {
    cv::Rect2f bbox(100.0f, 100.0f, 50.0f, 50.0f);
    KalmanBoxTracker tracker(bbox, 1);
    
    // Predict next state without update
    auto predicted = tracker.predict();
    
    EXPECT_GT(predicted.width, 0.0f);
    EXPECT_GT(predicted.height, 0.0f);
}

TEST(KalmanTrackerTest, Update) {
    cv::Rect2f bbox1(100.0f, 100.0f, 50.0f, 50.0f);
    KalmanBoxTracker tracker(bbox1, 1);
    
    (void)tracker.predict();
    
    // Object moves diagonally
    cv::Rect2f bbox2(110.0f, 110.0f, 50.0f, 50.0f);
    tracker.update(bbox2);
    
    auto state = tracker.getState();
    
    // State should be close to new measurement
    EXPECT_NEAR(state.x, 110.0f, 5.0f);
}

TEST(KalmanTrackerTest, HitStreakAndAge) {
    cv::Rect2f bbox(100.0f, 100.0f, 50.0f, 50.0f);
    KalmanBoxTracker tracker(bbox, 1);
    
    EXPECT_EQ(tracker.getTimeSinceUpdate(), 0);
    
    tracker.update(bbox);
    EXPECT_EQ(tracker.getTimeSinceUpdate(), 0);
    
    (void)tracker.predict();
    EXPECT_EQ(tracker.getTimeSinceUpdate(), 1);
}
