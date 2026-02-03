#include "latency_logger.hpp"
#include <numeric>
#include <iomanip>

void LatencyLogger::startTimer(const std::string& key, uint64_t frame_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    cleanupOldTimers(key, frame_id);
    active_timers_[key][frame_id] = {std::chrono::high_resolution_clock::now()};
}

void LatencyLogger::cleanupOldTimers(const std::string& key, uint64_t current_frame_id) {
    auto& timers = active_timers_[key];
    if (timers.size() > MAX_STALE_TIMERS) {
        // Remove timers that are significantly older than the current frame ID
        // (Assuming frame IDs are monotonically increasing)
        for (auto it = timers.begin(); it != timers.end(); ) {
            if (it->first < current_frame_id && (current_frame_id - it->first) > MAX_STALE_TIMERS) {
                it = timers.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void LatencyLogger::stopTimer(const std::string& key, uint64_t frame_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& timers = active_timers_[key];
    auto it = timers.find(frame_id);
    if (it != timers.end()) {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - it->second.start;
        stats_[key].push_back(elapsed.count());
        timers.erase(it);
    }
}

void LatencyLogger::logStats(int interval) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [key, latencies] : stats_) {
        if (latencies.size() >= static_cast<size_t>(interval)) {
            double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
            double avg = sum / latencies.size();
            
            std::cout << "[Latency] " << key << ": " 
                      << std::fixed << std::setprecision(2) << avg << " ms (avg over " 
                      << latencies.size() << " samples)" << std::endl;
            
            latencies.clear();
        }
    }
}

std::unordered_map<std::string, double> LatencyLogger::getAndClearStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, double> results;
    
    for (auto& [key, latencies] : stats_) {
        if (!latencies.empty()) {
            double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
            results[key] = sum / latencies.size();
            latencies.clear();
        }
    }
    return results;
}
