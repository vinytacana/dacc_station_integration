#include "config-dacc/functions.hpp"
#include "config-dacc/ConfigResult.hpp"
#include "config-dacc/DisplayParsing.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::mutex g_display_mutex;
std::mutex g_brightness_mutex;
std::vector<DisplayOutput> g_display_cache;
system_result g_display_cache_result;
std::chrono::steady_clock::time_point g_display_cache_atualizado;
bool g_display_cache_valido = false;
int g_brightness_cache_valor = 0;
system_result g_brightness_cache_result;
std::chrono::steady_clock::time_point g_brightness_cache_atualizado;
bool g_brightness_cache_valido = false;
constexpr auto DISPLAY_CACHE_TTL = std::chrono::seconds(3);
constexpr auto BRIGHTNESS_CACHE_TTL = std::chrono::seconds(2);

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

void salvar_cache_display(const system_result& result, const std::vector<DisplayOutput>& displays) {
    g_display_cache_result = result;
    g_display_cache = displays;
    g_display_cache_atualizado = std::chrono::steady_clock::now();
    g_display_cache_valido = true;
}

bool tentar_cache_display(std::vector<DisplayOutput>& displays, system_result& result) {
    if (!g_display_cache_valido) {
        return false;
    }
    if (std::chrono::steady_clock::now() - g_display_cache_atualizado > DISPLAY_CACHE_TTL) {
        return false;
    }
    displays = g_display_cache;
    result = g_display_cache_result;
    return true;
}

void invalidar_cache_display() {
    std::lock_guard<std::mutex> lock(g_display_mutex);
    g_display_cache_valido = false;
}

void salvar_cache_brilho(const system_result& result, int brilho) {
    g_brightness_cache_result = result;
    g_brightness_cache_valor = brilho;
    g_brightness_cache_atualizado = std::chrono::steady_clock::now();
    g_brightness_cache_valido = true;
}

bool tentar_cache_brilho(int& brilho, system_result& result) {
    if (!g_brightness_cache_valido) {
        return false;
    }
    if (std::chrono::steady_clock::now() - g_brightness_cache_atualizado > BRIGHTNESS_CACHE_TTL) {
        return false;
    }
    brilho = g_brightness_cache_valor;
    result = g_brightness_cache_result;
    return true;
}

void invalidar_cache_brilho() {
    std::lock_guard<std::mutex> lock(g_brightness_mutex);
    g_brightness_cache_valido = false;
}

std::string descrever_tentativas(const std::vector<std::string>& tentativas) {
    std::string detalhes;
    for (const auto& tentativa : tentativas) {
        detalhes += tentativa + "\n";
    }
    return detalhes;
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

bool ler_brilho_sysfs(int& brilho, std::string& detalhes) {
    auto backlights = listar_backlights();
    if (backlights.empty()) {
        detalhes = "/sys/class/backlight vazio ou indisponivel.";
        return false;
    }

    for (const auto& backlight : backlights) {
        int atual = 0;
        int maximo = 0;
        if (!ler_int_arquivo(backlight / "brightness", atual) ||
            !ler_int_arquivo(backlight / "max_brightness", maximo) ||
            maximo <= 0) {
            detalhes += backlight.string() + ": leitura falhou; ";
            continue;
        }

        brilho = std::max(0, std::min(100, (atual * 100) / maximo));
        detalhes = backlight.string();
        return true;
    }

    return false;
}

bool definir_brilho_sysfs(int valor, std::string& detalhes) {
    auto backlights = listar_backlights();
    if (backlights.empty()) {
        detalhes = "/sys/class/backlight vazio ou indisponivel.";
        return false;
    }

    valor = std::max(0, std::min(100, valor));
    for (const auto& backlight : backlights) {
        int maximo = 0;
        if (!ler_int_arquivo(backlight / "max_brightness", maximo) || maximo <= 0) {
            detalhes += backlight.string() + ": leitura de max_brightness falhou; ";
            continue;
        }

        int novo = std::max(0, std::min(maximo, (maximo * valor) / 100));
        if (valor > 0 && novo == 0) {
            novo = 1;
        }
        if (escrever_int_arquivo(backlight / "brightness", novo)) {
            detalhes = backlight.string();
            return true;
        }
        detalhes += backlight.string() + ": escrita falhou; ";
    }

    return false;
}

bool parse_int_command_output(const std::string& output, int& valor) {
    try {
        valor = std::stoi(output);
        return true;
    } catch (...) {
        return false;
    }
}

bool obter_brilho_brightnessctl(int& brilho, std::string& detalhes) {
    if (comando_existe("brightnessctl")) {
        command_result atual_result = exec_command_args_result({"brightnessctl", "get"});
        command_result max_result = exec_command_args_result({"brightnessctl", "max"});
        int atual = 0;
        int maximo = 0;
        if (atual_result.ok && max_result.ok &&
            parse_int_command_output(atual_result.stdout_output, atual) &&
            parse_int_command_output(max_result.stdout_output, maximo) &&
            maximo > 0) {
            brilho = std::max(0, std::min(100, (atual * 100) / maximo));
            detalhes = "backend=brightnessctl";
            return true;
        }
        detalhes = atual_result.mensagem + max_result.mensagem;
        return false;
    }

    detalhes = "brightnessctl ausente.";
    return false;
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
    (void)listar_displays_result(displays);
    return displays;
}

system_result listar_displays_result(std::vector<DisplayOutput>& displays) {
    std::lock_guard<std::mutex> lock(g_display_mutex);
    displays.clear();

    system_result cache_result;
    if (tentar_cache_display(displays, cache_result)) {
        return cache_result;
    }

    const bool preferir_wlr = obter_tipo_sessao() == "wayland";
    const bool tem_xrandr = comando_existe("xrandr");
    const bool tem_wlr = comando_existe("wlr-randr");
    std::vector<std::string> tentativas;

    if (!tem_xrandr && !tem_wlr) {
        system_result result = config_result::error(
            "display_subsystem_missing",
            "Subsistema de display indisponivel.",
            "xrandr e wlr-randr ausentes."
        );
        salvar_cache_display(result, displays);
        return result;
    }

    if (preferir_wlr && tem_wlr) {
        command_result result = exec_command_args_result({"wlr-randr"});
        if (result.ok) {
            displays = config_dacc::display_parsing::parse_wlr_randr(result.stdout_output);
            if (!displays.empty()) {
                system_result sucesso = config_result::success("Displays listados.", "backend=wlr-randr");
                salvar_cache_display(sucesso, displays);
                return sucesso;
            }
            tentativas.push_back("wlr-randr: comando ok, nenhum display parseado");
        } else {
            tentativas.push_back("wlr-randr: " + result.mensagem);
        }
    }

    if (tem_xrandr) {
        command_result result = exec_command_args_result({"xrandr", "--verbose"});
        if (result.ok) {
            displays = config_dacc::display_parsing::parse_xrandr_verbose(result.stdout_output);
            if (!displays.empty()) {
                system_result sucesso = config_result::success("Displays listados.", "backend=xrandr");
                salvar_cache_display(sucesso, displays);
                return sucesso;
            }
            tentativas.push_back("xrandr: comando ok, nenhum display parseado");
        } else {
            tentativas.push_back("xrandr: " + result.mensagem);
        }
    }

    if (!preferir_wlr && tem_wlr) {
        command_result result = exec_command_args_result({"wlr-randr"});
        if (result.ok) {
            displays = config_dacc::display_parsing::parse_wlr_randr(result.stdout_output);
            if (!displays.empty()) {
                system_result sucesso = config_result::success("Displays listados.", "backend=wlr-randr");
                salvar_cache_display(sucesso, displays);
                return sucesso;
            }
            tentativas.push_back("wlr-randr: comando ok, nenhum display parseado");
        } else {
            tentativas.push_back("wlr-randr: " + result.mensagem);
        }
    }

    system_result result = config_result::error(
        "display_no_outputs",
        "Nenhum display encontrado.",
        descrever_tentativas(tentativas)
    );
    salvar_cache_display(result, displays);
    return result;
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
    system_result traduzido = traduzir_display_result(result, "display_scale_failed", "Falha ao alterar escala.");
    if (traduzido.ok) invalidar_cache_display();
    return traduzido;
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
    system_result traduzido = traduzir_display_result(result, "display_resolution_failed", "Falha ao alterar resolucao.");
    if (traduzido.ok) invalidar_cache_display();
    return traduzido;
}

void aumentar_brilho() {
    (void)aumentar_brilho_result();
}

void diminuir_brilho() {
    (void)diminuir_brilho_result();
}

system_result aumentar_brilho_result() {
    return alterar_brilho_result(10);
}

system_result diminuir_brilho_result() {
    return alterar_brilho_result(-10);
}

system_result obter_brilho_result(int& brilho) {
    std::lock_guard<std::mutex> lock(g_brightness_mutex);
    system_result cache_result;
    if (tentar_cache_brilho(brilho, cache_result)) {
        return cache_result;
    }

    std::vector<std::string> tentativas;
    std::string detalhes;
    if (obter_brilho_brightnessctl(brilho, detalhes)) {
        system_result result = config_result::success("Brilho obtido.", detalhes);
        salvar_cache_brilho(result, brilho);
        return result;
    }
    tentativas.push_back("brightnessctl: " + detalhes);

    if (ler_brilho_sysfs(brilho, detalhes)) {
        system_result result = config_result::success("Brilho obtido.", "backend=sysfs " + detalhes);
        salvar_cache_brilho(result, brilho);
        return result;
    }
    tentativas.push_back("sysfs: " + detalhes);

    system_result result = config_result::error(
        "brightness_not_supported",
        "Controle de brilho nao suportado neste ambiente.",
        descrever_tentativas(tentativas)
    );
    salvar_cache_brilho(result, brilho);
    return result;
}

system_result definir_brilho_result(int valor) {
    valor = std::max(0, std::min(100, valor));
    std::vector<std::string> tentativas;

    if (comando_existe("brightnessctl")) {
        command_result result = exec_command_args_result({"brightnessctl", "set", std::to_string(valor) + "%"});
        if (result.ok) {
            invalidar_cache_brilho();
            return config_result::success("Brilho definido.", result.mensagem);
        }
        tentativas.push_back("brightnessctl: " + result.mensagem);
    } else {
        tentativas.push_back("brightnessctl: ausente");
    }

    std::string detalhes;
    if (definir_brilho_sysfs(valor, detalhes)) {
        invalidar_cache_brilho();
        return config_result::success("Brilho definido.", "backend=sysfs " + detalhes);
    }
    tentativas.push_back("sysfs: " + detalhes);

    return config_result::error(
        "brightness_not_supported",
        "Controle de brilho nao suportado neste ambiente.",
        descrever_tentativas(tentativas)
    );
}

system_result alterar_brilho_result(int delta) {
    if (delta == 0) {
        int atual = 0;
        system_result result = obter_brilho_result(atual);
        if (!result.ok) {
            return result;
        }
        return config_result::success("Brilho mantido.", "delta=0");
    }

    std::vector<std::string> tentativas;
    if (comando_existe("brightnessctl")) {
        const std::string arg = delta >= 0
            ? "+" + std::to_string(delta) + "%"
            : std::to_string(std::abs(delta)) + "%-";
        command_result result = exec_command_args_result({"brightnessctl", "set", arg});
        if (result.ok) {
            invalidar_cache_brilho();
            return config_result::success("Brilho alterado.", result.mensagem);
        }
        tentativas.push_back("brightnessctl: " + result.mensagem);
    } else {
        tentativas.push_back("brightnessctl: ausente");
    }

    int atual = 0;
    std::string detalhes;
    if (ler_brilho_sysfs(atual, detalhes)) {
        int novo = std::max(0, std::min(100, atual + delta));
        if (definir_brilho_sysfs(novo, detalhes)) {
            invalidar_cache_brilho();
            return config_result::success("Brilho alterado.", "backend=sysfs " + detalhes);
        }
        tentativas.push_back("sysfs escrita: " + detalhes);
    } else {
        tentativas.push_back("sysfs leitura: " + detalhes);
    }

    return config_result::error(
        "brightness_not_supported",
        "Controle de brilho nao suportado neste ambiente.",
        descrever_tentativas(tentativas)
    );
}
