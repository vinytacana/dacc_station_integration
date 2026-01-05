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
// Includes do sistema de log (vêm de ../logs/include)
#include "SocketLogger.hpp"
#include "FallbackLogger.hpp"
#include "CompositeLogger.hpp"
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

    std::unique_ptr<ILogger> primary_logger;
    std::unique_ptr<ILogger> fallback_logger;

    // 1. Configurar Fallback Logger (Local)
    try {
        fallback_logger = std::make_unique<FallbackLogger>("pm_fallback.log");
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL: Failed to create fallback logger: " << e.what() << std::endl;
        return 1;
    }

    // 2. Configurar Primary Logger (Socket)
    try {
        // Tenta ler config da raiz ou de ../ (dependendo de onde roda o binário)
        std::ifstream config_file("config.json"); 
        if (!config_file.is_open()) {
            // Tenta um nível acima se estiver rodando de bin/
            config_file.open("../config.json");
        }
        
        if (!config_file.is_open()) {
            throw std::runtime_error("Could not open config.json");
        }

        nlohmann::json config = nlohmann::json::parse(config_file);
        const std::string socket_path = config["server"]["socket_path"];

        primary_logger = std::make_unique<SocketLogger>(socket_path);
        
        fallback_logger->log("[INFO] Primary logger (SocketLogger) configured for " + socket_path);

    } catch (const std::exception& e) {
        fallback_logger->log("[WARNING] Failed to create primary logger: " + std::string(e.what()));
        fallback_logger->log("[WARNING] ProcessManager will use fallback logging only.");
    }
    
    // 3. Criar Composite Logger
    auto composite_logger = std::make_unique<CompositeLogger>(
        std::move(primary_logger),
        std::move(fallback_logger)
    );

    try {
        composite_logger->log("Starting Process Manager...");
        
        ProcessManager pm(std::move(composite_logger));
        
        // Exemplo de aplicação (substituindo a antiga classe 'Game')
        // ID, Caminho do Executável, Tipo
        ApplicationDefinition snake_game(
            "snake_game_01", 
            "/home/feliph/game_scripts/run_snake.sh", 
            "native"
        );
        
        // Validação básica antes de iniciar
        if (pm.isAppValid(snake_game)) {
            pid_t pid = pm.startApplication(snake_game);
            if (pid > 0) {
                 std::cout << "App started successfully with PID: " << pid << std::endl;
            } else {
                 std::cerr << "Failed to start the app." << std::endl;
            }
        } else {
            std::cerr << "Application definition is invalid." << std::endl;
        }

        std::cout << "\n>>> Process Manager is active. Press Ctrl+C to exit." << std::endl;
        
        // Aguarda sinal de desligamento
        std::unique_lock<std::mutex> lock(shutdown_mutex);
        shutdown_cv.wait(lock, []{ return shutdown_flag.load(); });
    
    } catch (const std::exception& e) {
        std::cerr << "A critical error occurred: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Main thread finished." << std::endl;
    return 0;
}
