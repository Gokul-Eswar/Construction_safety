#include "inference_engine.hpp"
#include <iostream>

InferenceEngine::InferenceEngine(const InferenceConfig& config) : config_(config) {
}

InferenceEngine::~InferenceEngine() {
}

bool InferenceEngine::init() {
    model_loader_ = std::make_unique<ModelLoader>(config_.model_path);
    if (!model_loader_->load()) {
        std::cerr << "Failed to load model: " << config_.model_path << std::endl;
        return false;
    }
    return true;
}

std::vector<Detection> InferenceEngine::runInference(const cv::Mat& frame) {
    if (frame.empty()) return {};

    // 1. Preprocess
    cv::Mat input_blob;
    preprocess(frame, input_blob);
    
    // 2. Inference
    if (!model_loader_ || !model_loader_->isLoaded()) {
        std::cerr << "Model not loaded!" << std::endl;
        return {};
    }

    auto& net = model_loader_->getNet();
    net.setInput(input_blob);
    
    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());

    // 3. Parse Output (YOLOv8/v11 format: [1, 84, 8400] -> [1, 4+classes, anchors])
    // We need to handle the output format. Usually it's 1x84x8400.
    if (outputs.empty()) return {};

    cv::Mat& output = outputs[0]; // Take the first output
    // Transpose to (anchors, 84) for easier access: 1x84x8400 -> 8400x84
    // Reshape to 2D first: 84 x 8400
    int dimensions = output.dims;
    int rows = output.size[1]; // 84
    int cols = output.size[2]; // 8400
    
    // If the output is actually [1, 8400, 84] (some exporters do this), check dims
    // Standard YOLOv8 export is [1, 84, 8400]
    
    cv::Mat output_2d(rows, cols, CV_32F, output.ptr<float>());
    cv::Mat output_t = output_2d.t(); // Transpose to 8400 x 84

    std::vector<Detection> raw_detections;
    float* data = (float*)output_t.data;
    
    float x_scale = (float)frame.cols / config_.input_width;
    float y_scale = (float)frame.rows / config_.input_height;

    for (int i = 0; i < cols; ++i) {
        float* row_ptr = data + (i * rows);
        
        // Structure: [x, y, w, h, class0_conf, class1_conf, ...]
        // Find best class score
        float max_score = 0.0f;
        int class_id = -1;
        
        // Person class is usually index 0 in COCO. 
        // We only care about Person (class 0) for this safety app.
        // But let's generalise slightly or just check index 4 (first class score).
        
        // Loop through classes (starting at index 4)
        // For COCO (80 classes), rows = 84.
        for (int c = 4; c < rows; ++c) {
            float score = row_ptr[c];
            if (score > max_score) {
                max_score = score;
                class_id = c - 4;
            }
        }

        if (max_score >= config_.conf_threshold) {
            // Only keeping "Person" class (ID 0)
            if (class_id == 0) {
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
    
    // 4. Postprocess (NMS)
    return applyNMS(raw_detections, config_.nms_threshold);
}

void InferenceEngine::preprocess(const cv::Mat& input, cv::Mat& output) {
    if (input.empty()) return;
    
    // Create 4D blob (NCHW)
    // SwapRB=true, Crop=false
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
