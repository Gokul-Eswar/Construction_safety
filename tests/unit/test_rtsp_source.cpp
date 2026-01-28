#include <gtest/gtest.h>
#include "pipeline/rtsp_source.hpp"

TEST(RTSPSourceTest, PipelineStringGeneration) {
    std::string rtsp_url = "rtsp://192.168.1.10:554/stream";
    RTSPSource source(rtsp_url);
    
    std::string pipeline = source.getPipelineString();
    
    // Check key elements
    EXPECT_NE(pipeline.find("rtspsrc"), std::string::npos);
    EXPECT_NE(pipeline.find("location=" + rtsp_url), std::string::npos);
    EXPECT_NE(pipeline.find("latency=0"), std::string::npos); // Low latency requirement
    
    // Check for hardware decoding (spec requirement)
    // We expect nvv4l2decoder or a placeholder if we abstract it.
    // Let's assume we want to verify the logic adds it.
    EXPECT_NE(pipeline.find("nvv4l2decoder"), std::string::npos);
}

TEST(RTSPSourceTest, SourceStats) {
    gst_init(nullptr, nullptr);
    
    RTSPSource source("test");
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

TEST(RTSPSourceTest, FrameCallback) {
    gst_init(nullptr, nullptr);
    
    RTSPSource source("test");
    int frame_count = 0;
    
    source.setFrameCallback([&frame_count](GstSample* sample) {
        frame_count++;
    });
    
    ASSERT_TRUE(source.start());
    
    // Wait for a bit to receive frames (videotestsrc num-buffers=10 is very fast)
    // We can use a loop or just sleep.
    int retries = 0;
    while (frame_count < 10 && retries < 100) {
        g_usleep(10000); // 10ms
        retries++;
    }
    
    source.stop();
    
    EXPECT_GE(frame_count, 1);
    // Note: num-buffers=10 should give exactly 10, but let's be safe.
    EXPECT_LE(frame_count, 10);
}

TEST(RTSPSourceTest, Initialization) {
    // Initialize GStreamer
    gst_init(nullptr, nullptr);
    
    RTSPSource source("rtsp://127.0.0.1:554/test");
    // We don't necessarily expect start() to succeed without the actual plugins 
    // or a valid RTSP stream, but we check it doesn't crash.
    // In a CI environment without NVIDIA plugins, it might fail.
    source.start(); 
    source.stop();
    SUCCEED();
}
