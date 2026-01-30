#include <gtest/gtest.h>
#include "pipeline/pipeline_manager.hpp"
#include <fstream>
#include <cstdio>

TEST(PipelineManagerTest, InitializationFailsWithInvalidModel) {
    gst_init(nullptr, nullptr);
    
    // Create dummy model (garbage)
    std::string dummy_model = "pm_test.onnx";
    std::ofstream f(dummy_model);
    f << "dummy data";
    f.close();

    AppConfig config;
    config.rtsp_uri = "test";
    config.model_path = dummy_model;
    config.database_path = "pm_test_db.sqlite";

    PipelineManager manager(config);
    // Should fail to init
    EXPECT_FALSE(manager.init());
    
    // Cleanup
    std::remove(dummy_model.c_str());
    std::remove(config.database_path.c_str());
}
