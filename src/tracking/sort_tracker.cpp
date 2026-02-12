#include "sort_tracker.hpp"
#include <algorithm>
#include <set>

SortTracker::SortTracker(int maxAge, int minHits, float iouThreshold)
    : max_age_(maxAge), min_hits_(minHits), iou_threshold_(iouThreshold), next_id_(1) {
}

std::vector<Detection> SortTracker::update(const std::vector<Detection>& detections) {
    // 1. Predict all current trackers
    std::vector<cv::Rect2f> predicted_boxes;
    for (auto& trk : trackers_) {
        predicted_boxes.push_back(trk->predict());
    }

    // 2. Match detections to predicted boxes (Greedy IOU matching)
    std::vector<int> matched_indices_det;
    std::vector<int> matched_indices_trk;
    
    // Create list of all possible matches with their IOU
    struct Match { int det_idx; int trk_idx; float iou; };
    std::vector<Match> matches;

    for (size_t i = 0; i < detections.size(); ++i) {
        for (size_t j = 0; j < trackers_.size(); ++j) {
            float iou = calculateIou(detections[i].box, predicted_boxes[j]);
            if (iou >= iou_threshold_) {
                matches.push_back({(int)i, (int)j, iou});
            }
        }
    }

    // Sort matches by IOU descending
    std::sort(matches.begin(), matches.end(), [](const Match& a, const Match& b) {
        return a.iou > b.iou;
    });

    std::set<int> used_det;
    std::set<int> used_trk;
    
    for (const auto& m : matches) {
        if (used_det.count(m.det_idx) == 0 && used_trk.count(m.trk_idx) == 0) {
            used_det.insert(m.det_idx);
            used_trk.insert(m.trk_idx);
            trackers_[m.trk_idx]->update(detections[m.det_idx].box);
        }
    }

    // 3. Create new trackers for unmatched detections
    for (size_t i = 0; i < detections.size(); ++i) {
        if (used_det.count(i) == 0) {
            trackers_.push_back(std::make_unique<KalmanBoxTracker>(detections[i].box, next_id_++));
        }
    }

    // 4. Output results and remove dead trackers
    auto it = trackers_.begin();
    while (it != trackers_.end()) {
        if ((*it)->getTimeSinceUpdate() < 1 && ((*it)->getHitStreak() >= min_hits_ || next_id_ <= min_hits_ + 1)) {
            it++;
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
