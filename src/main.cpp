#include <iostream>
#include <csignal>
#include <atomic>
#include "pipeline/pipeline_manager.hpp"
#include "utils/config_loader.hpp"
#include "utils/logger.hpp"

std::atomic<bool> keep_running(true);

void signalHandler(int signum) {
    spdlog::info("Interrupt signal ({}) received.", signum);
    keep_running = false;
}

int main(int argc, char* argv[]) {
    Logger::init();
    
    // Register signal handler for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::string config_path = "config.json";
    if (argc > 1) {
        config_path = argv[1];
    }
    
    spdlog::info("Starting Construction Safety Inference System...");
    spdlog::info("Loading configuration from: {}", config_path);

    AppConfig config = ConfigLoader::load(config_path);
    
    if (config.streams.empty()) {
        spdlog::warn("No streams configured in {}", config_path);
    }
    if (config.model_path.empty()) config.model_path = "yolo11n.onnx";

    gst_init(&argc, &argv);

    PipelineManager manager(config);

    if (!manager.init()) {
        spdlog::error("Failed to initialize pipeline manager.");
        return 1;
    }

    manager.start();

    spdlog::info("System is running. Press Ctrl+C to stop.");

    while (keep_running) {
        g_usleep(100000); // 100ms
    }

    spdlog::info("Stopping system...");
    manager.stop();

    return 0;
}
