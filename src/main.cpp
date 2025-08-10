#include <iostream>
#include <exception> 
#include "GameManager.hpp"


int main(){
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
    std::cin.get();

    } catch (const std::exception& e) {
        std::cerr << "A critical error occurred: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Shutting down." << std::endl;
    return 0;
}