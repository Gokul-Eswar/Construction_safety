#include <iostream>
#include <fstream>
#include <csignal>
#include <atomic>
#include "pipeline/pipeline_manager.hpp"
#include "inference/model_loader.hpp"
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
    bool build_engine_only = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--build-engine-only") {
            build_engine_only = true;
        } else {
            config_path = arg;
        }
    }
    
    spdlog::info("Starting Construction Safety Inference System...");
    spdlog::info("Loading configuration from: {}", config_path);

    AppConfig config = ConfigLoader::load(config_path);
    
    // Mode: Build Engine Only
    if (build_engine_only) {
        spdlog::info("Running in ENGINE BUILD MODE only.");
        if (config.model_path.empty()) config.model_path = "yolo11n.onnx";
        
        spdlog::info("Target Model: {}", config.model_path);
        
        ModelLoader loader(config.model_path);
        if (loader.load()) {
            spdlog::info("Engine build/verification complete. Exiting.");
            return 0;
        } else {
            spdlog::error("Failed to build or load the engine.");
            return 1;
        }
    }

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

    auto last_heartbeat = std::chrono::steady_clock::now();

    while (keep_running) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeat).count() >= 5) {
            std::ofstream heartbeat_file("heartbeat.json");
            if (heartbeat_file.is_open()) {
                auto sys_now = std::chrono::system_clock::now();
                auto sys_time = std::chrono::system_clock::to_time_t(sys_now);
                heartbeat_file << "{\"timestamp\": " << sys_time << ", \"status\": \"running\"}";
                heartbeat_file.close();
            }
            last_heartbeat = now;
        }

        g_usleep(100000); // 100ms
    }

    spdlog::info("Stopping system...");
    manager.stop();

    return 0;
}
