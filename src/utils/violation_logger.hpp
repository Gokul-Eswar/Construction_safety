#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <sqlite3.h>
#include <vector>
#include <array>
#include <deque>
#include <thread>
#include <condition_variable>
#include <atomic>

namespace safety {

struct ViolationRecord {
    int id;
    std::string timestamp;
    int zone_id;
    float confidence;
    int object_id;
    // Spatial coordinates added for analytics
    float detection_box_x = 0.0f;   // bounding box x in image coords
    float detection_box_y = 0.0f;   // bounding box y in image coords
    float detection_box_w = 0.0f;   // bounding box width in image coords
    float detection_box_h = 0.0f;   // bounding box height in image coords
    float world_coord_x = 0.0f;     // world coordinate x in meters
    float world_coord_y = 0.0f;     // world coordinate y in meters
    std::string camera_id;          // camera/stream identifier
};

struct ViolationQueueMetrics {
    uint64_t enqueued = 0;
    uint64_t written = 0;
    uint64_t dropped_oldest = 0;
    uint64_t coalesced_duplicates = 0;
    uint64_t write_retries = 0;
    uint64_t write_failures = 0;
    uint64_t queue_overflow_events = 0;
    size_t queue_depth = 0;
    size_t max_queue_depth = 0;
};

class ViolationLogger {
public:
    ViolationLogger();
    ~ViolationLogger();

    // Prevent copying
    ViolationLogger(const ViolationLogger&) = delete;
    ViolationLogger& operator=(const ViolationLogger&) = delete;

    /**
     * @brief Initialize the logger with the database path.
     * @param db_path Path to the SQLite database file.
     * @param retention_days Number of days to keep logs.
     * @return true if successful, false otherwise.
     */
    bool init(const std::string& db_path, int retention_days = 30);

    /**
     * @brief Log a violation event to the database.
     * @param zone_id ID of the zone where violation occurred.
     * @param confidence Detection confidence (0.0 - 1.0).
     * @param object_id Optional ID of the detected object (if tracking enabled).
     * @param detection_box Bounding box in image coordinates (x, y, w, h).
     * @param world_coords World coordinates in meters (x, y).
     * @param camera_id Camera/stream identifier.
     * @return true if logged successfully, false otherwise.
     */
    bool log_violation(int zone_id, float confidence, int object_id = -1,
                      const std::array<float, 4>& detection_box = {0.0f, 0.0f, 0.0f, 0.0f},
                      const std::array<float, 2>& world_coords = {0.0f, 0.0f},
                      const std::string& camera_id = "");

    /**
     * @brief Delete logs older than the specified number of days.
     * @param days Number of days to keep.
     */
    void cleanup_old_logs(int days = 30);
    
    std::vector<ViolationRecord> get_pending_uploads(int limit = 10);
    void mark_uploaded(const std::vector<int>& ids);

    // Async writer queue controls
    bool flush(uint32_t timeout_ms = 5000);
    ViolationQueueMetrics get_metrics() const;

    // Test/runtime tuning hooks
    void set_queue_limits(size_t max_queue_size, size_t batch_size);
    void set_retry_policy(int max_retries, int retry_delay_ms, int busy_timeout_ms);

private:
    struct QueuedViolation {
        int zone_id = 0;
        float confidence = 0.0f;
        int object_id = -1;
        std::array<float, 4> detection_box = {0.0f, 0.0f, 0.0f, 0.0f};
        std::array<float, 2> world_coords = {0.0f, 0.0f};
        std::string camera_id;
        std::string timestamp;
    };

    sqlite3* db_ = nullptr;
    std::string db_path_;
    std::mutex db_mutex_;

    std::deque<QueuedViolation> write_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::condition_variable flush_cv_;
    std::thread writer_thread_;
    std::atomic<bool> writer_running_{false};
    bool writer_busy_ = false;

    size_t max_queue_size_ = 10000;
    size_t batch_size_ = 64;
    int max_retries_ = 5;
    int retry_delay_ms_ = 20;
    int busy_timeout_ms_ = 2000;

    std::atomic<uint64_t> metric_enqueued_{0};
    std::atomic<uint64_t> metric_written_{0};
    std::atomic<uint64_t> metric_dropped_oldest_{0};
    std::atomic<uint64_t> metric_coalesced_duplicates_{0};
    std::atomic<uint64_t> metric_write_retries_{0};
    std::atomic<uint64_t> metric_write_failures_{0};
    std::atomic<uint64_t> metric_queue_overflow_events_{0};
    std::atomic<size_t> metric_max_queue_depth_{0};

    bool create_tables_if_not_exist();
    void writer_loop();
    bool write_batch_with_retry(const std::vector<QueuedViolation>& batch);
    static std::string make_timestamp();
};

} // namespace safety
