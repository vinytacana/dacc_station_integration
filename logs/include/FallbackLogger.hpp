#pragma once

#include "ILogger.hpp"
#include "ConsoleWriter.hpp"
#include "FileWriter.hpp"
#include <memory>
#include <string>

// FallbackLogger é uma implementação de ILogger que escreve para múltiplos
// destinos (ex: console e arquivo) e não deve falhar.
class FallbackLogger : public ILogger {
private:
    ConsoleWriter console_writer_;
    FileWriter file_writer_;

public:
    explicit FallbackLogger(const std::string& fallback_filename)
        : file_writer_(fallback_filename) {}

    void log(const std::string& message) override {
        // Escreve a mesma mensagem em ambos os destinos
        console_writer_.write(message + "\n");
        file_writer_.write(message + "\n");
    }
};

