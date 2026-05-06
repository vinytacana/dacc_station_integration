#ifndef CONFIG_DACC_AUDIO_PARSING_HPP
#define CONFIG_DACC_AUDIO_PARSING_HPP

#include "config-dacc/functions.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace config_dacc {
namespace audio_parsing {

inline std::string trim_audio_name(std::string raw) {
    size_t colchete = raw.find('[');
    if (colchete != std::string::npos) {
        raw = raw.substr(0, colchete);
    }
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.back()))) {
        raw.pop_back();
    }
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.front()))) {
        raw.erase(raw.begin());
    }
    return raw;
}

inline std::string trim_line(std::string raw) {
    while (!raw.empty() && (raw.back() == '\n' || raw.back() == '\r' ||
                            std::isspace(static_cast<unsigned char>(raw.back())))) {
        raw.pop_back();
    }
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.front()))) {
        raw.erase(raw.begin());
    }
    return raw;
}

inline std::vector<device_audio> parse_wpctl_sinks(const std::string& output) {
    std::vector<device_audio> lista;
    std::stringstream ss(output);
    std::string linha;
    bool na_secao_sinks = false;

    while (std::getline(ss, linha)) {
        if (linha.find("Sinks:") != std::string::npos) {
            na_secao_sinks = true;
            continue;
        }
        if (na_secao_sinks &&
            (linha.find("Sources:") != std::string::npos ||
             linha.find("Filters:") != std::string::npos ||
             linha.empty())) {
            break;
        }
        if (!na_secao_sinks) {
            continue;
        }

        size_t ponto_pos = linha.find('.');
        if (ponto_pos == std::string::npos || ponto_pos == 0 ||
            !std::isdigit(static_cast<unsigned char>(linha[ponto_pos - 1]))) {
            continue;
        }

        size_t inicio_num = ponto_pos - 1;
        while (inicio_num > 0 && std::isdigit(static_cast<unsigned char>(linha[inicio_num - 1]))) {
            inicio_num--;
        }

        try {
            device_audio dev;
            dev.id = std::stoi(linha.substr(inicio_num, ponto_pos - inicio_num));
            dev.backend_id = std::to_string(dev.id);
            dev.padrao = linha.find('*') != std::string::npos;
            dev.backend = audio_backend::wpctl;
            dev.descricao = ponto_pos + 2 < linha.size() ? trim_audio_name(linha.substr(ponto_pos + 2)) : "";
            if (!dev.descricao.empty()) {
                lista.push_back(dev);
            }
        } catch (...) {
        }
    }

    return lista;
}

inline std::vector<device_audio> parse_pactl_sinks_short(
    const std::string& output,
    const std::string& default_sink = ""
) {
    std::vector<device_audio> lista;
    std::stringstream ss(output);
    std::string linha;
    const std::string default_sink_limpo = trim_line(default_sink);

    while (std::getline(ss, linha)) {
        std::stringstream line_ss(linha);
        std::string id_str;
        std::string nome;
        if (!(line_ss >> id_str >> nome)) {
            continue;
        }
        try {
            device_audio dev;
            dev.id = std::stoi(id_str);
            dev.backend_id = nome;
            dev.descricao = nome;
            dev.padrao = !default_sink_limpo.empty() && nome == default_sink_limpo;
            dev.backend = audio_backend::pactl;
            lista.push_back(dev);
        } catch (...) {
        }
    }

    return lista;
}

inline std::vector<device_audio> parse_aplay_devices(const std::string& output) {
    std::vector<device_audio> lista;
    std::stringstream ss(output);
    std::string linha;
    int id = 0;

    while (std::getline(ss, linha)) {
        if (linha.find("card ") == std::string::npos ||
            linha.find("device ") == std::string::npos) {
            continue;
        }

        device_audio dev;
        dev.id = id++;
        dev.backend_id = "alsa:" + std::to_string(dev.id);
        dev.padrao = dev.id == 0;
        dev.backend = audio_backend::alsa;
        dev.descricao = linha;
        lista.push_back(dev);
    }

    return lista;
}

inline bool parse_wpctl_volume(const std::string& output, int& volume) {
    size_t pos = output.find("Volume:");
    if (pos == std::string::npos) {
        return false;
    }
    try {
        float valor = std::stof(output.substr(pos + 7));
        volume = static_cast<int>(valor * 100.0f + 0.5f);
        volume = std::max(0, std::min(100, volume));
        return true;
    } catch (...) {
        return false;
    }
}

inline bool parse_percent_token(const std::string& output, int& volume) {
    size_t percent = output.find('%');
    while (percent != std::string::npos) {
        size_t inicio = percent;
        while (inicio > 0 && std::isdigit(static_cast<unsigned char>(output[inicio - 1]))) {
            inicio--;
        }
        if (inicio < percent) {
            try {
                volume = std::stoi(output.substr(inicio, percent - inicio));
                volume = std::max(0, std::min(100, volume));
                return true;
            } catch (...) {
            }
        }
        percent = output.find('%', percent + 1);
    }
    return false;
}

inline bool parse_pactl_volume(const std::string& output, int& volume) {
    return parse_percent_token(output, volume);
}

inline bool parse_amixer_volume(const std::string& output, int& volume) {
    return parse_percent_token(output, volume);
}

} // namespace audio_parsing
} // namespace config_dacc

#endif
