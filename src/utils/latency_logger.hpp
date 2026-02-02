#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <iostream>
#include <mutex>
#include <unordered_map>

class LatencyLogger {
public:
    static LatencyLogger& getInstance() {
        static LatencyLogger instance;
        return instance;
    }

    void startTimer(const std::string& key, uint64_t frame_id);
    void stopTimer(const std::string& key, uint64_t frame_id);
    
    // Logs average latency every 'interval' frames
    void logStats(int interval = 100);

private:
    LatencyLogger() = default;
    void cleanupOldTimers(const std::string& key, uint64_t current_frame_id);
    
    struct TimerData {
        std::chrono::high_resolution_clock::time_point start;
    };

    std::mutex mutex_;
    const size_t MAX_STALE_TIMERS = 500;
    std::unordered_map<std::string, std::unordered_map<uint64_t, TimerData>> active_timers_;
    std::unordered_map<std::string, std::vector<double>> stats_;
};
