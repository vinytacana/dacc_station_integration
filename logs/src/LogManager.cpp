#include "LogManager.hpp"
#include "SocketLogger.hpp"
#include "FallbackLogger.hpp"
#include <iostream>
#include <cstring>

LogManager& LogManager::getInstance() {
    static LogManager instance;
    return instance;
}

void LogManager::initialize(const std::string& appName, const std::string& socketPath) {
    appName_ = appName;
    try {
        // Tenta conectar ao Socket
        logger_ = std::make_shared<SocketLogger>(socketPath);
        logger_->info(appName + " LogManager initialized connected to " + socketPath);
    } catch (const std::exception& e) {
        // Se falhar, usa Fallback (arquivo local ou stderr)
        std::cerr << "LogManager: Failed to connect to socket (" << e.what() << "). Using fallback." << std::endl;
        logger_ = std::make_shared<FallbackLogger>(appName + "_fallback.log");
    }
}

std::shared_ptr<ILogger> LogManager::getLogger() {
    if (!logger_) {
        // Se não inicializado, cria um logger dummy ou fallback temporário
        logger_ = std::make_shared<FallbackLogger>("default_fallback.log");
    }
    return logger_;
}

void LogManager::logError(const std::string& context, bool includeErrno){
    // Legacy support
    getInstance().getLogger()->error(context + (includeErrno ? (" | errno: " + std::string(strerror(errno))) : ""));
}

