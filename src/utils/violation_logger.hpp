#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <sqlite3.h>
#include <vector>
#include <array>

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

private:
    sqlite3* db_ = nullptr;
    std::string db_path_;
    std::mutex db_mutex_;

    bool create_tables_if_not_exist();
};

} // namespace safety
