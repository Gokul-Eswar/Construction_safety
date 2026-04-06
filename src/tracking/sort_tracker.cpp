#include "sort_tracker.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

// ================================================================================
// Constructor with individual parameters (backward compatibility)
// ================================================================================
SortTracker::SortTracker(int maxAge, int minHits, float iouThreshold, float featureThreshold, int occlusionExtensionFrames)
    : max_age_(maxAge),
      min_hits_(minHits),
      iou_threshold_(iouThreshold),
      feature_threshold_(featureThreshold),
      occlusion_extension_frames_(occlusionExtensionFrames),
      dormant_age_(std::max(maxAge + occlusionExtensionFrames, maxAge * 2)),
      reactivation_iou_threshold_(std::max(0.15f, iouThreshold)),
      max_spatial_distance_(150.0f),
      next_id_(1) {
}

// ================================================================================
// Constructor with TrackingConfig (from AppConfig)
// ================================================================================
SortTracker::SortTracker(const TrackingConfig& config)
    : max_age_(config.max_age),
      min_hits_(config.min_hits),
      iou_threshold_(config.iou_threshold),
      feature_threshold_(config.feature_threshold),
      occlusion_extension_frames_(config.occlusion_extension_frames),
      dormant_age_(std::max(config.max_age + config.occlusion_extension_frames, config.max_age * 2)),
      reactivation_iou_threshold_(std::max(0.15f, config.iou_threshold)),
      max_spatial_distance_(150.0f),
      next_id_(1) {
}

// ================================================================================
// Occlusion Detection Heuristic
// ================================================================================
bool SortTracker::isLikelyOccluded(const cv::Rect2f& prev_box, const cv::Rect2f& current_box) const {
    float prev_area = prev_box.area();
    float curr_area = current_box.area();

    if (prev_area < 1.0f) {
        return false;
    }

    float area_ratio = curr_area / prev_area;

    cv::Point2f prev_center(prev_box.x + prev_box.width / 2.0f, prev_box.y + prev_box.height / 2.0f);
    cv::Point2f curr_center(current_box.x + current_box.width / 2.0f, current_box.y + current_box.height / 2.0f);
    float center_distance = cv::norm(prev_center - curr_center);

    return (area_ratio < 0.5f) && (center_distance < 30.0f);
}

std::vector<int> SortTracker::solveHungarian(const std::vector<std::vector<float>>& cost_matrix) const {
    if (cost_matrix.empty() || cost_matrix[0].empty()) {
        return {};
    }

    const int rows = static_cast<int>(cost_matrix.size());
    const int cols = static_cast<int>(cost_matrix[0].size());
    const int size = std::max(rows, cols);
    const float LARGE_COST = 1e6f;

    std::vector<std::vector<float>> a(size + 1, std::vector<float>(size + 1, LARGE_COST));
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            a[i + 1][j + 1] = cost_matrix[i][j];
        }
    }

    std::vector<float> u(size + 1, 0.0f), v(size + 1, 0.0f);
    std::vector<int> p(size + 1, 0), way(size + 1, 0);

    for (int i = 1; i <= size; ++i) {
        p[0] = i;
        int j0 = 0;
        std::vector<float> minv(size + 1, std::numeric_limits<float>::max());
        std::vector<bool> used(size + 1, false);

        do {
            used[j0] = true;
            int i0 = p[j0];
            float delta = std::numeric_limits<float>::max();
            int j1 = 0;
            for (int j = 1; j <= size; ++j) {
                if (used[j]) {
                    continue;
                }
                float cur = a[i0][j] - u[i0] - v[j];
                if (cur < minv[j]) {
                    minv[j] = cur;
                    way[j] = j0;
                }
                if (minv[j] < delta) {
                    delta = minv[j];
                    j1 = j;
                }
            }
            for (int j = 0; j <= size; ++j) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }
            j0 = j1;
        } while (p[j0] != 0);

        do {
            int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0 != 0);
    }

    std::vector<int> assignment(rows, -1);
    for (int j = 1; j <= size; ++j) {
        if (p[j] >= 1 && p[j] <= rows && j <= cols) {
            assignment[p[j] - 1] = j - 1;
        }
    }

    return assignment;
}

SortTracker::AssociationResult SortTracker::associateDetectionsToTrackers(
    const std::vector<Detection>& detections,
    const std::vector<cv::Rect2f>& predicted_boxes,
    const std::vector<int>& candidate_tracker_indices,
    float min_iou_gate,
    bool allow_appearance_gate) const {
    AssociationResult result;

    if (detections.empty()) {
        result.unmatched_trackers = candidate_tracker_indices;
        return result;
    }

    if (candidate_tracker_indices.empty()) {
        result.unmatched_detections.resize(detections.size());
        for (size_t i = 0; i < detections.size(); ++i) {
            result.unmatched_detections[i] = static_cast<int>(i);
        }
        return result;
    }

    const float INF_COST = 1e5f;
    std::vector<std::vector<float>> cost(detections.size(), std::vector<float>(candidate_tracker_indices.size(), INF_COST));

    for (size_t det_idx = 0; det_idx < detections.size(); ++det_idx) {
        for (size_t local_trk_idx = 0; local_trk_idx < candidate_tracker_indices.size(); ++local_trk_idx) {
            int trk_idx = candidate_tracker_indices[local_trk_idx];

            float iou = calculateIou(detections[det_idx].box, predicted_boxes[trk_idx]);
            float spatial_dist = calculateSpatialDistance(detections[det_idx].box, predicted_boxes[trk_idx]);

            bool pass_spatial_gate = spatial_dist <= max_spatial_distance_;
            bool pass_iou_gate = iou >= min_iou_gate;

            float feature_dist = 1.0f;
            bool has_features = !detections[det_idx].feature.empty() && !trackers_[trk_idx]->getFeature().empty();
            bool pass_appearance_gate = false;
            if (allow_appearance_gate && has_features) {
                feature_dist = calculateFeatureDist(detections[det_idx].feature, trackers_[trk_idx]->getFeature());
                pass_appearance_gate = feature_dist < feature_threshold_;
            }

            if (pass_spatial_gate && (pass_iou_gate || pass_appearance_gate)) {
                float visual_similarity = has_features ? (1.0f - feature_dist) : 0.0f;
                float spatial_penalty = spatial_dist / std::max(1.0f, max_spatial_distance_);
                float score = 0.55f * iou + 0.30f * visual_similarity - 0.15f * spatial_penalty;
                score = std::max(-1.0f, std::min(1.0f, score));
                cost[det_idx][local_trk_idx] = 1.0f - score;
            }
        }
    }

    std::vector<int> assignment = solveHungarian(cost);
    std::set<int> matched_trk_local;

    for (size_t det_idx = 0; det_idx < assignment.size(); ++det_idx) {
        int local_trk_idx = assignment[det_idx];
        if (local_trk_idx >= 0 && local_trk_idx < static_cast<int>(candidate_tracker_indices.size()) &&
            cost[det_idx][local_trk_idx] < (INF_COST * 0.5f)) {
            matched_trk_local.insert(local_trk_idx);
            result.matches.emplace_back(static_cast<int>(det_idx), candidate_tracker_indices[local_trk_idx]);
        } else {
            result.unmatched_detections.push_back(static_cast<int>(det_idx));
        }
    }

    for (size_t local_trk_idx = 0; local_trk_idx < candidate_tracker_indices.size(); ++local_trk_idx) {
        if (matched_trk_local.find(static_cast<int>(local_trk_idx)) == matched_trk_local.end()) {
            result.unmatched_trackers.push_back(candidate_tracker_indices[local_trk_idx]);
        }
    }

    return result;
}

std::vector<Detection> SortTracker::update(const std::vector<Detection>& detections) {
    std::vector<Detection> tracked_detections = detections;
    for (auto& d : tracked_detections) {
        d.track_id = -1;
    }

    std::vector<cv::Rect2f> predicted_boxes;
    predicted_boxes.reserve(trackers_.size());
    for (auto& trk : trackers_) {
        predicted_boxes.push_back(trk->predict());
    }

    std::vector<int> active_tracker_indices;
    std::vector<int> dormant_tracker_indices;
    for (size_t i = 0; i < trackers_.size(); ++i) {
        if (trackers_[i]->getLifecycleState() == KalmanBoxTracker::LifecycleState::Dormant) {
            dormant_tracker_indices.push_back(static_cast<int>(i));
        } else {
            active_tracker_indices.push_back(static_cast<int>(i));
        }
    }

    AssociationResult primary_assoc = associateDetectionsToTrackers(
        detections,
        predicted_boxes,
        active_tracker_indices,
        iou_threshold_,
        true);

    std::vector<bool> matched_detection(detections.size(), false);
    std::vector<bool> matched_tracker(trackers_.size(), false);

    for (const auto& match : primary_assoc.matches) {
        int det_idx = match.first;
        int trk_idx = match.second;
        trackers_[trk_idx]->update(detections[det_idx].box, detections[det_idx].feature);
        if (trackers_[trk_idx]->getHitStreak() >= min_hits_) {
            trackers_[trk_idx]->markConfirmed();
        }

        occlusion_frames_[trk_idx] = 0;
        previous_boxes_[trk_idx] = detections[det_idx].box;

        matched_detection[det_idx] = true;
        matched_tracker[trk_idx] = true;
        tracked_detections[det_idx].track_id = trackers_[trk_idx]->getId();
    }

    if (!primary_assoc.unmatched_detections.empty() && !dormant_tracker_indices.empty()) {
        std::vector<Detection> unmatched_dets;
        std::vector<int> unmatched_det_to_global;
        unmatched_dets.reserve(primary_assoc.unmatched_detections.size());
        unmatched_det_to_global.reserve(primary_assoc.unmatched_detections.size());

        for (int det_idx : primary_assoc.unmatched_detections) {
            unmatched_dets.push_back(detections[det_idx]);
            unmatched_det_to_global.push_back(det_idx);
        }

        AssociationResult reactivate_assoc = associateDetectionsToTrackers(
            unmatched_dets,
            predicted_boxes,
            dormant_tracker_indices,
            reactivation_iou_threshold_,
            true);

        for (const auto& match : reactivate_assoc.matches) {
            int local_det_idx = match.first;
            int trk_idx = match.second;
            int det_idx = unmatched_det_to_global[local_det_idx];

            trackers_[trk_idx]->update(detections[det_idx].box, detections[det_idx].feature);
            trackers_[trk_idx]->markConfirmed();

            occlusion_frames_[trk_idx] = 0;
            previous_boxes_[trk_idx] = detections[det_idx].box;

            matched_detection[det_idx] = true;
            matched_tracker[trk_idx] = true;
            tracked_detections[det_idx].track_id = trackers_[trk_idx]->getId();
        }
    }

    for (size_t det_idx = 0; det_idx < detections.size(); ++det_idx) {
        if (!matched_detection[det_idx]) {
            trackers_.push_back(std::make_unique<KalmanBoxTracker>(detections[det_idx].box, next_id_++));
            occlusion_frames_.push_back(0);
            previous_boxes_.push_back(detections[det_idx].box);
            trackers_.back()->update(detections[det_idx].box, detections[det_idx].feature);

            if (trackers_.back()->getHitStreak() >= min_hits_) {
                trackers_.back()->markConfirmed();
            } else {
                trackers_.back()->markTentative();
            }

            tracked_detections[det_idx].track_id = trackers_.back()->getId();
        }
    }

    const int MAX_STATIONARY_AGE_LOCAL = 10;
    for (size_t idx = 0; idx < trackers_.size();) {
        auto& trk = trackers_[idx];
        int time_since_update = trk->getTimeSinceUpdate();

        bool was_matched_this_frame = (idx < matched_tracker.size()) ? matched_tracker[idx] : true;
        if (!was_matched_this_frame && idx < previous_boxes_.size() && idx < predicted_boxes.size()) {
            if (isLikelyOccluded(previous_boxes_[idx], predicted_boxes[idx])) {
                occlusion_frames_[idx]++;
            } else if (occlusion_frames_[idx] > 0) {
                occlusion_frames_[idx]--;
            }
        }

        bool erase_track = false;
        KalmanBoxTracker::LifecycleState state = trk->getLifecycleState();

        if (state == KalmanBoxTracker::LifecycleState::Tentative) {
            if (trk->getHitStreak() >= min_hits_) {
                trk->markConfirmed();
            } else if (time_since_update > 1) {
                erase_track = true;
            }
        } else if (state == KalmanBoxTracker::LifecycleState::Confirmed) {
            int effective_max_age = max_age_;
            if (occlusion_frames_[idx] > 0) {
                effective_max_age += occlusion_extension_frames_;
            }
            if (trk->isStationary()) {
                effective_max_age = std::max(effective_max_age, max_age_ + MAX_STATIONARY_AGE_LOCAL);
            }
            if (time_since_update > effective_max_age) {
                trk->markDormant();
            }
        } else if (state == KalmanBoxTracker::LifecycleState::Dormant) {
            if (time_since_update > dormant_age_) {
                erase_track = true;
            }
        }

        if (erase_track) {
            trackers_.erase(trackers_.begin() + idx);
            occlusion_frames_.erase(occlusion_frames_.begin() + idx);
            previous_boxes_.erase(previous_boxes_.begin() + idx);
        } else {
            ++idx;
        }
    }

    return tracked_detections;
}

float SortTracker::calculateIou(cv::Rect2f bbTest, cv::Rect2f bbGt) const {
    float in = (bbTest & bbGt).area();
    float un = bbTest.area() + bbGt.area() - in;
    if (un <= 0) {
        return 0;
    }
    return in / un;
}

float SortTracker::calculateFeatureDist(const cv::Mat& f1, const cv::Mat& f2) const {
    if (f1.empty() || f2.empty()) {
        return 1.0f;
    }
    double dot = f1.dot(f2);
    return 1.0f - static_cast<float>(dot);
}

float SortTracker::calculateSpatialDistance(cv::Rect2f det, cv::Rect2f pred) const {
    cv::Point2f det_center(det.x + det.width / 2.0f, det.y + det.height / 2.0f);
    cv::Point2f pred_center(pred.x + pred.width / 2.0f, pred.y + pred.height / 2.0f);
    return cv::norm(det_center - pred_center);
}
