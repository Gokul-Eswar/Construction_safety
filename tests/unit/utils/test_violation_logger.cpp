#include <gtest/gtest.h>
#include "utils/violation_logger.hpp"
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>

class ViolationLoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a temporary database file
        db_path = "test_violations.db";
        // Ensure clean state
        std::remove(db_path.c_str());
    }

    void TearDown() override {
        // Cleanup
        std::remove(db_path.c_str());
    }

    std::string db_path;
};

TEST_F(ViolationLoggerTest, InitCreatesDatabaseAndTable) {
    safety::ViolationLogger logger;
    EXPECT_TRUE(logger.init(db_path));

    // Verify file exists
    std::ifstream f(db_path.c_str());
    EXPECT_TRUE(f.good());
}

TEST_F(ViolationLoggerTest, LogViolationInsertsRecord) {
    safety::ViolationLogger logger;
    ASSERT_TRUE(logger.init(db_path));

    EXPECT_TRUE(logger.log_violation(1, 0.85f, 101));

    // Verify insertion using raw sqlite3 (independent verification)
    sqlite3* db;
    ASSERT_EQ(sqlite3_open(db_path.c_str(), &db), SQLITE_OK);

    const char* query = "SELECT count(*) FROM violations WHERE zone_id=1 AND object_id=101;";
    sqlite3_stmt* stmt;
    ASSERT_EQ(sqlite3_prepare_v2(db, query, -1, &stmt, 0), SQLITE_OK);
    
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    int count = sqlite3_column_int(stmt, 0);
    EXPECT_EQ(count, 1);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

TEST_F(ViolationLoggerTest, BurstQueueBackpressureDropsOldestAndTracksMetrics) {
    safety::ViolationLogger logger;
    ASSERT_TRUE(logger.init(db_path));
    logger.set_queue_limits(32, 8);

    for (int i = 0; i < 300; ++i) {
        ASSERT_TRUE(logger.log_violation(i % 3, 0.9f, i % 2));
    }

    ASSERT_TRUE(logger.flush(5000));
    auto metrics = logger.get_metrics();

    EXPECT_GT(metrics.enqueued, 0);
    EXPECT_GE(metrics.written, 1);
    EXPECT_GE(metrics.dropped_oldest, 1);
    EXPECT_GE(metrics.queue_overflow_events, 1);
}

TEST_F(ViolationLoggerTest, BusyLockRetryEventuallyWrites) {
    safety::ViolationLogger logger;
    ASSERT_TRUE(logger.init(db_path));
    logger.set_retry_policy(25, 20, 50);

    sqlite3* locker = nullptr;
    ASSERT_EQ(sqlite3_open(db_path.c_str(), &locker), SQLITE_OK);
    char* err = nullptr;
    ASSERT_EQ(sqlite3_exec(locker, "BEGIN EXCLUSIVE;", nullptr, nullptr, &err), SQLITE_OK);

    std::thread producer([&logger]() {
        for (int i = 0; i < 20; ++i) {
            logger.log_violation(7, 0.8f, i);
        }
    });

    // Hold lock long enough to force SQLITE_BUSY retries.
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    ASSERT_EQ(sqlite3_exec(locker, "COMMIT;", nullptr, nullptr, &err), SQLITE_OK);
    sqlite3_close(locker);

    producer.join();
    ASSERT_TRUE(logger.flush(8000));

    auto metrics = logger.get_metrics();
    EXPECT_GE(metrics.write_retries, 1);
    EXPECT_EQ(metrics.write_failures, 0);
    EXPECT_GE(metrics.written, 1);
}
