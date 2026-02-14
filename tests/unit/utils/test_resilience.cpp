#include <gtest/gtest.h>
#include "pipeline/pipeline_manager.hpp"
#include <opencv2/opencv.hpp>

// Helper to access protected members for testing
class TestablePipelineManager : public PipelineManager {
public:
    using PipelineManager::PipelineManager;
    using PipelineManager::checkAlerts;
};

TEST(ResilienceTest, FootprintLogicAccuracy) {
    AppConfig config;
    config.database_path = "test_resilience.db";
    TestablePipelineManager manager(config);
    
    // Define a zone: A square from (100, 100) to (200, 200)
    ZoneConfig zone;
    zone.id = 1;
    zone.name = "Test Zone";
    zone.points = { {100, 100}, {200, 100}, {200, 200}, {100, 200} };
    
    std::vector<ZoneConfig> zones = { zone };
    
    // Case 1: Person leaning IN, but feet OUT
    // Box: x=50, y=50, w=100, h=40 (Top part is in zone, but feet are at 100, 90)
    Detection det1;
    det1.box = cv::Rect(50, 50, 100, 40);
    det1.confidence = 0.9f;
    det1.track_id = 1;
    
    // We expect NO violation because feet (100, 90) are outside the [100-200] zone.
    // Note: checkAlerts logs to DB and MQTT. We'd need mocks to verify perfectly,
    // but we can verify the math here.
    cv::Point2f feet1(det1.box.x + det1.box.width / 2.0f, det1.box.y + det1.box.height);
    EXPECT_EQ(feet1.x, 100);
    EXPECT_EQ(feet1.y, 90);
    EXPECT_LT(cv::pointPolygonTest(zone.points, feet1, false), 0);

    // Case 2: Person feet just INSIDE
    Detection det2;
    det2.box = cv::Rect(100, 100, 20, 50); // Feet at (110, 150)
    cv::Point2f feet2(det2.box.x + det2.box.width / 2.0f, det2.box.y + det2.box.height);
    EXPECT_GE(cv::pointPolygonTest(zone.points, feet2, false), 0);
}

TEST(ResilienceTest, MemoryLeakDetectionPlaceholder) {
    // This would ideally use Valgrind, but we can check if 
    // repeated init/stop cycles are stable.
    AppConfig config;
    config.model_path = "invalid.onnx";
    
    for(int i=0; i<10; i++) {
        PipelineManager manager(config);
        manager.init(); // Fails but should clean up
    }
    SUCCEED();
}
