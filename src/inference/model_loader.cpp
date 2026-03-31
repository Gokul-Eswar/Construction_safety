#include "model_loader.hpp"
#include <fstream>
#include <iostream>
#include <vector>

#ifdef ENABLE_TENSORRT
#include <NvOnnxParser.h>
#endif

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
        std::cerr << "Model file not found: " << model_path_ << "\n";
        return false;
    }

    // Check extension
    if (model_path_.find(".onnx") != std::string::npos) {
        // Optimization: Check if a serialized .engine file already exists
        std::string engine_path = model_path_.substr(0, model_path_.find_last_of('.')) + ".engine";
        std::ifstream engine_file(engine_path);
        
        if (engine_file.good()) {
            std::cout << "Found existing TensorRT engine: " << engine_path << ". Loading directly..." << "\n";
            // Temporarily switch path to load the engine
            std::string original_path = model_path_;
            model_path_ = engine_path;
            bool success = deserializeEngine();
            model_path_ = original_path; // Restore original path
            return success;
        }

        std::cout << "Detected ONNX model. Starting conversion to TensorRT..." << "\n";
        return buildFromOnnx();
    } else if (model_path_.find(".engine") != std::string::npos) {
         std::cout << "Detected TensorRT engine. Deserializing..." << "\n";
         return deserializeEngine();
    }
    
    loaded_ = false;
    return false;
}

bool ModelLoader::buildFromOnnx() {
#ifdef ENABLE_TENSORRT
    std::cout << "Building TensorRT Engine from ONNX: " << model_path_ << "\n";
    
    auto builder = std::unique_ptr<nvinfer1::IBuilder, InferDeleter>(
        nvinfer1::createInferBuilder(*logger_)
    );
    if (!builder) return false;

    const auto explicitBatch = 1U << static_cast<uint32_t>(nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    auto network = std::unique_ptr<nvinfer1::INetworkDefinition, InferDeleter>(
        builder->createNetworkV2(explicitBatch)
    );
    if (!network) return false;

    auto parser = std::unique_ptr<nvonnxparser::IParser, InferDeleter>(
        nvonnxparser::createParser(*network, *logger_)
    );
    if (!parser) return false;

    if (!parser->parseFromFile(model_path_.c_str(), static_cast<int>(nvinfer1::ILogger::Severity::kWARNING))) {
        std::cerr << "Failed to parse ONNX file." << "\n";
        return false;
    }

    auto config = std::unique_ptr<nvinfer1::IBuilderConfig, InferDeleter>(
        builder->createBuilderConfig()
    );
    if (!config) return false;

    if (builder->platformHasFastFp16()) {
        config->setFlag(nvinfer1::BuilderFlag::kFP16);
        std::cout << "FP16 mode enabled." << "\n";
    }

    // Dynamic Shape Support (Required for YOLO models with dynamic axes)
    // We create a profile that forces a standard resolution for optimization
    auto profile = builder->createOptimizationProfile();
    // Assuming standard YOLO input "images" - checking first input usually works
    if (network->getNbInputs() > 0) {
        auto input = network->getInput(0);
        if (input->getDimensions().nbDims == 4) {
            // Check if any dimension is dynamic (-1)
            bool isDynamic = false;
            for(int i=0; i<4; i++) {
                if(input->getDimensions().d[i] == -1) isDynamic = true;
            }

            if (isDynamic) {
                std::cout << "Dynamic input detected: " << input->getName() << ". Creating optimization profile." << "\n";
                // Min, Opt, Max dimensions
                // Forcing 640x640 for consistency and speed
                profile->setDimensions(input->getName(), nvinfer1::OptProfileSelector::kMIN, nvinfer1::Dims4{1, 3, 640, 640});
                profile->setDimensions(input->getName(), nvinfer1::OptProfileSelector::kOPT, nvinfer1::Dims4{1, 3, 640, 640});
                profile->setDimensions(input->getName(), nvinfer1::OptProfileSelector::kMAX, nvinfer1::Dims4{1, 3, 640, 640});
                config->addOptimizationProfile(profile);
            }
        }
    }

    std::unique_ptr<nvinfer1::IHostMemory, InferDeleter> plan{
        builder->buildSerializedNetwork(*network, *config)
    };
    if (!plan) {
        std::cerr << "Failed to build serialized network." << "\n";
        return false;
    }

    runtime_ = std::unique_ptr<nvinfer1::IRuntime, InferDeleter>(
        nvinfer1::createInferRuntime(*logger_)
    );
    
    engine_ = std::unique_ptr<nvinfer1::ICudaEngine, InferDeleter>(
        runtime_->deserializeCudaEngine(plan->data(), plan->size(), nullptr)
    );

    if (!engine_) {
        std::cerr << "Failed to create engine from plan." << "\n";
        return false;
    }
    
    std::cout << "Successfully built and loaded TensorRT engine." << "\n";
    loaded_ = true;

    // Auto-save engine
    std::string engine_path = model_path_.substr(0, model_path_.find_last_of('.')) + ".engine";
    saveEngine(engine_path);

    return true;

#else
    try {
        std::cout << "Loading ONNX model using OpenCV DNN: " << model_path_ << "\n";
        net_ = cv::dnn::readNetFromONNX(model_path_);
        
        // Use CUDA if available, otherwise CPU
#ifdef ENABLE_CUDA
        net_.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        net_.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        std::cout << "OpenCV DNN: Using CUDA backend." << "\n";
#else
        net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        std::cout << "OpenCV DNN: Using CPU backend." << "\n";
#endif

        if (net_.empty()) {
            std::cerr << "Failed to load network with OpenCV DNN." << "\n";
            return false;
        }

        loaded_ = true;
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV Exception loading ONNX: " << e.what() << "\n";
        return false;
    }
#endif
}

bool ModelLoader::deserializeEngine() {
#ifdef ENABLE_TENSORRT
    std::cout << "Deserializing TensorRT engine from: " << model_path_ << "\n";
    
    std::ifstream file(model_path_, std::ios::binary | std::ios::ate);
    if (!file.good()) {
        std::cerr << "Error reading engine file: " << model_path_ << "\n";
        return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        std::cerr << "Error reading engine file content." << "\n";
        return false;
    }

    runtime_ = std::unique_ptr<nvinfer1::IRuntime, InferDeleter>(
        nvinfer1::createInferRuntime(*logger_)
    );
    if (!runtime_) {
        std::cerr << "Failed to create TensorRT Runtime." << "\n";
        return false;
    }

    engine_ = std::unique_ptr<nvinfer1::ICudaEngine, InferDeleter>(
        runtime_->deserializeCudaEngine(buffer.data(), size, nullptr)
    );

    if (!engine_) {
        std::cerr << "Failed to deserialize CUDA Engine." << "\n";
        return false;
    }

    std::cout << "TensorRT Engine loaded successfully." << "\n";
    loaded_ = true;
    return true;

#elif defined(ENABLE_CUDA)
    std::cout << "[Mock] Deserializing TensorRT engine..." << "\n";
    loaded_ = true;
    return true;
#else
    std::cout << "[Mock] CUDA/TRT disabled. Skipping actual deserialization." << "\n";
    loaded_ = true;
    return true;
#endif
}

bool ModelLoader::saveEngine(const std::string& engine_path) {
#ifdef ENABLE_TENSORRT
    if (!engine_) return false;
    
    std::cout << "Serializing TensorRT engine to: " << engine_path << "\n";
    std::unique_ptr<nvinfer1::IHostMemory, InferDeleter> plan{ engine_->serialize() };
    if (!plan) {
        std::cerr << "Failed to serialize engine." << "\n";
        return false;
    }

    std::ofstream f(engine_path, std::ios::binary);
    if (!f.good()) {
        std::cerr << "Cannot open file for writing: " << engine_path << "\n";
        return false;
    }
    f.write(reinterpret_cast<const char*>(plan->data()), plan->size());
    f.close();
    std::cout << "Engine saved successfully." << "\n";
    return true;
#else
    // If TRT is not enabled, we can't save a TRT engine.
    (void)engine_path;
    std::cerr << "Cannot save TensorRT engine: TRT disabled." << "\n";
    return false;
#endif
}

bool ModelLoader::isLoaded() const {
    return loaded_;
}
