#include "model_loader.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <spdlog/spdlog.h>

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
        spdlog::error("Model file not found: {}", model_path_);
        return false;
    }

    // Check extension
    if (model_path_.find(".onnx") != std::string::npos) {
        // Optimization: Check if a serialized .engine file already exists
        std::string engine_path = model_path_.substr(0, model_path_.find_last_of('.')) + ".engine";
        std::ifstream engine_file(engine_path);

        if (engine_file.good()) {
#ifdef ENABLE_TENSORRT
            spdlog::info("Found existing TensorRT engine: {}. Loading directly...", engine_path);
            // Temporarily switch path to load the engine
            std::string original_path = model_path_;
            model_path_ = engine_path;
            bool success = deserializeEngine();
            model_path_ = original_path; // Restore original path
            return success;
#else
            spdlog::info("Found existing TensorRT engine: {}, but TensorRT is disabled. Falling back to ONNX.", engine_path);
#endif
        }

        spdlog::info("Detected ONNX model. Loading with the available inference backend...");
        return buildFromOnnx();
    } else if (model_path_.find(".engine") != std::string::npos) {
         spdlog::info("Detected TensorRT engine. Deserializing...");
         return deserializeEngine();
    }

    loaded_ = false;
    return false;
}

bool ModelLoader::loadOnnxWithOpenCVDnn() {
    try {
        spdlog::info("Loading ONNX model using OpenCV DNN: {}", model_path_);
        net_ = cv::dnn::readNetFromONNX(model_path_);

        // Keep fallback predictable and portable across environments.
        net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        spdlog::info("OpenCV DNN: Using CPU backend.");

        if (net_.empty()) {
            spdlog::error("Failed to load network with OpenCV DNN.");
            return false;
        }

        loaded_ = true;
        return true;
    } catch (const cv::Exception& e) {
        spdlog::error("OpenCV Exception loading ONNX: {}", e.what());
        return false;
    }
}

bool ModelLoader::buildFromOnnx() {
#ifdef ENABLE_TENSORRT
    spdlog::info("Building TensorRT Engine from ONNX: {}", model_path_);
    
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
        spdlog::error("Failed to parse ONNX file with TensorRT. Falling back to OpenCV DNN.");
        return loadOnnxWithOpenCVDnn();
    }

    auto config = std::unique_ptr<nvinfer1::IBuilderConfig, InferDeleter>(
        builder->createBuilderConfig()
    );
    if (!config) return false;

    if (builder->platformHasFastFp16()) {
        config->setFlag(nvinfer1::BuilderFlag::kFP16);
        spdlog::info("FP16 mode enabled.");
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
                spdlog::info("Dynamic input detected: {}. Creating optimization profile.", input->getName());
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
        spdlog::error("Failed to build serialized TensorRT network. Falling back to OpenCV DNN.");
        return loadOnnxWithOpenCVDnn();
    }

    runtime_ = std::unique_ptr<nvinfer1::IRuntime, InferDeleter>(
        nvinfer1::createInferRuntime(*logger_)
    );
    
    engine_ = std::unique_ptr<nvinfer1::ICudaEngine, InferDeleter>(
        runtime_->deserializeCudaEngine(plan->data(), plan->size(), nullptr)
    );

    if (!engine_) {
        spdlog::error("Failed to create TensorRT engine from plan. Falling back to OpenCV DNN.");
        return loadOnnxWithOpenCVDnn();
    }
    
    spdlog::info("Successfully built and loaded TensorRT engine.");
    loaded_ = true;

    // Auto-save engine
    std::string engine_path = model_path_.substr(0, model_path_.find_last_of('.')) + ".engine";
    saveEngine(engine_path);

    return true;

#else
    return loadOnnxWithOpenCVDnn();
#endif
}

bool ModelLoader::deserializeEngine() {
#ifdef ENABLE_TENSORRT
    spdlog::info("Deserializing TensorRT engine from: {}", model_path_);

    std::ifstream file(model_path_, std::ios::binary | std::ios::ate);
    if (!file.good()) {
        spdlog::error("Error reading engine file: {}", model_path_);
        return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) return false;

    runtime_ = std::unique_ptr<nvinfer1::IRuntime, InferDeleter>(
        nvinfer1::createInferRuntime(*logger_)
    );

    engine_ = std::unique_ptr<nvinfer1::ICudaEngine, InferDeleter>(
        runtime_->deserializeCudaEngine(buffer.data(), size, nullptr)
    );

    if (!engine_) {
        spdlog::error("Failed to deserialize TensorRT engine.");
        return false;
    }

    spdlog::info("Successfully loaded TensorRT engine.");
    loaded_ = true;
    return true;
#else
    return false;
#endif
}

void ModelLoader::saveEngine(const std::string& path) {
#ifdef ENABLE_TENSORRT
    if (!engine_) return;

    std::unique_ptr<nvinfer1::IHostMemory, InferDeleter> serialized{
        engine_->serialize()
    };
    if (!serialized) return;

    std::ofstream p(path, std::ios::binary);
    if (!p) {
        spdlog::error("Failed to open file for writing engine: {}", path);
        return;
    }
    p.write(reinterpret_cast<const char*>(serialized->data()), serialized->size());
    spdlog::info("Saved TensorRT engine to: {}", path);
#endif
}
