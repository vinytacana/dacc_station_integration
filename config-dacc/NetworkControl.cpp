#include "config-dacc/functions.hpp"
#include "config-dacc/ConfigResult.hpp"
#include "config-dacc/NetworkParsing.hpp"
#include "config-dacc/ErrorCodes.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace err = config_dacc::errors;

std::mutex g_wifi_mutex;
std::vector<wifi_network> g_wifi_cache_redes;
system_result g_wifi_cache_result;
std::chrono::steady_clock::time_point g_wifi_cache_atualizado;
bool g_wifi_cache_valido = false;
constexpr auto WIFI_SCAN_CACHE_TTL = std::chrono::seconds(8);

system_result network_manager_missing_result() {
    return config_result::error(
        err::NETWORK_MANAGER_MISSING,
        "NetworkManager/nmcli indisponivel.",
        "nmcli ausente no PATH."
    );
}

bool ler_primeira_linha(const std::filesystem::path& path, std::string& valor) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }
    std::getline(in, valor);
    return !valor.empty();
}

bool interface_ativa_sysfs(const std::filesystem::path& interface_path) {
    std::string operstate;
    if (ler_primeira_linha(interface_path / "operstate", operstate) && operstate == "up") {
        return true;
    }

    std::string carrier;
    return ler_primeira_linha(interface_path / "carrier", carrier) && carrier == "1";
}

network_connection_status obter_status_conexao_rede_sysfs() {
    network_connection_status status;
    const std::filesystem::path root{"/sys/class/net"};
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) {
        return status;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec) {
            break;
        }
        const std::string nome = entry.path().filename().string();
        if (nome == "lo" || !interface_ativa_sysfs(entry.path())) {
            continue;
        }

        const bool wifi = std::filesystem::exists(entry.path() / "wireless", ec);
        status.conectado = true;
        if (wifi && !status.wifi_conectado) {
            status.wifi_conectado = true;
            status.dispositivo_wifi = nome;
            status.conexao_wifi = nome;
            if (status.tipo.empty()) {
                status.tipo = "wifi";
                status.dispositivo = nome;
                status.conexao = nome;
            }
            continue;
        }

        if (!wifi && !status.cabeado_conectado) {
            status.cabeado_conectado = true;
            status.dispositivo_cabeado = nome;
            status.conexao_cabeada = nome;
            if (status.tipo.empty() || status.tipo == "wifi") {
                status.tipo = "ethernet";
                status.dispositivo = nome;
                status.conexao = nome;
            }
        }
    }

    return status;
}

void salvar_cache_wifi(const system_result& result, const std::vector<wifi_network>& redes) {
    g_wifi_cache_result = result;
    g_wifi_cache_redes = redes;
    g_wifi_cache_atualizado = std::chrono::steady_clock::now();
    g_wifi_cache_valido = true;
}

bool tentar_cache_wifi(std::vector<wifi_network>& redes, system_result& result) {
    if (!g_wifi_cache_valido) {
        return false;
    }
    if (std::chrono::steady_clock::now() - g_wifi_cache_atualizado > WIFI_SCAN_CACHE_TTL) {
        return false;
    }
    redes = g_wifi_cache_redes;
    result = g_wifi_cache_result;
    return true;
}

void invalidar_cache_wifi() {
    std::lock_guard<std::mutex> lock(g_wifi_mutex);
    g_wifi_cache_valido = false;
}

system_result traduzir_wifi_result(
    const command_result& command,
    const std::string& codigo_falha,
    const std::string& mensagem_falha
) {
    const std::string& output = command.mensagem;
    if (command.ok) {
        return config_result::success("Operacao Wi-Fi executada com sucesso.", output);
    }
    if (output.find("Secrets were required") != std::string::npos ||
        output.find("No password") != std::string::npos) {
        return config_result::error(err::WIFI_PASSWORD_REQUIRED, "A rede exige senha.", output);
    }
    if (output.find("wrong password") != std::string::npos ||
        output.find("invalid secrets") != std::string::npos) {
        return config_result::error(err::WIFI_AUTH_FAILED, "Senha incorreta ou autenticacao recusada.", output);
    }
    if (output.find("No network with SSID") != std::string::npos) {
        return config_result::error(err::WIFI_NOT_FOUND, "Rede Wi-Fi nao encontrada.", output);
    }
    if (output.find("Wi-Fi is disabled") != std::string::npos ||
        output.find("radio is disabled") != std::string::npos) {
        return config_result::error(err::WIFI_DISABLED, "Wi-Fi desativado.", output);
    }
    if (output.find("not running") != std::string::npos) {
        return config_result::error(err::NETWORK_MANAGER_UNAVAILABLE, "NetworkManager indisponivel.", output);
    }
    return config_result::error(codigo_falha, mensagem_falha, output);
}

} // namespace

void listar_wifi() {
    std::vector<wifi_network> redes;
    (void)listar_wifi_result(redes);
}

std::vector<wifi_network> listar_wifi_parsed() {
    std::vector<wifi_network> redes;
    (void)listar_wifi_result(redes);
    return redes;
}

system_result listar_wifi_result(std::vector<wifi_network>& redes) {
    std::lock_guard<std::mutex> lock(g_wifi_mutex);
    redes.clear();

    system_result cache_result;
    if (tentar_cache_wifi(redes, cache_result)) {
        return cache_result;
    }

    if (!comando_existe("nmcli")) {
        system_result result = network_manager_missing_result();
        salvar_cache_wifi(result, redes);
        return result;
    }

    command_result radio_result = exec_command_args_result({"nmcli", "radio", "wifi"});
    if (!radio_result.ok) {
        system_result result = traduzir_wifi_result(
            radio_result,
            err::WIFI_STATUS_FAILED,
            "Falha ao consultar estado do Wi-Fi."
        );
        salvar_cache_wifi(result, redes);
        return result;
    }

    std::string radio = radio_result.stdout_output;
    radio.erase(std::remove(radio.begin(), radio.end(), '\n'), radio.end());
    radio.erase(std::remove(radio.begin(), radio.end(), '\r'), radio.end());
    if (radio != "enabled") {
        system_result result = config_result::error(
            err::WIFI_DISABLED,
            "Wi-Fi desativado.",
            radio_result.mensagem
        );
        salvar_cache_wifi(result, redes);
        return result;
    }

    command_result scan_result = exec_command_args_result({
        "nmcli", "-t", "-f", "IN-USE,BSSID,SSID,SIGNAL,SECURITY", "device", "wifi", "list"
    });
    if (!scan_result.ok) {
        system_result result = traduzir_wifi_result(
            scan_result,
            err::WIFI_SCAN_FAILED,
            "Falha ao escanear redes Wi-Fi."
        );
        salvar_cache_wifi(result, redes);
        return result;
    }

    redes = config_dacc::network_parsing::parse_nmcli_wifi_list(scan_result.stdout_output);
    if (redes.empty()) {
        system_result result = config_result::error(
            err::WIFI_NO_NETWORKS,
            "Nenhuma rede Wi-Fi encontrada.",
            scan_result.mensagem
        );
        salvar_cache_wifi(result, redes);
        return result;
    }

    system_result result = config_result::success("Redes Wi-Fi listadas.", "backend=nmcli");
    salvar_cache_wifi(result, redes);
    return result;
}

wifi_adapter_status obter_status_wifi() {
    wifi_adapter_status status;
    if (!comando_existe("nmcli")) {
        status.disponivel = false;
        status.output = "nmcli ausente no PATH.";
        network_connection_status conexao = obter_status_conexao_rede_sysfs();
        status.conectado_wifi = conexao.wifi_conectado;
        status.conectado_cabeado = conexao.cabeado_conectado;
        status.enabled = conexao.wifi_conectado;
        status.dispositivo_wifi = conexao.dispositivo_wifi;
        status.conexao_wifi = conexao.conexao_wifi;
        status.dispositivo_cabeado = conexao.dispositivo_cabeado;
        status.conexao_cabeada = conexao.conexao_cabeada;
        return status;
    }
    command_result result = exec_command_args_result({"nmcli", "radio", "wifi"});
    status.output = result.mensagem;
    if (!result.ok) {
        status.disponivel = false;
        return status;
    }

    std::string output = result.stdout_output;
    output.erase(std::remove(output.begin(), output.end(), '\n'), output.end());
    output.erase(std::remove(output.begin(), output.end(), '\r'), output.end());
    status.enabled = output == "enabled";

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
    if (!comando_existe("nmcli")) {
        return obter_status_conexao_rede_sysfs();
    }
    command_result result = exec_command_args_result(
        {"nmcli", "-t", "-f", "TYPE,DEVICE,STATE,CONNECTION", "device", "status"}
    );
    if (!result.ok) {
        return status;
    }

    std::stringstream ss(result.stdout_output);
    std::string linha;
    while (std::getline(ss, linha)) {
        auto campos = config_dacc::network_parsing::split_nmcli_escaped_fields(linha);
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
    if (!comando_existe("nmcli")) {
        return network_manager_missing_result();
    }
    command_result result = exec_command_args_result(
        {"nmcli", "radio", "wifi", ligar ? "on" : "off"}
    );
    system_result traduzido = traduzir_wifi_result(
        result,
        err::WIFI_TOGGLE_FAILED,
        ligar ? "Falha ao ativar o Wi-Fi." : "Falha ao desativar o Wi-Fi."
    );
    if (!traduzido.ok) {
        return traduzido;
    }

    wifi_adapter_status status = obter_status_wifi();
    if (status.disponivel && status.enabled == ligar) {
        invalidar_cache_wifi();
        return config_result::success(ligar ? "Wi-Fi ativado." : "Wi-Fi desativado.", result.mensagem);
    }
    return config_result::error(
        err::WIFI_STATE_MISMATCH,
        "O estado do Wi-Fi nao refletiu a solicitacao.",
        status.output
    );
}

void conectar_wifi(const std::string& ssid, const std::string& senha) {
    (void)conectar_wifi_result(ssid, senha);
}

system_result conectar_wifi_result(const std::string& ssid, const std::string& senha) {
    if (!comando_existe("nmcli")) {
        return network_manager_missing_result();
    }
    std::vector<std::string> args = {"nmcli", "device", "wifi", "connect", ssid};
    if (!senha.empty()) {
        args.push_back("password");
        args.push_back(senha);
    }
    command_result result = exec_command_args_result(args);
    system_result traduzido = traduzir_wifi_result(result, err::WIFI_CONNECT_FAILED, "Falha ao conectar na rede Wi-Fi.");
    if (traduzido.ok) {
        invalidar_cache_wifi();
    }
    return traduzido;
}

void desconectar_wifi(const std::string& id) {
    (void)desconectar_wifi_result(id);
}

system_result desconectar_wifi_result(const std::string& id) {
    if (!comando_existe("nmcli")) {
        return network_manager_missing_result();
    }
    command_result result = exec_command_args_result({"nmcli", "connection", "down", "id", id});
    system_result traduzido = traduzir_wifi_result(result, err::WIFI_DISCONNECT_FAILED, "Falha ao desconectar a rede Wi-Fi.");
    if (traduzido.ok) {
        invalidar_cache_wifi();
    }
    return traduzido;
}
