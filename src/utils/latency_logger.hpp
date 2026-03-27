#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include <cmath>

struct LatencyStats {
    double avg;
    double min;
    double max;
    double p99;
};

struct GPUStats {
    uint32_t utilization;  // Percentage
    uint32_t temperature;  // Celsius
    uint64_t memory_used;  // Bytes
    uint64_t memory_total; // Bytes
};

class LatencyLogger {
public:
    static LatencyLogger& getInstance() {
        static LatencyLogger instance;
        return instance;
    }

    ~LatencyLogger();

    void startTimer(const std::string& key, uint64_t frame_id);
    void stopTimer(const std::string& key, uint64_t frame_id);
    
    // Logs average latency every 'interval' frames
    void logStats(int interval = 100);

    // Returns detailed stats for all keys and clears the buffer
    std::unordered_map<std::string, LatencyStats> getAndClearStats();

    // Returns current GPU metrics
    GPUStats getGPUStats();

private:
    LatencyLogger();
    void cleanupOldTimers(const std::string& key, uint64_t current_frame_id);
    void initNVML();
    
    struct TimerData {
        std::chrono::high_resolution_clock::time_point start;
    };

    std::mutex mutex_;
    const size_t MAX_STALE_TIMERS = 500;
    std::unordered_map<std::string, std::unordered_map<uint64_t, TimerData>> active_timers_;
    std::unordered_map<std::string, std::vector<double>> stats_;

    bool nvml_initialized_ = false;
    void* nvml_device_ = nullptr;
};
