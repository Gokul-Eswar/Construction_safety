#include <gtest/gtest.h>
#include "inference/inference_engine.hpp"
#include <opencv2/opencv.hpp>
#include <fstream>
#include <cstdio>

TEST(InferenceEngineTest, PreProcessing) {
    // Create a dummy image 1920x1080
    cv::Mat input_img = cv::Mat::zeros(1080, 1920, CV_8UC3);
    
    // Config: Target size 640x640 for YOLO
    InferenceConfig config;
    config.input_width = 640;
    config.input_height = 640;
    config.model_path = "yolov11.engine";
    
    InferenceEngine engine(config);
    
    cv::Mat preprocessed;
    engine.preprocess(input_img, preprocessed);
    
    EXPECT_EQ(preprocessed.cols, 640);
    EXPECT_EQ(preprocessed.rows, 640);
    EXPECT_EQ(preprocessed.type(), CV_32FC3); // Normalized to float
}

TEST(InferenceEngineTest, PostProcessingNMS) {
    InferenceConfig config;
    InferenceEngine engine(config);
    
    // Create mock detections
    std::vector<Detection> detections = {
        {0, 0.9f, {10, 10, 50, 50}},
        {0, 0.8f, {12, 12, 48, 48}}, // Overlapping
        {1, 0.9f, {100, 100, 50, 50}}
    };
    
    std::vector<Detection> result = engine.applyNMS(detections, 0.5f);
    
    // Should remove one overlapping box
    EXPECT_EQ(result.size(), 2);
}

TEST(InferenceEngineTest, Initialization) {
    // Create dummy model
    std::string dummy_model = "test_yolo.onnx";
    std::ofstream f(dummy_model);
    f << "dummy data";
    f.close();

    InferenceConfig config;
    config.model_path = dummy_model;
    
    InferenceEngine engine(config);
    EXPECT_TRUE(engine.init());
    
    // Cleanup
    std::remove(dummy_model.c_str());
}
