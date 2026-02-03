/**
 * @file SystemStatus.cpp
 * @brief Implementação da classe SystemStatus para monitoramento de informações do sistema.
 * 
 * Este arquivo gerencia a coleta e cache de dados do sistema como horário atual,
 * status de conexão WiFi e nível de bateria, atualizando periodicamente para
 * exibição na interface do usuário.
 */

#include "SystemStatus.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <SDL2/SDL.h>

using namespace MeuProjeto;

/**
 * @brief Construtor da classe SystemStatus.
 * 
 * Inicializa o sistema de monitoramento com valores padrão e realiza
 * a primeira atualização dos dados do sistema.
 * 
 * O cache é preenchido com valores iniciais:
 * - Horário: "--:--" (placeholder até primeira atualização)
 * - WiFi: true (conectado)
 * - Bateria: 100% (carga completa)
 */
SystemStatus::SystemStatus() : lastUpdate(0) {
    // Valor inicial
    cache = {"--:--", true, 100};
    update();
}

/**
 * @brief Atualiza as informações do sistema em cache.
 * 
 * Implementa um sistema de throttling que atualiza os dados apenas a cada
 * 1 segundo (1000ms) para evitar processamento desnecessário e melhorar performance.
 * 
 * Informações atualizadas:
 * - **Relógio**: Horário local no formato HH:MM (24 horas)
 * - **WiFi**: Status de conexão (mock, sempre true)
 * - **Bateria**: Nível de carga percentual (mock, fixo em 85%)
 * 
 * @note Atualmente WiFi e bateria usam valores simulados (mock).
 * @todo Implementar leitura real de /sys/class/power_supply/ e /sys/class/net/
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
    
    /**
     * Formata o horário no padrão HH:MM usando std::put_time.
     * Exemplo de saída: "14:35", "09:07"
     */
    std::stringstream ss;
    ss << std::put_time(nowTm, "%H:%M");
    cache.currentTime = ss.str();

    /**
     * Mock de dados de hardware que serão implementados futuramente.
     * Em produção, estes valores devem ser lidos de:
     * - WiFi: /sys/class/net/<interface>/operstate
     * - Bateria: /sys/class/power_supply/BAT0/capacity
     */
    cache.wifiConnected = true; 
    cache.batteryLevel = 85; 
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
    return cache;
}