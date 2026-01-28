#include <iostream>
#include <csignal>
#include <atomic>
#include "pipeline/pipeline_manager.hpp"

std::atomic<bool> keep_running(true);

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    keep_running = false;
}

int main(int argc, char* argv[]) {
    // Register signal handler for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::string rtsp_uri = "rtsp://127.0.0.1:8554/live";
    if (argc > 1) {
        rtsp_uri = argv[1];
    }
    
    std::string model_path = "yolov11n.engine";
    if (argc > 2) {
        model_path = argv[2];
    }

    std::cout << "Starting Construction Safety Inference System..." << std::endl;
    std::cout << "RTSP URI: " << rtsp_uri << std::endl;
    std::cout << "Model: " << model_path << std::endl;

    gst_init(&argc, &argv);

    PipelineManager manager(rtsp_uri, model_path);

    // Default configuration for prototype
    std::vector<std::vector<cv::Point>> zones = {
        {{100, 100}, {540, 100}, {540, 380}, {100, 380}} // Example zone
    };
    manager.setSafetyZones(zones);
    manager.setMQTTConfig("localhost", 1883, "safety/alerts");

    if (!manager.init()) {
        std::cerr << "Failed to initialize pipeline manager." << std::endl;
        return 1;
    }

    manager.start();

    std::cout << "System is running. Press Ctrl+C to stop." << std::endl;

    while (keep_running) {
        g_usleep(100000); // 100ms
    }

    std::cout << "Stopping system..." << std::endl;
    manager.stop();

    return 0;
}
