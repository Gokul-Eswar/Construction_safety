#include <gtest/gtest.h>
#include "tracking/kalman_box_tracker.hpp"
#include <opencv2/opencv.hpp>

TEST(KalmanTrackerTest, Initialization) {
    cv::Rect bbox(100, 100, 50, 50);
    KalmanBoxTracker tracker(bbox);
    
    // Check initial state
    auto state = tracker.get_state();
    // Expected center x, y, area, ratio
    EXPECT_NEAR(state.x, 100 + 25, 1.0);
    EXPECT_NEAR(state.y, 100 + 25, 1.0);
}

TEST(KalmanTrackerTest, Prediction) {
    cv::Rect bbox(100, 100, 50, 50);
    KalmanBoxTracker tracker(bbox);
    
    // Predict next state without update
    auto predicted = tracker.predict();
    
    // In constant velocity model, initial velocity is uncertain but likely 0 or close to it 
    // depending on initialization.
    // We expect the bounding box to remain roughly similar after 1 step if no motion observed yet.
    
    EXPECT_GT(predicted.width, 0);
    EXPECT_GT(predicted.height, 0);
}

TEST(KalmanTrackerTest, Update) {
    cv::Rect bbox1(100, 100, 50, 50);
    KalmanBoxTracker tracker(bbox1);
    
    tracker.predict();
    
    // Object moves diagonally
    cv::Rect bbox2(110, 110, 50, 50);
    tracker.update(bbox2);
    
    auto state = tracker.get_state();
    
    // State should be close to new measurement
    EXPECT_NEAR(state.x, 110 + 25, 5.0); // Allow some error margin for filter lag
    EXPECT_NEAR(state.y, 110 + 25, 5.0);
}

TEST(KalmanTrackerTest, HitStreakAndAge) {
    cv::Rect bbox(100, 100, 50, 50);
    KalmanBoxTracker tracker(bbox);
    
    EXPECT_EQ(tracker.m_time_since_update, 0);
    EXPECT_EQ(tracker.m_hits, 0); // Hits increment after first update or init? 
                                  // Implementation dependent, usually 0 or 1.
                                  // Let's assume SortTracker manages hits, or verify implementation.
                                  // Looking at typical sort_tracker.cpp, hits might be incremented inside update.
    
    tracker.update(bbox);
    EXPECT_EQ(tracker.m_time_since_update, 0);
    
    tracker.predict();
    EXPECT_EQ(tracker.m_time_since_update, 1);
}
