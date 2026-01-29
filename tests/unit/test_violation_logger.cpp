#include <gtest/gtest.h>
#include "utils/violation_logger.hpp"
#include <cstdio>
#include <fstream>

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
