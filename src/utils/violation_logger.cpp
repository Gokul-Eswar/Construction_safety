#include "violation_logger.hpp"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace safety {

ViolationLogger::ViolationLogger() : db_(nullptr) {}

ViolationLogger::~ViolationLogger() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool ViolationLogger::init(const std::string& db_path, int retention_days) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    db_path_ = db_path;

    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    // Enable WAL mode for concurrent write reliability
    char* zErrMsg = 0;
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", 0, 0, &zErrMsg);
    if (zErrMsg) {
        std::cerr << "Failed to set WAL mode: " << zErrMsg << "\n";
        sqlite3_free(zErrMsg);
        zErrMsg = 0; 
    }

    // Recommended for WAL: synchronous=NORMAL (good balance of safety vs speed)
    sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", 0, 0, &zErrMsg);
    if (zErrMsg) {
        std::cerr << "Failed to set synchronous mode: " << zErrMsg << "\n";
        sqlite3_free(zErrMsg);
        zErrMsg = 0;
    }

    if (!create_tables_if_not_exist()) return false;
    
    // Migration: Attempt to add uploaded column if it doesn't exist
    const char* alter_sql = "ALTER TABLE violations ADD COLUMN uploaded INTEGER DEFAULT 0;";
    sqlite3_exec(db_, alter_sql, 0, 0, &zErrMsg);
    if (zErrMsg) sqlite3_free(zErrMsg); // Ignore error if column exists

    // Migration: Add spatial analytics columns (idempotent - ignore error if exists)
    const char* spatial_migrations[] = {
        "ALTER TABLE violations ADD COLUMN detection_box_x REAL DEFAULT 0.0;",
        "ALTER TABLE violations ADD COLUMN detection_box_y REAL DEFAULT 0.0;",
        "ALTER TABLE violations ADD COLUMN detection_box_w REAL DEFAULT 0.0;",
        "ALTER TABLE violations ADD COLUMN detection_box_h REAL DEFAULT 0.0;",
        "ALTER TABLE violations ADD COLUMN world_coord_x REAL DEFAULT 0.0;",
        "ALTER TABLE violations ADD COLUMN world_coord_y REAL DEFAULT 0.0;",
        "ALTER TABLE violations ADD COLUMN camera_id TEXT DEFAULT '';"
    };
    
    for (const auto& migration : spatial_migrations) {
        sqlite3_exec(db_, migration, 0, 0, &zErrMsg);
        if (zErrMsg) sqlite3_free(zErrMsg); // Ignore error if column exists
    }

    // Cleanup old logs
    cleanup_old_logs(retention_days);
    return true;
}

bool ViolationLogger::create_tables_if_not_exist() {
    const char* sql = "CREATE TABLE IF NOT EXISTS violations ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "timestamp TEXT NOT NULL,"
                      "zone_id INTEGER NOT NULL,"
                      "confidence REAL NOT NULL,"
                      "object_id INTEGER,"
                      "is_active INTEGER DEFAULT 1,"
                      "uploaded INTEGER DEFAULT 0"
                      ");";

    char* zErrMsg = 0;
    int rc = sqlite3_exec(db_, sql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << "\n";
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

bool ViolationLogger::log_violation(int zone_id, float confidence, int object_id,
                                     const std::array<float, 4>& detection_box,
                                     const std::array<float, 2>& world_coords,
                                     const std::string& camera_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_) return false;

    // Get current time in ISO 8601 format
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    std::string timestamp = ss.str();

    // --- CORNER CASE: Database Reliability (Prepared Statements) ---
    const char* sql = "INSERT INTO violations ("
                      "timestamp, zone_id, confidence, object_id, uploaded, "
                      "detection_box_x, detection_box_y, detection_box_w, detection_box_h, "
                      "world_coord_x, world_coord_y, camera_id) "
                      "VALUES (?, ?, ?, ?, 0, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare violation insert: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, zone_id);
    sqlite3_bind_double(stmt, 3, static_cast<double>(confidence));
    sqlite3_bind_int(stmt, 4, object_id);
    sqlite3_bind_double(stmt, 5, static_cast<double>(detection_box[0])); // x
    sqlite3_bind_double(stmt, 6, static_cast<double>(detection_box[1])); // y
    sqlite3_bind_double(stmt, 7, static_cast<double>(detection_box[2])); // w
    sqlite3_bind_double(stmt, 8, static_cast<double>(detection_box[3])); // h
    sqlite3_bind_double(stmt, 9, static_cast<double>(world_coords[0]));  // world_x
    sqlite3_bind_double(stmt, 10, static_cast<double>(world_coords[1])); // world_y
    sqlite3_bind_text(stmt, 11, camera_id.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to log violation record: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    return true;
}

void ViolationLogger::cleanup_old_logs(int days) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_) return;

    std::stringstream sql;
    sql << "DELETE FROM violations WHERE timestamp < date('now', '-" << days << " days');";
    
    std::string query = sql.str();
    char* zErrMsg = 0;
    int rc = sqlite3_exec(db_, query.c_str(), 0, 0, &zErrMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "DB Cleanup Error: " << zErrMsg << "\n";
        sqlite3_free(zErrMsg);
    } else {
        std::cout << "Database cleanup completed. Logs older than " << days << " days removed." << "\n";
    }
}

std::vector<ViolationRecord> ViolationLogger::get_pending_uploads(int limit) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<ViolationRecord> records;
    if (!db_) return records;

    std::stringstream sql;
    sql << "SELECT id, timestamp, zone_id, confidence, object_id, "
        << "detection_box_x, detection_box_y, detection_box_w, detection_box_h, "
        << "world_coord_x, world_coord_y, camera_id "
        << "FROM violations WHERE uploaded = 0 LIMIT " << limit << ";";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, 0) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ViolationRecord rec;
            rec.id = sqlite3_column_int(stmt, 0);
            rec.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            rec.zone_id = sqlite3_column_int(stmt, 2);
            rec.confidence = static_cast<float>(sqlite3_column_double(stmt, 3));
            rec.object_id = sqlite3_column_int(stmt, 4);
            rec.detection_box_x = static_cast<float>(sqlite3_column_double(stmt, 5));
            rec.detection_box_y = static_cast<float>(sqlite3_column_double(stmt, 6));
            rec.detection_box_w = static_cast<float>(sqlite3_column_double(stmt, 7));
            rec.detection_box_h = static_cast<float>(sqlite3_column_double(stmt, 8));
            rec.world_coord_x = static_cast<float>(sqlite3_column_double(stmt, 9));
            rec.world_coord_y = static_cast<float>(sqlite3_column_double(stmt, 10));
            rec.camera_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
            records.push_back(rec);
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Failed to query pending uploads." << "\n";
    }
    return records;
}

void ViolationLogger::mark_uploaded(const std::vector<int>& ids) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_ || ids.empty()) return;

    std::stringstream sql;
    sql << "UPDATE violations SET uploaded = 1 WHERE id IN (";
    for (size_t i = 0; i < ids.size(); ++i) {
        sql << ids[i];
        if (i < ids.size() - 1) sql << ",";
    }
    sql << ");";

    char* zErrMsg = 0;
    int rc = sqlite3_exec(db_, sql.str().c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to update uploaded status: " << zErrMsg << "\n";
        sqlite3_free(zErrMsg);
    }
}

} // namespace safety
