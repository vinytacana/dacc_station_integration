#include "config-dacc/functions.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern char** environ;

namespace {

using relogio = std::chrono::steady_clock;

constexpr auto INTERVALO_SUPERVISAO = std::chrono::milliseconds(25);
constexpr auto TEMPO_DRENAGEM_FINAL = std::chrono::milliseconds(100);

std::string descrever_status_processo(int status) {
    if (status == -1) {
        return "Falha ao aguardar termino do comando.";
    }
    if (WIFEXITED(status)) {
        return "Comando finalizado com codigo " + std::to_string(WEXITSTATUS(status)) + ".";
    }
    if (WIFSIGNALED(status)) {
        return "Comando finalizado por sinal " + std::to_string(WTERMSIG(status)) + ".";
    }
    if (WIFSTOPPED(status)) {
        return "Comando interrompido por sinal " + std::to_string(WSTOPSIG(status)) + ".";
    }
    return "Comando terminou com status inesperado.";
}

void fechar_fd(int& fd) {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

bool criar_pipes_comando(int stdout_pipe[2], int stderr_pipe[2], std::string& erro) {
    if (pipe2(stdout_pipe, O_CLOEXEC) != 0) {
        erro = "pipe2(stdout) falhou: " + std::string(std::strerror(errno));
        return false;
    }
    if (pipe2(stderr_pipe, O_CLOEXEC) != 0) {
        erro = "pipe2(stderr) falhou: " + std::string(std::strerror(errno));
        fechar_fd(stdout_pipe[0]);
        fechar_fd(stdout_pipe[1]);
        return false;
    }
    return true;
}

void adicionar_mensagem(std::string& destino, const std::string& mensagem) {
    if (mensagem.empty()) {
        return;
    }
    if (!destino.empty() && destino.back() != '\n') {
        destino.push_back('\n');
    }
    destino += mensagem;
}

std::string combinar_saidas(
    const std::string& stdout_output,
    const std::string& stderr_output
) {
    if (stdout_output.empty()) {
        return stderr_output;
    }
    if (stderr_output.empty()) {
        return stdout_output;
    }

    std::string combinado = stdout_output;
    if (combinado.back() != '\n') {
        combinado.push_back('\n');
    }
    combinado += stderr_output;
    return combinado;
}

void preencher_status_comando(
    command_result& result,
    int status,
    bool saidas_ok,
    const std::string& erro_saidas
) {
    if (status != -1 && WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (status != -1 && WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    } else {
        result.exit_code = status;
    }

    result.ok = saidas_ok && !result.timed_out && !result.cancelled &&
        status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
    result.mensagem = combinar_saidas(result.stdout_output, result.stderr_output);

    if (!saidas_ok) {
        adicionar_mensagem(result.mensagem, erro_saidas);
    }
    if (result.timed_out) {
        adicionar_mensagem(result.mensagem, "Tempo limite do comando excedido.");
    } else if (result.cancelled) {
        adicionar_mensagem(result.mensagem, "Execucao do comando cancelada.");
    }
    if (!result.ok && result.mensagem.empty()) {
        result.mensagem = descrever_status_processo(status);
    }
}

bool tornar_nao_bloqueante(int fd, std::string& erro) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        adicionar_mensagem(
            erro,
            "fcntl() falhou ao configurar pipe: " + std::string(std::strerror(errno))
        );
        return false;
    }
    return true;
}

void drenar_descritor(
    struct pollfd& descritor,
    std::string& destino,
    std::string& erro
) {
    if (descritor.fd < 0) {
        return;
    }

    std::array<char, 4096> buffer{};
    while (true) {
        ssize_t lidos = read(descritor.fd, buffer.data(), buffer.size());
        if (lidos > 0) {
            destino.append(buffer.data(), static_cast<size_t>(lidos));
            continue;
        }
        if (lidos == 0) {
            fechar_fd(descritor.fd);
            descritor.events = 0;
            return;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        adicionar_mensagem(
            erro,
            "read() falhou ao ler saidas: " + std::string(std::strerror(errno))
        );
        fechar_fd(descritor.fd);
        descritor.events = 0;
        return;
    }
}

void sinalizar_grupo(pid_t pid, int sinal) {
    if (kill(-pid, sinal) == 0 || errno != ESRCH) {
        return;
    }
    kill(pid, sinal);
}

int timeout_poll(
    const command_options& options,
    bool encerramento_solicitado,
    relogio::time_point inicio,
    relogio::time_point limite_terminate
) {
    auto espera = INTERVALO_SUPERVISAO;
    const auto agora = relogio::now();

    if (!encerramento_solicitado && options.timeout.count() > 0) {
        const auto restante = inicio + options.timeout - agora;
        espera = std::min(espera, std::chrono::duration_cast<std::chrono::milliseconds>(restante));
    } else if (encerramento_solicitado) {
        const auto restante = limite_terminate - agora;
        espera = std::min(espera, std::chrono::duration_cast<std::chrono::milliseconds>(restante));
    }

    return static_cast<int>(std::max(espera, std::chrono::milliseconds(0)).count());
}

command_result executar_comando_spawn(
    const std::vector<std::string>& args,
    const command_options& options
) {
    command_result result;
    if (args.empty() || args.front().empty()) {
        result.mensagem = "Nenhum comando informado.";
        return result;
    }
    if (options.cancel_requested != nullptr && options.cancel_requested->load()) {
        result.cancelled = true;
        result.mensagem = "Execucao do comando cancelada.";
        return result;
    }

    int stdout_pipe[2]{-1, -1};
    int stderr_pipe[2]{-1, -1};
    if (!criar_pipes_comando(stdout_pipe, stderr_pipe, result.mensagem)) {
        return result;
    }

    posix_spawn_file_actions_t actions;
    int spawn_error = posix_spawn_file_actions_init(&actions);
    if (spawn_error != 0) {
        fechar_fd(stdout_pipe[0]);
        fechar_fd(stdout_pipe[1]);
        fechar_fd(stderr_pipe[0]);
        fechar_fd(stderr_pipe[1]);
        result.mensagem = "posix_spawn_file_actions_init() falhou: " +
            std::string(std::strerror(spawn_error));
        return result;
    }

    posix_spawnattr_t attributes;
    bool attributes_initialized = false;
    spawn_error = posix_spawnattr_init(&attributes);
    if (spawn_error == 0) {
        attributes_initialized = true;
        spawn_error = posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP);
    }
    if (spawn_error == 0) {
        spawn_error = posix_spawnattr_setpgroup(&attributes, 0);
    }

    const std::array<int, 4> descritores{{
        stdout_pipe[0], stdout_pipe[1], stderr_pipe[0], stderr_pipe[1]
    }};
    if (spawn_error == 0) {
        spawn_error = posix_spawn_file_actions_adddup2(&actions, stdout_pipe[1], STDOUT_FILENO);
    }
    if (spawn_error == 0) {
        spawn_error = posix_spawn_file_actions_adddup2(&actions, stderr_pipe[1], STDERR_FILENO);
    }
    for (int fd : descritores) {
        if (spawn_error == 0 && fd != STDOUT_FILENO && fd != STDERR_FILENO) {
            spawn_error = posix_spawn_file_actions_addclose(&actions, fd);
        }
    }

    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = -1;
    if (spawn_error == 0) {
        spawn_error = posix_spawnp(
            &pid,
            argv.front(),
            &actions,
            &attributes,
            argv.data(),
            environ
        );
    }

    posix_spawn_file_actions_destroy(&actions);
    if (attributes_initialized) {
        posix_spawnattr_destroy(&attributes);
    }
    fechar_fd(stdout_pipe[1]);
    fechar_fd(stderr_pipe[1]);

    if (spawn_error != 0) {
        fechar_fd(stdout_pipe[0]);
        fechar_fd(stderr_pipe[0]);
        result.exit_code = spawn_error == ENOENT ? 127 : -1;
        result.mensagem = "posix_spawnp() falhou: " + std::string(std::strerror(spawn_error));
        return result;
    }

    std::string erro_saidas;
    if (!tornar_nao_bloqueante(stdout_pipe[0], erro_saidas)) {
        fechar_fd(stdout_pipe[0]);
    }
    if (!tornar_nao_bloqueante(stderr_pipe[0], erro_saidas)) {
        fechar_fd(stderr_pipe[0]);
    }

    std::array<struct pollfd, 2> descritores_poll{{
        {stdout_pipe[0], POLLIN | POLLHUP, 0},
        {stderr_pipe[0], POLLIN | POLLHUP, 0}
    }};
    std::array<std::string*, 2> destinos{{&result.stdout_output, &result.stderr_output}};

    const auto inicio = relogio::now();
    auto limite_terminate = relogio::time_point::max();
    auto limite_drenagem = relogio::time_point::max();
    bool encerramento_solicitado = false;
    bool kill_enviado = false;
    bool processo_finalizado = false;
    int status = -1;

    while (true) {
        const auto agora = relogio::now();
        if (!encerramento_solicitado) {
            if (options.cancel_requested != nullptr && options.cancel_requested->load()) {
                result.cancelled = true;
                encerramento_solicitado = true;
            } else if (options.timeout.count() > 0 && agora - inicio >= options.timeout) {
                result.timed_out = true;
                encerramento_solicitado = true;
            }

            if (encerramento_solicitado) {
                sinalizar_grupo(pid, SIGTERM);
                limite_terminate = agora + std::max(
                    options.terminate_grace_period,
                    std::chrono::milliseconds(0)
                );
            }
        }

        if (encerramento_solicitado && !processo_finalizado && !kill_enviado &&
            agora >= limite_terminate) {
            sinalizar_grupo(pid, SIGKILL);
            kill_enviado = true;
        }

        if (!processo_finalizado) {
            pid_t aguardado;
            do {
                aguardado = waitpid(pid, &status, WNOHANG);
            } while (aguardado < 0 && errno == EINTR);

            if (aguardado == pid) {
                processo_finalizado = true;
                limite_drenagem = agora + TEMPO_DRENAGEM_FINAL;
            } else if (aguardado < 0) {
                adicionar_mensagem(
                    erro_saidas,
                    "waitpid() falhou: " + std::string(std::strerror(errno))
                );
                processo_finalizado = true;
                limite_drenagem = agora + TEMPO_DRENAGEM_FINAL;
            }
        }

        bool algum_pipe_aberto = false;
        for (const auto& descritor : descritores_poll) {
            algum_pipe_aberto = algum_pipe_aberto || descritor.fd >= 0;
        }
        if (processo_finalizado && !algum_pipe_aberto) {
            break;
        }
        if (processo_finalizado && agora >= limite_drenagem) {
            for (auto& descritor : descritores_poll) {
                fechar_fd(descritor.fd);
            }
            break;
        }

        int espera_ms = timeout_poll(
            options,
            encerramento_solicitado,
            inicio,
            limite_terminate
        );
        int pr;
        do {
            pr = poll(descritores_poll.data(), descritores_poll.size(), espera_ms);
        } while (pr < 0 && errno == EINTR);

        if (pr < 0) {
            adicionar_mensagem(
                erro_saidas,
                "poll() falhou ao ler saidas: " + std::string(std::strerror(errno))
            );
            for (auto& descritor : descritores_poll) {
                fechar_fd(descritor.fd);
            }
            continue;
        }

        for (size_t i = 0; i < descritores_poll.size(); ++i) {
            auto& descritor = descritores_poll[i];
            if (descritor.fd < 0 || descritor.revents == 0) {
                continue;
            }
            if ((descritor.revents & POLLNVAL) != 0) {
                adicionar_mensagem(erro_saidas, "Descritor invalido ao ler saidas do comando.");
                fechar_fd(descritor.fd);
                continue;
            }
            if ((descritor.revents & (POLLIN | POLLHUP | POLLERR)) != 0) {
                drenar_descritor(descritor, *destinos[i], erro_saidas);
            }
        }
    }

    preencher_status_comando(result, status, erro_saidas.empty(), erro_saidas);
    return result;
}

} // namespace

bool comando_existe(const std::string& cmd) {
    if (cmd.empty() || cmd.find('/') != std::string::npos) {
        return !cmd.empty() && access(cmd.c_str(), X_OK) == 0;
    }

    const char* path_env = std::getenv("PATH");
    if (path_env == nullptr || *path_env == '\0') {
        return false;
    }

    std::stringstream ss(path_env);
    std::string dir;
    while (std::getline(ss, dir, ':')) {
        if (dir.empty()) {
            dir = ".";
        }
        const std::string full_path = dir + "/" + cmd;
        if (access(full_path.c_str(), X_OK) == 0) {
            return true;
        }
    }

    return false;
}

command_result exec_command_result(const std::string& cmd) {
    return exec_command_result(cmd, command_options{});
}

command_result exec_command_result(const std::string& cmd, const command_options& options) {
    if (cmd.empty()) {
        command_result result;
        result.mensagem = "Nenhum comando informado.";
        return result;
    }
    return executar_comando_spawn({"sh", "-c", cmd}, options);
}

command_result exec_command_args_result(const std::vector<std::string>& args) {
    return exec_command_args_result(args, command_options{});
}

command_result exec_command_args_result(
    const std::vector<std::string>& args,
    const command_options& options
) {
    return executar_comando_spawn(args, options);
}

std::string exec_command(const char* cmd) {
    command_result result = exec_command_result(cmd);
    if (!result.ok) {
        throw std::runtime_error(result.mensagem.empty() ? "exec_command falhou." : result.mensagem);
    }
    return result.stdout_output;
}
