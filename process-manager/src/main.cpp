#include <iostream>
#include <exception>
#include <mutex>
#include <atomic>
#include <csignal>
#include <memory>
#include <fstream>
#include <thread>
#include <condition_variable>

#include "ProcessManager.hpp"
#include "LogManager.hpp" 
#include "json.hpp"

std::atomic_bool shutdown_flag{false};
std::mutex shutdown_mutex;
std::condition_variable shutdown_cv;

void signal_handler(int signal){
    if(signal == SIGINT || signal == SIGTERM){
        std::cout << "\nShutdown signal received. Exiting..." << std::endl;
        {
            std::lock_guard<std::mutex> lock(shutdown_mutex);
            shutdown_flag = true;
        }
        shutdown_cv.notify_all();
    }
}

int main(){
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try {
        std::string socket_path = "/tmp/dacc-station.sock"; 

        std::ifstream config_file("config.json"); 
        if (!config_file.is_open()) {
            config_file.open("../config.json");
        }
        if (!config_file.is_open()) {
            config_file.open("process-manager/config.json");
        }
        
        if (config_file.is_open()) {
            try {
                nlohmann::json config = nlohmann::json::parse(config_file);
                if (config.contains("server") && config["server"].contains("socket_path")) {
                    socket_path = config["server"]["socket_path"];
                }
            } catch(...) {
                std::cerr << "Warning: Failed to parse config.json, using default socket path." << std::endl;
            }
        }

        LogManager::getInstance().initialize("PM", socket_path);
        
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Failed to initialize LogManager: " << e.what() << std::endl;
    }

    try {
        auto logger = LogManager::getInstance().getLogger();
        logger->info("Starting Process Manager...");
        
        ProcessManager pm(logger);

        std::cout << "\n>>> Process Manager is active. Press Ctrl+C to exit." << std::endl;
        
        std::unique_lock<std::mutex> lock(shutdown_mutex);
        shutdown_cv.wait(lock, []{ return shutdown_flag.load(); });
    
    } catch (const std::exception& e) {
        std::cerr << "A critical error occurred: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Main thread finished." << std::endl;
    return 0;
}