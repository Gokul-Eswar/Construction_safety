#include <gtest/gtest.h>
#include "pipeline/camera_source.hpp"

TEST(CameraSourceTest, RTSPPipelineGeneration) {
    std::string rtsp_url = "rtsp://192.168.1.10:554/stream";
    CameraSource source("test_id", "rtsp", rtsp_url);
    
    std::string pipeline = source.getPipelineString();
    
    // Check key elements
    EXPECT_NE(pipeline.find("rtspsrc"), std::string::npos);
    EXPECT_NE(pipeline.find("location=" + rtsp_url), std::string::npos);
    EXPECT_NE(pipeline.find("latency=0"), std::string::npos); // Low latency requirement
}

TEST(CameraSourceTest, USBPipelineGeneration) {
    std::string device_uri = "0";
    CameraSource source("test_id", "usb", device_uri);
    
    std::string pipeline = source.getPipelineString();
    
    // Check key elements
#ifdef _WIN32
    EXPECT_NE(pipeline.find("ksvideosrc"), std::string::npos);
    EXPECT_NE(pipeline.find("device-index=0"), std::string::npos);
#else
    EXPECT_NE(pipeline.find("v4l2src"), std::string::npos);
    EXPECT_NE(pipeline.find("device-index=0"), std::string::npos);
#endif
    EXPECT_NE(pipeline.find("videoconvert"), std::string::npos);
}

TEST(CameraSourceTest, SourceStats) {
    gst_init(nullptr, nullptr);
    
    CameraSource source("test_id", "test", "test");
    ASSERT_TRUE(source.start());
    
    // Wait for at least 1.1 seconds to allow FPS calculation
    g_usleep(1100000);
    
    SourceStats stats = source.getStats();
    
    EXPECT_TRUE(stats.is_running);
    EXPECT_GT(stats.frame_count, 0);
    EXPECT_GT(stats.fps, 0.0);
    
    source.stop();
    stats = source.getStats();
    EXPECT_FALSE(stats.is_running);
}

TEST(CameraSourceTest, FrameCallback) {
    gst_init(nullptr, nullptr);
    
    CameraSource source("test_id", "test", "test");
    int frame_count = 0;
    
    source.setFrameCallback([&frame_count](GstSample* sample) {
        (void)sample;
        frame_count++;
    });
    
    ASSERT_TRUE(source.start());
    
    // Wait for a bit to receive frames
    int retries = 0;
    while (frame_count < 10 && retries < 100) {
        g_usleep(10000); // 10ms
        retries++;
    }
    
    source.stop();
    
    EXPECT_GE(frame_count, 1);
}

TEST(CameraSourceTest, Initialization) {
    // Initialize GStreamer
    gst_init(nullptr, nullptr);
    
    CameraSource source("test_id", "rtsp", "rtsp://127.0.0.1:554/test");
    source.start(); 
    source.stop();
    SUCCEED();
}
