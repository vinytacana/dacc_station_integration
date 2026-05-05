#include "functions.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <limits>
#include <thread>
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <cstdio>
#include <iomanip>
#include <mutex>

using namespace std;

// Mutex para proteger concorrência
static std::mutex g_audio_mutex;

// ==========================================
// --- UTILITÁRIOS ---
// ==========================================

long long obter_tempo_ms() {
    auto agora = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(agora.time_since_epoch()).count();
}

int obter_bateria() {
    vector<string> caminhos = {
        "/sys/class/power_supply/BAT0/capacity",
        "/sys/class/power_supply/BAT1/capacity"
    };
    for (const auto &path : caminhos) {
        ifstream arquivo(path);
        if (arquivo.is_open()) {
            int porcentagem;
            arquivo >> porcentagem;
            return porcentagem;
        }
    }
    return -1;
}

// ==========================================
// --- ÁUDIO ---
// ==========================================

namespace {

system_result make_audio_error(const std::string& codigo, const std::string& mensagem, const std::string& detalhes = "") {
    system_result result;
    result.ok = false;
    result.codigo = codigo;
    result.mensagem = mensagem;
    result.detalhes = detalhes;
    return result;
}

system_result make_audio_success(const std::string& mensagem, const std::string& detalhes = "") {
    system_result result;
    result.ok = true;
    result.codigo = "ok";
    result.mensagem = mensagem;
    result.detalhes = detalhes;
    return result;
}

system_result make_display_error(const std::string& codigo, const std::string& mensagem, const std::string& detalhes = "") {
    system_result result;
    result.ok = false;
    result.codigo = codigo;
    result.mensagem = mensagem;
    result.detalhes = detalhes;
    return result;
}

system_result make_display_success(const std::string& mensagem, const std::string& detalhes = "") {
    system_result result;
    result.ok = true;
    result.codigo = "ok";
    result.mensagem = mensagem;
    result.detalhes = detalhes;
    return result;
}

system_result traduzir_audio_result(const command_result& command, const std::string& codigo, const std::string& mensagem) {
    if (command.ok) {
        return make_audio_success(mensagem, command.mensagem);
    }
    if (command.mensagem.find("No such entity") != std::string::npos ||
        command.mensagem.find("not found") != std::string::npos) {
        return make_audio_error("audio_device_not_found", "Dispositivo de audio nao encontrado.", command.mensagem);
    }
    return make_audio_error(codigo, mensagem, command.mensagem);
}

system_result traduzir_display_result(const command_result& command, const std::string& codigo, const std::string& mensagem) {
    if (command.ok) {
        return make_display_success(mensagem, command.mensagem);
    }
    if (command.mensagem.find("unknown output") != std::string::npos ||
        command.mensagem.find("cannot find output") != std::string::npos) {
        return make_display_error("display_output_not_found", "Saida de video nao encontrada.", command.mensagem);
    }
    if (command.mensagem.find("cannot find mode") != std::string::npos ||
        command.mensagem.find("bad mode") != std::string::npos) {
        return make_display_error("display_mode_unsupported", "Resolucao nao suportada para a saida.", command.mensagem);
    }
    return make_display_error(codigo, mensagem, command.mensagem);
}

} // namespace

std::string limpar_nome_audio(std::string raw) {
    size_t colchete = raw.find('[');
    if (colchete != std::string::npos) {
        raw = raw.substr(0, colchete);
    }
    while (!raw.empty() && isspace(raw.back())) {
        raw.pop_back();
    }
    return raw;
}

bool executar_comando_audio(const string &cmd_wp, const string &cmd_pa, const string &cmd_alsa) {
    std::thread([cmd_wp, cmd_pa, cmd_alsa]() {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        bool sucesso = false;
        if (comando_existe("wpctl")) {
            if (system((cmd_wp + " > /dev/null 2>&1").c_str()) == 0) sucesso = true;
        }
        if (!sucesso && comando_existe("pactl")) {
            if (system((cmd_pa + " > /dev/null 2>&1").c_str()) == 0) sucesso = true;
        }
        if (!sucesso && comando_existe("amixer")) {
            system((cmd_alsa + " > /dev/null 2>&1").c_str());
        } 
    }).detach();
    return true;
}

void aumentar_volume() {
    executar_comando_audio("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%+",
                           "pactl set-sink-volume @DEFAULT_SINK@ +5%",
                           "amixer sset Master 5%+");
}

void diminuir_volume() {
    executar_comando_audio("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-",
                           "pactl set-sink-volume @DEFAULT_SINK@ -5%",
                           "amixer sset Master 5%-");
}

void definir_volume(int valor_int) {
    if (valor_int > 100) valor_int = 100;
    if (valor_int < 0) valor_int = 0;
    float valor_float = static_cast<float>(valor_int) / 100.0f;
    string v_str = to_string(valor_float);
    std::replace(v_str.begin(), v_str.end(), ',', '.');
    string v_perc = to_string(valor_int) + "%";

    executar_comando_audio("wpctl set-volume @DEFAULT_AUDIO_SINK@ " + v_str,
                           "pactl set-sink-volume @DEFAULT_SINK@ " + v_perc,
                           "amixer sset Master " + v_perc);
}

int obter_volume_atual() {
    try {
        std::string saida = exec_command("wpctl get-volume @DEFAULT_AUDIO_SINK@");
        size_t pos = saida.find("Volume: ");
        if (pos != std::string::npos) {
            std::string volStr = saida.substr(pos + 8);
            return static_cast<int>(std::stof(volStr) * 100);
        }
    } catch (...) {}
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
        if (na_secao_sinks && (linha.find("Sources:") != std::string::npos || linha.find("Filters:") != std::string::npos || linha.empty())) {
            break;
        }
        if (na_secao_sinks) {
            size_t pontoPos = linha.find('.');
            if (pontoPos != std::string::npos && pontoPos > 0 && isdigit(linha[pontoPos-1])) {
                device_audio dev;
                dev.padrao = (linha.find('*') != std::string::npos);
                size_t inicioNum = pontoPos - 1;
                while (inicioNum > 0 && isdigit(linha[inicioNum-1])) inicioNum--;
                
                try {
                    dev.id = std::stoi(linha.substr(inicioNum, pontoPos - inicioNum));
                    if (pontoPos + 2 < linha.size()) {
                        dev.descricao = limpar_nome_audio(linha.substr(pontoPos + 2));
                        lista.push_back(dev);
                    }
                } catch (...) { continue; }
            }
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

// ==========================================
// --- VÍDEO ---
// ==========================================

std::string obter_tipo_sessao() {
    const char *sessao = getenv("XDG_SESSION_TYPE");
    return sessao ? std::string(sessao) : "unknown";
}

void verificarSessao() {
    std::string sessaoStr = obter_tipo_sessao();
    const char *compositor = getenv("XDG_SESSION_DESKTOP");
    string comp = compositor ? std::string(compositor) : "unknown";

    cout << "Sessão atual: " << sessaoStr << "\n";
    if (sessaoStr == "wayland") cout << " Você está em Wayland.\n";
    else if (sessaoStr == "x11") cout << "Sessão Xorg detectada.\n";
    
    cout << "Compositor atual: " << comp << "\n";
    if (comp == "gnome") cout << " O compositor GNOME detectado.\n";
}

std::vector<DisplayOutput> obter_info_displays() {
    std::vector<DisplayOutput> displays;
    std::string output;
    try {
        output = exec_command("xrandr --verbose");
    } catch (...) { return displays; }

    std::stringstream ss(output);
    std::string linha;
    DisplayOutput *currentDisplay = nullptr;

    while (std::getline(ss, linha)) {
        if (linha.find(" connected ") != std::string::npos) {
            DisplayOutput disp;
            std::stringstream lineSS(linha);
            lineSS >> disp.name;
            disp.connected = true;
            disp.current_scale = 1.0f;
            displays.push_back(disp);
            currentDisplay = &displays.back();
            continue;
        }

        if (currentDisplay && !displays.empty() && linha.size() > 2 && linha[0] == ' ' && linha.find("x") != std::string::npos) {
            std::stringstream modeSS(linha);
            std::string token;
            int w = 0, h = 0;
            bool foundRes = false;

            while (modeSS >> token) {
                size_t xPos = token.find('x');
                if (xPos != std::string::npos && isdigit(token[0])) {
                    try {
                        w = std::stoi(token.substr(0, xPos));
                        size_t iPos = xPos + 1;
                        std::string hStr;
                        while (iPos < token.size() && isdigit(token[iPos])) hStr += token[iPos++];
                        h = std::stoi(hStr);
                        foundRes = true;
                        break;
                    } catch (...) {}
                }
            }

            if (foundRes) {
                DisplayMode mode;
                mode.width = w;
                mode.height = h;
                mode.refresh_rate = 60.0f;
                mode.is_current = (linha.find("*current") != std::string::npos || linha.find("*") != std::string::npos);
                currentDisplay->modes.push_back(mode);
                if (mode.is_current) currentDisplay->current_mode = mode;
            }
        }
    }
    return displays;
}

void listar_resolucao() {
    cout << "Lista de saidas e resolucoes suportadas: \n";
    system("xrandr");
}

bool alterarEscala(const string &saida, float escala) {
    return alterarEscala_result(saida, escala).ok;
}

system_result alterarEscala_result(const string &saida, float escala) {
    std::string sessao = obter_tipo_sessao();
    std::vector<std::string> args;

    if (escala < 0.5f) escala = 0.5f;
    if (escala > 3.0f) escala = 3.0f;

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << escala;
    std::string scaleStr = ss.str();

    if (sessao == "x11") {
        args = {"xrandr", "--output", saida, "--scale", scaleStr + "x" + scaleStr};
    } else if (sessao == "wayland") {
        const char *desktop = getenv("XDG_SESSION_DESKTOP");
        std::string de = desktop ? std::string(desktop) : "";
        if (de.find("gnome") != std::string::npos) {
            int scaleInt = static_cast<int>(escala + 0.5f);
            args = {"gsettings", "set", "org.gnome.desktop.interface", "scaling-factor", std::to_string(scaleInt)};
        } else {
            args = {"wlr-randr", "--output", saida, "--scale", scaleStr};
        }
    } else {
        return make_display_error("display_session_unknown", "Sessao grafica nao suportada.");
    }

    command_result result = exec_command_args_result(args);
    return traduzir_display_result(result, "display_scale_failed", "Falha ao alterar escala.");
}

bool alterarResolucao(const string &saida, int width, int height, float rate) {
    return alterarResolucao_result(saida, width, height, rate).ok;
}

system_result alterarResolucao_result(const string &saida, int width, int height, float rate) {
    (void)rate;
    std::string sessao = obter_tipo_sessao();
    std::string modeStr = std::to_string(width) + "x" + std::to_string(height);
    std::vector<std::string> args;
    if (sessao == "wayland") {
        args = {"wlr-randr", "--output", saida, "--mode", modeStr};
    } else if (sessao == "x11") {
        args = {"xrandr", "--output", saida, "--mode", modeStr};
    } else {
        return make_display_error("display_session_unknown", "Sessao grafica nao suportada.");
    }

    command_result result = exec_command_args_result(args);
    return traduzir_display_result(result, "display_resolution_failed", "Falha ao alterar resolucao.");
}

void aumentar_brilho() {
    system("brightnessctl set +10% > /dev/null 2>&1");
    cout << "Brilho aumentado\n";
}

void diminuir_brilho() {
    system("brightnessctl set 10%- > /dev/null 2>&1");
    cout << "Brilho diminuído\n";
}

// ==========================================
// --- WI-FI ---
// ==========================================

namespace {

system_result make_wifi_error(const std::string& codigo, const std::string& mensagem, const std::string& detalhes = "") {
    system_result result;
    result.ok = false;
    result.codigo = codigo;
    result.mensagem = mensagem;
    result.detalhes = detalhes;
    return result;
}

system_result make_wifi_success(const std::string& mensagem, const std::string& detalhes = "") {
    system_result result;
    result.ok = true;
    result.codigo = "ok";
    result.mensagem = mensagem;
    result.detalhes = detalhes;
    return result;
}

std::vector<std::string> split_nmcli_escaped_fields(const std::string& linha) {
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

system_result traduzir_wifi_result(
    const command_result& command,
    const std::string& codigo_falha,
    const std::string& mensagem_falha
) {
    const std::string& output = command.mensagem;
    if (command.ok) {
        return make_wifi_success("Operacao Wi-Fi executada com sucesso.", output);
    }
    if (output.find("Secrets were required") != std::string::npos ||
        output.find("No password") != std::string::npos) {
        return make_wifi_error("wifi_password_required", "A rede exige senha.", output);
    }
    if (output.find("wrong password") != std::string::npos ||
        output.find("invalid secrets") != std::string::npos) {
        return make_wifi_error("wifi_auth_failed", "Senha incorreta ou autenticacao recusada.", output);
    }
    if (output.find("No network with SSID") != std::string::npos) {
        return make_wifi_error("wifi_not_found", "Rede Wi-Fi nao encontrada.", output);
    }
    if (output.find("Wi-Fi is disabled") != std::string::npos ||
        output.find("radio is disabled") != std::string::npos) {
        return make_wifi_error("wifi_disabled", "Wi-Fi desativado.", output);
    }
    if (output.find("not running") != std::string::npos) {
        return make_wifi_error("network_manager_unavailable", "NetworkManager indisponivel.", output);
    }
    return make_wifi_error(codigo_falha, mensagem_falha, output);
}

} // namespace

void listar_wifi() {
    system("nmcli device wifi list");
}

std::vector<wifi_network> listar_wifi_parsed() {
    std::vector<wifi_network> redes;
    std::string saida;
    try {
        saida = exec_command("nmcli -t -f IN-USE,SSID,SIGNAL,SECURITY device wifi list");
    } catch (...) { return redes; }

    std::stringstream ss(saida);
    std::string linha;

    while (std::getline(ss, linha)) {
        std::vector<std::string> campos = split_nmcli_escaped_fields(linha);

        if (campos.size() >= 4) {
            wifi_network net;
            net.em_uso = (campos[0] == "*");
            net.ssid = campos[1];
            try { net.sinal = std::stoi(campos[2]); } catch (...) { net.sinal = 0; }
            net.seguranca = campos[3];
            if (!net.ssid.empty()) redes.push_back(net);
        }
    }
    return redes;
}

wifi_adapter_status obter_status_wifi() {
    wifi_adapter_status status;
    command_result result = exec_command_args_result({"nmcli", "radio", "wifi"});
    status.output = result.mensagem;
    if (!result.ok) {
        status.disponivel = false;
        return status;
    }

    std::string output = result.stdout_output;
    output.erase(std::remove(output.begin(), output.end(), '\n'), output.end());
    output.erase(std::remove(output.begin(), output.end(), '\r'), output.end());
    status.enabled = (output == "enabled");

    network_connection_status conexao = obter_status_conexao_rede();
    status.conectado_wifi = conexao.wifi_conectado;
    status.conectado_cabeado = conexao.cabeado_conectado;
    if (conexao.wifi_conectado) {
        status.dispositivo_wifi = conexao.dispositivo_wifi;
        status.conexao_wifi = conexao.conexao_wifi;
    }
    if (conexao.cabeado_conectado) {
        status.dispositivo_cabeado = conexao.dispositivo_cabeado;
        status.conexao_cabeada = conexao.conexao_cabeada;
    }
    return status;
}

network_connection_status obter_status_conexao_rede() {
    network_connection_status status;
    command_result result = exec_command_args_result(
        {"nmcli", "-t", "-f", "TYPE,DEVICE,STATE,CONNECTION", "device", "status"}
    );
    if (!result.ok) {
        return status;
    }

    std::stringstream ss(result.stdout_output);
    std::string linha;
    while (std::getline(ss, linha)) {
        auto campos = split_nmcli_escaped_fields(linha);
        if (campos.size() < 4 || campos[2] != "connected") {
            continue;
        }

        if (campos[0] == "ethernet") {
            status.conectado = true;
            status.cabeado_conectado = true;
            status.dispositivo_cabeado = campos[1];
            status.conexao_cabeada = campos[3];
            if (status.tipo.empty() || status.tipo == "wifi") {
                status.tipo = "ethernet";
                status.dispositivo = campos[1];
                status.conexao = campos[3];
            }
            continue;
        }

        if (campos[0] == "wifi" && !status.wifi_conectado) {
            status.conectado = true;
            status.wifi_conectado = true;
            status.dispositivo_wifi = campos[1];
            status.conexao_wifi = campos[3];
            if (status.tipo.empty()) {
                status.tipo = "wifi";
                status.dispositivo = campos[1];
                status.conexao = campos[3];
            }
        }
    }
    return status;
}

bool wifi_conectado() {
    return obter_status_conexao_rede().wifi_conectado;
}

system_result definir_estado_wifi_result(bool ligar) {
    command_result result = exec_command_args_result(
        {"nmcli", "radio", "wifi", ligar ? "on" : "off"}
    );
    system_result traduzido = traduzir_wifi_result(
        result,
        "wifi_toggle_failed",
        ligar ? "Falha ao ativar o Wi-Fi." : "Falha ao desativar o Wi-Fi."
    );
    if (!traduzido.ok) {
        return traduzido;
    }

    wifi_adapter_status status = obter_status_wifi();
    if (status.disponivel && status.enabled == ligar) {
        return make_wifi_success(ligar ? "Wi-Fi ativado." : "Wi-Fi desativado.", result.mensagem);
    }
    return make_wifi_error(
        "wifi_state_mismatch",
        "O estado do Wi-Fi nao refletiu a solicitacao.",
        status.output
    );
}

void conectar_wifi(const string &ssid, const string &senha) {
    (void)conectar_wifi_result(ssid, senha);
}

system_result conectar_wifi_result(const string &ssid, const string &senha) {
    std::vector<std::string> args = {"nmcli", "device", "wifi", "connect", ssid};
    if (!senha.empty()) {
        args.push_back("password");
        args.push_back(senha);
    }
    command_result result = exec_command_args_result(args);
    return traduzir_wifi_result(result, "wifi_connect_failed", "Falha ao conectar na rede Wi-Fi.");
}

void desconectar_wifi(const string &id) {
    (void)desconectar_wifi_result(id);
}

system_result desconectar_wifi_result(const string &id) {
    command_result result = exec_command_args_result({"nmcli", "connection", "down", "id", id});
    return traduzir_wifi_result(result, "wifi_disconnect_failed", "Falha ao desconectar a rede Wi-Fi.");
}
