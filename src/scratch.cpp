// ProcessManager.h
#include <string>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <iostream>

class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();

    void launch_process(const std::string& exec_path);

private:
    int sig_pipe[2];
    std::unordered_map<pid_t, std::chrono::high_resolution_clock::time_point> start_times;
    std::thread monitor_thread;
    std::atomic<bool> running;

    static ProcessManager* instance;
    static void sigchld_handler(int);

    void monitor_pipe();
    void install_sigchld_handler();
};

// ProcessManager.cpp
// #include "ProcessManager.h"

ProcessManager* ProcessManager::instance = nullptr;

ProcessManager::ProcessManager() : running(true) {
    instance = this;

    if (pipe(sig_pipe) == -1) {
        perror("pipe");
        exit(1);
    }

    fcntl(sig_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(sig_pipe[1], F_SETFL, O_NONBLOCK);

    install_sigchld_handler();
    monitor_thread = std::thread(&ProcessManager::monitor_pipe, this);
}

ProcessManager::~ProcessManager() {
    running = false;
    close(sig_pipe[0]);
    close(sig_pipe[1]);
    if (monitor_thread.joinable()) monitor_thread.join();
}

void ProcessManager::install_sigchld_handler() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        perror("sigaction");
        exit(1);
    }
}

void ProcessManager::sigchld_handler(int) {
    char ch = 'x';
    write(instance->sig_pipe[1], &ch, 1);
}

void ProcessManager::monitor_pipe() {
    while (running) {
        char buf;
        ssize_t n = read(sig_pipe[0], &buf, 1);
        if (n <= 0) continue;

        int status;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto it = start_times.find(pid);
            if (it != start_times.end()) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - it->second).count();
                std::cout << "Processo " << pid << " terminou em " << duration << " ms\n";
                start_times.erase(it);
            }
        }
    }
}

void ProcessManager::launch_process(const std::string& exec_path) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp(exec_path.c_str(), exec_path.c_str(), nullptr);
        perror("execlp");
        exit(1);
    } else if (pid > 0) {
        start_times[pid] = std::chrono::high_resolution_clock::now();
        std::cout << "Processo iniciado: PID " << pid << "\n";
    } else {
        perror("fork");
    }
}
