#include "SystemStatus.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <SDL2/SDL.h>

using namespace MeuProjeto;

SystemStatus::SystemStatus() : lastUpdate(0) {
    // Valor inicial
    cache = {"--:--", true, 100};
    update();
}

void SystemStatus::update() {
    unsigned int now = SDL_GetTicks();
    
    // Atualizar apenas a cada 1000ms (1 segundo)
    if (now - lastUpdate < 1000) {
        return;
    }
    lastUpdate = now;

    // 1. Relógio
    std::time_t t = std::time(nullptr);
    std::tm* nowTm = std::localtime(&t);
    
    std::stringstream ss;
    ss << std::put_time(nowTm, "%H:%M");
    cache.currentTime = ss.str();

    // 2. Mock de Bateria e Wifi (TODO: Ler de /sys/class/...)
    cache.wifiConnected = true; 
    cache.batteryLevel = 85; 
}

SystemData SystemStatus::getCachedData() const {
    return cache;
}