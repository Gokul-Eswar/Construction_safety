#pragma once
#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include "model_loader.hpp"

struct InferenceConfig {
    std::string model_path;
    int input_width = 640;
    int input_height = 640;
    float conf_threshold = 0.25f;
    float nms_threshold = 0.45f;
};

struct Detection {
    int class_id;
    float confidence;
    cv::Rect box;
};

class InferenceEngine {
public:
    InferenceEngine(const InferenceConfig& config);
    ~InferenceEngine();

    bool init();
    std::vector<Detection> runInference(const cv::Mat& frame);
    
    // Exposed for testing
    void preprocess(const cv::Mat& input, cv::Mat& output);
    std::vector<Detection> applyNMS(const std::vector<Detection>& detections, float nms_thresh);

private:
    InferenceConfig config_;
    std::unique_ptr<ModelLoader> model_loader_;
};
