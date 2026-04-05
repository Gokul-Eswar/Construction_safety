#include "inference_engine.hpp"
#include <iostream>
#include <cstddef>  // for ptrdiff_t

#ifdef ENABLE_CUDA
#include <cuda_runtime_api.h>
#endif

InferenceEngine::InferenceEngine(const InferenceConfig& config) : config_(config) {
}

InferenceEngine::~InferenceEngine() {
#ifdef ENABLE_TENSORRT
#ifdef ENABLE_CUDA
    if (stream_) cudaStreamDestroy(stream_);
    if (buffers_[0]) cudaFree(buffers_[0]);
    if (buffers_[1]) cudaFree(buffers_[1]);
#endif
#endif
}

bool InferenceEngine::init() {
    model_loader_ = std::make_unique<ModelLoader>(config_.model_path);
    if (!model_loader_->load()) {
        std::cerr << "Failed to load model: " << config_.model_path
                  << ". Continuing in degraded mode (no inference)." << "\n";
        operational_ = false;
        return true;
    }

    // Initialize CLAHE object if enabled
    if (config_.clahe.enabled) {
        clahe_ = cv::createCLAHE(config_.clahe.clip_limit, 
                                 cv::Size(config_.clahe.tile_size, config_.clahe.tile_size));
        if (!clahe_) {
            std::cerr << "Failed to initialize CLAHE; continuing without lighting correction" << "\n";
            config_.clahe.enabled = false;
        }
    }

#ifdef ENABLE_TENSORRT
    if (auto engine = model_loader_->getEngine()) {
        context_ = std::unique_ptr<nvinfer1::IExecutionContext, InferDeleter>(
            engine->createExecutionContext()
        );
        if (!context_) return false;

        // Calculate memory requirement before allocation
        size_t mem_requirement = 0;
        
        // Allocate memory & Stream
        int nbBindings = engine->getNbBindings();
        for (int i = 0; i < nbBindings; ++i) {
            nvinfer1::Dims dims = engine->getBindingDimensions(i);
            
            // Calculate size
            size_t vol = 1; 
            for (int j = 0; j < dims.nbDims; ++j) {
                vol *= dims.d[j] > 0 ? dims.d[j] : 1; 
            }
            size_t size = vol * sizeof(float);
            mem_requirement += size;

            if (engine->bindingIsInput(i)) {
                input_bytes_ = size;
            } else {
                output_bytes_ = size;
                
                // Assuming [1, 84, 8400]
                if (dims.nbDims >= 3) {
                     output_rows_ = dims.d[1];
                     output_cols_ = dims.d[2];
                }
            }
        }
        
        required_memory_bytes_ = mem_requirement + (10 * 1024 * 1024);  // Add 10MB buffer for internal buffers
        
        // Check available GPU memory before allocation
        if (!checkAvailableGPUMemory(required_memory_bytes_)) {
            std::cerr << "Insufficient GPU memory for inference engine initialization" << "\n";
            return false;
        }
        
        // Safe to allocate now
        for (int i = 0; i < nbBindings; ++i) {
            if (i == 0 && input_bytes_ > 0) {
                cudaMalloc(&buffers_[i], input_bytes_);
            } else if (i == 1 && output_bytes_ > 0) {
                cudaMalloc(&buffers_[i], output_bytes_);
            }
        }

        cudaStreamCreate(&stream_);
        std::cout << "TensorRT Execution Context initialized." << "\n";
        operational_ = true;
        return true;
    }
#endif
    
    // Check for OpenCV Net
    if (model_loader_->getNet().empty()) {
        operational_ = false;
        return true;
    }
    operational_ = true;
    return true;
}

std::vector<Detection> InferenceEngine::runInference(const cv::Mat& frame) {
    if (frame.empty() || !operational_) return {};

    cv::Mat input_blob;
    preprocess(frame, input_blob);

#ifdef ENABLE_TENSORRT
    if (context_) {
        // TRT Inference
        if (!input_bytes_ || !output_bytes_) return {};

        cudaMemcpyAsync(buffers_[0], input_blob.ptr<float>(), input_bytes_, cudaMemcpyHostToDevice, stream_);
        context_->enqueueV2(buffers_, stream_, nullptr);
        
        std::vector<float> cpu_output(output_bytes_ / sizeof(float));
        cudaMemcpyAsync(cpu_output.data(), buffers_[1], output_bytes_, cudaMemcpyDeviceToHost, stream_);
        cudaStreamSynchronize(stream_);

        // Wrap in Mat [rows, cols]
        cv::Mat output_2d(output_rows_, output_cols_, CV_32F, cpu_output.data());
        cv::Mat output_t = output_2d.t(); 
        
        return applyNMS(parseDetections(output_t, frame.cols, frame.rows), config_.nms_threshold);
    }
#endif

    // OpenCV DNN Fallback
    auto& net = model_loader_->getNet();
    if (net.empty()) return {};

    net.setInput(input_blob);
    
    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());

    if (outputs.empty()) return {};

    cv::Mat& output = outputs[0]; 
    int rows = output.size[1]; 
    int cols = output.size[2]; 
    
    cv::Mat output_2d(rows, cols, CV_32F, output.ptr<float>());
    cv::Mat output_t = output_2d.t(); 

    return applyNMS(parseDetections(output_t, frame.cols, frame.rows), config_.nms_threshold);
}

std::vector<Detection> InferenceEngine::parseDetections(const cv::Mat& output_t, int frame_w, int frame_h) {
    std::vector<Detection> raw_detections;
    float* data = (float*)output_t.data;
    
    int rows = output_t.rows; // 8400
    int cols = output_t.cols; // 84

    // Calculate scaling params (matching preprocess logic)
    float ratio_w = static_cast<float>(config_.input_width) / static_cast<float>(frame_w);
    float ratio_h = static_cast<float>(config_.input_height) / static_cast<float>(frame_h);
    float scale = std::min(ratio_w, ratio_h);
    
    float new_unpad_w = static_cast<float>(frame_w) * scale;
    float new_unpad_h = static_cast<float>(frame_h) * scale;
    
    float dw = (static_cast<float>(config_.input_width) - new_unpad_w) / 2.0f;
    float dh = (static_cast<float>(config_.input_height) - new_unpad_h) / 2.0f;

    for (int i = 0; i < rows; ++i) {
        // Safe pointer arithmetic using ptrdiff_t
        float* row_ptr = data + (static_cast<ptrdiff_t>(i) * static_cast<ptrdiff_t>(cols));
        
        float max_score = 0.0f;
        int class_id = -1;
        
        for (int c = 4; c < cols; ++c) {
            float score = row_ptr[c];
            if (score > max_score) {
                max_score = score;
                class_id = c - 4;
            }
        }

        if (max_score >= config_.conf_threshold) {
            if (class_id == 0) { // Person
                float cx = row_ptr[0];
                float cy = row_ptr[1];
                float w = row_ptr[2];
                float h = row_ptr[3];

                // Remap from Letterbox -> Original
                float cx_orig = (cx - dw) / scale;
                float cy_orig = (cy - dh) / scale;
                float w_orig = w / scale;
                float h_orig = h / scale;

                int left = static_cast<int>(cx_orig - 0.5f * w_orig);
                int top = static_cast<int>(cy_orig - 0.5f * h_orig);
                int width = static_cast<int>(w_orig);
                int height = static_cast<int>(h_orig);

                Detection det;
                det.class_id = class_id;
                det.confidence = max_score;
                det.box = cv::Rect(left, top, width, height);
                raw_detections.push_back(det);
            }
        }
    }
    return raw_detections;
}

void InferenceEngine::applyCLAHE(const cv::Mat& input, cv::Mat& output) {
    if (!config_.clahe.enabled || !clahe_) {
        output = input.clone();
        return;
    }
    
    // ================================================================================
    // CLAHE Preprocessing for Extreme Lighting (Issue 2 Implementation)
    // ================================================================================
    
    // Step 1: Convert BGR to LAB color space (L = lightness, preserves color info)
    cv::Mat lab_image;
    cv::cvtColor(input, lab_image, cv::COLOR_BGR2Lab);

    // Step 2: Split LAB channels
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab_image, lab_planes);

    // Step 3: Apply CLAHE to L (lightness) channel only
    // This boosts contrast locally without destroying color information
    cv::Mat dst;
    clahe_->apply(lab_planes[0], dst);
    dst.copyTo(lab_planes[0]);

    // Step 4: Merge channels back
    cv::Mat lab_corrected;
    cv::merge(lab_planes, lab_corrected);

    // Step 5: Convert back to BGR
    cv::Mat bgr_corrected;
    cv::cvtColor(lab_corrected, bgr_corrected, cv::COLOR_Lab2BGR);
    
    // Step 6: Apply Gaussian blur to reduce CLAHE tile artifacts
    // (CLAHE can create visible tile boundaries; blur smooths them)
    if (config_.clahe.blur_kernel > 0 && config_.clahe.blur_kernel % 2 == 1) {
        cv::GaussianBlur(bgr_corrected, output, cv::Size(config_.clahe.blur_kernel, config_.clahe.blur_kernel), 1.0);
    } else {
        output = bgr_corrected;
    }
}

void InferenceEngine::preprocess(const cv::Mat& input, cv::Mat& output) {
    if (input.empty()) return;

    // ================================================================================
    // Step 1: CLAHE Preprocessing for Extreme Lighting (configurable)
    // ================================================================================
    cv::Mat lighting_corrected;
    applyCLAHE(input, lighting_corrected);
    
    // ================================================================================
    // Step 2: Letterbox Resize
    // ================================================================================
    int iw = lighting_corrected.cols;
    int ih = lighting_corrected.rows;
    int w = config_.input_width;
    int h = config_.input_height;
    
    float scale = std::min((float)w / iw, (float)h / ih);
    int nw = int(iw * scale);
    int nh = int(ih * scale);
    
    cv::Mat resized;
    cv::resize(lighting_corrected, resized, cv::Size(nw, nh));
    
    // Create canvas with padding (114 is YOLO grey)
    cv::Mat canvas(h, w, CV_8UC3, cv::Scalar(114, 114, 114));
    
    // Center the image
    int dx = (w - nw) / 2;
    int dy = (h - nh) / 2;
    
    resized.copyTo(canvas(cv::Rect(dx, dy, nw, nh)));
    
    // ================================================================================
    // Step 3: Normalize [0,1] and NCHW for YOLO model
    // ================================================================================
    cv::dnn::blobFromImage(canvas, output, 1.0/255.0, cv::Size(), cv::Scalar(), true, false);
}

std::vector<Detection> InferenceEngine::applyNMS(const std::vector<Detection>& detections, float nms_thresh) {
    if (detections.empty()) return {};

    std::vector<int> indices;
    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;

    for (const auto& det : detections) {
        boxes.push_back(det.box);
        confidences.push_back(det.confidence);
    }

    cv::dnn::NMSBoxes(boxes, confidences, config_.conf_threshold, nms_thresh, indices);

    std::vector<Detection> result;
    for (int idx : indices) {
        result.push_back(detections[idx]);
    }

    return result;
}

bool InferenceEngine::checkAvailableGPUMemory(size_t required_bytes) {
#ifdef ENABLE_CUDA
    size_t free_bytes = 0, total_bytes = 0;
    cudaError_t cuda_status = cudaMemGetInfo(&free_bytes, &total_bytes);
    
    if (cuda_status != cudaSuccess) {
        std::cerr << "Error querying GPU memory: " << cudaGetErrorString(cuda_status) << "\n";
        return false;
    }
    
    std::cout << "GPU Memory Status: " << (free_bytes / 1024 / 1024) << " MB free / " 
              << (total_bytes / 1024 / 1024) << " MB total" << "\n";
    
    if (free_bytes < required_bytes) {
        std::cerr << "Insufficient GPU memory. Required: " << (required_bytes / 1024 / 1024) << " MB, "
                  << "Available: " << (free_bytes / 1024 / 1024) << " MB" << "\n";
        return false;
    }
    
    return true;
#else
    // Without CUDA, can't check; assume OK
    return true;
#endif
}
