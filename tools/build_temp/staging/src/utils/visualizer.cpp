#include "visualizer.hpp"

Visualizer::Visualizer() {
}

Visualizer::~Visualizer() {
}

void Visualizer::drawDetections(cv::Mat& frame, const std::vector<Detection>& detections) {
    for (const auto& det : detections) {
        cv::Scalar color = getColor(det.class_id);
        cv::rectangle(frame, det.box, color, 2);

        std::string label = "Person";
        if (det.track_id != -1) {
            label += " #" + std::to_string(det.track_id);
        }
        label += ": " + std::to_string(static_cast<int>(det.confidence * 100)) + "%";

        int baseLine;
        cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);
        cv::rectangle(frame, cv::Point(det.box.x, det.box.y - labelSize.height),
                      cv::Point(det.box.x + labelSize.width, det.box.y + baseLine),
                      color, cv::FILLED);
        cv::putText(frame, label, cv::Point(det.box.x, det.box.y),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    }
}

void Visualizer::drawZones(cv::Mat& frame, const std::vector<std::vector<cv::Point>>& zones) {
    for (const auto& zone : zones) {
        std::vector<std::vector<cv::Point>> pts = {zone};
        cv::polylines(frame, pts, true, cv::Scalar(0, 255, 255), 2);
        
        // Semi-transparent fill
        cv::Mat overlay = frame.clone();
        cv::fillPoly(overlay, pts, cv::Scalar(0, 255, 255));
        cv::addWeighted(overlay, 0.3, frame, 0.7, 0, frame);
    }
}

void Visualizer::drawStatus(cv::Mat& frame, const std::string& status, const cv::Point& pos) {
    cv::putText(frame, status, pos, cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
}

cv::Scalar Visualizer::getColor(int class_id) {
    // Simple color palette
    std::vector<cv::Scalar> colors = {
        cv::Scalar(255, 0, 0), cv::Scalar(0, 255, 0), cv::Scalar(0, 0, 255),
        cv::Scalar(255, 255, 0), cv::Scalar(255, 0, 255), cv::Scalar(0, 255, 255)
    };
    return colors[class_id % colors.size()];
}
