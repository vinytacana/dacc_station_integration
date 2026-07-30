#include "config-dacc/functions.hpp"

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

void testar_separacao_com_argumentos() {
    command_result resultado = exec_command_args_result({
        "sh",
        "-c",
        "printf 'saida'; printf 'aviso' >&2"
    });

    exigir(resultado.ok, "comando com stdout e stderr deve finalizar com sucesso");
    exigir(resultado.stdout_output == "saida", "stdout deve conter apenas a saida padrao");
    exigir(resultado.stderr_output == "aviso", "stderr deve conter apenas a saida de erro");
    exigir(
        resultado.mensagem.find("saida") != std::string::npos &&
        resultado.mensagem.find("aviso") != std::string::npos,
        "mensagem deve preservar o diagnostico combinado por compatibilidade"
    );
}

void testar_separacao_com_comando_textual() {
    command_result resultado = exec_command_result(
        "printf 'texto'; printf 'erro' >&2; exit 7"
    );

    exigir(!resultado.ok, "codigo de saida diferente de zero deve indicar falha");
    exigir(resultado.exit_code == 7, "codigo de saida deve ser preservado");
    exigir(resultado.stdout_output == "texto", "stdout textual deve permanecer separado");
    exigir(resultado.stderr_output == "erro", "stderr textual deve permanecer separado");
}

void testar_drenagem_paralela_dos_pipes() {
    constexpr size_t REPETICOES = 8192;
    constexpr size_t TAMANHO_BLOCO = 16;

    command_result resultado = exec_command_args_result({
        "sh",
        "-c",
        "i=0; while [ \"$i\" -lt 8192 ]; do "
        "printf '0123456789abcdef' >&2; i=$((i + 1)); done; "
        "printf 'concluido'"
    });

    exigir(resultado.ok, "grande volume em stderr nao deve bloquear o comando");
    exigir(resultado.stdout_output == "concluido", "stdout deve ser lido apos stderr volumoso");
    exigir(
        resultado.stderr_output.size() == REPETICOES * TAMANHO_BLOCO,
        "stderr volumoso deve ser coletado integralmente"
    );
}

} // namespace

int main() {
    testar_separacao_com_argumentos();
    testar_separacao_com_comando_textual();
    testar_drenagem_paralela_dos_pipes();
    std::cout << "Config command tests passed." << std::endl;
    return 0;
}
