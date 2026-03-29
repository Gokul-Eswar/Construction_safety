#pragma once

#include <unordered_map>
#include <chrono>
#include <mutex>

namespace safety {

struct AlertKey {
    int zone_id;
    int object_id;

    bool operator==(const AlertKey& other) const {
        return zone_id == other.zone_id && object_id == other.object_id;
    }
};

struct AlertKeyHash {
    std::size_t operator()(const AlertKey& k) const {
        return std::hash<int>()(k.zone_id) ^ (std::hash<int>()(k.object_id) << 1);
    }
};

struct AlertState {
    std::chrono::steady_clock::time_point last_alert_time;
    int consecutive_violations = 0;
    bool currently_violating = false;
};

class AlertThrottler {
public:
    AlertThrottler();
    
    /**
     * @brief Configure the cooldown period and bouncing detection.
     * @param cooldown_ms Cooldown time in milliseconds.
     * @param min_consecutive_frames Minimum consecutive frames needed to trigger alert.
     */
    void set_cooldown(int cooldown_ms);
    void set_min_consecutive_frames(int min_frames);

    /**
     * @brief Check if an alert should be sent for the given object in the zone.
     *        Requires consecutive violations before alerting to prevent bouncing.
     * @param zone_id ID of the zone.
     * @param object_id ID of the object (or -1 if tracking not used).
     * @param is_violating Whether the object is currently violating the zone.
     * @return true if alert should be sent, false if suppressed.
     */
    bool should_alert(int zone_id, int object_id, bool is_violating);

private:
    int cooldown_ms_ = 5000; // Default 5 seconds
    int min_consecutive_frames_ = 3; // Require 3 consecutive frames to avoid bouncing
    std::unordered_map<AlertKey, AlertState, AlertKeyHash> alert_states_;
    std::mutex mutex_;
};

} // namespace safety
