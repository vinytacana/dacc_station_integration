#include "config-dacc/functions.hpp"
#include "config-dacc/ConfigResult.hpp"
#include "config-dacc/DisplayParsing.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
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

std::vector<std::filesystem::path> listar_backlights() {
    std::vector<std::filesystem::path> backlights;
    const std::filesystem::path root{"/sys/class/backlight"};
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) {
        return backlights;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (!entry.is_directory(ec)) {
            continue;
        }
        const auto brightness = entry.path() / "brightness";
        const auto max_brightness = entry.path() / "max_brightness";
        if (std::filesystem::exists(brightness, ec) &&
            std::filesystem::exists(max_brightness, ec)) {
            backlights.push_back(entry.path());
        }
    }
    return backlights;
}

bool ler_int_arquivo(const std::filesystem::path& path, int& valor) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }
    in >> valor;
    return !in.fail();
}

bool escrever_int_arquivo(const std::filesystem::path& path, int valor) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }
    out << valor;
    return !out.fail();
}

system_result alterar_brilho_sysfs(int delta_percentual) {
    auto backlights = listar_backlights();
    if (backlights.empty()) {
        return config_result::error(
            "backlight_not_supported",
            "Controle de brilho nao suportado neste ambiente.",
            "/sys/class/backlight vazio ou indisponivel."
        );
    }

    std::string detalhes;
    for (const auto& backlight : backlights) {
        int atual = 0;
        int maximo = 0;
        if (!ler_int_arquivo(backlight / "brightness", atual) ||
            !ler_int_arquivo(backlight / "max_brightness", maximo) ||
            maximo <= 0) {
            detalhes += backlight.string() + ": leitura falhou; ";
            continue;
        }

        int passo = std::max(1, (maximo * std::abs(delta_percentual)) / 100);
        int novo = delta_percentual >= 0 ? atual + passo : atual - passo;
        novo = std::max(0, std::min(maximo, novo));

        if (escrever_int_arquivo(backlight / "brightness", novo)) {
            return config_result::success("Brilho alterado.", backlight.string());
        }
        detalhes += backlight.string() + ": escrita falhou; ";
    }

    return config_result::error(
        "backlight_permission_denied",
        "Sem permissao para alterar brilho via backlight.",
        detalhes
    );
}

system_result alterar_brilho(int delta_percentual) {
    if (comando_existe("brightnessctl")) {
        command_result result = exec_command_args_result({
            "brightnessctl",
            "set",
            delta_percentual >= 0 ? "+" + std::to_string(delta_percentual) + "%" : std::to_string(std::abs(delta_percentual)) + "%-"
        });
        if (result.ok) {
            return config_result::success("Brilho alterado.", result.mensagem);
        }
    }

    return alterar_brilho_sysfs(delta_percentual);
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
    const bool preferir_wlr = obter_tipo_sessao() == "wayland";

    if (preferir_wlr && comando_existe("wlr-randr")) {
        command_result result = exec_command_args_result({"wlr-randr"});
        if (result.ok) {
            displays = config_dacc::display_parsing::parse_wlr_randr(result.stdout_output);
            if (!displays.empty()) {
                return displays;
            }
        }
    }

    if (comando_existe("xrandr")) {
        command_result result = exec_command_args_result({"xrandr", "--verbose"});
        if (result.ok) {
            displays = config_dacc::display_parsing::parse_xrandr_verbose(result.stdout_output);
            if (!displays.empty()) {
                return displays;
            }
        }
    }

    if (!preferir_wlr && comando_existe("wlr-randr")) {
        command_result result = exec_command_args_result({"wlr-randr"});
        if (result.ok) {
            return config_dacc::display_parsing::parse_wlr_randr(result.stdout_output);
        }
    }

    return displays;
}

void listar_resolucao() {
    std::cout << "Lista de saidas e resolucoes suportadas: \n";
    std::vector<std::string> args;
    if (comando_existe("xrandr")) {
        args = {"xrandr"};
    } else if (comando_existe("wlr-randr")) {
        args = {"wlr-randr"};
    } else {
        std::cout << "Nenhuma ferramenta de display disponivel.\n";
        return;
    }
    command_result result = exec_command_args_result(args);
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
        if (!comando_existe("xrandr")) {
            return config_result::error("feature_unavailable", "Controle de escala indisponivel.", "xrandr ausente.");
        }
        args = {"xrandr", "--output", saida, "--scale", scale_str + "x" + scale_str};
    } else if (sessao == "wayland") {
        const char* desktop = getenv("XDG_SESSION_DESKTOP");
        std::string de = desktop ? std::string(desktop) : "";
        if (de.find("gnome") != std::string::npos) {
            if (!comando_existe("gsettings")) {
                return config_result::error("feature_unavailable", "Controle de escala indisponivel.", "gsettings ausente.");
            }
            int scale_int = static_cast<int>(escala + 0.5f);
            args = {"gsettings", "set", "org.gnome.desktop.interface", "scaling-factor", std::to_string(scale_int)};
        } else {
            if (!comando_existe("wlr-randr")) {
                return config_result::error("feature_unavailable", "Controle de escala indisponivel.", "wlr-randr ausente.");
            }
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
        if (!comando_existe("wlr-randr")) {
            return config_result::error("feature_unavailable", "Controle de resolucao indisponivel.", "wlr-randr ausente.");
        }
        args = {"wlr-randr", "--output", saida, "--mode", mode_str};
    } else if (sessao == "x11") {
        if (!comando_existe("xrandr")) {
            return config_result::error("feature_unavailable", "Controle de resolucao indisponivel.", "xrandr ausente.");
        }
        args = {"xrandr", "--output", saida, "--mode", mode_str};
    } else {
        return config_result::error("display_session_unknown", "Sessao grafica nao suportada.");
    }

    command_result result = exec_command_args_result(args);
    return traduzir_display_result(result, "display_resolution_failed", "Falha ao alterar resolucao.");
}

void aumentar_brilho() {
    (void)aumentar_brilho_result();
}

void diminuir_brilho() {
    (void)diminuir_brilho_result();
}

system_result aumentar_brilho_result() {
    return alterar_brilho(10);
}

system_result diminuir_brilho_result() {
    return alterar_brilho(-10);
}
