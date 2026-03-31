#pragma once
#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include "model_loader.hpp"
#include "trt_utils.hpp"

struct InferenceConfig {
    std::string model_path;
    int input_width = 640;
    int input_height = 640;
    float conf_threshold = 0.20f; // Lowered for higher recall (Safety Critical)
    float nms_threshold = 0.50f;  // Adjusted for crowded scenes
};

struct Detection {
    int class_id;
    float confidence;
    cv::Rect box;
    int track_id = -1; // -1 indicates untracked
    cv::Mat feature;   // Visual feature embedding (Re-ID)
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
    
    // GPU Memory Management
    static bool checkAvailableGPUMemory(size_t required_bytes);
    [[nodiscard]] size_t getRequiredMemory() const { return required_memory_bytes_; }

private:
    std::vector<Detection> parseDetections(const cv::Mat& output_t, int frame_w, int frame_h);

    InferenceConfig config_;
    std::unique_ptr<ModelLoader> model_loader_;
    size_t required_memory_bytes_ = 0;  // Per-stream memory requirement

#ifdef ENABLE_TENSORRT
    struct InferDeleter {
        template <typename T>
        void operator()(T* obj) const {
            if (obj) obj->destroy();
        }
    };
    
    std::unique_ptr<nvinfer1::IExecutionContext, InferDeleter> context_;
    void* buffers_[2] = {nullptr, nullptr}; // 0: Input, 1: Output
    
    size_t input_bytes_ = 0;
    size_t output_bytes_ = 0;
    
    // Output dimensions (assumes YOLO format)
    int output_rows_ = 0;
    int output_cols_ = 0;

#ifdef ENABLE_CUDA
    cudaStream_t stream_ = nullptr;
#endif

#endif
};
