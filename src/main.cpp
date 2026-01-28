#include <iostream>
#include <csignal>
#include <atomic>
#include "pipeline/pipeline_manager.hpp"
#include "utils/config_loader.hpp"

std::atomic<bool> keep_running(true);

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    keep_running = false;
}

int main(int argc, char* argv[]) {
    // Register signal handler for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::string config_path = "config.json";
    if (argc > 1) {
        config_path = argv[1];
    }
    
    std::cout << "Starting Construction Safety Inference System..." << std::endl;
    std::cout << "Loading configuration from: " << config_path << std::endl;

    AppConfig config = ConfigLoader::load(config_path);
    
    // Provide some overrides or defaults if config is empty
    if (config.rtsp_uri.empty()) config.rtsp_uri = "rtsp://127.0.0.1:8554/live";
    if (config.model_path.empty()) config.model_path = "yolov11n.engine";

    gst_init(&argc, &argv);

    PipelineManager manager(config);

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
