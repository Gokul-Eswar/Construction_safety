#include <gtest/gtest.h>
#include "spatial/spatial_mapper.hpp"
#include <vector>

TEST(SpatialMapperTest, IdentityTransformation) {
    // 4 points forming a square
    std::vector<cv::Point2f> image_pts = {{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    std::vector<cv::Point2f> world_pts = {{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    
    SpatialMapper mapper;
    EXPECT_TRUE(mapper.setCalibration(image_pts, world_pts));
    
    cv::Point2f test_pt(50, 50);
    cv::Point2f result = mapper.mapToWorld(test_pt);
    
    EXPECT_NEAR(result.x, 50.0, 1e-4);
    EXPECT_NEAR(result.y, 50.0, 1e-4);
}

TEST(SpatialMapperTest, PerspectiveTransformation) {
    // 4 points in image (trapezoid) mapped to a square in world
    std::vector<cv::Point2f> image_pts = {{20, 20}, {80, 20}, {100, 100}, {0, 100}};
    std::vector<cv::Point2f> world_pts = {{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    
    SpatialMapper mapper;
    ASSERT_TRUE(mapper.setCalibration(image_pts, world_pts));
    
    // Bottom right corner
    cv::Point2f result = mapper.mapToWorld(cv::Point2f(100, 100));
    EXPECT_NEAR(result.x, 100.0, 1e-3);
    EXPECT_NEAR(result.y, 100.0, 1e-3);
    
    // Center-ish
    cv::Point2f center = mapper.mapToWorld(cv::Point2f(50, 100));
    EXPECT_NEAR(center.x, 50.0, 1e-3);
    EXPECT_NEAR(center.y, 100.0, 1e-3);
}
