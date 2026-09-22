#include "SystemStatus.hpp"
#include "config-dacc/functions.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

namespace {

std::mutex g_mutex;
std::condition_variable g_condition;
bool g_consulta_executada = false;
bool g_segunda_consulta_iniciada = false;
std::thread::id g_thread_consulta;
std::atomic_int g_total_consultas{0};
std::atomic_bool g_cancelamento_shutdown_observado{false};

void exigir(bool condicao, const std::string& mensagem) {
    if (!condicao) {
        std::cerr << "[FAIL] " << mensagem << std::endl;
        std::exit(1);
    }
}

} // namespace

network_connection_status obter_status_conexao_rede(const command_options& options) {
    const int numero_consulta = ++g_total_consultas;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_consulta_executada = true;
        g_thread_consulta = std::this_thread::get_id();
        g_segunda_consulta_iniciada = numero_consulta >= 2;
    }
    g_condition.notify_all();

    if (numero_consulta >= 2) {
        while (options.cancel_requested != nullptr && !options.cancel_requested->load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        g_cancelamento_shutdown_observado.store(true);
        return {};
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    network_connection_status status;
    status.conectado = true;
    status.wifi_conectado = true;
    return status;
}

int obter_bateria() {
    return 75;
}

void verificar_shutdown() {
    if (!g_cancelamento_shutdown_observado.load()) {
        std::cerr << "[FAIL] shutdown deve cancelar e aguardar a consulta de rede" << std::endl;
        std::_Exit(1);
    }
}

int main() {
    exigir(std::atexit(verificar_shutdown) == 0, "deve registrar verificacao de shutdown");
    const std::thread::id thread_principal = std::this_thread::get_id();
    MeuProjeto::SystemStatus& status = MeuProjeto::SystemStatus::getInstance();

    {
        std::unique_lock<std::mutex> lock(g_mutex);
        exigir(
            g_condition.wait_for(lock, std::chrono::seconds(1), []() {
                return g_consulta_executada;
            }),
            "consulta de rede em background deve ser iniciada"
        );
        exigir(
            g_thread_consulta != thread_principal,
            "consulta de rede nao deve executar na thread principal da SDL"
        );
    }

    bool cache_atualizado = false;
    for (int tentativa = 0; tentativa < 20; ++tentativa) {
        if (status.getCachedData().wifiConnected) {
            cache_atualizado = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    exigir(cache_atualizado, "resultado da consulta deve atualizar o cache compartilhado");

    {
        std::unique_lock<std::mutex> lock(g_mutex);
        exigir(
            g_condition.wait_for(lock, std::chrono::seconds(2), []() {
                return g_segunda_consulta_iniciada;
            }),
            "segunda consulta deve estar ativa para exercitar o shutdown"
        );
    }

    std::cout << "System status tests passed." << std::endl;
    return 0;
}
