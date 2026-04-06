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

TEST(SortTrackerTest, CrossoverIdContinuity) {
    SortTracker tracker(30, 1, 0.2f, 0.5f, 10);

    std::vector<Detection> frame1 = {
        {0, 0.9f, cv::Rect(20, 20, 30, 60), -1},
        {0, 0.9f, cv::Rect(180, 20, 30, 60), -1}
    };
    auto t1 = tracker.update(frame1);
    ASSERT_EQ(t1.size(), 2);
    int id_left = t1[0].track_id;
    int id_right = t1[1].track_id;

    // Cross over each other over multiple frames.
    for (int i = 1; i <= 10; ++i) {
        std::vector<Detection> f = {
            {0, 0.9f, cv::Rect(20 + i * 14, 20, 30, 60), -1},
            {0, 0.9f, cv::Rect(180 - i * 14, 20, 30, 60), -1}
        };
        auto tracked = tracker.update(f);
        ASSERT_EQ(tracked.size(), 2);
    }

    std::vector<Detection> frame_last = {
        {0, 0.9f, cv::Rect(170, 20, 30, 60), -1},
        {0, 0.9f, cv::Rect(30, 20, 30, 60), -1}
    };
    auto t_last = tracker.update(frame_last);
    ASSERT_EQ(t_last.size(), 2);

    int id_at_right = (t_last[0].box.x > t_last[1].box.x) ? t_last[0].track_id : t_last[1].track_id;
    int id_at_left = (t_last[0].box.x > t_last[1].box.x) ? t_last[1].track_id : t_last[0].track_id;

    // Left-origin track should end up on the right without ID switch.
    EXPECT_EQ(id_at_right, id_left);
    EXPECT_EQ(id_at_left, id_right);
}

TEST(SortTrackerTest, StationaryGhostingReducedWithDormantReactivation) {
    SortTracker tracker(12, 1, 0.2f, 0.5f, 8);

    std::vector<Detection> f1 = {{0, 0.95f, cv::Rect(100, 100, 40, 80), -1}};
    auto t1 = tracker.update(f1);
    ASSERT_EQ(t1.size(), 1);
    int stable_id = t1[0].track_id;

    // Stationary person with intermittent detector confidence drops.
    for (int i = 0; i < 6; ++i) {
        tracker.update({});
    }

    std::vector<Detection> reappear = {{0, 0.9f, cv::Rect(102, 100, 40, 80), -1}};
    auto tr = tracker.update(reappear);
    ASSERT_EQ(tr.size(), 1);
    EXPECT_EQ(tr[0].track_id, stable_id);
}
