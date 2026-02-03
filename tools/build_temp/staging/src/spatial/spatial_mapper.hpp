#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

class SpatialMapper {
public:
    SpatialMapper();
    ~SpatialMapper();

    bool setCalibration(const std::vector<cv::Point2f>& image_pts, const std::vector<cv::Point2f>& world_pts);
    cv::Point2f mapToWorld(const cv::Point2f& image_pt) const;
    std::vector<cv::Point2f> mapToWorld(const std::vector<cv::Point2f>& image_pts) const;

private:
    cv::Mat homography_;
    bool calibrated_;
};
