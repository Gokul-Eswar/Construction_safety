#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include "inference/inference_engine.hpp"

class Visualizer {
public:
    Visualizer();
    ~Visualizer();

    void drawDetections(cv::Mat& frame, const std::vector<Detection>& detections);
    void drawZones(cv::Mat& frame, const std::vector<std::vector<cv::Point>>& zones);
    void drawStatus(cv::Mat& frame, const std::string& status, const cv::Point& pos);

private:
    cv::Scalar getColor(int class_id);
};
