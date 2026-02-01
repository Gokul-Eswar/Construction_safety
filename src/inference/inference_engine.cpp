#include "inference_engine.hpp"
#include <iostream>

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
        std::cerr << "Failed to load model: " << config_.model_path << std::endl;
        return false;
    }

#ifdef ENABLE_TENSORRT
    if (auto engine = model_loader_->getEngine()) {
        context_ = std::unique_ptr<nvinfer1::IExecutionContext, InferDeleter>(
            engine->createExecutionContext()
        );
        if (!context_) return false;

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

            if (engine->bindingIsInput(i)) {
                input_bytes_ = size;
                cudaMalloc(&buffers_[i], input_bytes_);
            } else {
                output_bytes_ = size;
                cudaMalloc(&buffers_[i], output_bytes_);
                
                // Assuming [1, 84, 8400]
                if (dims.nbDims >= 3) {
                     output_rows_ = dims.d[1];
                     output_cols_ = dims.d[2];
                }
            }
        }

        cudaStreamCreate(&stream_);
        std::cout << "TensorRT Execution Context initialized." << std::endl;
        return true;
    }
#endif
    
    // Check for OpenCV Net
    if (model_loader_->getNet().empty()) return false;
    return true;
}

std::vector<Detection> InferenceEngine::runInference(const cv::Mat& frame) {
    if (frame.empty()) return {};

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

    float x_scale = (float)frame_w / config_.input_width;
    float y_scale = (float)frame_h / config_.input_height;

    for (int i = 0; i < rows; ++i) {
        float* row_ptr = data + (i * cols);
        
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

                int left = int((cx - 0.5 * w) * x_scale);
                int top = int((cy - 0.5 * h) * y_scale);
                int width = int(w * x_scale);
                int height = int(h * y_scale);

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

void InferenceEngine::preprocess(const cv::Mat& input, cv::Mat& output) {
    if (input.empty()) return;
    cv::dnn::blobFromImage(input, output, 1.0/255.0, 
        cv::Size(config_.input_width, config_.input_height), 
        cv::Scalar(), true, false);
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