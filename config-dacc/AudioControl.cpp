#include "config-dacc/functions.hpp"
#include "config-dacc/AudioParsing.hpp"
#include "config-dacc/ConfigResult.hpp"

#include <algorithm>
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

std::string descrever_tentativas(const std::vector<std::string>& tentativas) {
    std::string detalhes;
    for (const auto& tentativa : tentativas) {
        detalhes += tentativa + "\n";
    }
    return detalhes;
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
    std::vector<device_audio> lista;
    (void)listar_dispositivos_audio_result(lista);
    return lista;
}

system_result listar_dispositivos_audio_result(std::vector<device_audio>& dispositivos) {
    dispositivos.clear();
    std::vector<std::string> tentativas;

    const bool tem_wpctl = comando_existe("wpctl");
    const bool tem_pactl = comando_existe("pactl");
    const bool tem_aplay = comando_existe("aplay");

    if (!tem_wpctl && !tem_pactl && !tem_aplay) {
        return config_result::error(
            "audio_subsystem_missing",
            "Subsistema de audio compativel indisponivel.",
            "wpctl, pactl e aplay ausentes."
        );
    }

    if (tem_wpctl) {
        command_result result = exec_command_args_result({"wpctl", "status"});
        if (result.ok) {
            dispositivos = config_dacc::audio_parsing::parse_wpctl_sinks(result.stdout_output);
            if (!dispositivos.empty()) {
                return config_result::success("Dispositivos de audio listados.", "backend=wpctl");
            }
            tentativas.push_back("wpctl: comando ok, nenhum sink parseado");
        } else {
            tentativas.push_back("wpctl: " + result.mensagem);
        }
    }

    if (tem_pactl) {
        command_result result = exec_command_args_result({"pactl", "list", "sinks", "short"});
        if (result.ok) {
            std::string default_sink;
            command_result default_result = exec_command_args_result({"pactl", "get-default-sink"});
            if (default_result.ok) {
                default_sink = default_result.stdout_output;
            }

            dispositivos = config_dacc::audio_parsing::parse_pactl_sinks_short(result.stdout_output, default_sink);
            if (!dispositivos.empty()) {
                return config_result::success("Dispositivos de audio listados.", "backend=pactl");
            }
            tentativas.push_back("pactl: comando ok, nenhum sink parseado");
        } else {
            tentativas.push_back("pactl: " + result.mensagem);
        }
    }

    if (tem_aplay) {
        command_result result = exec_command_args_result({"aplay", "-l"});
        if (result.ok) {
            dispositivos = config_dacc::audio_parsing::parse_aplay_devices(result.stdout_output);
            if (!dispositivos.empty()) {
                return config_result::success("Dispositivos de audio listados.", "backend=alsa");
            }
            tentativas.push_back("aplay: comando ok, nenhum dispositivo parseado");
        } else {
            tentativas.push_back("aplay: " + result.mensagem);
        }
    }

    return config_result::error(
        "audio_no_devices",
        "Nenhum dispositivo de audio encontrado.",
        descrever_tentativas(tentativas)
    );
}

void selecionar_dispositivo_audio(int id) {
    (void)selecionar_dispositivo_audio_result(id);
}

system_result selecionar_dispositivo_audio_result(int id) {
    std::vector<device_audio> dispositivos;
    system_result lista_result = listar_dispositivos_audio_result(dispositivos);
    if (!lista_result.ok) {
        return lista_result;
    }

    for (const auto& dispositivo : dispositivos) {
        if (dispositivo.id == id) {
            return selecionar_dispositivo_audio_result(dispositivo);
        }
    }

    return config_result::error(
        "audio_device_not_found",
        "Dispositivo de audio nao encontrado.",
        "id=" + std::to_string(id)
    );
}

system_result selecionar_dispositivo_audio_result(const device_audio& dispositivo) {
    if (dispositivo.backend == audio_backend::wpctl) {
        if (!comando_existe("wpctl")) {
            return config_result::error("audio_subsystem_missing", "Subsistema de audio compativel indisponivel.", "wpctl ausente.");
        }
        const std::string id = dispositivo.backend_id.empty() ? std::to_string(dispositivo.id) : dispositivo.backend_id;
        command_result result = exec_command_args_result({"wpctl", "set-default", id});
        return traduzir_audio_result(result, "audio_select_failed", "Falha ao definir dispositivo de audio.");
    }

    if (dispositivo.backend == audio_backend::pactl) {
        if (!comando_existe("pactl")) {
            return config_result::error("audio_subsystem_missing", "Subsistema de audio compativel indisponivel.", "pactl ausente.");
        }
        if (dispositivo.backend_id.empty()) {
            return config_result::error("audio_device_not_found", "Dispositivo de audio nao encontrado.", "backend_id pactl vazio.");
        }
        command_result result = exec_command_args_result({"pactl", "set-default-sink", dispositivo.backend_id});
        return traduzir_audio_result(result, "audio_select_failed", "Falha ao definir dispositivo de audio.");
    }

    return config_result::error(
        "audio_select_unsupported",
        "Selecao de dispositivo nao suportada neste backend de audio.",
        "backend=alsa backend_id=" + dispositivo.backend_id
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
