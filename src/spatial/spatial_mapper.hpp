#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

class SpatialMapper {
public:
    SpatialMapper();
    ~SpatialMapper();

    [[nodiscard]] bool setCalibration(const std::vector<cv::Point2f>& image_pts, const std::vector<cv::Point2f>& world_pts);
    [[nodiscard]] cv::Point2f mapToWorld(const cv::Point2f& image_pt) const;
    [[nodiscard]] std::vector<cv::Point2f> mapToWorld(const std::vector<cv::Point2f>& image_pts) const;
    [[nodiscard]] bool isCalibrated() const { return calibrated_; }

private:
    cv::Mat homography_;
    bool calibrated_;
};
