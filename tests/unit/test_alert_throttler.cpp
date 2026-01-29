#include <gtest/gtest.h>
#include "utils/alert_throttler.hpp"
#include <thread>

TEST(AlertThrottlerTest, FirstAlertAlwaysPasses) {
    safety::AlertThrottler throttler;
    EXPECT_TRUE(throttler.should_alert(1, 101));
}

TEST(AlertThrottlerTest, SuppressesDuplicateAlertsWithinCooldown) {
    safety::AlertThrottler throttler;
    throttler.set_cooldown(1000); // 1 second

    EXPECT_TRUE(throttler.should_alert(1, 101)); // First alert
    EXPECT_FALSE(throttler.should_alert(1, 101)); // Immediate duplicate -> Suppressed
}

TEST(AlertThrottlerTest, AllowsAlertAfterCooldown) {
    safety::AlertThrottler throttler;
    throttler.set_cooldown(100); // 100 ms for test speed

    EXPECT_TRUE(throttler.should_alert(1, 101));
    EXPECT_FALSE(throttler.should_alert(1, 101));

    // Wait for cooldown
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_TRUE(throttler.should_alert(1, 101));
}

TEST(AlertThrottlerTest, IndependentKeys) {
    safety::AlertThrottler throttler;
    throttler.set_cooldown(1000);

    EXPECT_TRUE(throttler.should_alert(1, 101));
    EXPECT_TRUE(throttler.should_alert(2, 101)); // Different zone
    EXPECT_TRUE(throttler.should_alert(1, 102)); // Different object
}
