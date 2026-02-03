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

    return create_tables_if_not_exist();
}

bool ViolationLogger::create_tables_if_not_exist() {
    const char* sql = "CREATE TABLE IF NOT EXISTS violations ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "timestamp TEXT NOT NULL,"
                      "zone_id INTEGER NOT NULL,"
                      "confidence REAL NOT NULL,"
                      "object_id INTEGER,"
                      "is_active INTEGER DEFAULT 1"
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
    sql << "INSERT INTO violations (timestamp, zone_id, confidence, object_id) VALUES ('"
        << timestamp << "', "
        << zone_id << ", "
        << confidence << ", "
        << object_id << ");";

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

} // namespace safety
