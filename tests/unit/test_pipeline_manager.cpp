#include <gtest/gtest.h>
#include "pipeline/pipeline_manager.hpp"
#include <fstream>
#include <cstdio>

TEST(PipelineManagerTest, InitializationAndLifecycle) {
    gst_init(nullptr, nullptr);
    
    // Create dummy model
    std::string dummy_model = "pm_test.onnx";
    std::ofstream f(dummy_model);
    f << "dummy data";
    f.close();

    AppConfig config;
    config.rtsp_uri = "test";
    config.model_path = dummy_model;
    config.database_path = "pm_test_db.sqlite";

    PipelineManager manager(config);
    ASSERT_TRUE(manager.init());
    
    manager.start();
    // Let it run for a few frames
    g_usleep(500000); 
    manager.stop();
    
    SUCCEED();
    
    std::remove(dummy_model.c_str());
    std::remove(config.database_path.c_str());
}
