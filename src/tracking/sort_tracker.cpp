#include "sort_tracker.hpp"
#include <algorithm>
#include <set>
SortTracker::SortTracker(int maxAge, int minHits, float iouThreshold, float featureThreshold)
    : max_age_(maxAge), min_hits_(minHits), iou_threshold_(iouThreshold), feature_threshold_(featureThreshold), next_id_(1) {
}

std::vector<Detection> SortTracker::update(const std::vector<Detection>& detections) {
    // 1. Predict all current trackers
    std::vector<cv::Rect2f> predicted_boxes;
    for (auto& trk : trackers_) {
        predicted_boxes.push_back(trk->predict());
    }

    // 2. Match detections to predicted boxes (Fused Score matching)
    std::vector<int> matched_indices_det;
    std::vector<int> matched_indices_trk;

    struct Match { int det_idx; int trk_idx; float score; };
    std::vector<Match> matches;

    for (size_t i = 0; i < detections.size(); ++i) {
        for (size_t j = 0; j < trackers_.size(); ++j) {
            float iou = calculateIou(detections[i].box, predicted_boxes[j]);
            float dist = calculateFeatureDist(detections[i].feature, trackers_[j]->getFeature());

            // Fused matching: If IOU is high OR feature is very similar
            // This handles occlusion where IOU drops but visual identity remains
            bool match_gate = (iou >= iou_threshold_);
            if (!match_gate && !detections[i].feature.empty() && !trackers_[j]->getFeature().empty()) {
                if (dist < feature_threshold_) match_gate = true;
            }

            if (match_gate) {
                // Score combines spatial overlap and visual similarity
                float visual_sim = 1.0f - dist;
                float score = 0.6f * iou + 0.4f * visual_sim;
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
...
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
        } else if ((*it)->getTimeSinceUpdate() > max_age_) {
            it = trackers_.erase(it);
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
