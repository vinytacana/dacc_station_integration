#pragma once

#include "spdlog/sinks/base_sink.h"
#include "spdlog/details/null_mutex.h"
#include <chrono>
#include <mutex>
#include <memory>
#include <iostream>

#include "UnixSocketClient.hpp"

namespace spdlog {
namespace sinks {

template<typename Mutex>
class unix_socket_sink : public base_sink<Mutex>
{
public:
    explicit unix_socket_sink(const std::string& socket_path)
    {
        try {
            client_ = std::make_unique<UnixSocketClient>(socket_path);
        } catch (const std::exception& e) {
            // Se falhar ao criar o cliente (socket), apenas logamos no stderr
            // O sink continuará existindo, mas operando em modo fallback desde o início
            std::cerr << "[UnixSocketSink] Failed to initialize socket client: " << e.what() << std::endl;
        }
    }

protected:
    void sink_it_(const details::log_msg& msg) override
    {
        // 1. Formata a mensagem usando o formatador do spdlog
        memory_buf_t formatted;
        base_sink<Mutex>::formatter_->format(msg, formatted);
        
        // 2. Converte para string para envio
        std::string payload(formatted.data(), formatted.size());

        // O transporte de logs nunca bloqueia. Falhas sao descartadas porque
        // este sink nao pode registrar erros em si mesmo sem causar recursao.
        if (client_ && client_->isConnected()) {
            const ipc::SendResult result = client_->send(
                payload,
                std::chrono::milliseconds::zero()
            );
            if (result.status == ipc::SendStatus::Disconnected ||
                result.status == ipc::SendStatus::Desynced) {
                client_.reset();
            }
        }
    }

    void flush_() override
    {
        // Sockets unix geralmente enviam imediatamente (especialmente em modo stream sem buffer de usuário grande),
        // mas se houvesse buffer no client_, chamaríamos aqui.
    }

private:
    std::unique_ptr<UnixSocketClient> client_;
};

using unix_socket_sink_mt = unix_socket_sink<std::mutex>;
using unix_socket_sink_st = unix_socket_sink<details::null_mutex>;

} // namespace sinks
} // namespace spdlog
