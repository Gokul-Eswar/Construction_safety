#include "violation_logger.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace safety {

ViolationLogger::ViolationLogger() : db_(nullptr) {}

ViolationLogger::~ViolationLogger() {
    writer_running_ = false;
    queue_cv_.notify_all();
    if (writer_thread_.joinable()) {
        writer_thread_.join();
    }

    std::lock_guard<std::mutex> lock(db_mutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

std::string ViolationLogger::make_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &in_time_t);
#else
    localtime_r(&in_time_t, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void ViolationLogger::set_queue_limits(size_t max_queue_size, size_t batch_size) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    max_queue_size_ = std::max<size_t>(64, max_queue_size);
    batch_size_ = std::max<size_t>(1, batch_size);
}

void ViolationLogger::set_retry_policy(int max_retries, int retry_delay_ms, int busy_timeout_ms) {
    max_retries_ = std::max(0, max_retries);
    retry_delay_ms_ = std::max(1, retry_delay_ms);
    busy_timeout_ms_ = std::max(100, busy_timeout_ms);

    std::lock_guard<std::mutex> lock(db_mutex_);
    if (db_) {
        sqlite3_busy_timeout(db_, busy_timeout_ms_);
    }
}

bool ViolationLogger::init(const std::string& db_path, int retention_days) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    db_path_ = db_path;

    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc) {
        spdlog::error("Can't open database: {}", sqlite3_errmsg(db_));
        return false;
    }

    sqlite3_busy_timeout(db_, busy_timeout_ms_);

    char* zErrMsg = nullptr;
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", 0, 0, &zErrMsg);
    if (zErrMsg) {
        spdlog::error("Failed to set WAL mode: {}", zErrMsg);
        sqlite3_free(zErrMsg);
        zErrMsg = nullptr;
    }

    sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", 0, 0, &zErrMsg);
    if (zErrMsg) {
        spdlog::error("Failed to set synchronous mode: {}", zErrMsg);
        sqlite3_free(zErrMsg);
        zErrMsg = nullptr;
    }

    if (!create_tables_if_not_exist()) {
        return false;
    }

    const char* alter_sql = "ALTER TABLE violations ADD COLUMN uploaded INTEGER DEFAULT 0;";
    sqlite3_exec(db_, alter_sql, 0, 0, &zErrMsg);
    if (zErrMsg) {
        sqlite3_free(zErrMsg);
        zErrMsg = nullptr;
    }

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
        if (zErrMsg) {
            sqlite3_free(zErrMsg);
            zErrMsg = nullptr;
        }
    }

    cleanup_old_logs(retention_days);

    if (!writer_running_) {
        writer_running_ = true;
        writer_thread_ = std::thread(&ViolationLogger::writer_loop, this);
    }

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

    char* zErrMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        spdlog::error("SQL error: {}", zErrMsg);
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

bool ViolationLogger::log_violation(int zone_id, float confidence, int object_id,
                                    const std::array<float, 4>& detection_box,
                                    const std::array<float, 2>& world_coords,
                                    const std::string& camera_id) {
    {
        std::lock_guard<std::mutex> db_lock(db_mutex_);
        if (!db_) {
            return false;
        }
    }

    QueuedViolation item;
    item.zone_id = zone_id;
    item.confidence = confidence;
    item.object_id = object_id;
    item.detection_box = detection_box;
    item.world_coords = world_coords;
    item.camera_id = camera_id;
    item.timestamp = make_timestamp();

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        // Coalesce duplicate consecutive events for the same logical actor and camera.
        if (!write_queue_.empty()) {
            QueuedViolation& tail = write_queue_.back();
            if (tail.zone_id == item.zone_id && tail.object_id == item.object_id && tail.camera_id == item.camera_id) {
                tail.confidence = std::max(tail.confidence, item.confidence);
                tail.timestamp = item.timestamp;
                ++metric_coalesced_duplicates_;
                return true;
            }
        }

        // Backpressure policy: bounded queue with drop-oldest.
        if (write_queue_.size() >= max_queue_size_) {
            write_queue_.pop_front();
            ++metric_dropped_oldest_;
            ++metric_queue_overflow_events_;
        }

        write_queue_.push_back(std::move(item));
        ++metric_enqueued_;
        metric_max_queue_depth_ = std::max(metric_max_queue_depth_.load(), write_queue_.size());
    }

    queue_cv_.notify_one();
    return true;
}

bool ViolationLogger::write_batch_with_retry(const std::vector<QueuedViolation>& batch) {
    if (batch.empty()) {
        return true;
    }

    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_) {
        return false;
    }

    const char* sql = "INSERT INTO violations ("
                      "timestamp, zone_id, confidence, object_id, uploaded, "
                      "detection_box_x, detection_box_y, detection_box_w, detection_box_h, "
                      "world_coord_x, world_coord_y, camera_id) "
                      "VALUES (?, ?, ?, ?, 0, ?, ?, ?, ?, ?, ?, ?);";

    for (int attempt = 0; attempt <= max_retries_; ++attempt) {
        char* err = nullptr;
        int rc = sqlite3_exec(db_, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            if (err) {
                sqlite3_free(err);
            }
            if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
                ++metric_write_retries_;
                std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms_));
                continue;
            }
            ++metric_write_failures_;
            return false;
        }

        sqlite3_stmt* stmt = nullptr;
        rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            ++metric_write_failures_;
            return false;
        }

        bool batch_ok = true;
        for (const auto& item : batch) {
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);

            sqlite3_bind_text(stmt, 1, item.timestamp.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, item.zone_id);
            sqlite3_bind_double(stmt, 3, static_cast<double>(item.confidence));
            sqlite3_bind_int(stmt, 4, item.object_id);
            sqlite3_bind_double(stmt, 5, static_cast<double>(item.detection_box[0]));
            sqlite3_bind_double(stmt, 6, static_cast<double>(item.detection_box[1]));
            sqlite3_bind_double(stmt, 7, static_cast<double>(item.detection_box[2]));
            sqlite3_bind_double(stmt, 8, static_cast<double>(item.detection_box[3]));
            sqlite3_bind_double(stmt, 9, static_cast<double>(item.world_coords[0]));
            sqlite3_bind_double(stmt, 10, static_cast<double>(item.world_coords[1]));
            sqlite3_bind_text(stmt, 11, item.camera_id.c_str(), -1, SQLITE_TRANSIENT);

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                batch_ok = false;
                break;
            }
        }

        sqlite3_finalize(stmt);

        if (!batch_ok) {
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
            if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
                ++metric_write_retries_;
                std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms_));
                continue;
            }
            ++metric_write_failures_;
            return false;
        }

        rc = sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &err);
        if (rc == SQLITE_OK) {
            metric_written_ += static_cast<uint64_t>(batch.size());
            return true;
        }

        if (err) {
            sqlite3_free(err);
        }
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);

        if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
            ++metric_write_retries_;
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms_));
            continue;
        }

        ++metric_write_failures_;
        return false;
    }

    ++metric_write_failures_;
    return false;
}

void ViolationLogger::writer_loop() {
    while (writer_running_ || !write_queue_.empty()) {
        std::vector<QueuedViolation> batch;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait_for(lock, std::chrono::milliseconds(100), [this]() {
                return !writer_running_ || !write_queue_.empty();
            });

            if (write_queue_.empty()) {
                writer_busy_ = false;
                flush_cv_.notify_all();
                continue;
            }

            writer_busy_ = true;
            size_t take = std::min(batch_size_, write_queue_.size());
            batch.reserve(take);
            for (size_t i = 0; i < take; ++i) {
                batch.push_back(std::move(write_queue_.front()));
                write_queue_.pop_front();
            }
        }

        (void)write_batch_with_retry(batch);

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (write_queue_.empty()) {
                writer_busy_ = false;
                flush_cv_.notify_all();
            }
        }
    }
}

bool ViolationLogger::flush(uint32_t timeout_ms) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return flush_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() {
        return write_queue_.empty() && !writer_busy_;
    });
}

ViolationQueueMetrics ViolationLogger::get_metrics() const {
    ViolationQueueMetrics metrics;
    metrics.enqueued = metric_enqueued_.load();
    metrics.written = metric_written_.load();
    metrics.dropped_oldest = metric_dropped_oldest_.load();
    metrics.coalesced_duplicates = metric_coalesced_duplicates_.load();
    metrics.write_retries = metric_write_retries_.load();
    metrics.write_failures = metric_write_failures_.load();
    metrics.queue_overflow_events = metric_queue_overflow_events_.load();
    metrics.max_queue_depth = metric_max_queue_depth_.load();

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        metrics.queue_depth = write_queue_.size();
    }

    return metrics;
}

void ViolationLogger::cleanup_old_logs(int days) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_) {
        return;
    }

    std::stringstream sql;
    sql << "DELETE FROM violations WHERE timestamp < date('now', '-" << days << " days');";

    std::string query = sql.str();
    char* zErrMsg = nullptr;
    int rc = sqlite3_exec(db_, query.c_str(), 0, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        spdlog::error("DB Cleanup Error: {}", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        spdlog::info("Database cleanup completed. Logs older than {} days removed.", days);
    }
}

std::vector<ViolationRecord> ViolationLogger::get_pending_uploads(int limit) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<ViolationRecord> records;
    if (!db_) {
        return records;
    }

    std::stringstream sql;
    sql << "SELECT id, timestamp, zone_id, confidence, object_id, "
        << "detection_box_x, detection_box_y, detection_box_w, detection_box_h, "
        << "world_coord_x, world_coord_y, camera_id "
        << "FROM violations WHERE uploaded = 0 LIMIT " << limit << ";";

    sqlite3_stmt* stmt = nullptr;
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
        spdlog::error("Failed to query pending uploads.");
    }
    return records;
}

void ViolationLogger::mark_uploaded(const std::vector<int>& ids) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (!db_ || ids.empty()) {
        return;
    }

    std::stringstream sql;
    sql << "UPDATE violations SET uploaded = 1 WHERE id IN (";
    for (size_t i = 0; i < ids.size(); ++i) {
        sql << ids[i];
        if (i < ids.size() - 1) {
            sql << ",";
        }
    }
    sql << ");";

    char* zErrMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.str().c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        spdlog::error("Failed to update uploaded status: {}", zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

} // namespace safety
