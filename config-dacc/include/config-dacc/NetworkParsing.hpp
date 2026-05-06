#ifndef CONFIG_DACC_NETWORK_PARSING_HPP
#define CONFIG_DACC_NETWORK_PARSING_HPP

#include "config-dacc/functions.hpp"

#include <string>
#include <vector>
#include <sstream>

namespace config_dacc {
namespace network_parsing {

inline std::vector<std::string> split_nmcli_escaped_fields(const std::string& linha) {
    std::vector<std::string> campos;
    std::string atual;
    bool escape = false;

    for (char c : linha) {
        if (escape) {
            atual.push_back(c);
            escape = false;
            continue;
        }
        if (c == '\\') {
            escape = true;
            continue;
        }
        if (c == ':') {
            campos.push_back(atual);
            atual.clear();
            continue;
        }
        atual.push_back(c);
    }
    campos.push_back(atual);
    return campos;
}

inline std::vector<wifi_network> parse_nmcli_wifi_list(const std::string& output) {
    std::vector<wifi_network> redes;
    std::stringstream ss(output);
    std::string linha;

    while (std::getline(ss, linha)) {
        auto campos = split_nmcli_escaped_fields(linha);
        if (campos.size() < 5) {
            continue;
        }

        wifi_network net;
        net.em_uso = campos[0] == "*";
        net.bssid = campos[1];
        net.backend_id = !net.bssid.empty() ? net.bssid : campos[2];
        net.ssid = campos[2];
        try {
            net.sinal = std::stoi(campos[3]);
        } catch (...) {
            net.sinal = 0;
        }
        net.seguranca = campos[4];
        if (!net.ssid.empty()) {
            redes.push_back(net);
        }
    }

    return redes;
}

} // namespace network_parsing
} // namespace config_dacc

#endif
