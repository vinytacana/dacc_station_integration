#ifndef BLUETOOTH_INTERNAL_H
#define BLUETOOTH_INTERNAL_H

#include "functions.hpp"

#include <chrono>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace bluetooth_internal {

inline constexpr int kBluetoothctlTimeoutMs = 8000;
inline constexpr int kBluetoothStatePollAttempts = 10;
inline constexpr int kBluetoothStatePollIntervalMs = 150;

std::string remover_ansi(std::string str);
system_result make_bt_error(
    const std::string& codigo,
    const std::string& mensagem,
    const std::string& detalhes = ""
);
system_result make_bt_success(const std::string& mensagem, const std::string& detalhes = "");
system_result executar_bluetoothctl(
    const std::string& comando,
    int timeout_ms = kBluetoothctlTimeoutMs
);
system_result executar_bluetoothctl_ate(
    const std::vector<std::string>& comandos,
    const std::vector<std::string>& marcadores_sucesso,
    const std::vector<std::string>& marcadores_falha,
    const std::vector<std::string>& comandos_finalizacao = {},
    int timeout_ms = kBluetoothctlTimeoutMs
);
bool bluetoothctl_saida_indica_sucesso(const std::string& saida);
std::string resumir_bluetooth_status(const bluetooth_adapter_status& status);
device_bt consultar_dispositivo_bluetooth(const std::string& mac);
std::unordered_map<std::string, device_bt> listar_dispositivos_por_comando(const std::string& comando);
void mesclar_dispositivo_bluetooth(device_bt& destino, const device_bt& origem);
void marcar_campo_bool_em_lote(
    std::unordered_map<std::string, device_bt>& destino,
    const std::unordered_map<std::string, device_bt>& origem,
    bool device_bt::*campo
);
std::vector<device_bt> ordenar_dispositivos(const std::unordered_map<std::string, device_bt>& mapa);
system_result garantir_bluetooth_desbloqueado();
system_result validar_adaptador_pronto();
system_result verificar_saida_operacao(
    const system_result& comando_result,
    const std::string& codigo_falha,
    const std::string& mensagem_falha
);
bool dispositivo_esta_conhecido(const std::string& mac);

template <typename Predicate>
bool aguardar_estado_dispositivo(
    const std::string& mac,
    Predicate predicate,
    device_bt* ultimo_estado = nullptr,
    int tentativas = kBluetoothStatePollAttempts,
    int intervalo_ms = kBluetoothStatePollIntervalMs
) {
    for (int tentativa = 0; tentativa < tentativas; ++tentativa) {
        device_bt dispositivo = consultar_dispositivo_bluetooth(mac);
        if (ultimo_estado != nullptr) {
            *ultimo_estado = dispositivo;
        }
        if (predicate(dispositivo)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalo_ms));
    }
    return false;
}

} // namespace bluetooth_internal

#endif
