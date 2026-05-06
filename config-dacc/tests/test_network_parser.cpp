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

} // namespace

int main() {
    testar_campos_simples();
    testar_dois_pontos_escapado();
    testar_barra_invertida_escapada();
    testar_campo_vazio_final();
    std::cout << "Network parser tests passed." << std::endl;
    return 0;
}
