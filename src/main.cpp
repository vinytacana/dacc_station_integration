#include <iostream>
#include <exception>
#include <mutex>
#include <atomic>
#include <csignal>
#include <memory>
#include <fstream>

#include "GameManager.hpp"
#include "logs/SocketLogger.hpp"
#include "logs/FallbackLogger.hpp"
#include "logs/CompositeLogger.hpp"
#include "json.hpp"

std::atomic_bool shutdown_flag{false};
std::mutex shutdown_mutex;
std::condition_variable shutdown_cv;

void signal_handler(int signal){
    if(signal == SIGINT || signal == SIGTERM){
        std::cout << "\nShutdown signal received. Exiting...";
        {
            std::lock_guard<std::mutex> lock(shutdown_mutex);;
            shutdown_flag = true;
        }
    }
    shutdown_cv.notify_all();
}

int main(){
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::unique_ptr<ILogger> primary_logger;
    std::unique_ptr<ILogger> fallback_logger;

    
    try {
        fallback_logger = std::make_unique<FallbackLogger>("pm_fallback.log");
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Failed to create fallback logger: " << e.what() << std::endl;
        return 1;
    }

    try {
        std::ifstream config_file("../config.json");
        
        if (!config_file.is_open()) {
            throw std::runtime_error("Could not open config.json");
        }

        nlohmann::json config = nlohmann::json::parse(config_file);
        const std::string socket_path = config["server"]["socket_path"];

        primary_logger = std::make_unique<SocketLogger>(socket_path);
        
        fallback_logger->log("[INFO] Primary logger (SocketLogger) connected successfully to " + socket_path + ".\n");

    } catch (const std::exception& e) {
        fallback_logger->log("[WARNING] Failed to create primary logger: " + std::string(e.what()) + "\n");
        fallback_logger->log("[WARNING] ProcessManager will use fallback logging only.\n");
    }
    
    auto composite_logger = std::make_unique<CompositeLogger>(
        std::move(primary_logger),
        std::move(fallback_logger)
    );

    try {
        composite_logger->log("Starting Process Manager...\n");
        
        ProcessManager pm(std::move(composite_logger));
        
        Game snake_game("SnakeGame.jar", "/home/feliph/game_scripts/run_snake.sh", "native");
        
        if (pm.startGame(&snake_game)) {
            std::cout << "Game started successfully." << std::endl;
        } else {
            std::cerr << "Failed to start the game." << std::endl;
            return 1; 
        }

        std::cout << "\n>>> Process Manager is active and monitoring." << std::endl;
        std::unique_lock<std::mutex> lock(shutdown_mutex);
        shutdown_cv.wait(lock, []{return shutdown_flag.load(); });
    
    } catch (const std::exception& e) {
        std::cerr << "A critical error occurred during ProcessManager operation: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Shutting down." << std::endl;
    return 0;
}
