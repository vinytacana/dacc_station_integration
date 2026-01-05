#pragma once

#include "ILogger.hpp"
#include <memory>
#include <string>
#include <stdexcept>

// CompositeLogger implementa a interface ILogger e gerencia a lógica de fallback.
class CompositeLogger : public ILogger {
private:
    std::unique_ptr<ILogger> primary_logger_;
    std::unique_ptr<ILogger> fallback_logger_;

public:
    CompositeLogger(std::unique_ptr<ILogger> primary, std::unique_ptr<ILogger> fallback)
        : primary_logger_( std::move(primary) ), fallback_logger_( std::move(fallback) ) {
        if (not fallback_logger_) {
            // O logger de fallback é obrigatório para garantir que sempre haja um log.
            throw std::invalid_argument("FallbackLogger cannot be null.");
        }
    }

    void log(const std::string& message) override {
        if (primary_logger_) {
            try {
                primary_logger_->log(message);
                return; 
            } catch (...) {
                fallback_logger_->log("[ERROR] Primary logger failed to send message. Falling back.\n");
            }
        }
        fallback_logger_->log(message);
    }
};
