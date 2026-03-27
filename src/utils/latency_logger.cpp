#include "latency_logger.hpp"
#include <numeric>
#include <iomanip>

#ifdef ENABLE_NVML
#include <nvml.h>
#endif

LatencyLogger::LatencyLogger() {
    initNVML();
}

LatencyLogger::~LatencyLogger() {
#ifdef ENABLE_NVML
    if (nvml_initialized_) {
        nvmlShutdown();
    }
#endif
}

void LatencyLogger::initNVML() {
#ifdef ENABLE_NVML
    nvmlReturn_t result = nvmlInit();
    if (NVML_SUCCESS != result) {
        std::cerr << "Failed to initialize NVML: " << nvmlErrorString(result) << std::endl;
        nvml_initialized_ = false;
        return;
    }

    nvmlDevice_t device;
    result = nvmlDeviceGetHandleByIndex(0, &device);
    if (NVML_SUCCESS != result) {
        std::cerr << "Failed to get handle for GPU 0: " << nvmlErrorString(result) << std::endl;
        nvmlShutdown();
        nvml_initialized_ = false;
        return;
    }

    nvml_device_ = static_cast<void*>(device);
    nvml_initialized_ = true;
#else
    nvml_initialized_ = false;
#endif
}

GPUStats LatencyLogger::getGPUStats() {
    GPUStats stats = {0, 0, 0, 0};
#ifdef ENABLE_NVML
    if (!nvml_initialized_) return stats;

    nvmlDevice_t device = static_cast<nvmlDevice_t>(nvml_device_);
    
    // Utilization
    nvmlUtilization_t utilization;
    if (nvmlDeviceGetUtilizationRates(device, &utilization) == NVML_SUCCESS) {
        stats.utilization = utilization.gpu;
    }

    // Temperature
    uint32_t temp;
    if (nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp) == NVML_SUCCESS) {
        stats.temperature = temp;
    }

    // Memory
    nvmlMemory_t memory;
    if (nvmlDeviceGetMemoryInfo(device, &memory) == NVML_SUCCESS) {
        stats.memory_used = memory.used;
        stats.memory_total = memory.total;
    }
#endif
    return stats;
}

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

std::unordered_map<std::string, LatencyStats> LatencyLogger::getAndClearStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, LatencyStats> results;
    
    for (auto& [key, latencies] : stats_) {
        if (!latencies.empty()) {
            // Sort to calculate percentiles and min/max
            std::sort(latencies.begin(), latencies.end());
            
            double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
            double avg = sum / latencies.size();
            double min = latencies.front();
            double max = latencies.back();
            
            // P99 Calculation
            size_t p99_idx = static_cast<size_t>(std::ceil(0.99 * latencies.size())) - 1;
            if (p99_idx >= latencies.size()) p99_idx = latencies.size() - 1;
            double p99 = latencies[p99_idx];

            results[key] = {avg, min, max, p99};
            latencies.clear();
        }
    }
    return results;
}
