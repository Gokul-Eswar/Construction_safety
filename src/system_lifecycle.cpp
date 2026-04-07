#include "system_lifecycle.hpp"
#include "inference/model_loader.hpp"
#include <iostream>
#include <spdlog/spdlog.h>
#include <gst/gst.h>

SystemLifecycle::SystemLifecycle() 
    : current_state_(SystemState::NONE),
      shutdown_requested_(false) {
}

SystemLifecycle::~SystemLifecycle() {
    shutdown();
}

void SystemLifecycle::transitionState(SystemState new_state) {
    SystemState old_state = current_state_.exchange(new_state);
    if (old_state != new_state) {
        spdlog::info("[Lifecycle] Transitioning state: {} -> {}", 
                     stateToString(old_state), stateToString(new_state));
    }
}

std::string SystemLifecycle::stateToString(SystemState state) const {
    switch (state) {
        case SystemState::NONE: return "NONE";
        case SystemState::INITIALIZING: return "INITIALIZING";
        case SystemState::RUNNING: return "RUNNING";
        case SystemState::SHUTTING_DOWN: return "SHUTTING_DOWN";
        case SystemState::STOPPED: return "STOPPED";
        default: return "UNKNOWN";
    }
}

bool SystemLifecycle::start(int argc, char* argv[]) {
    if (!initialize(argc, argv)) {
        transitionState(SystemState::STOPPED);
        return false;
    }
    
    run();
    return true;
}

bool SystemLifecycle::initialize(int argc, char* argv[]) {
    transitionState(SystemState::INITIALIZING);
    
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
            exit(EXIT_SUCCESS);
        } else {
            config_path = arg;
        }
    }

    if (expect_config_path) {
        spdlog::error("Missing value for --config");
        return false;
    }
    
    spdlog::info("Starting Construction Safety Inference System...");
    spdlog::info("Loading configuration from: {}", config_path);

    AppConfig config;
    try {
        config = ConfigLoader::load(config_path);
    } catch(const std::exception& e) {
        spdlog::error("Failed to load config: {}", e.what());
        return false;
    }
    
    if (build_engine_only) {
        spdlog::info("Running in ENGINE BUILD MODE only.");
        if (config.model_path.empty()) config.model_path = "yolo11n.onnx";
        
        ModelLoader loader(config.model_path);
        if (loader.load()) {
            spdlog::info("Engine build/verification complete. Exiting.");
            exit(EXIT_SUCCESS);
        } else {
            spdlog::error("Engine build/verification failed.");
            return false;
        }
    }

    gst_init(&argc, &argv);

    pipeline_manager_ = std::make_unique<PipelineManager>(config);
    if (!pipeline_manager_->init()) {
        spdlog::error("Failed to initialize pipeline manager.");
        return false;
    }

    pipeline_manager_->start();
    return true;
}

void SystemLifecycle::run() {
    transitionState(SystemState::RUNNING);

    health_check_thread_ = std::make_unique<std::thread>(&SystemLifecycle::healthCheckLoop, this);

    while (!shutdown_requested_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    shutdown();
}

void SystemLifecycle::healthCheckLoop() {
    while (!shutdown_requested_) {
        for(int i = 0; i < 100 && !shutdown_requested_; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (shutdown_requested_) break;

        if (pipeline_manager_) {
            // Heartbeat/health check logging
            spdlog::debug("[Lifecycle] Health check: Pipeline is running.");
        }
    }
}

void SystemLifecycle::requestShutdown() {
    shutdown_requested_ = true;
}

void SystemLifecycle::shutdown() {
    if (current_state_ == SystemState::STOPPED || current_state_ == SystemState::SHUTTING_DOWN) {
        return;
    }
    
    transitionState(SystemState::SHUTTING_DOWN);
    shutdown_requested_ = true;

    if (health_check_thread_ && health_check_thread_->joinable()) {
        health_check_thread_->join();
    }

    if (pipeline_manager_) {
        spdlog::info("[Lifecycle] Shutting down pipeline manager...");
        pipeline_manager_->stop();
        pipeline_manager_.reset();
    }
    
    transitionState(SystemState::STOPPED);
}

SystemState SystemLifecycle::getState() const {
    return current_state_.load();
}
