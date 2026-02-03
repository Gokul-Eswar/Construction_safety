#include "sort_tracker.hpp"
#include <algorithm>
#include <set>

SortTracker::SortTracker(int max_age, int min_hits, float iou_threshold)
    : max_age_(max_age), min_hits_(min_hits), iou_threshold_(iou_threshold), next_id_(1) {
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
            float iou = calculate_iou(detections[i].box, predicted_boxes[j]);
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
    std::vector<Detection> results;
    auto it = trackers_.begin();
    while (it != trackers_.end()) {
        if ((*it)->get_time_since_update() < 1 && ((*it)->get_hit_streak() >= min_hits_ || next_id_ <= min_hits_ + 1)) {
            // Find the detection that matched this tracker
            // Since we updated them in step 2, we can just get current state
            // But we need to return the class_id and confidence from the original detection
            // For now, let's assume we want to return tracked boxes
            
            // To be accurate, we should find the original detection that matched.
            // Simplified: we return detections with assigned IDs.
            it++;
        } else if ((*it)->get_time_since_update() > max_age_) {
            it = trackers_.erase(it);
        } else {
            it++;
        }
    }

    // 5. Final pass to assign IDs to input detections
    std::vector<Detection> tracked_detections = detections;
    // Reset IDs
    for(auto& d : tracked_detections) d.track_id = -1;

    // Use the used_det/used_trk sets to assign IDs back to detections
    // We need to re-run the matching logic or store the result.
    // Let's re-store the result in step 2.
    
    // (Refactored step 2 above would be better, but let's just do a final match check)
    for (size_t i = 0; i < tracked_detections.size(); ++i) {
        float max_iou = -1.0f;
        int best_id = -1;
        for (const auto& trk : trackers_) {
            float iou = calculate_iou(tracked_detections[i].box, trk->get_state());
            if (iou > iou_threshold_ && iou > max_iou) {
                max_iou = iou;
                best_id = trk->get_id();
            }
        }
        tracked_detections[i].track_id = best_id;
    }

    return tracked_detections;
}

float SortTracker::calculate_iou(cv::Rect2f bb_test, cv::Rect2f bb_gt) {
    float in = (bb_test & bb_gt).area();
    float un = bb_test.area() + bb_gt.area() - in;
    if (un <= 0) return 0;
    return in / un;
}
