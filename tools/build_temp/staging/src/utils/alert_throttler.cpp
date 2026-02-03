#include "alert_throttler.hpp"

namespace safety {

AlertThrottler::AlertThrottler() {}

void AlertThrottler::set_cooldown(int cooldown_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    cooldown_ms_ = cooldown_ms;
}

bool AlertThrottler::should_alert(int zone_id, int object_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    AlertKey key{zone_id, object_id};
    auto now = std::chrono::steady_clock::now();
    
    auto it = last_alert_times_.find(key);
    if (it == last_alert_times_.end()) {
        // First time alerting for this key
        last_alert_times_[key] = now;
        return true;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
    if (elapsed > cooldown_ms_) {
        // Cooldown expired, alert again
        it->second = now;
        return true;
    }

    // Suppress alert
    return false;
}

} // namespace safety
