#include "ProcessManager.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h> 
#include <sys/signalfd.h> 
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <cstdlib>
#include <cstring>
#include <cerrno> 
#include <vector>
#include <stdexcept>
#include <thread>

#include "json.hpp"

using json = nlohmann::json;

namespace {

constexpr std::size_t MAX_IPC_MESSAGE_SIZE = 64 * 1024;

} // namespace

ProcessManager::ProcessManager(std::shared_ptr<spdlog::logger> logger) : logger_{std::move(logger)} {

    // a data type to represent multiple signals
    // "signal_mask" is showing wich signals are blocked
    // all blocked signals can't reach the parent process
    sigset_t signal_mask;
    
    // clean initializing 
    sigemptyset(&signal_mask);

    // adding "SIGCHILD" to blocked signals list
    sigaddset(&signal_mask, SIGCHLD);

    // other threads created will inherit a copy of the signal mask
    if (pthread_sigmask(SIG_BLOCK, &signal_mask, nullptr) != 0) {
        throw std::runtime_error("Failed to block SIGCHLD");
    }

    // If the fd argument is -1, then the call creates a new file descriptor and associates the signal set specified in mask with that file descriptor.
    signal_fd_ = signalfd(-1, &signal_mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (signal_fd_ == -1) {
        throw std::runtime_error("Failed to create signalfd");
    }

    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);   
    if (epoll_fd_ == -1) {
        close(signal_fd_);
        throw std::runtime_error("Failed to create epoll instance");
    }

    epoll_event event{};
    event.events = EPOLLIN; 
    event.data.fd = signal_fd_;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, signal_fd_, &event) == -1) {
        close(epoll_fd_);
        close(signal_fd_);
        throw std::runtime_error("Failed to add signalfd to epoll");
    }

    setupServerSocket();

    std::future<void> ready_future = ready_promise.get_future();

    try {
        event_loop_thread_ = std::thread(&ProcessManager::eventLoop, this);
    
    } catch (const std::system_error& e) {

        close(epoll_fd_);
        close(signal_fd_);

        throw std::runtime_error("Failed to start event loop thread: " + std::string(e.what()));
    }

    ready_future.get();
}

ProcessManager::~ProcessManager() {
    if (logger_) logger_->info("Shutting down ProcessManager...");
    stop_loop_ = true;

    if (event_loop_thread_.joinable()) {
        event_loop_thread_.join();
    }

    for (int client_fd : connected_clients_) {
        close(client_fd);
    }
    connected_clients_.clear();
    client_input_buffers_.clear();

    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
    if (signal_fd_ != -1) {
        close(signal_fd_);
    }
    if (server_socket_fd_ != -1) {
        close(server_socket_fd_);
        unlink(SOCKET_PATH);
    }
    if (logger_) logger_->info("ProcessManager shut down cleanly.");
}

void ProcessManager::eventLoop() {
    // sinaliza que a thread esta pronta
    ready_promise.set_value();
    epoll_event events[MAX_EVENTS];
    if (logger_) logger_->info("Event loop started and is ready.");
    
    while (!stop_loop_) {
        int n_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1000);
        if (n_events < 0){
            if (errno == EINTR) // Se for interrompido por um sinal, apenas continue o loop
                continue;
            // Para outros erros, saia do loop.
            if (logger_) logger_->error("epoll_wait error: {}", strerror(errno));
            break;
        }

        for (int i = 0; i < n_events; ++i) {
            int current_fd = events[i].data.fd;
            if (current_fd == signal_fd_) {
                if (logger_) logger_->info("received signal on signal_fd. Handling child signal");
                handleChildSignal();
            } else if (current_fd == server_socket_fd_) {
                handleNewConnection();
            } else {
                handleClientMessage(current_fd);
            }
        }
    }
    if (logger_) logger_->info("Event loop finished.");
}

void ProcessManager::handleChildSignal() {
    signalfd_siginfo ssi;
        while (read(signal_fd_, &ssi, sizeof(ssi)) == sizeof(ssi)) {
        // continue lendo ate que não haja mais nada
    }
    
    int status;
    pid_t child_pid;
    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        auto end_time = std::chrono::steady_clock::now();
        
        std::lock_guard<std::mutex> lock(processes_mutex_);
        auto it = running_processes_.find(child_pid);
        
        if (it != running_processes_.end()) {
            auto start_time = it->second;
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
            
            if (logger_) {
                logger_->info("Process PID={} finished.", child_pid);
                logger_->info("Total execution time: {} seconds.", duration.count());
            }
            
            // Notify connected clients
            json response;
            response["event"] = "game_finished";
            response["pid"] = child_pid;
            response["status"] = status;
            
            std::string msg = response.dump() + '\n';
            for (int client_fd : connected_clients_) {
                ssize_t bytes_sent = send(client_fd, msg.c_str(), msg.length(), MSG_NOSIGNAL);
                if (bytes_sent < 0 && logger_) {
                    logger_->warn(
                        "Failed to notify client FD={}: {}",
                        client_fd,
                        strerror(errno)
                    );
                } else if (bytes_sent != static_cast<ssize_t>(msg.length()) && logger_) {
                    logger_->warn(
                        "Partial notification sent to client FD={} ({}/{})",
                        client_fd,
                        bytes_sent,
                        msg.length()
                    );
                }
            }

            running_processes_.erase(it);
        }
        else{
            if (logger_) logger_->warn("Reaped untracked child process PID={}", child_pid);
        }
    }
}

pid_t ProcessManager::createProcess(){
    pid_t child_process;
    child_process = fork();
    switch (child_process)
    {
    case -1:
        throw std::runtime_error("fork() failed: " + std::string(strerror(errno)));
    case 0:
        return child_process; // returning 0   CHILD
    default:
        return child_process; // returning > 0 PARENT
        break;
    }
}

bool ProcessManager::isAppValid(const ApplicationDefinition& app) const {
    const std::string& app_path = app.getPath();

    if (app_path.empty()){ 
        if (logger_) logger_->error("Application executable path is empty for app_id: {}", app.getId());
        return false;
    }
    
    return true;
}
    

pid_t ProcessManager::startApplication(ApplicationDefinition& app){
    if(not isAppValid(app)) return -1;    
    
    pid_t child_pid;
    try{   
        child_pid = createProcess();
    }catch(const std::runtime_error& e)
    {
        if (logger_) logger_->error(e.what());
        return -1;
    }
    
    if (child_pid == 0){
        std::vector<std::string> argsVector = buildCommand(app);

        // Convert std::vector<std::string> to char* const*
        std::vector<char*> execArgs;
        for (const auto& arg : argsVector) {
            execArgs.push_back(const_cast<char*>(arg.c_str()));
        }
        execArgs.push_back(nullptr); 

        execvp(execArgs[0], execArgs.data());
        
        std::cerr << "CRITICAL: execvp failed for " << app.getPath() << ": " << strerror(errno) << std::endl;
        _exit(EXIT_FAILURE);
        return -1; 
    }

    // parent process
    auto start_time = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(processes_mutex_);
        running_processes_[child_pid] = start_time;
    }

    if (logger_) {
        logger_->info("App {} started successfully", app.getId());
        logger_->info("PID={}", child_pid);
    }
        
    return child_pid;
}

std::vector<std::string> ProcessManager::buildCommand(const ApplicationDefinition& app) {
    std::vector<std::string> args;

    if (app.useGamescope) {
        args.push_back("gamescope");
        
        // Resolução Interna
        args.push_back("-w");
        args.push_back(std::to_string(app.renderWidth));
        args.push_back("-h");
        args.push_back(std::to_string(app.renderHeight));

        // Resolução de Saída
        if (app.outputWidth > 0 && app.outputHeight > 0) {
            args.push_back("-W");
            args.push_back(std::to_string(app.outputWidth));
            args.push_back("-H");
            args.push_back(std::to_string(app.outputHeight));
        }

        // Taxa de Atualização
        args.push_back("-r");
        args.push_back(std::to_string(app.refreshRate));

        // Fullscreen
        if (app.fullscreen) {
            args.push_back("-f");
        }

        // Separador
        args.push_back("--");
    }

    // Check if it is a shell script to execute with bash
    // This is safer for external scripts that might not have +x permission
    if (app.getPath().length() >= 3 && 
        app.getPath().substr(app.getPath().length() - 3) == ".sh") {
        args.push_back("/bin/bash");
    }

    // O comando do jogo (pode ser um script sh)
    args.push_back(app.getPath());
    
    return args;
}
    
    void ProcessManager::setupServerSocket() {
        server_socket_fd_ = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (server_socket_fd_ == -1) {
            throw std::runtime_error("Failed to create server socket: " + std::string(strerror(errno)));
        }
    
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
        unlink(SOCKET_PATH); 
    
        if (bind(server_socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            close(server_socket_fd_);
            throw std::runtime_error("Failed to bind server socket: " + std::string(strerror(errno)));
        }
    
        if (listen(server_socket_fd_, 5) == -1) {
            close(server_socket_fd_);
            throw std::runtime_error("Failed to listen on server socket: " + std::string(strerror(errno)));
        }
    
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = server_socket_fd_;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_socket_fd_, &ev) == -1) {
            close(server_socket_fd_);
            throw std::runtime_error("Failed to add server socket to epoll: " + std::string(strerror(errno)));
        }
    
        if (logger_) logger_->info("Server socket setup successfully at {}", SOCKET_PATH);
    }
    
    void ProcessManager::handleNewConnection() {
        int client_fd = accept4(server_socket_fd_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (client_fd == -1) {
            if (logger_) logger_->error("accept4 failed: {}", strerror(errno));
            return;
        }
    
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = client_fd;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            if (logger_) logger_->error("Failed to add client socket to epoll: {}", strerror(errno));
            close(client_fd);
            return;
        }
        
        connected_clients_.insert(client_fd);
        client_input_buffers_.emplace(client_fd, std::string{});
        if (logger_) logger_->info("New client connection accepted (FD: {})", client_fd);
    }
    
    void ProcessManager::handleClientMessage(int client_fd) {
        char buffer[4096];

        while (true) {
            ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);

            if (bytes_read > 0) {
                std::string& input_buffer = client_input_buffers_[client_fd];
                input_buffer.append(buffer, static_cast<std::size_t>(bytes_read));

                std::size_t delimiter_position = std::string::npos;
                while ((delimiter_position = input_buffer.find('\n')) != std::string::npos) {
                    std::string message = input_buffer.substr(0, delimiter_position);
                    input_buffer.erase(0, delimiter_position + 1);

                    if (!message.empty() && message.back() == '\r') {
                        message.pop_back();
                    }
                    if (!message.empty()) {
                        processClientMessage(message);
                    }
                }

                if (input_buffer.size() > MAX_IPC_MESSAGE_SIZE) {
                    if (logger_) {
                        logger_->error(
                            "IPC message exceeded {} bytes (FD: {})",
                            MAX_IPC_MESSAGE_SIZE,
                            client_fd
                        );
                    }
                    disconnectClient(client_fd);
                    return;
                }

                continue;
            }

            if (bytes_read == 0) {
                if (logger_) logger_->info("Client disconnected (FD: {})", client_fd);
                disconnectClient(client_fd);
                return;
            }

            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }

            if (logger_) logger_->error("recv failed (FD: {}): {}", client_fd, strerror(errno));
            disconnectClient(client_fd);
            return;
        }
    }

    void ProcessManager::processClientMessage(const std::string& message) {
        if (logger_) logger_->info("Received message from client: {}", message);

        try {
            auto j = json::parse(message);
            if (j.contains("action") && j["action"] == "start") {
                std::string id = j.value("id", "unknown");
                std::string path = j.value("path", "");
                
                if (path.empty()) {
                    if (logger_) logger_->error("Error: Start action received but path is empty");
                    return;
                }
    
                ApplicationDefinition app(id, path.c_str(), "game");

                if (j.contains("graphics")) {
                    auto g = j["graphics"];
                    app.useGamescope = g.value("use_gamescope", false);
                    app.renderWidth = g.value("width", 1280);
                    app.renderHeight = g.value("height", 720);
                    app.refreshRate = g.value("fps", 60);
                }

                startApplication(app);
            }
        } catch (const json::exception& e) {
            if (logger_) logger_->error("Invalid IPC JSON message: {}", e.what());
        }
    }

    void ProcessManager::disconnectClient(int client_fd) {
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr) == -1 &&
            errno != ENOENT && logger_) {
            logger_->warn("Failed to remove client FD={} from epoll: {}", client_fd, strerror(errno));
        }

        connected_clients_.erase(client_fd);
        client_input_buffers_.erase(client_fd);
        close(client_fd);
    }
