/**
 * @file SystemStatus.cpp
 * @brief Implementação da classe SystemStatus para monitoramento de informações do sistema.
 * 
 * Este arquivo gerencia a coleta e cache de dados do sistema como horário atual,
 * status de conexão WiFi e nível de bateria, atualizando periodicamente para
 * exibição na interface do usuário.
 */

#include "SystemStatus.hpp"
#include "config-dacc/functions.hpp"
#include <chrono>
#include <ctime>
#include <SDL2/SDL.h>

using namespace MeuProjeto;

/**
 * @brief Construtor da classe SystemStatus.
 * 
 * Inicializa o sistema de monitoramento com valores padrão e realiza
 * a primeira atualização dos dados do sistema.
 * 
 */
SystemStatus::SystemStatus() : lastUpdate(0) {
    cache = {"--:--", false, false, -1};
    update();
    networkThread = std::thread(&SystemStatus::atualizarRedeEmBackground, this);
}

SystemStatus::~SystemStatus() {
    encerrarNetworkThread.store(true);
    networkWaitCondition.notify_all();
    if (networkThread.joinable()) {
        networkThread.join();
    }
}

void SystemStatus::atualizarRedeEmBackground() {
    while (!encerrarNetworkThread.load()) {
        command_options options;
        options.timeout = std::chrono::seconds(2);
        options.terminate_grace_period = std::chrono::milliseconds(250);
        options.cancel_requested = &encerrarNetworkThread;

        network_connection_status rede;
        try {
            rede = ::obter_status_conexao_rede(options);
        } catch (...) {
            rede = {};
        }

        if (!encerrarNetworkThread.load()) {
            std::lock_guard<std::mutex> lock(cacheMutex);
            cache.wifiConnected = rede.wifi_conectado;
            cache.wiredConnected = rede.cabeado_conectado;
        }

        std::unique_lock<std::mutex> lock(networkWaitMutex);
        networkWaitCondition.wait_for(
            lock,
            std::chrono::seconds(1),
            [this]() { return encerrarNetworkThread.load(); }
        );
    }
}

/**
 * @brief Atualiza as informações do sistema em cache.
 * 
 * Implementa um sistema de throttling que atualiza os dados apenas a cada
 * 1 segundo (1000ms) para evitar processamento desnecessário e melhorar performance.
 * 
 */
void SystemStatus::update() {
    /// Obtém o tempo atual em milissegundos desde inicialização da SDL
    unsigned int now = SDL_GetTicks();
    
    /**
     * Sistema de throttling: só atualiza se passou pelo menos 1 segundo.
     * Evita processamento redundante quando chamado em alta frequência.
     */
    if (now - lastUpdate < 1000) {
        return;
    }
    lastUpdate = now;

    /**
     * Coleta o horário atual do sistema usando a biblioteca <ctime>.
     * std::localtime() converte para fuso horário local do sistema.
     */
    std::time_t t = std::time(nullptr);
    std::tm* nowTm = std::localtime(&t);
    
    if (nowTm) {
        char hora[6] = "--:--";
        if (std::strftime(hora, sizeof(hora), "%H:%M", nowTm) > 0) {
            std::lock_guard<std::mutex> lock(cacheMutex);
            cache.currentTime = hora;
        }
    }

    int battery = ::obter_bateria();
    if (battery == BATERIA_INDISPONIVEL) {
        SDL_GetPowerInfo(nullptr, &battery);
    }
    if (battery < 0 || battery > 100) {
        battery = BATERIA_INDISPONIVEL;
    }
    std::lock_guard<std::mutex> lock(cacheMutex);
    cache.batteryLevel = battery;
}

/**
 * @brief Retorna os dados do sistema em cache.
 * 
 * Acesso rápido aos últimos valores coletados sem necessidade de
 * nova leitura do sistema. Ideal para renderização contínua da UI.
 * 
 * @return SystemData Estrutura contendo horário, status WiFi e nível de bateria.
 */
SystemData SystemStatus::getCachedData() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return cache;
}
