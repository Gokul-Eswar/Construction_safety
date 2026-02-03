#pragma once
#include <string>
#include <vector>
#include <memory>
#include <opencv2/dnn.hpp>
#include "trt_utils.hpp"

class ModelLoader {
public:
    ModelLoader(const std::string& model_path);
    ~ModelLoader();

    bool load();
    bool saveEngine(const std::string& engine_path);
    bool isLoaded() const;

    // Get the OpenCV DNN Net object (for fallback/CPU)
    cv::dnn::Net& getNet() { return net_; }

#ifdef ENABLE_TENSORRT
    nvinfer1::ICudaEngine* getEngine() { return engine_.get(); }
#endif

private:
    bool buildFromOnnx();
    bool deserializeEngine();

    std::string model_path_;
    bool loaded_;
    
    cv::dnn::Net net_;

#ifdef ENABLE_TENSORRT
    struct TRTDeleter {
        template <typename T>
        void operator()(T* obj) const {
            if (obj) delete obj; // Helper classes might not have destroy(), usually IRuntime/ICudaEngine do.
            // Wait, standard TRT classes use destroy(), but ILogger usually doesn't need to be destroyed if stack allocated or unique_ptr handles it.
            // Actually, IRuntime and ICudaEngine usually have destroy() and are created via create...() functions.
            // But strict unique_ptr with deleter is better.
        }
    };
    
    // Use specific deleters for TRT objects that require destroy()
    struct InferDeleter {
        template <typename T>
        void operator()(T* obj) const {
            if (obj) obj->destroy();
        }
    };

    std::unique_ptr<nvinfer1::IRuntime, InferDeleter> runtime_;
    std::unique_ptr<nvinfer1::ICudaEngine, InferDeleter> engine_;
    std::unique_ptr<trt::Logger> logger_;
#endif
};