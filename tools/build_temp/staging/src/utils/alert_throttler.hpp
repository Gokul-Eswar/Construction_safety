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

class AlertThrottler {
public:
    AlertThrottler();
    
    /**
     * @brief Configure the cooldown period.
     * @param cooldown_ms Cooldown time in milliseconds.
     */
    void set_cooldown(int cooldown_ms);

    /**
     * @brief Check if an alert should be sent for the given object in the zone.
     *        Updates the last alert time if it returns true.
     * @param zone_id ID of the zone.
     * @param object_id ID of the object (or -1 if tracking not used).
     * @return true if alert should be sent, false if suppressed.
     */
    bool should_alert(int zone_id, int object_id);

private:
    int cooldown_ms_ = 5000; // Default 5 seconds
    std::unordered_map<AlertKey, std::chrono::steady_clock::time_point, AlertKeyHash> last_alert_times_;
    std::mutex mutex_;
};

} // namespace safety
