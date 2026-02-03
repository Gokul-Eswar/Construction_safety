#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <sqlite3.h>

namespace safety {

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
     * @return true if successful, false otherwise.
     */
    bool init(const std::string& db_path);

    /**
     * @brief Log a violation event to the database.
     * @param zone_id ID of the zone where violation occurred.
     * @param confidence Detection confidence (0.0 - 1.0).
     * @param object_id Optional ID of the detected object (if tracking enabled).
     * @return true if logged successfully, false otherwise.
     */
    bool log_violation(int zone_id, float confidence, int object_id = -1);

private:
    sqlite3* db_ = nullptr;
    std::string db_path_;
    std::mutex db_mutex_;

    bool create_tables_if_not_exist();
};

} // namespace safety
