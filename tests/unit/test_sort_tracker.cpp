#include <gtest/gtest.h>
#include "tracking/sort_tracker.hpp"

TEST(SortTrackerTest, SingleObjectTracking) {
    SortTracker tracker;
    
    // Frame 1
    std::vector<Detection> dets1 = {{0, 0.9f, cv::Rect(10, 10, 50, 50), -1}};
    auto tracked1 = tracker.update(dets1);
    
    ASSERT_EQ(tracked1.size(), 1);
    EXPECT_NE(tracked1[0].track_id, -1);
    int first_id = tracked1[0].track_id;
    
    // Frame 2: Slight movement
    std::vector<Detection> dets2 = {{0, 0.9f, cv::Rect(12, 12, 50, 50), -1}};
    auto tracked2 = tracker.update(dets2);
    
    ASSERT_EQ(tracked2.size(), 1);
    EXPECT_EQ(tracked2[0].track_id, first_id); // Should keep the same ID
}

TEST(SortTrackerTest, MultiObjectTracking) {
    SortTracker tracker;
    
    // Frame 1: Two objects
    std::vector<Detection> dets1 = {
        {0, 0.9f, cv::Rect(10, 10, 50, 50), -1},
        {0, 0.9f, cv::Rect(100, 100, 50, 50), -1}
    };
    auto tracked1 = tracker.update(dets1);
    ASSERT_EQ(tracked1.size(), 2);
    EXPECT_NE(tracked1[0].track_id, tracked1[1].track_id);
}

TEST(SortTrackerTest, OcclusionHandling) {
    SortTracker tracker(5, 1); // max_age = 5, min_hits = 1
    
    // Frame 1: Object appears
    std::vector<Detection> dets1 = {{0, 0.9f, cv::Rect(10, 10, 50, 50), -1}};
    auto tracked1 = tracker.update(dets1);
    int id = tracked1[0].track_id;
    
    // Frame 2: Object disappears (occlusion)
    std::vector<Detection> dets2 = {};
    tracker.update(dets2);
    
    // Frame 3: Object reappears nearby
    std::vector<Detection> dets3 = {{0, 0.9f, cv::Rect(12, 12, 50, 50), -1}};
    auto tracked3 = tracker.update(dets3);
    
    ASSERT_EQ(tracked3.size(), 1);
    EXPECT_EQ(tracked3[0].track_id, id); // Should recover the same ID
}
