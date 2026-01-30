#include "model_loader.hpp"
#include <fstream>
#include <iostream>

#ifdef ENABLE_CUDA
#include <NvInfer.h>
#endif

ModelLoader::ModelLoader(const std::string& model_path) 
    : model_path_(model_path), loaded_(false) {
}

ModelLoader::~ModelLoader() {
    // Cleanup logic
}

bool ModelLoader::load() {
    std::ifstream f(model_path_.c_str());
    if (!f.good()) {
        std::cerr << "Model file not found: " << model_path_ << std::endl;
        return false;
    }

    // Check extension
    if (model_path_.find(".onnx") != std::string::npos) {
        std::cout << "Detected ONNX model. Starting conversion to TensorRT..." << std::endl;
        return buildFromOnnx();
    } else if (model_path_.find(".engine") != std::string::npos) {
         std::cout << "Detected TensorRT engine. Deserializing..." << std::endl;
         return deserializeEngine();
    }
    
    loaded_ = false;
    return false;
}

bool ModelLoader::buildFromOnnx() {
    try {
        std::cout << "Loading ONNX model using OpenCV DNN: " << model_path_ << std::endl;
        net_ = cv::dnn::readNetFromONNX(model_path_);
        
        // Use CUDA if available, otherwise CPU
#ifdef ENABLE_CUDA
        net_.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        net_.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        std::cout << "OpenCV DNN: Using CUDA backend." << std::endl;
#else
        net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        std::cout << "OpenCV DNN: Using CPU backend." << std::endl;
#endif

        if (net_.empty()) {
            std::cerr << "Failed to load network with OpenCV DNN." << std::endl;
            return false;
        }

        loaded_ = true;
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV Exception loading ONNX: " << e.what() << std::endl;
        return false;
    }
}

bool ModelLoader::deserializeEngine() {
#ifdef ENABLE_CUDA
    // Real implementation would:
    // 1. Read file into memory
    // 2. Create runtime
    // 3. deserializeCudaEngine
    std::cout << "[Mock] Deserializing TensorRT engine..." << std::endl;
#else
    std::cout << "[Mock] CUDA disabled. Skipping actual deserialization." << std::endl;
#endif
    loaded_ = true;
    return true;
}

bool ModelLoader::saveEngine(const std::string& engine_path) {
    if (!loaded_) return false;
    // Mock save
    std::ofstream f(engine_path);
    f << "TRT-ENGINE-MOCK-DATA";
    f.close();
    return true;
}

bool ModelLoader::isLoaded() const {
    return loaded_;
}
