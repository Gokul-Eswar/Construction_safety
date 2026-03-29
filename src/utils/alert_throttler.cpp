#include "alert_throttler.hpp"

namespace safety {

AlertThrottler::AlertThrottler() {}

void AlertThrottler::set_cooldown(int cooldown_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    cooldown_ms_ = cooldown_ms;
}

void AlertThrottler::set_min_consecutive_frames(int min_frames) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_consecutive_frames_ = min_frames;
}

bool AlertThrottler::should_alert(int zone_id, int object_id, bool is_violating) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    AlertKey key{zone_id, object_id};
    auto now = std::chrono::steady_clock::now();
    
    auto it = alert_states_.find(key);
    if (it == alert_states_.end()) {
        // First time seeing this key
        alert_states_[key] = {now, is_violating ? 1 : 0, is_violating};
        return false; // Don't alert on first detection
    }

    AlertState& state = it->second;
    
    if (is_violating) {
        if (state.currently_violating) {
            // Continuing violation - increment counter
            state.consecutive_violations++;
            
            // Check if we've met the threshold and cooldown has expired
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - state.last_alert_time).count();
            if (state.consecutive_violations >= min_consecutive_frames_ && elapsed > cooldown_ms_) {
                state.last_alert_time = now;
                return true;
            }
        } else {
            // New violation started
            state.currently_violating = true;
            state.consecutive_violations = 1;
        }
    } else {
        // No longer violating
        if (state.currently_violating) {
            state.currently_violating = false;
            state.consecutive_violations = 0;
        }
    }

    return false;
}

} // namespace safety
