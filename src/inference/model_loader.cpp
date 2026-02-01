#include "model_loader.hpp"
#include <fstream>
#include <iostream>
#include <vector>

ModelLoader::ModelLoader(const std::string& model_path) 
    : model_path_(model_path), loaded_(false) {
#ifdef ENABLE_TENSORRT
    logger_ = std::make_unique<trt::Logger>();
#endif
}

ModelLoader::~ModelLoader() {
    // unique_ptr handles cleanup
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
#ifdef ENABLE_TENSORRT
    std::cout << "Deserializing TensorRT engine from: " << model_path_ << std::endl;
    
    std::ifstream file(model_path_, std::ios::binary | std::ios::ate);
    if (!file.good()) {
        std::cerr << "Error reading engine file: " << model_path_ << std::endl;
        return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        std::cerr << "Error reading engine file content." << std::endl;
        return false;
    }

    runtime_ = std::unique_ptr<nvinfer1::IRuntime, InferDeleter>(
        nvinfer1::createInferRuntime(*logger_)
    );
    if (!runtime_) {
        std::cerr << "Failed to create TensorRT Runtime." << std::endl;
        return false;
    }

    engine_ = std::unique_ptr<nvinfer1::ICudaEngine, InferDeleter>(
        runtime_->deserializeCudaEngine(buffer.data(), size, nullptr)
    );

    if (!engine_) {
        std::cerr << "Failed to deserialize CUDA Engine." << std::endl;
        return false;
    }

    std::cout << "TensorRT Engine loaded successfully." << std::endl;
    loaded_ = true;
    return true;

#elif defined(ENABLE_CUDA)
    std::cout << "[Mock] Deserializing TensorRT engine..." << std::endl;
    loaded_ = true;
    return true;
#else
    std::cout << "[Mock] CUDA/TRT disabled. Skipping actual deserialization." << std::endl;
    loaded_ = true;
    return true;
#endif
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