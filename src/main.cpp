#include "system_lifecycle.hpp"
#include "utils/logger.hpp"
#include <csignal>
#include <spdlog/spdlog.h>

SystemLifecycle g_lifecycle;

void signalHandler(int signum) {
    spdlog::info("Interrupt signal ({}) received. Initiating graceful shutdown...", signum);
    g_lifecycle.requestShutdown();
}

int main(int argc, char* argv[]) {
    Logger::init();
    
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        if (!g_lifecycle.start(argc, argv)) {
            spdlog::error("System failed to start or stopped unexpectedly.");
            return EXIT_FAILURE;
        }

        spdlog::info("System exited gracefully.");
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        spdlog::error("Unknown fatal error occurred");
        return EXIT_FAILURE;
    }
}
