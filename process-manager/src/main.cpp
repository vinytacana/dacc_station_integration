#include <iostream>
#include <exception>
#include <csignal>
#include <memory>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <cstdlib>
#include <poll.h>

#include "ProcessManager.hpp"
#include "LogManager.hpp" 

volatile sig_atomic_t shutdown_requested = 0;

void signal_handler(int) {
    shutdown_requested = 1;
}

int main() {
    struct sigaction action {};
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, nullptr) == -1 ||
        sigaction(SIGTERM, &action, nullptr) == -1) {
        std::cerr << "Failed to configure shutdown signal handlers: "
                  << std::strerror(errno) << std::endl;
        return 1;
    }

    try {
        LogManager::getInstance().initialize("PM", ipc::logSocketPath());
        
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Failed to initialize LogManager: " << e.what() << std::endl;
    }

    try {
        auto logger = LogManager::getInstance().getLogger();
        logger->info("Starting Process Manager...");
        
        const char* catalog_env = std::getenv("DACC_GAME_CATALOG");
        const std::string catalog_path = catalog_env && *catalog_env
            ? catalog_env
            : defaultGameCatalogPath();
        ProcessManager pm(logger, ipc::processManagerSocketPath(), catalog_path);

        std::cout << "\n>>> Process Manager is active. Press Ctrl+C to exit." << std::endl;

        while (!shutdown_requested) {
            int poll_result = poll(nullptr, 0, 250);
            if (poll_result == -1 && errno != EINTR) {
                throw std::runtime_error(
                    "Failed while waiting for shutdown: " + std::string(std::strerror(errno))
                );
            }
        }

        logger->info("Shutdown signal received. Exiting...");
    
    } catch (const std::exception& e) {
        std::cerr << "A critical error occurred: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Main thread finished." << std::endl;
    return 0;
}
