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

bool ViolationLogger::init(const std::string& db_path) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    db_path_ = db_path;

    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    if (!create_tables_if_not_exist()) return false;
    
    // Migration: Attempt to add uploaded column if it doesn't exist
    const char* alter_sql = "ALTER TABLE violations ADD COLUMN uploaded INTEGER DEFAULT 0;";
    char* zErrMsg = 0;
    sqlite3_exec(db_, alter_sql, 0, 0, &zErrMsg);
    if (zErrMsg) sqlite3_free(zErrMsg); // Ignore error if column exists

    // Cleanup old logs
    cleanup_old_logs(30);
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
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

bool ViolationLogger::log_violation(int zone_id, float confidence, int object_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_) return false;

    // Get current time in ISO 8601 format
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    std::string timestamp = ss.str();

    std::stringstream sql;
    sql << "INSERT INTO violations (timestamp, zone_id, confidence, object_id, uploaded) VALUES ('"
        << timestamp << "', "
        << zone_id << ", "
        << confidence << ", "
        << object_id << ", 0);";

    std::string query = sql.str();
    char* zErrMsg = 0;
    int rc = sqlite3_exec(db_, query.c_str(), 0, 0, &zErrMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
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
        std::cerr << "DB Cleanup Error: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    } else {
        std::cout << "Database cleanup completed. Logs older than " << days << " days removed." << std::endl;
    }
}

std::vector<ViolationRecord> ViolationLogger::get_pending_uploads(int limit) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<ViolationRecord> records;
    if (!db_) return records;

    std::stringstream sql;
    sql << "SELECT id, timestamp, zone_id, confidence, object_id FROM violations WHERE uploaded = 0 LIMIT " << limit << ";";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, 0) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            ViolationRecord rec;
            rec.id = sqlite3_column_int(stmt, 0);
            rec.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            rec.zone_id = sqlite3_column_int(stmt, 2);
            rec.confidence = static_cast<float>(sqlite3_column_double(stmt, 3));
            rec.object_id = sqlite3_column_int(stmt, 4);
            records.push_back(rec);
        }
        sqlite3_finalize(stmt);
    } else {
        std::cerr << "Failed to query pending uploads." << std::endl;
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
        std::cerr << "Failed to update uploaded status: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    }
}

} // namespace safety
