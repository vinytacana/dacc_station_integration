#include "config-dacc/functions.hpp"

#include <cerrno>
#include <cstring>
#include <array>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <fcntl.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace {

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

bool criar_pipes_comando(int stdout_pipe[2], int stderr_pipe[2], std::string& erro) {
    if (pipe(stdout_pipe) != 0) {
        erro = "pipe(stdout) falhou: " + std::string(std::strerror(errno));
        return false;
    }
    if (pipe(stderr_pipe) != 0) {
        erro = "pipe(stderr) falhou: " + std::string(std::strerror(errno));
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return false;
    }
    return true;
}

void fechar_descritor_poll(struct pollfd& descritor, int& abertos) {
    if (descritor.fd < 0) {
        return;
    }
    close(descritor.fd);
    descritor.fd = -1;
    descritor.events = 0;
    --abertos;
}

bool coletar_saidas_comando(
    int stdout_fd,
    int stderr_fd,
    std::string& stdout_output,
    std::string& stderr_output,
    std::string& erro
) {
    std::array<struct pollfd, 2> descritores{{
        {stdout_fd, POLLIN | POLLHUP, 0},
        {stderr_fd, POLLIN | POLLHUP, 0}
    }};
    std::array<std::string*, 2> destinos{{&stdout_output, &stderr_output}};
    std::array<char, 4096> buffer{};
    int abertos = static_cast<int>(descritores.size());

    while (abertos > 0) {
        int pr;
        do {
            pr = poll(descritores.data(), descritores.size(), -1);
        } while (pr < 0 && errno == EINTR);

        if (pr < 0) {
            erro = "poll() falhou ao ler saidas: " + std::string(std::strerror(errno));
            for (auto& descritor : descritores) {
                fechar_descritor_poll(descritor, abertos);
            }
            return false;
        }

        for (size_t i = 0; i < descritores.size(); ++i) {
            auto& descritor = descritores[i];
            if (descritor.fd < 0 || descritor.revents == 0) {
                continue;
            }

            if ((descritor.revents & POLLNVAL) != 0) {
                if (erro.empty()) {
                    erro = "Descritor invalido ao ler saidas do comando.";
                }
                fechar_descritor_poll(descritor, abertos);
                continue;
            }

            if ((descritor.revents & (POLLIN | POLLHUP | POLLERR)) == 0) {
                continue;
            }

            ssize_t n;
            do {
                n = read(descritor.fd, buffer.data(), buffer.size());
            } while (n < 0 && errno == EINTR);

            if (n > 0) {
                destinos[i]->append(buffer.data(), static_cast<size_t>(n));
                continue;
            }
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                continue;
            }
            if (n < 0 && erro.empty()) {
                erro = "read() falhou ao ler saidas: " + std::string(std::strerror(errno));
            }
            fechar_descritor_poll(descritor, abertos);
        }
    }

    return erro.empty();
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
    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    } else {
        result.exit_code = status;
    }

    result.ok = saidas_ok && WIFEXITED(status) && WEXITSTATUS(status) == 0;
    result.mensagem = combinar_saidas(result.stdout_output, result.stderr_output);

    if (!saidas_ok) {
        if (!result.mensagem.empty() && result.mensagem.back() != '\n') {
            result.mensagem.push_back('\n');
        }
        result.mensagem += erro_saidas;
    }
    if (!result.ok && result.mensagem.empty()) {
        result.mensagem = descrever_status_processo(status);
    }
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
    command_result result;
    if (cmd.empty()) {
        result.mensagem = "Nenhum comando informado.";
        return result;
    }

    int stdout_pipe[2];
    int stderr_pipe[2];
    if (!criar_pipes_comando(stdout_pipe, stderr_pipe, result.mensagem)) {
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        result.mensagem = "fork() falhou: " + std::string(std::strerror(errno));
        return result;
    }

    if (pid == 0) {
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0 ||
            dup2(stderr_pipe[1], STDERR_FILENO) < 0) {
            _exit(127);
        }
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        execlp("sh", "sh", "-c", cmd.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    std::string erro_saidas;
    bool saidas_ok = coletar_saidas_comando(
        stdout_pipe[0],
        stderr_pipe[0],
        result.stdout_output,
        result.stderr_output,
        erro_saidas
    );

    int status = -1;
    if (waitpid(pid, &status, 0) < 0) {
        result.mensagem = "waitpid() falhou: " + std::string(std::strerror(errno));
        return result;
    }

    preencher_status_comando(result, status, saidas_ok, erro_saidas);

    return result;
}

command_result exec_command_args_result(const std::vector<std::string>& args) {
    command_result result;
    if (args.empty()) {
        result.mensagem = "Nenhum comando informado.";
        return result;
    }

    int stdout_pipe[2];
    int stderr_pipe[2];
    if (!criar_pipes_comando(stdout_pipe, stderr_pipe, result.mensagem)) {
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        result.mensagem = "fork() falhou: " + std::string(std::strerror(errno));
        return result;
    }

    if (pid == 0) {
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0 ||
            dup2(stderr_pipe[1], STDERR_FILENO) < 0) {
            _exit(127);
        }
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
        _exit(127);
    }

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    std::string erro_saidas;
    bool saidas_ok = coletar_saidas_comando(
        stdout_pipe[0],
        stderr_pipe[0],
        result.stdout_output,
        result.stderr_output,
        erro_saidas
    );

    int status = -1;
    if (waitpid(pid, &status, 0) < 0) {
        result.mensagem = "waitpid() falhou: " + std::string(std::strerror(errno));
        return result;
    }

    preencher_status_comando(result, status, saidas_ok, erro_saidas);
    return result;
}

std::string exec_command(const char* cmd) {
    command_result result = exec_command_result(cmd);
    if (!result.ok) {
        throw std::runtime_error(result.mensagem.empty() ? "exec_command falhou." : result.mensagem);
    }
    return result.stdout_output;
}
