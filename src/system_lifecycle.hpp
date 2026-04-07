#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include "pipeline/pipeline_manager.hpp"
#include "utils/config_loader.hpp"

enum class SystemState {
    NONE,
    INITIALIZING,
    RUNNING,
    SHUTTING_DOWN,
    STOPPED
};

class SystemLifecycle {
public:
    SystemLifecycle();
    ~SystemLifecycle();

    bool start(int argc, char* argv[]);
    void requestShutdown();
    SystemState getState() const;

private:
    std::atomic<SystemState> current_state_;
    std::atomic<bool> shutdown_requested_;
    
    std::unique_ptr<PipelineManager> pipeline_manager_;
    std::unique_ptr<std::thread> health_check_thread_;

    bool initialize(int argc, char* argv[]);
    void run();
    void shutdown();
    void healthCheckLoop();
    
    void transitionState(SystemState new_state);
    std::string stateToString(SystemState state) const;
};
