#pragma once
#include <string>
#include <vector>
#include <memory>

// Forward declaration for TensorRT runtime
namespace nvinfer1 {
    class ICudaEngine;
    class IRuntime;
    class ILogger;
}

class ModelLoader {
public:
    ModelLoader(const std::string& model_path);
    ~ModelLoader();

    bool load();
    bool saveEngine(const std::string& engine_path);
    
    // In a real implementation, this would return the raw engine pointer
    // For now, we use a boolean or void* placeholder if headers aren't available
    bool isLoaded() const;

private:
    std::string model_path_;
    bool loaded_;
    
#ifdef ENABLE_CUDA
    std::unique_ptr<nvinfer1::IRuntime> runtime_;
    std::shared_ptr<nvinfer1::ICudaEngine> engine_;
    std::unique_ptr<nvinfer1::ILogger> logger_;
#endif
};
