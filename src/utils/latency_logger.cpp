#include "latency_logger.hpp"
#include <numeric>
#include <iomanip>

void LatencyLogger::startTimer(const std::string& key, uint64_t frame_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    active_timers_[key][frame_id] = {std::chrono::high_resolution_clock::now()};
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
