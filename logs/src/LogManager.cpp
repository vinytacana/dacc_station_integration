#include "LogManager.hpp"
#include "UnixSocketSink.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>
#include <cstring>
#include <vector>

LogManager& LogManager::getInstance() {
    static LogManager instance;
    return instance;
}

void LogManager::initialize(const std::string& appName, const std::string& socketPath) {
    appName_ = appName;
    
    try {
        std::vector<spdlog::sink_ptr> sinks;

        // 1. Sink para o Socket (Principal)
        auto socket_sink = std::make_shared<spdlog::sinks::unix_socket_sink_mt>(socketPath);
        // O padrão do spdlog é formatar [Data] [Logger] [Level] Msg
        // Podemos customizar se quisermos. O padrão é bom.
        sinks.push_back(socket_sink);

        // 2. Sink para Console (Opcional, bom para debug local)
        // Isso replica o comportamento do antigo CompositeLogger se quisermos sempre ver no console
        // Ou podemos deixar só o socket se o socket sink já tiver fallback.
        // Por segurança no início, vamos adicionar console também.
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        sinks.push_back(console_sink);

        // Criação do Logger com múltiplos sinks
        logger_ = std::make_shared<spdlog::logger>(appName, sinks.begin(), sinks.end());
        
        // Configurações globais
        logger_->set_level(spdlog::level::trace); // Logar tudo por padrão
        logger_->flush_on(spdlog::level::info);   // Flush imediato em INFO ou superior
        
        // Registra globalmente (opcional, permite spdlog::get("name"))
        spdlog::register_logger(logger_);
        spdlog::set_default_logger(logger_);

        logger_->info("LogManager initialized. Connected to socket: {}", socketPath);

    } catch (const std::exception& e) {
        std::cerr << "LogManager: Critical failure initializing spdlog: " << e.what() << std::endl;
        // Fallback de emergência: Console logger simples
        logger_ = spdlog::stdout_color_mt("console_fallback");
    }
}

std::shared_ptr<spdlog::logger> LogManager::getLogger() {
    if (!logger_) {
        // Se chamado antes de initialize, retorna um logger de console temporário
        try {
            logger_ = spdlog::stdout_color_mt("temp_console");
        } catch (const spdlog::spdlog_ex&) {
            // Se já existe, pega ele
            logger_ = spdlog::get("temp_console");
        }
    }
    return logger_;
}

void LogManager::logError(const std::string& context, bool includeErrno){
    std::string err_msg = includeErrno ? strerror(errno) : "";
    getInstance().getLogger()->error("{} | errno: {}", context, err_msg);
}