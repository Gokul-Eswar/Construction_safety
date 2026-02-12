#include "spatial_mapper.hpp"

SpatialMapper::SpatialMapper() : calibrated_(false) {
}

SpatialMapper::~SpatialMapper() {
}

bool SpatialMapper::setCalibration(const std::vector<cv::Point2f>& image_pts, const std::vector<cv::Point2f>& world_pts) {
    if (image_pts.size() < 4 || world_pts.size() < 4) return false;
    
    homography_ = cv::findHomography(image_pts, world_pts);
    calibrated_ = !homography_.empty();
    return calibrated_;
}

cv::Point2f SpatialMapper::mapToWorld(const cv::Point2f& image_pt) const {
    if (!calibrated_) return image_pt;
    
    std::vector<cv::Point2f> pts = {image_pt};
    std::vector<cv::Point2f> dst;
    cv::perspectiveTransform(pts, dst, homography_);
    return dst[0];
}

std::vector<cv::Point2f> SpatialMapper::mapToWorld(const std::vector<cv::Point2f>& image_pts) const {
    if (!calibrated_ || image_pts.empty()) return image_pts;
    
    std::vector<cv::Point2f> dst;
    cv::perspectiveTransform(image_pts, dst, homography_);
    return dst;
}
