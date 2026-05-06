#ifndef CONFIG_DACC_DISPLAY_PARSING_HPP
#define CONFIG_DACC_DISPLAY_PARSING_HPP

#include "config-dacc/functions.hpp"

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace config_dacc {
namespace display_parsing {

inline std::string trim(const std::string& valor) {
    size_t inicio = 0;
    while (inicio < valor.size() && std::isspace(static_cast<unsigned char>(valor[inicio]))) {
        inicio++;
    }
    size_t fim = valor.size();
    while (fim > inicio && std::isspace(static_cast<unsigned char>(valor[fim - 1]))) {
        fim--;
    }
    return valor.substr(inicio, fim - inicio);
}

inline bool starts_with(const std::string& valor, const std::string& prefixo) {
    return valor.rfind(prefixo, 0) == 0;
}

inline bool extrair_resolucao(const std::string& token, int& width, int& height) {
    size_t x_pos = token.find('x');
    if (x_pos == std::string::npos || x_pos == 0 || !std::isdigit(static_cast<unsigned char>(token[0]))) {
        return false;
    }

    try {
        width = std::stoi(token.substr(0, x_pos));
        size_t i = x_pos + 1;
        std::string h_str;
        while (i < token.size() && std::isdigit(static_cast<unsigned char>(token[i]))) {
            h_str.push_back(token[i++]);
        }
        if (h_str.empty()) {
            return false;
        }
        height = std::stoi(h_str);
        return true;
    } catch (...) {
        return false;
    }
}

inline std::vector<DisplayOutput> parse_xrandr_verbose(const std::string& output) {
    std::vector<DisplayOutput> displays;
    std::stringstream ss(output);
    std::string linha;
    DisplayOutput* current_display = nullptr;

    while (std::getline(ss, linha)) {
        if (linha.find(" connected ") != std::string::npos) {
            DisplayOutput disp;
            std::stringstream line_ss(linha);
            line_ss >> disp.name;
            disp.connected = true;
            disp.current_scale = 1.0f;
            displays.push_back(disp);
            current_display = &displays.back();
            continue;
        }

        if (!current_display || linha.size() <= 2 || linha[0] != ' ' || linha.find('x') == std::string::npos) {
            continue;
        }

        std::stringstream mode_ss(linha);
        std::string token;
        int width = 0;
        int height = 0;
        while (mode_ss >> token) {
            if (!extrair_resolucao(token, width, height)) {
                continue;
            }

            DisplayMode mode;
            mode.width = width;
            mode.height = height;
            mode.refresh_rate = 60.0f;
            mode.is_current = linha.find("*current") != std::string::npos || linha.find('*') != std::string::npos;
            current_display->modes.push_back(mode);
            if (mode.is_current) {
                current_display->current_mode = mode;
            }
            break;
        }
    }

    return displays;
}

inline std::vector<DisplayOutput> parse_wlr_randr(const std::string& output) {
    std::vector<DisplayOutput> displays;
    std::stringstream ss(output);
    std::string linha;
    DisplayOutput* current_display = nullptr;
    bool em_modes = false;

    while (std::getline(ss, linha)) {
        std::string limpa = trim(linha);
        if (limpa.empty()) {
            continue;
        }

        if (!std::isspace(static_cast<unsigned char>(linha[0]))) {
            DisplayOutput disp;
            std::stringstream line_ss(limpa);
            line_ss >> disp.name;
            disp.connected = true;
            disp.current_scale = 1.0f;
            displays.push_back(disp);
            current_display = &displays.back();
            em_modes = false;
            continue;
        }

        if (!current_display) {
            continue;
        }

        if (starts_with(limpa, "Enabled:")) {
            current_display->connected = limpa.find("yes") != std::string::npos;
            continue;
        }
        if (starts_with(limpa, "Scale:")) {
            try {
                current_display->current_scale = std::stof(trim(limpa.substr(6)));
            } catch (...) {
                current_display->current_scale = 1.0f;
            }
            continue;
        }
        if (starts_with(limpa, "Modes:")) {
            em_modes = true;
            continue;
        }
        if (!em_modes || limpa.find('x') == std::string::npos) {
            continue;
        }

        std::stringstream mode_ss(limpa);
        std::string res_token;
        mode_ss >> res_token;

        int width = 0;
        int height = 0;
        if (!extrair_resolucao(res_token, width, height)) {
            continue;
        }

        DisplayMode mode;
        mode.width = width;
        mode.height = height;
        mode.refresh_rate = 60.0f;
        size_t hz_pos = limpa.find("Hz");
        if (hz_pos != std::string::npos) {
            size_t inicio = limpa.rfind(' ', hz_pos);
            if (inicio != std::string::npos) {
                try {
                    mode.refresh_rate = std::stof(limpa.substr(inicio + 1, hz_pos - inicio - 1));
                } catch (...) {
                }
            }
        }
        mode.is_current = limpa.find("current") != std::string::npos;
        current_display->modes.push_back(mode);
        if (mode.is_current) {
            current_display->current_mode = mode;
        }
    }

    return displays;
}

} // namespace display_parsing
} // namespace config_dacc

#endif
