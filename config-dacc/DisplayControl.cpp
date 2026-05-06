#include "config-dacc/functions.hpp"
#include "config-dacc/ConfigResult.hpp"

#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

system_result traduzir_display_result(
    const command_result& command,
    const std::string& codigo,
    const std::string& mensagem
) {
    if (command.ok) {
        return config_result::success(mensagem, command.mensagem);
    }
    if (command.mensagem.find("unknown output") != std::string::npos ||
        command.mensagem.find("cannot find output") != std::string::npos) {
        return config_result::error("display_output_not_found", "Saida de video nao encontrada.", command.mensagem);
    }
    if (command.mensagem.find("cannot find mode") != std::string::npos ||
        command.mensagem.find("bad mode") != std::string::npos) {
        return config_result::error("display_mode_unsupported", "Resolucao nao suportada para a saida.", command.mensagem);
    }
    return config_result::error(codigo, mensagem, command.mensagem);
}

} // namespace

std::string obter_tipo_sessao() {
    const char* sessao = getenv("XDG_SESSION_TYPE");
    return sessao ? std::string(sessao) : "unknown";
}

void verificarSessao() {
    std::string sessao_str = obter_tipo_sessao();
    const char* compositor = getenv("XDG_SESSION_DESKTOP");
    std::string comp = compositor ? std::string(compositor) : "unknown";

    std::cout << "Sessão atual: " << sessao_str << "\n";
    if (sessao_str == "wayland") std::cout << " Você está em Wayland.\n";
    else if (sessao_str == "x11") std::cout << "Sessão Xorg detectada.\n";

    std::cout << "Compositor atual: " << comp << "\n";
    if (comp == "gnome") std::cout << " O compositor GNOME detectado.\n";
}

std::vector<DisplayOutput> obter_info_displays() {
    std::vector<DisplayOutput> displays;
    std::string output;
    try {
        output = exec_command("xrandr --verbose");
    } catch (...) {
        return displays;
    }

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

        if (!current_display || displays.empty() || linha.size() <= 2 ||
            linha[0] != ' ' || linha.find("x") == std::string::npos) {
            continue;
        }

        std::stringstream mode_ss(linha);
        std::string token;
        int w = 0;
        int h = 0;
        bool found_res = false;

        while (mode_ss >> token) {
            size_t x_pos = token.find('x');
            if (x_pos == std::string::npos || !std::isdigit(static_cast<unsigned char>(token[0]))) {
                continue;
            }
            try {
                w = std::stoi(token.substr(0, x_pos));
                size_t i_pos = x_pos + 1;
                std::string h_str;
                while (i_pos < token.size() && std::isdigit(static_cast<unsigned char>(token[i_pos]))) {
                    h_str += token[i_pos++];
                }
                h = std::stoi(h_str);
                found_res = true;
                break;
            } catch (...) {
            }
        }

        if (found_res) {
            DisplayMode mode;
            mode.width = w;
            mode.height = h;
            mode.refresh_rate = 60.0f;
            mode.is_current = linha.find("*current") != std::string::npos || linha.find("*") != std::string::npos;
            current_display->modes.push_back(mode);
            if (mode.is_current) current_display->current_mode = mode;
        }
    }
    return displays;
}

void listar_resolucao() {
    std::cout << "Lista de saidas e resolucoes suportadas: \n";
    command_result result = exec_command_args_result({"xrandr"});
    std::cout << result.stdout_output;
}

bool alterarEscala(const std::string& saida, float escala) {
    return alterarEscala_result(saida, escala).ok;
}

system_result alterarEscala_result(const std::string& saida, float escala) {
    std::string sessao = obter_tipo_sessao();
    std::vector<std::string> args;

    if (escala < 0.5f) escala = 0.5f;
    if (escala > 3.0f) escala = 3.0f;

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << escala;
    std::string scale_str = ss.str();

    if (sessao == "x11") {
        args = {"xrandr", "--output", saida, "--scale", scale_str + "x" + scale_str};
    } else if (sessao == "wayland") {
        const char* desktop = getenv("XDG_SESSION_DESKTOP");
        std::string de = desktop ? std::string(desktop) : "";
        if (de.find("gnome") != std::string::npos) {
            int scale_int = static_cast<int>(escala + 0.5f);
            args = {"gsettings", "set", "org.gnome.desktop.interface", "scaling-factor", std::to_string(scale_int)};
        } else {
            args = {"wlr-randr", "--output", saida, "--scale", scale_str};
        }
    } else {
        return config_result::error("display_session_unknown", "Sessao grafica nao suportada.");
    }

    command_result result = exec_command_args_result(args);
    return traduzir_display_result(result, "display_scale_failed", "Falha ao alterar escala.");
}

bool alterarResolucao(const std::string& saida, int width, int height, float rate) {
    return alterarResolucao_result(saida, width, height, rate).ok;
}

system_result alterarResolucao_result(const std::string& saida, int width, int height, float rate) {
    (void)rate;
    std::string sessao = obter_tipo_sessao();
    std::string mode_str = std::to_string(width) + "x" + std::to_string(height);
    std::vector<std::string> args;

    if (sessao == "wayland") {
        args = {"wlr-randr", "--output", saida, "--mode", mode_str};
    } else if (sessao == "x11") {
        args = {"xrandr", "--output", saida, "--mode", mode_str};
    } else {
        return config_result::error("display_session_unknown", "Sessao grafica nao suportada.");
    }

    command_result result = exec_command_args_result(args);
    return traduzir_display_result(result, "display_resolution_failed", "Falha ao alterar resolucao.");
}

void aumentar_brilho() {
    (void)exec_command_args_result({"brightnessctl", "set", "+10%"});
    std::cout << "Brilho aumentado\n";
}

void diminuir_brilho() {
    (void)exec_command_args_result({"brightnessctl", "set", "10%-"});
    std::cout << "Brilho diminuído\n";
}
