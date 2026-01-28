#include <gtest/gtest.h>
#include "utils/visualizer.hpp"
#include "inference/inference_engine.hpp"

TEST(VisualizerTest, DrawDetections) {
    cv::Mat frame = cv::Mat::zeros(480, 640, CV_8UC3);
    std::vector<Detection> detections = {
        {0, 0.9f, {100, 100, 50, 50}}
    };
    
    Visualizer visualizer;
    visualizer.drawDetections(frame, detections);
    
    // Check if anything was drawn (convert to grayscale for countNonZero)
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    EXPECT_GT(cv::countNonZero(gray), 0);
}

TEST(VisualizerTest, DrawSafetyZones) {
    cv::Mat frame = cv::Mat::zeros(480, 640, CV_8UC3);
    std::vector<std::vector<cv::Point>> zones = {
        {{10, 10}, {100, 10}, {100, 100}, {10, 100}}
    };
    
    Visualizer visualizer;
    visualizer.drawZones(frame, zones);
    
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    EXPECT_GT(cv::countNonZero(gray), 0);
}
