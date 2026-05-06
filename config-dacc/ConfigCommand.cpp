#include "config-dacc/functions.hpp"

#include <cerrno>
#include <cstring>
#include <array>
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

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        result.mensagem = "pipe() falhou: " + std::string(std::strerror(errno));
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        result.mensagem = "fork() falhou: " + std::string(std::strerror(errno));
        return result;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        execlp("sh", "sh", "-c", cmd.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    close(pipefd[1]);
    std::array<char, 256> buffer{};
    std::string output;
    while (true) {
        ssize_t n = read(pipefd[0], buffer.data(), buffer.size());
        if (n > 0) {
            output.append(buffer.data(), static_cast<size_t>(n));
            continue;
        }
        if (n == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        break;
    }
    close(pipefd[0]);

    int status = -1;
    if (waitpid(pid, &status, 0) < 0) {
        result.mensagem = "waitpid() falhou: " + std::string(std::strerror(errno));
        return result;
    }

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    } else {
        result.exit_code = status;
    }

    result.ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    result.stdout_output = output;
    result.stderr_output = output;
    result.mensagem = output;
    if (!result.ok && result.mensagem.empty()) {
        result.mensagem = descrever_status_processo(status);
    }

    return result;
}

command_result exec_command_args_result(const std::vector<std::string>& args) {
    command_result result;
    if (args.empty()) {
        result.mensagem = "Nenhum comando informado.";
        return result;
    }

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        result.mensagem = "pipe() falhou: " + std::string(std::strerror(errno));
        return result;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        result.mensagem = "fork() falhou: " + std::string(std::strerror(errno));
        return result;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
        _exit(127);
    }

    close(pipefd[1]);
    std::array<char, 256> buffer{};
    std::string output;
    while (true) {
        ssize_t n = read(pipefd[0], buffer.data(), buffer.size());
        if (n > 0) {
            output.append(buffer.data(), static_cast<size_t>(n));
            continue;
        }
        if (n == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        break;
    }
    close(pipefd[0]);

    int status = -1;
    if (waitpid(pid, &status, 0) < 0) {
        result.mensagem = "waitpid() falhou: " + std::string(std::strerror(errno));
        return result;
    }

    if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    } else {
        result.exit_code = status;
    }

    result.ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    result.stdout_output = output;
    result.stderr_output = output;
    result.mensagem = output;
    if (!result.ok && result.mensagem.empty()) {
        result.mensagem = descrever_status_processo(status);
    }
    return result;
}

std::string exec_command(const char* cmd) {
    command_result result = exec_command_result(cmd);
    if (!result.ok) {
        throw std::runtime_error(result.mensagem.empty() ? "exec_command falhou." : result.mensagem);
    }
    return result.stdout_output;
}
