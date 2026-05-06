#include "config-dacc/functions.hpp"

#include <chrono>
#include <fstream>
#include <string>
#include <vector>

long long obter_tempo_ms() {
    auto agora = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(agora.time_since_epoch()).count();
}

int obter_bateria() {
    std::vector<std::string> caminhos = {
        "/sys/class/power_supply/BAT0/capacity",
        "/sys/class/power_supply/BAT1/capacity"
    };

    for (const auto& path : caminhos) {
        std::ifstream arquivo(path);
        if (arquivo.is_open()) {
            int porcentagem = -1;
            arquivo >> porcentagem;
            return porcentagem;
        }
    }

    return -1;
}
