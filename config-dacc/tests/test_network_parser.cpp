#include "config-dacc/NetworkParsing.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void exigir(bool condicao, const std::string& mensagem) {
    if (!condicao) {
        std::cerr << "[FAIL] " << mensagem << std::endl;
        std::exit(1);
    }
}

void exigir_campos(
    const std::string& entrada,
    const std::vector<std::string>& esperado,
    const std::string& caso
) {
    auto campos = config_dacc::network_parsing::split_nmcli_escaped_fields(entrada);
    exigir(campos == esperado, caso);
}

void testar_campos_simples() {
    exigir_campos(
        "*:DACC:80:WPA2",
        {"*", "DACC", "80", "WPA2"},
        "deve separar campos simples por dois-pontos"
    );
}

void testar_dois_pontos_escapado() {
    exigir_campos(
        ":Lab\\:Jogos:72:WPA2",
        {"", "Lab:Jogos", "72", "WPA2"},
        "deve preservar dois-pontos escapado dentro do SSID"
    );
}

void testar_barra_invertida_escapada() {
    exigir_campos(
        ":Rede\\\\Teste:65:WPA1 WPA2",
        {"", "Rede\\Teste", "65", "WPA1 WPA2"},
        "deve preservar barra invertida escapada"
    );
}

void testar_campo_vazio_final() {
    exigir_campos(
        ":Aberta:40:",
        {"", "Aberta", "40", ""},
        "deve preservar campo vazio no final da linha"
    );
}

void testar_parse_wifi_com_bssid() {
    const std::string entrada =
        "*:AA\\:BB\\:CC\\:DD\\:EE\\:FF:DACC:80:WPA2\n"
        ":11\\:22\\:33\\:44\\:55\\:66:Lab\\:Jogos:72:WPA1 WPA2\n";

    auto redes = config_dacc::network_parsing::parse_nmcli_wifi_list(entrada);

    exigir(redes.size() == 2, "deve parsear redes Wi-Fi com BSSID");
    exigir(redes[0].em_uso, "deve marcar rede em uso");
    exigir(redes[0].bssid == "AA:BB:CC:DD:EE:FF", "deve desescapar BSSID");
    exigir(redes[0].backend_id == "AA:BB:CC:DD:EE:FF", "backend_id deve preferir BSSID");
    exigir(redes[1].ssid == "Lab:Jogos", "deve preservar SSID com dois-pontos");
    exigir(redes[1].sinal == 72, "deve parsear sinal");
}

} // namespace

int main() {
    testar_campos_simples();
    testar_dois_pontos_escapado();
    testar_barra_invertida_escapada();
    testar_campo_vazio_final();
    testar_parse_wifi_com_bssid();
    std::cout << "Network parser tests passed." << std::endl;
    return 0;
}
