#include <iostream>
#include <exception>
#include <mutex>
#include <atomic>
#include <csignal>

#include "GameManager.hpp"

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
try {
    std::cout << "Starting Process Manager..." << std::endl;
    
    ProcessManager pm;          
        // Game snake_game("SnakeGame.jar", "/usr/games/SnakeGame/SnakeGame.jar", "java");
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
        std::cerr << "A critical error occurred: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Shutting down." << std::endl;
    return 0;
}