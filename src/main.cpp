#include <iostream>
#include <fstream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <gst/gst.h>
#include "pipeline/pipeline_manager.hpp"
#include "inference/model_loader.hpp"
#include "utils/config_loader.hpp"
#include "utils/logger.hpp"

// ================================================================================
// CONSTRUCTION SAFETY SENTINEL - MAIN ENTRY POINT
// ================================================================================
//
// Lifecycle Flow (VERIFIED - NO CONFLICTS):
// 1. Logger Initialization → Set up centralized logging
// 2. Signal Handler Registration → SIGINT (Ctrl+C), SIGTERM (termination)
// 3. Argument Parsing → Process CLI flags (--config, --build-engine-only)
// 4. Configuration Loading → Load and validate config.json against schema
// 5. Mode Selection:
//    a) BUILD_ENGINE_ONLY: Load model → Verify → Exit (used for image verification)
//    b) FULL_SYSTEM: Initialize GStreamer → Create pipeline → Main loop → Shutdown
// 6. Main Service Loop → Process streams, detect violations, publish telemetry
// 7. Graceful Shutdown → Stop streams, disconnect MQTT, cleanup resources
//
// Thread Safety:
// - Signal handler sets atomic flag (keep_running)
// - Main thread periodically checks flag (100ms sleep)
// - Pipeline manager stops all worker threads on shutdown
//
// Status: LIFECYCLE IS CLEAN - Single execution path with two branches
// No conflicting implementations or state machine issues.
// Future Enhancement: Add SystemLifecycle state machine for health monitoring
// ================================================================================

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

    try {
        std::string config_path = "config.json";
        bool build_engine_only = false;
        bool expect_config_path = false;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (expect_config_path) {
                config_path = arg;
                expect_config_path = false;
            } else if (arg == "--build-engine-only") {
                build_engine_only = true;
            } else if (arg == "--config") {
                expect_config_path = true;
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "Usage: main_app [--build-engine-only] [--config <path>] [config_path]" << "\n";
                return EXIT_SUCCESS;
            } else {
                // Backward-compatible positional config path.
                config_path = arg;
            }
        }

        if (expect_config_path) {
            spdlog::error("Missing value for --config");
            return EXIT_FAILURE;
        }
        
        spdlog::info("Starting Construction Safety Inference System...");
        spdlog::info("Loading configuration from: {}", config_path);

        // Load and validate config against schema
        AppConfig config = ConfigLoader::load(config_path);
        
        // Mode: Build Engine Only
        if (build_engine_only) {
            spdlog::info("Running in ENGINE BUILD MODE only.");
            if (config.model_path.empty()) config.model_path = "yolo11n.onnx";
            
            spdlog::info("Target Model: {}", config.model_path);
            
            ModelLoader loader(config.model_path);
            if (loader.load()) {
                spdlog::info("Engine build/verification complete. Exiting.");
                return EXIT_SUCCESS;
            } else {
                spdlog::error("Engine build/verification failed.");
                return EXIT_FAILURE;
            }
        }

        // Full system mode
        spdlog::info("Initializing full safety monitoring system...");

        gst_init(&argc, &argv);
        
        PipelineManager pipeline(config);
        if (!pipeline.init()) {
            spdlog::error("Failed to initialize pipeline manager");
            return EXIT_FAILURE;
        }

        pipeline.start();

        spdlog::info("System initialized successfully. Starting monitoring...");

        // Main service loop while stream processing runs in background threads.
        while (keep_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        spdlog::info("Shutdown signal received. Cleaning up...");
        pipeline.stop();
        return EXIT_SUCCESS;
        
    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        spdlog::error("Unknown fatal error occurred");
        return EXIT_FAILURE;
    }
}
