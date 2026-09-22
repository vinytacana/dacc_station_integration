#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <unistd.h>

namespace {

char g_signal_log[4096]{};
volatile sig_atomic_t g_exit_on_term = 0;

void escrever_arquivo(const char* caminho, const std::string& conteudo) {
    if (caminho == nullptr || *caminho == '\0') {
        return;
    }

    int fd = open(caminho, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        return;
    }
    (void)write(fd, conteudo.data(), conteudo.size());
    close(fd);
}

void tratar_sigterm(int) {
    if (g_signal_log[0] != '\0') {
        int fd = open(g_signal_log, O_WRONLY | O_CREAT | O_APPEND, 0600);
        if (fd >= 0) {
            static constexpr char MARCADOR[] = "SIGTERM\n";
            (void)write(fd, MARCADOR, sizeof(MARCADOR) - 1);
            close(fd);
        }
    }
    if (g_exit_on_term != 0) {
        _exit(42);
    }
}

void instalar_handler_sigterm(bool encerrar) {
    const char* signal_log = std::getenv("DACC_BT_FAKE_SIGNAL_LOG");
    if (signal_log != nullptr) {
        std::strncpy(g_signal_log, signal_log, sizeof(g_signal_log) - 1);
    }
    g_exit_on_term = encerrar ? 1 : 0;

    struct sigaction action{};
    action.sa_handler = tratar_sigterm;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, nullptr);
}

void escrever_saida_parcial() {
    static constexpr char SAIDA[] = "saida parcial do bluetoothctl falso\n";
    (void)write(STDOUT_FILENO, SAIDA, sizeof(SAIDA) - 1);
}

} // namespace

int main() {
    escrever_arquivo(
        std::getenv("DACC_BT_FAKE_PID_LOG"),
        std::to_string(static_cast<long long>(getpid())) + "\n"
    );

    const char* mode_env = std::getenv("DACC_BT_FAKE_MODE");
    const std::string mode = mode_env == nullptr ? "exit0" : mode_env;

    escrever_saida_parcial();
    if (mode == "exit0") {
        return 0;
    }
    if (mode == "exit23") {
        return 23;
    }
    if (mode == "signal") {
        raise(SIGUSR1);
        return 1;
    }
    if (mode == "timeout_term") {
        instalar_handler_sigterm(true);
    } else if (mode == "timeout_kill") {
        instalar_handler_sigterm(false);
    } else {
        return 64;
    }

    while (true) {
        if (mode == "timeout_kill") {
            static constexpr char CONTINUA[] = "saida continua\n";
            (void)write(STDOUT_FILENO, CONTINUA, sizeof(CONTINUA) - 1);
            usleep(10000);
        } else {
            pause();
        }
    }
}
