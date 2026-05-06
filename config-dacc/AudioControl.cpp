#include "config-dacc/functions.hpp"
#include "config-dacc/AudioParsing.hpp"
#include "config-dacc/ConfigResult.hpp"

#include <algorithm>
#include <iostream>
#include <mutex>
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

std::vector<device_audio> listar_dispositivos_wpctl() {
    std::vector<device_audio> lista;
    if (!comando_existe("wpctl")) {
        return lista;
    }

    command_result result = exec_command_args_result({"wpctl", "status"});
    if (!result.ok) {
        return lista;
    }

    return config_dacc::audio_parsing::parse_wpctl_sinks(result.stdout_output);
}

std::vector<device_audio> listar_dispositivos_pactl() {
    std::vector<device_audio> lista;
    if (!comando_existe("pactl")) {
        return lista;
    }

    command_result result = exec_command_args_result({"pactl", "list", "sinks", "short"});
    if (!result.ok) {
        return lista;
    }

    return config_dacc::audio_parsing::parse_pactl_sinks_short(result.stdout_output);
}

std::vector<device_audio> listar_dispositivos_aplay() {
    std::vector<device_audio> lista;
    if (!comando_existe("aplay")) {
        return lista;
    }

    command_result result = exec_command_args_result({"aplay", "-l"});
    if (!result.ok) {
        return lista;
    }

    return config_dacc::audio_parsing::parse_aplay_devices(result.stdout_output);
}

system_result executar_comando_audio_result(
    const std::vector<std::string>& cmd_wp,
    const std::vector<std::string>& cmd_pa,
    const std::vector<std::string>& cmd_alsa,
    const std::string& codigo_erro,
    const std::string& mensagem_erro
) {
    if (comando_existe("wpctl")) {
        command_result result = exec_command_args_result(cmd_wp);
        if (result.ok) {
            return config_result::success("Audio atualizado.", result.mensagem);
        }
    }
    if (comando_existe("pactl")) {
        command_result result = exec_command_args_result(cmd_pa);
        if (result.ok) {
            return config_result::success("Audio atualizado.", result.mensagem);
        }
    }
    if (comando_existe("amixer")) {
        command_result result = exec_command_args_result(cmd_alsa);
        if (result.ok) {
            return config_result::success("Audio atualizado.", result.mensagem);
        }
        return traduzir_audio_result(result, codigo_erro, mensagem_erro);
    }

    return config_result::error(
        "audio_subsystem_missing",
        "Subsistema de audio compativel indisponivel.",
        "wpctl, pactl e amixer ausentes."
    );
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
    int volume = 50;
    if (obter_volume_atual_result(volume).ok) {
        return volume;
    }
    return 50;
}

system_result obter_volume_atual_result(int& volume) {
    if (comando_existe("wpctl")) {
        command_result result = exec_command_args_result({"wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@"});
        if (result.ok && config_dacc::audio_parsing::parse_wpctl_volume(result.stdout_output, volume)) {
            return config_result::success("Volume obtido.", result.mensagem);
        }
    }

    if (comando_existe("pactl")) {
        command_result result = exec_command_args_result({"pactl", "get-sink-volume", "@DEFAULT_SINK@"});
        if (result.ok && config_dacc::audio_parsing::parse_pactl_volume(result.stdout_output, volume)) {
            return config_result::success("Volume obtido.", result.mensagem);
        }
    }

    if (comando_existe("amixer")) {
        command_result result = exec_command_args_result({"amixer", "get", "Master"});
        if (result.ok && config_dacc::audio_parsing::parse_amixer_volume(result.stdout_output, volume)) {
            return config_result::success("Volume obtido.", result.mensagem);
        }
    }

    return config_result::error(
        "audio_subsystem_missing",
        "Subsistema de audio compativel indisponivel.",
        "wpctl, pactl e amixer ausentes ou sem volume parseavel."
    );
}

std::vector<device_audio> listar_dispositivos_audio() {
    auto lista = listar_dispositivos_wpctl();
    if (!lista.empty()) return lista;

    lista = listar_dispositivos_pactl();
    if (!lista.empty()) return lista;

    return listar_dispositivos_aplay();
}

void selecionar_dispositivo_audio(int id) {
    (void)selecionar_dispositivo_audio_result(id);
}

system_result selecionar_dispositivo_audio_result(int id) {
    if (comando_existe("wpctl")) {
        command_result result = exec_command_args_result({"wpctl", "set-default", std::to_string(id)});
        return traduzir_audio_result(result, "audio_select_failed", "Falha ao definir dispositivo de audio.");
    }

    if (comando_existe("pactl")) {
        command_result result = exec_command_args_result({"pactl", "set-default-sink", std::to_string(id)});
        return traduzir_audio_result(result, "audio_select_failed", "Falha ao definir dispositivo de audio.");
    }

    return config_result::error(
        "audio_subsystem_missing",
        "Subsistema de audio compativel indisponivel.",
        "wpctl e pactl ausentes; selecao via ALSA/aplay nao e suportada."
    );
}

system_result aumentar_volume_result() {
    return executar_comando_audio_result(
        {"wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%+"},
        {"pactl", "set-sink-volume", "@DEFAULT_SINK@", "+5%"},
        {"amixer", "sset", "Master", "5%+"},
        "audio_volume_failed",
        "Falha ao aumentar volume."
    );
}

system_result diminuir_volume_result() {
    return executar_comando_audio_result(
        {"wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%-"},
        {"pactl", "set-sink-volume", "@DEFAULT_SINK@", "-5%"},
        {"amixer", "sset", "Master", "5%-"},
        "audio_volume_failed",
        "Falha ao diminuir volume."
    );
}

system_result definir_volume_result(int valor_int) {
    if (valor_int > 100) valor_int = 100;
    if (valor_int < 0) valor_int = 0;

    float valor_float = static_cast<float>(valor_int) / 100.0f;
    std::string v_str = std::to_string(valor_float);
    std::replace(v_str.begin(), v_str.end(), ',', '.');
    std::string v_perc = std::to_string(valor_int) + "%";

    return executar_comando_audio_result(
        {"wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", v_str},
        {"pactl", "set-sink-volume", "@DEFAULT_SINK@", v_perc},
        {"amixer", "sset", "Master", v_perc},
        "audio_volume_failed",
        "Falha ao definir volume."
    );
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
