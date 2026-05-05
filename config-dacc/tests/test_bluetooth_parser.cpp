#include "functions.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace {

void exigir(bool condicao, const std::string& mensagem) {
    if (!condicao) {
        std::cerr << "[FAIL] " << mensagem << std::endl;
        std::exit(1);
    }
}

void testar_info_multilinha_com_ansi() {
    std::unordered_map<std::string, device_bt> mapa;
    std::string contexto;
    std::stringstream entrada(
        "\033[0;94mDevice AA:BB:CC:DD:EE:01\033[0m Controle Pro\n"
        "    Name: Controle Pro\n"
        "    Alias: Controle Sala\n"
        "    Icon: input-gaming\n"
        "    Connected: yes\n"
        "    Paired: yes\n"
        "    Trusted: yes\n"
    );

    parsing_bluetooth_stream(entrada, mapa, contexto);

    exigir(mapa.size() == 1, "deve parsear um unico dispositivo");
    const auto& dev = mapa["AA:BB:CC:DD:EE:01"];
    exigir(dev.nome == "Controle Sala", "Alias deve sobrescrever o nome quando presente");
    exigir(dev.icon == "input-gaming", "Icon deve ser preservado");
    exigir(dev.conectado, "Connected: yes deve marcar conectado");
    exigir(dev.pareado, "Paired: yes deve marcar pareado");
    exigir(dev.confiavel, "Trusted: yes deve marcar confiavel");
}

void testar_eventos_chg_sem_repetir_mac() {
    std::unordered_map<std::string, device_bt> mapa;
    std::string contexto;
    std::stringstream entrada(
        "[NEW] Device 11:22:33:44:55:66 Fone Azul\n"
        "[CHG] Device 11:22:33:44:55:66 Connected: yes\n"
        "[CHG] Device 11:22:33:44:55:66 Paired: no\n"
        "[CHG] Device 11:22:33:44:55:66 Icon: audio-headphones\n"
    );

    parsing_bluetooth_stream(entrada, mapa, contexto);

    exigir(mapa.size() == 1, "eventos CHG do mesmo MAC nao devem duplicar dispositivo");
    const auto& dev = mapa["11:22:33:44:55:66"];
    exigir(dev.nome == "Fone Azul", "nome vindo do evento NEW deve ser mantido");
    exigir(dev.conectado, "evento Connected: yes deve ser aplicado");
    exigir(!dev.pareado, "evento Paired: no deve ser aplicado");
    exigir(dev.icon == "audio-headphones", "evento Icon deve ser aplicado");
}

void testar_delete_remove_dispositivo() {
    std::unordered_map<std::string, device_bt> mapa;
    std::string contexto;
    std::stringstream entrada(
        "[NEW] Device 22:33:44:55:66:77 Teclado\n"
        "[DEL] Device 22:33:44:55:66:77 Teclado\n"
    );

    parsing_bluetooth_stream(entrada, mapa, contexto);

    exigir(mapa.empty(), "evento DEL deve remover dispositivo do mapa");
    exigir(contexto.empty(), "evento DEL deve limpar contexto do MAC removido");
}

} // namespace

command_result exec_command_result(const std::string&) {
    return {};
}

bluetooth_adapter_status obter_status_bluetooth() {
    return {};
}

int main() {
    testar_info_multilinha_com_ansi();
    testar_eventos_chg_sem_repetir_mac();
    testar_delete_remove_dispositivo();
    std::cout << "Bluetooth parser tests passed." << std::endl;
    return 0;
}
