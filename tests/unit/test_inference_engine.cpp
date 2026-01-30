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
    
    // blobFromImage returns NCHW: 1 x 3 x H x W
    EXPECT_EQ(preprocessed.dims, 4);
    EXPECT_EQ(preprocessed.size[0], 1);
    EXPECT_EQ(preprocessed.size[1], 3);
    EXPECT_EQ(preprocessed.size[2], 640);
    EXPECT_EQ(preprocessed.size[3], 640);
}

TEST(InferenceEngineTest, InitializationWithInvalidModel) {
    // Create dummy model (garbage data)
    std::string dummy_model = "test_yolo.onnx";
    std::ofstream f(dummy_model);
    f << "dummy data";
    f.close();

    InferenceConfig config;
    config.model_path = dummy_model;
    
    InferenceEngine engine(config);
    // Should FAIL because cv::dnn::readNetFromONNX will parse and reject it
    EXPECT_FALSE(engine.init());
    
    // Cleanup
    std::remove(dummy_model.c_str());
}
