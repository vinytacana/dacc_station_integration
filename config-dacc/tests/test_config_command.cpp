#include "config-dacc/functions.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <string>
#include <thread>
#include <unistd.h>
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

void testar_descritores_internos_nao_vazam() {
    command_result resultado = exec_command_args_result({
        "sh",
        "-c",
        "for fd in /proc/self/fd/[3-9]*; do [ -e \"$fd\" ] && printf '%s\\n' \"$fd\"; done; exit 0"
    });

    exigir(resultado.ok, "inspecao dos descritores deve finalizar com sucesso");
    exigir(resultado.stdout_output.empty(), "pipes internos CLOEXEC nao devem vazar para o comando");
}

int contar_descritores_abertos() {
    DIR* diretorio = opendir("/proc/self/fd");
    exigir(diretorio != nullptr, "deve abrir /proc/self/fd para verificar vazamentos");

    int total = 0;
    while (dirent* entrada = readdir(diretorio)) {
        if (entrada->d_name[0] != '.') {
            ++total;
        }
    }
    closedir(diretorio);
    return total;
}

void testar_execucoes_repetidas_nao_vazam_descritores() {
    const int antes = contar_descritores_abertos();
    for (int i = 0; i < 20; ++i) {
        command_result resultado = exec_command_args_result({"sh", "-c", "printf ok"});
        exigir(resultado.ok, "execucao repetida deve finalizar com sucesso");
    }
    exigir(
        contar_descritores_abertos() == antes,
        "execucoes repetidas nao devem acumular descritores no processo chamador"
    );
}

int executar_com_saida_padrao_fechada() {
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    command_result resultado = exec_command_args_result({
        "sh",
        "-c",
        "printf 'saida'; printf 'erro' >&2"
    });
    return resultado.ok &&
        resultado.stdout_output == "saida" &&
        resultado.stderr_output == "erro" ? 0 : 1;
}

void testar_redirecionamento_com_saida_padrao_fechada() {
    command_result resultado = exec_command_args_result({
        "/proc/self/exe",
        "--testar-stdio-fechado"
    });

    exigir(resultado.ok, "executor deve funcionar quando o chamador fechou stdout e stderr");
}

void testar_falha_do_posix_spawn() {
    command_result resultado = exec_command_args_result({
        "dacc-station-comando-inexistente-para-teste"
    });

    exigir(!resultado.ok, "executavel ausente deve falhar sem quebrar a API");
    exigir(resultado.exit_code == 127, "executavel ausente deve preservar o codigo 127 legado");
    exigir(
        resultado.mensagem.find("posix_spawnp() falhou") != std::string::npos,
        "falha de spawn deve ter diagnostico explicito"
    );
}

void testar_timeout_com_term() {
    command_options options;
    options.timeout = std::chrono::milliseconds(100);
    options.terminate_grace_period = std::chrono::milliseconds(500);

    const auto inicio = std::chrono::steady_clock::now();
    command_result resultado = exec_command_args_result({
        "sh",
        "-c",
        "trap 'exit 42' TERM; printf 'iniciado'; while :; do sleep 10; done"
    }, options);
    const auto duracao = std::chrono::steady_clock::now() - inicio;

    exigir(!resultado.ok, "timeout deve indicar falha");
    exigir(resultado.timed_out, "resultado deve distinguir timeout");
    exigir(!resultado.cancelled, "timeout nao deve ser marcado como cancelamento");
    exigir(resultado.exit_code == 42, "SIGTERM deve permitir encerramento gracioso");
    exigir(resultado.stdout_output == "iniciado", "saida anterior ao timeout deve ser preservada");
    exigir(duracao < std::chrono::seconds(2), "timeout deve limitar a duracao do comando");
}

void testar_kill_apos_term_ignorado() {
    command_options options;
    options.timeout = std::chrono::milliseconds(100);
    options.terminate_grace_period = std::chrono::milliseconds(100);

    const auto inicio = std::chrono::steady_clock::now();
    command_result resultado = exec_command_args_result({
        "sh",
        "-c",
        "trap '' TERM; while :; do sleep 10; done"
    }, options);
    const auto duracao = std::chrono::steady_clock::now() - inicio;

    exigir(resultado.timed_out, "processo resistente a TERM deve continuar marcado como timeout");
    exigir(resultado.exit_code == 128 + SIGKILL, "SIGKILL deve encerrar processo resistente a TERM");
    exigir(duracao < std::chrono::seconds(2), "fallback com SIGKILL deve ser limitado");
}

void testar_timeout_encerra_grupo_de_processos() {
    char caminho_pid[] = "/tmp/dacc-command-child-XXXXXX";
    int fd = mkstemp(caminho_pid);
    exigir(fd >= 0, "deve criar arquivo temporario para PID descendente");
    close(fd);

    command_options options;
    options.timeout = std::chrono::milliseconds(150);
    options.terminate_grace_period = std::chrono::milliseconds(100);
    command_result resultado = exec_command_args_result({
        "sh",
        "-c",
        std::string("trap '' TERM; sleep 10 & echo $! > ") + caminho_pid + "; wait"
    }, options);

    exigir(resultado.timed_out, "timeout do processo pai deve ser reportado");
    exigir(
        resultado.exit_code == 128 + SIGKILL,
        "processo pai resistente a TERM deve receber o SIGKILL de fallback"
    );

    std::ifstream arquivo_pid(caminho_pid);
    pid_t pid_filho = -1;
    arquivo_pid >> pid_filho;
    unlink(caminho_pid);
    exigir(pid_filho > 0, "comando deve registrar o PID do processo descendente");

    bool filho_encerrado = false;
    for (int tentativa = 0; tentativa < 50; ++tentativa) {
        errno = 0;
        if (kill(pid_filho, 0) == -1 && errno == ESRCH) {
            filho_encerrado = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    exigir(filho_encerrado, "timeout deve encerrar todo o grupo sem deixar descendente ativo");
}

void testar_cancelamento() {
    std::atomic_bool cancelar{false};
    command_options options;
    options.timeout = std::chrono::seconds(5);
    options.terminate_grace_period = std::chrono::milliseconds(500);
    options.cancel_requested = &cancelar;

    std::thread solicitante([&cancelar]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cancelar.store(true);
    });
    command_result resultado = exec_command_args_result({
        "sh",
        "-c",
        "trap 'exit 43' TERM; while :; do sleep 10; done"
    }, options);
    solicitante.join();

    exigir(!resultado.ok, "cancelamento deve indicar falha");
    exigir(resultado.cancelled, "resultado deve distinguir cancelamento");
    exigir(!resultado.timed_out, "cancelamento nao deve ser marcado como timeout");
    exigir(resultado.exit_code == 43, "cancelamento deve tentar SIGTERM antes de SIGKILL");
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--testar-stdio-fechado") {
        return executar_com_saida_padrao_fechada();
    }

    testar_separacao_com_argumentos();
    testar_separacao_com_comando_textual();
    testar_drenagem_paralela_dos_pipes();
    testar_descritores_internos_nao_vazam();
    testar_execucoes_repetidas_nao_vazam_descritores();
    testar_redirecionamento_com_saida_padrao_fechada();
    testar_falha_do_posix_spawn();
    testar_timeout_com_term();
    testar_kill_apos_term_ignorado();
    testar_timeout_encerra_grupo_de_processos();
    testar_cancelamento();
    std::cout << "Config command tests passed." << std::endl;
    return 0;
}
