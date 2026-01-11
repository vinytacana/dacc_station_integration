#pragma once

#include <string>
#include <memory>
#include <spdlog/spdlog.h>

class LogManager {
public:
    static LogManager& getInstance();

    // Inicializa o spdlog com UnixSocketSink e (opcionalmente) ConsoleSink
    void initialize(const std::string& appName, const std::string& socketPath);
    
    // Retorna o logger principal configurado
    std::shared_ptr<spdlog::logger> getLogger();

    // Método estático de conveniência para erros de sistema
    static void logError(const std::string& context, bool includeErrno = true);

private:
    LogManager() = default;
    std::shared_ptr<spdlog::logger> logger_;
    std::string appName_;
};

// Macros de conveniência mantidas para compatibilidade (mas agora usam spdlog)
// AVISO: spdlog suporta formatação "{}", então evite concatenação manual de strings dentro da macro se possível.

#define LOG_ERROR(what) \
    LogManager::getInstance().getLogger()->error("{} -> {}", __func__, what)

#define LOG_INFO(what) \
    LogManager::getInstance().getLogger()->info(what)

#define LOG_WARNING(what) \
    LogManager::getInstance().getLogger()->warn(what)

#define LOG_DEBUG(what) \
    LogManager::getInstance().getLogger()->debug(what)
