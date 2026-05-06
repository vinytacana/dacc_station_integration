#include "config-dacc/functions.hpp"
#include "config-dacc/ConfigResult.hpp"
#include "config-dacc/NetworkParsing.hpp"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace {

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
        return config_result::error("wifi_password_required", "A rede exige senha.", output);
    }
    if (output.find("wrong password") != std::string::npos ||
        output.find("invalid secrets") != std::string::npos) {
        return config_result::error("wifi_auth_failed", "Senha incorreta ou autenticacao recusada.", output);
    }
    if (output.find("No network with SSID") != std::string::npos) {
        return config_result::error("wifi_not_found", "Rede Wi-Fi nao encontrada.", output);
    }
    if (output.find("Wi-Fi is disabled") != std::string::npos ||
        output.find("radio is disabled") != std::string::npos) {
        return config_result::error("wifi_disabled", "Wi-Fi desativado.", output);
    }
    if (output.find("not running") != std::string::npos) {
        return config_result::error("network_manager_unavailable", "NetworkManager indisponivel.", output);
    }
    return config_result::error(codigo_falha, mensagem_falha, output);
}

} // namespace

void listar_wifi() {
    (void)exec_command_args_result({"nmcli", "device", "wifi", "list"});
}

std::vector<wifi_network> listar_wifi_parsed() {
    std::vector<wifi_network> redes;
    std::string saida;
    try {
        saida = exec_command("nmcli -t -f IN-USE,SSID,SIGNAL,SECURITY device wifi list");
    } catch (...) {
        return redes;
    }

    std::stringstream ss(saida);
    std::string linha;

    while (std::getline(ss, linha)) {
        std::vector<std::string> campos = config_dacc::network_parsing::split_nmcli_escaped_fields(linha);
        if (campos.size() < 4) {
            continue;
        }

        wifi_network net;
        net.em_uso = campos[0] == "*";
        net.ssid = campos[1];
        try {
            net.sinal = std::stoi(campos[2]);
        } catch (...) {
            net.sinal = 0;
        }
        net.seguranca = campos[3];
        if (!net.ssid.empty()) {
            redes.push_back(net);
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
        return config_result::success(ligar ? "Wi-Fi ativado." : "Wi-Fi desativado.", result.mensagem);
    }
    return config_result::error(
        "wifi_state_mismatch",
        "O estado do Wi-Fi nao refletiu a solicitacao.",
        status.output
    );
}

void conectar_wifi(const std::string& ssid, const std::string& senha) {
    (void)conectar_wifi_result(ssid, senha);
}

system_result conectar_wifi_result(const std::string& ssid, const std::string& senha) {
    std::vector<std::string> args = {"nmcli", "device", "wifi", "connect", ssid};
    if (!senha.empty()) {
        args.push_back("password");
        args.push_back(senha);
    }
    command_result result = exec_command_args_result(args);
    return traduzir_wifi_result(result, "wifi_connect_failed", "Falha ao conectar na rede Wi-Fi.");
}

void desconectar_wifi(const std::string& id) {
    (void)desconectar_wifi_result(id);
}

system_result desconectar_wifi_result(const std::string& id) {
    command_result result = exec_command_args_result({"nmcli", "connection", "down", "id", id});
    return traduzir_wifi_result(result, "wifi_disconnect_failed", "Falha ao desconectar a rede Wi-Fi.");
}
