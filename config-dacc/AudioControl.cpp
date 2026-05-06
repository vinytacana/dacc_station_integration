#include "config-dacc/functions.hpp"
#include "config-dacc/ConfigResult.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

std::mutex g_audio_mutex;

system_result traduzir_audio_result(
    const command_result& command,
    const std::string& codigo,
    const std::string& mensagem
) {
    if (command.ok) {
        return config_result::success(mensagem, command.mensagem);
    }
    if (command.mensagem.find("No such entity") != std::string::npos ||
        command.mensagem.find("not found") != std::string::npos) {
        return config_result::error("audio_device_not_found", "Dispositivo de audio nao encontrado.", command.mensagem);
    }
    return config_result::error(codigo, mensagem, command.mensagem);
}

std::string limpar_nome_audio(std::string raw) {
    size_t colchete = raw.find('[');
    if (colchete != std::string::npos) {
        raw = raw.substr(0, colchete);
    }
    while (!raw.empty() && std::isspace(static_cast<unsigned char>(raw.back()))) {
        raw.pop_back();
    }
    return raw;
}

bool executar_comando_audio(
    const std::vector<std::string>& cmd_wp,
    const std::vector<std::string>& cmd_pa,
    const std::vector<std::string>& cmd_alsa
) {
    std::thread([cmd_wp, cmd_pa, cmd_alsa]() {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        bool sucesso = false;
        if (comando_existe("wpctl")) {
            sucesso = exec_command_args_result(cmd_wp).ok;
        }
        if (!sucesso && comando_existe("pactl")) {
            sucesso = exec_command_args_result(cmd_pa).ok;
        }
        if (!sucesso && comando_existe("amixer")) {
            (void)exec_command_args_result(cmd_alsa);
        }
    }).detach();
    return true;
}

} // namespace

void aumentar_volume() {
    executar_comando_audio({"wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%+"},
                           {"pactl", "set-sink-volume", "@DEFAULT_SINK@", "+5%"},
                           {"amixer", "sset", "Master", "5%+"});
}

void diminuir_volume() {
    executar_comando_audio({"wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%-"},
                           {"pactl", "set-sink-volume", "@DEFAULT_SINK@", "-5%"},
                           {"amixer", "sset", "Master", "5%-"});
}

void definir_volume(int valor_int) {
    if (valor_int > 100) valor_int = 100;
    if (valor_int < 0) valor_int = 0;

    float valor_float = static_cast<float>(valor_int) / 100.0f;
    std::string v_str = std::to_string(valor_float);
    std::replace(v_str.begin(), v_str.end(), ',', '.');
    std::string v_perc = std::to_string(valor_int) + "%";

    executar_comando_audio({"wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", v_str},
                           {"pactl", "set-sink-volume", "@DEFAULT_SINK@", v_perc},
                           {"amixer", "sset", "Master", v_perc});
}

int obter_volume_atual() {
    try {
        std::string saida = exec_command("wpctl get-volume @DEFAULT_AUDIO_SINK@");
        size_t pos = saida.find("Volume: ");
        if (pos != std::string::npos) {
            std::string vol_str = saida.substr(pos + 8);
            return static_cast<int>(std::stof(vol_str) * 100);
        }
    } catch (...) {
    }
    return 50;
}

std::vector<device_audio> listar_dispositivos_audio() {
    std::vector<device_audio> lista;
    std::string saida;
    try {
        saida = exec_command("wpctl status");
    } catch (...) {
        return lista;
    }

    std::stringstream ss(saida);
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

        device_audio dev;
        dev.padrao = linha.find('*') != std::string::npos;
        size_t inicio_num = ponto_pos - 1;
        while (inicio_num > 0 && std::isdigit(static_cast<unsigned char>(linha[inicio_num - 1]))) {
            inicio_num--;
        }

        try {
            dev.id = std::stoi(linha.substr(inicio_num, ponto_pos - inicio_num));
            if (ponto_pos + 2 < linha.size()) {
                dev.descricao = limpar_nome_audio(linha.substr(ponto_pos + 2));
                lista.push_back(dev);
            }
        } catch (...) {
        }
    }
    return lista;
}

void selecionar_dispositivo_audio(int id) {
    (void)selecionar_dispositivo_audio_result(id);
}

system_result selecionar_dispositivo_audio_result(int id) {
    command_result result = exec_command_args_result({"wpctl", "set-default", std::to_string(id)});
    return traduzir_audio_result(result, "audio_select_failed", "Falha ao definir dispositivo de audio.");
}

void imprimir_dispositivos_audio() {
    auto lista = listar_dispositivos_audio();
    if (lista.empty()) {
        std::cout << "Nenhum dispositivo de saída encontrado.\n";
        return;
    }

    std::cout << "=== Dispositivos de Saída ===\n";
    for (const auto& dev : lista) {
        std::cout << (dev.padrao ? "[*] " : "[ ] ") << "ID: " << dev.id << " | " << dev.descricao << "\n";
    }
    std::cout << "=============================\n";
}
