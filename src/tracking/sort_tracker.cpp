#include "sort_tracker.hpp"
#include <algorithm>
#include <set>
#include <cmath>

// ================================================================================
// Constructor with individual parameters (backward compatibility)
// ================================================================================
SortTracker::SortTracker(int maxAge, int minHits, float iouThreshold, float featureThreshold, int occlusionExtensionFrames)
    : max_age_(maxAge), min_hits_(minHits), iou_threshold_(iouThreshold), feature_threshold_(featureThreshold),
      occlusion_extension_frames_(occlusionExtensionFrames), next_id_(1) {
}

// ================================================================================
// Constructor with TrackingConfig (from AppConfig)
// ================================================================================
SortTracker::SortTracker(const TrackingConfig& config)
    : max_age_(config.max_age), min_hits_(config.min_hits), iou_threshold_(config.iou_threshold),
      feature_threshold_(config.feature_threshold), occlusion_extension_frames_(config.occlusion_extension_frames),
      next_id_(1) {
}

// ================================================================================
// Occlusion Detection Heuristic
// ================================================================================
bool SortTracker::isLikelyOccluded(const cv::Rect2f& prev_box, const cv::Rect2f& current_box) const {
    // If bounding box area drops significantly but center is relatively close,
    // likely occlusion occurred (person hiding behind object)
    
    float prev_area = prev_box.area();
    float curr_area = current_box.area();
    
    if (prev_area < 1.0f) return false;  // Avoid division by zero
    
    float area_ratio = curr_area / prev_area;
    
    // Calculate center distance
    cv::Point2f prev_center(prev_box.x + prev_box.width / 2.0f, prev_box.y + prev_box.height / 2.0f);
    cv::Point2f curr_center(current_box.x + current_box.width / 2.0f, current_box.y + current_box.height / 2.0f);
    float center_distance = cv::norm(prev_center - curr_center);
    
    // Heuristic: Area drop >50% but center distance <30px = likely occlusion
    // (object partially hidden but tracked position remains close)
    return (area_ratio < 0.5f) && (center_distance < 30.0f);
}

std::vector<Detection> SortTracker::update(const std::vector<Detection>& detections) {
    // 1. Predict all current trackers
    std::vector<cv::Rect2f> predicted_boxes;
    for (auto& trk : trackers_) {
        predicted_boxes.push_back(trk->predict());
    }

    // 2. Match detections to predicted boxes (Fused Score matching with distance gating)
    std::vector<int> matched_indices_det;
    std::vector<int> matched_indices_trk;

    struct Match { int det_idx; int trk_idx; float score; };
    std::vector<Match> matches;
    
    // Distance gating threshold (pixels): Prevent matching if boxes are too far apart
    const float MAX_SPATIAL_DISTANCE = 150.0f;

    for (size_t i = 0; i < detections.size(); ++i) {
        for (size_t j = 0; j < trackers_.size(); ++j) {
            float iou = calculateIou(detections[i].box, predicted_boxes[j]);
            float dist = calculateFeatureDist(detections[i].feature, trackers_[j]->getFeature());
            float spatial_dist = calculateSpatialDistance(detections[i].box, predicted_boxes[j]);

            // Fused matching: If IOU is high OR feature is very similar
            // This handles occlusion where IOU drops but visual identity remains
            // BUT gates on spatial distance to prevent ID swaps
            bool match_gate = (iou >= iou_threshold_) && (spatial_dist < MAX_SPATIAL_DISTANCE);
            if (!match_gate && !detections[i].feature.empty() && !trackers_[j]->getFeature().empty()) {
                if (dist < feature_threshold_ && spatial_dist < MAX_SPATIAL_DISTANCE) match_gate = true;
            }

            if (match_gate) {
                // Score combines spatial overlap, visual similarity, and distance penalty
                float visual_sim = 1.0f - dist;
                // Normalize spatial distance to [0, 1] for scoring
                float spatial_penalty = spatial_dist / MAX_SPATIAL_DISTANCE;
                float score = 0.5f * iou + 0.3f * visual_sim - 0.2f * spatial_penalty;
                matches.push_back({(int)i, (int)j, score});
            }
        }
    }

    // Sort matches by combined score descending
    std::sort(matches.begin(), matches.end(), [](const Match& a, const Match& b) {
        return a.score > b.score;
    });

    std::set<int> used_det;
    std::set<int> used_trk;

    for (const auto& m : matches) {
        if (used_det.count(m.det_idx) == 0 && used_trk.count(m.trk_idx) == 0) {
            used_det.insert(m.det_idx);
            used_trk.insert(m.trk_idx);
            trackers_[m.trk_idx]->update(detections[m.det_idx].box, detections[m.det_idx].feature);
            
            // Reset occlusion frame counter on successful match
            occlusion_frames_[m.trk_idx] = 0;
            previous_boxes_[m.trk_idx] = detections[m.det_idx].box;
        }
    }

    // 3. Create new trackers for unmatched detections
    for (size_t i = 0; i < detections.size(); ++i) {
        if (used_det.count(i) == 0) {
            trackers_.push_back(std::make_unique<KalmanBoxTracker>(detections[i].box, next_id_++));
            occlusion_frames_.push_back(0);
            previous_boxes_.push_back(detections[i].box);
            // Initialize with the first feature
            trackers_.back()->update(detections[i].box, detections[i].feature);
        }
    }

    // 4. Prune old trackers (with stationary & occlusion persistence support)
    const int MAX_STATIONARY_AGE_LOCAL = 10;  // Replicate private constant
    for (size_t idx = 0; idx < trackers_.size(); ) {
        auto& trk = trackers_[idx];
        
        if (trk->getHitStreak() < min_hits_) {
            trackers_.erase(trackers_.begin() + idx);
            occlusion_frames_.erase(occlusion_frames_.begin() + idx);
            previous_boxes_.erase(previous_boxes_.begin() + idx);
        } else {
            int time_since_update = trk->getTimeSinceUpdate();
            int effective_max_age = max_age_;
            
            // ================================================================================
            // OCCLUSION HANDLING (Issue 1 Fix)
            // ================================================================================
            if (occlusion_frames_[idx] > 0) {
                // Object is likely occluded; extend max_age
                effective_max_age = std::max(max_age_, max_age_ + occlusion_extension_frames_);
            } else if (trk->isStationary()) {
                // Object is stationary; extend max_age
                effective_max_age = std::max(max_age_, time_since_update + MAX_STATIONARY_AGE_LOCAL);
            }
            
            if (time_since_update > effective_max_age) {
                trackers_.erase(trackers_.begin() + idx);
                occlusion_frames_.erase(occlusion_frames_.begin() + idx);
                previous_boxes_.erase(previous_boxes_.begin() + idx);
            } else {
                // Increment occlusion frame counter for next update
                if (time_since_update > 1 && idx < previous_boxes_.size()) {
                    // Check if this unmatched detection looks occluded
                    if (isLikelyOccluded(previous_boxes_[idx], trk->getState())) {
                        occlusion_frames_[idx]++;
                    } else if (occlusion_frames_[idx] > 0) {
                        occlusion_frames_[idx]--;  // Decay occlusion counter
                    }
                }
                
                ++idx;
            }
        }
    }

    // 5. Final pass to assign IDs to input detections
    std::vector<Detection> tracked_detections = detections;
    // Reset IDs
    for(auto& d : tracked_detections) d.track_id = -1;

    for (size_t i = 0; i < tracked_detections.size(); ++i) {
        float max_iou = -1.0f;
        int best_id = -1;
        for (const auto& trk : trackers_) {
            float iou = calculateIou(tracked_detections[i].box, trk->getState());
            if (iou > iou_threshold_ && iou > max_iou) {
                max_iou = iou;
                best_id = trk->getId();
            }
        }
        tracked_detections[i].track_id = best_id;
    }

    return tracked_detections;
}

float SortTracker::calculateIou(cv::Rect2f bbTest, cv::Rect2f bbGt) const {
    float in = (bbTest & bbGt).area();
    float un = bbTest.area() + bbGt.area() - in;
    if (un <= 0) return 0;
    return in / un;
}

float SortTracker::calculateFeatureDist(const cv::Mat& f1, const cv::Mat& f2) const {
    if (f1.empty() || f2.empty()) return 1.0f; // Max distance
    // Using Cosine Distance (1.0 - Cosine Similarity)
    // Since we normalize vectors in KalmanBoxTracker, Dot Product = Similarity
    double dot = f1.dot(f2);
    return 1.0f - static_cast<float>(dot);
}

float SortTracker::calculateSpatialDistance(cv::Rect2f det, cv::Rect2f pred) const {
    cv::Point2f det_center(det.x + det.width / 2.0f, det.y + det.height / 2.0f);
    cv::Point2f pred_center(pred.x + pred.width / 2.0f, pred.y + pred.height / 2.0f);
    return cv::norm(det_center - pred_center);
}

std::vector<Detection> SortTracker::update(const std::vector<Detection>& detections) {
    // 1. Predict all current trackers
    std::vector<cv::Rect2f> predicted_boxes;
    for (auto& trk : trackers_) {
        predicted_boxes.push_back(trk->predict());
    }

    // 2. Match detections to predicted boxes (Fused Score matching with distance gating)
    std::vector<int> matched_indices_det;
    std::vector<int> matched_indices_trk;

    struct Match { int det_idx; int trk_idx; float score; };
    std::vector<Match> matches;
    
    // Distance gating threshold (pixels): Prevent matching if boxes are too far apart
    const float MAX_SPATIAL_DISTANCE = 150.0f;

    for (size_t i = 0; i < detections.size(); ++i) {
        for (size_t j = 0; j < trackers_.size(); ++j) {
            float iou = calculateIou(detections[i].box, predicted_boxes[j]);
            float dist = calculateFeatureDist(detections[i].feature, trackers_[j]->getFeature());
            float spatial_dist = calculateSpatialDistance(detections[i].box, predicted_boxes[j]);

            // Fused matching: If IOU is high OR feature is very similar
            // This handles occlusion where IOU drops but visual identity remains
            // BUT gates on spatial distance to prevent ID swaps
            bool match_gate = (iou >= iou_threshold_) && (spatial_dist < MAX_SPATIAL_DISTANCE);
            if (!match_gate && !detections[i].feature.empty() && !trackers_[j]->getFeature().empty()) {
                if (dist < feature_threshold_ && spatial_dist < MAX_SPATIAL_DISTANCE) match_gate = true;
            }

            if (match_gate) {
                // Score combines spatial overlap, visual similarity, and distance penalty
                float visual_sim = 1.0f - dist;
                // Normalize spatial distance to [0, 1] for scoring
                float spatial_penalty = spatial_dist / MAX_SPATIAL_DISTANCE;
                float score = 0.5f * iou + 0.3f * visual_sim - 0.2f * spatial_penalty;
                matches.push_back({(int)i, (int)j, score});
            }
        }
    }

    // Sort matches by combined score descending
    std::sort(matches.begin(), matches.end(), [](const Match& a, const Match& b) {
        return a.score > b.score;
    });

    std::set<int> used_det;
    std::set<int> used_trk;

    for (const auto& m : matches) {
        if (used_det.count(m.det_idx) == 0 && used_trk.count(m.trk_idx) == 0) {
            used_det.insert(m.det_idx);
            used_trk.insert(m.trk_idx);
            trackers_[m.trk_idx]->update(detections[m.det_idx].box, detections[m.det_idx].feature);
        }
    }

    // 3. Create new trackers for unmatched detections
    for (size_t i = 0; i < detections.size(); ++i) {
        if (used_det.count(i) == 0) {
            trackers_.push_back(std::make_unique<KalmanBoxTracker>(detections[i].box, next_id_++));
            // Initialize with the first feature
            trackers_.back()->update(detections[i].box, detections[i].feature);
        }
    }

    // 4. Prune old trackers (with stationary persistence support)
    const int MAX_STATIONARY_AGE_LOCAL = 10;  // Replicate private constant
    for (auto it = trackers_.begin(); it != trackers_.end(); ) {
        if ((*it)->getHitStreak() < min_hits_) {
            it = trackers_.erase(it);
        } else if ((*it)->getTimeSinceUpdate() > max_age_) {
            // Allow stationary objects to persist beyond max_age
            int effective_max_age = max_age_;
            if ((*it)->isStationary()) {
                effective_max_age = std::max(max_age_, (*it)->getTimeSinceUpdate() + MAX_STATIONARY_AGE_LOCAL);
            }
            
            if ((*it)->getTimeSinceUpdate() > effective_max_age) {
                it = trackers_.erase(it);
            } else {
                it++;
            }
        } else {
            it++;
        }
    }

    // 5. Final pass to assign IDs to input detections
    std::vector<Detection> tracked_detections = detections;
    // Reset IDs
    for(auto& d : tracked_detections) d.track_id = -1;

    for (size_t i = 0; i < tracked_detections.size(); ++i) {
        float max_iou = -1.0f;
        int best_id = -1;
        for (const auto& trk : trackers_) {
            float iou = calculateIou(tracked_detections[i].box, trk->getState());
            if (iou > iou_threshold_ && iou > max_iou) {
                max_iou = iou;
                best_id = trk->getId();
            }
        }
        tracked_detections[i].track_id = best_id;
    }

    return tracked_detections;
}

float SortTracker::calculateIou(cv::Rect2f bbTest, cv::Rect2f bbGt) const {
    float in = (bbTest & bbGt).area();
    float un = bbTest.area() + bbGt.area() - in;
    if (un <= 0) return 0;
    return in / un;
}

float SortTracker::calculateFeatureDist(const cv::Mat& f1, const cv::Mat& f2) const {
    if (f1.empty() || f2.empty()) return 1.0f; // Max distance
    // Using Cosine Distance (1.0 - Cosine Similarity)
    // Since we normalize vectors in KalmanBoxTracker, Dot Product = Similarity
    double dot = f1.dot(f2);
    return 1.0f - static_cast<float>(dot);
}

float SortTracker::calculateSpatialDistance(cv::Rect2f det, cv::Rect2f pred) const {
    cv::Point2f det_center(det.x + det.width / 2.0f, det.y + det.height / 2.0f);
    cv::Point2f pred_center(pred.x + pred.width / 2.0f, pred.y + pred.height / 2.0f);
    return cv::norm(det_center - pred_center);
}
