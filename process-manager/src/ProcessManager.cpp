#include "ProcessManager.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h> 
#include <sys/signalfd.h> 
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>

#include <climits>
#include <cstdlib>
#include <cstring>
#include <cerrno> 
#include <optional>
#include <vector>
#include <stdexcept>
#include <thread>
#include <chrono>

#include "json.hpp"

using json = nlohmann::json;

// handleChildSignal, handleClientMessage, processClientMessage, and disconnectClient run on the event-loop thread; processes_mutex_ only guards external access.
namespace {

constexpr auto kClientSendTimeout = std::chrono::milliseconds(250);
constexpr auto kChildTerminationTimeout = std::chrono::milliseconds(250);
// This synchronous wait occupies the event-loop thread, which is acceptable for
// the current one-game model; watching the pipe asynchronously with epoll is deferred.
constexpr auto kExecConfirmationTimeout = std::chrono::milliseconds(2000);

int remainingPollTimeout(std::chrono::steady_clock::time_point deadline) {
    const auto now = std::chrono::steady_clock::now();
    if (now >= deadline) {
        return 0;
    }

    const auto remaining = deadline - now;
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
    if (milliseconds < remaining) {
        ++milliseconds;
    }
    if (milliseconds.count() > INT_MAX) {
        return INT_MAX;
    }
    return static_cast<int>(milliseconds.count());
}

int normalizedExitStatus(int status) {
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return status;
}

bool requiresDisconnect(ipc::SendStatus status) {
    return status == ipc::SendStatus::Disconnected ||
        status == ipc::SendStatus::Desynced;
}

[[noreturn]] void reportChildFailure(int error_fd, int error_code) {
    const char* bytes = reinterpret_cast<const char*>(&error_code);
    std::size_t written = 0;
    while (written < sizeof(error_code)) {
        const ssize_t count = write(
            error_fd,
            bytes + written,
            sizeof(error_code) - written
        );
        if (count > 0) {
            written += static_cast<std::size_t>(count);
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
    _exit(127);
}

void prepareChildSignals(int error_fd) {
    struct sigaction default_action{};
    default_action.sa_handler = SIG_DFL;
    sigemptyset(&default_action.sa_mask);

    if (sigaction(SIGCHLD, &default_action, nullptr) != 0 ||
        sigaction(SIGINT, &default_action, nullptr) != 0 ||
        sigaction(SIGTERM, &default_action, nullptr) != 0 ||
        sigaction(SIGPIPE, &default_action, nullptr) != 0) {
        reportChildFailure(error_fd, errno);
    }

    sigset_t empty_mask;
    sigemptyset(&empty_mask);
    if (sigprocmask(SIG_SETMASK, &empty_mask, nullptr) != 0) {
        reportChildFailure(error_fd, errno);
    }
}

} // namespace

ProcessManager::ProcessManager(
    std::shared_ptr<spdlog::logger> logger,
    std::string socket_path
) : logger_{std::move(logger)}, socket_path_{std::move(socket_path)} {

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

    for (const auto& client : connected_clients_) {
        close(client.first);
    }
    connected_clients_.clear();
    client_frame_readers_.clear();

    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
    if (signal_fd_ != -1) {
        close(signal_fd_);
    }
    if (server_socket_fd_ != -1) {
        close(server_socket_fd_);
        unlink(socket_path_.c_str());
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
        
        std::optional<RunningProcess> process;
        {
            std::lock_guard<std::mutex> lock(processes_mutex_);
            auto it = running_processes_.find(child_pid);
            if (it != running_processes_.end()) {
                process = std::move(it->second);
                running_processes_.erase(it);
            }
        }

        if (process) {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                end_time - process->start_time
            );
            
            if (logger_) {
                logger_->info("Process PID={} finished.", child_pid);
                logger_->info("Total execution time: {} seconds.", duration.count());
            }
            
            json response;
            response["event"] = "game_finished";
            response["request_id"] = process->request_id;
            response["game_id"] = process->game_id;
            response["pid"] = child_pid;
            response["exit_status"] = normalizedExitStatus(status);

            const auto client = connected_clients_.find(process->client.fd);
            if (client != connected_clients_.end() &&
                client->second == process->client.generation) {
                const ipc::SendStatus send_status = sendClientMessage(
                    process->client.fd,
                    response.dump()
                );
                if (send_status != ipc::SendStatus::Ok && logger_) {
                    logger_->warn("game_finished was not delivered for PID={}", child_pid);
                }
                if (requiresDisconnect(send_status)) {
                    disconnectClient(process->client.fd);
                }
            } else if (logger_) {
                logger_->info(
                    "Discarding game_finished for PID={}, owner client is not connected",
                    child_pid
                );
            }
        } else {
            if (logger_) logger_->warn("Reaped untracked child process PID={}", child_pid);
        }
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
    

ProcessStartResult ProcessManager::startApplication(ApplicationDefinition& app) {
    if (!isAppValid(app)) {
        return {-1, EINVAL};
    }

    std::vector<std::string> args_vector = buildCommand(app);
    std::vector<char*> exec_args;
    exec_args.reserve(args_vector.size() + 1);
    for (auto& arg : args_vector) {
        exec_args.push_back(arg.data());
    }
    exec_args.push_back(nullptr);

    int exec_error_pipe[2];
    if (pipe2(exec_error_pipe, O_CLOEXEC | O_NONBLOCK) != 0) {
        const int pipe_error = errno;
        if (logger_) logger_->error("pipe2() failed: {}", strerror(pipe_error));
        return {-1, pipe_error};
    }

    const pid_t child_pid = fork();
    if (child_pid < 0) {
        const int fork_error = errno;
        close(exec_error_pipe[0]);
        close(exec_error_pipe[1]);
        if (logger_) logger_->error("fork() failed: {}", strerror(fork_error));
        return {-1, fork_error};
    }

    if (child_pid == 0) {
        close(exec_error_pipe[0]);
        prepareChildSignals(exec_error_pipe[1]);
        execvp(exec_args[0], exec_args.data());
        reportChildFailure(exec_error_pipe[1], errno);
    }

    close(exec_error_pipe[1]);
    int exec_error = 0;
    char* error_bytes = reinterpret_cast<char*>(&exec_error);
    std::size_t received = 0;
    bool pipe_read_failed = false;
    bool confirmation_timed_out = false;
    bool pipe_eof = false;
    const auto confirmation_deadline =
        std::chrono::steady_clock::now() + kExecConfirmationTimeout;

    while (received < sizeof(exec_error)) {
        const int poll_timeout = remainingPollTimeout(confirmation_deadline);
        if (poll_timeout == 0) {
            confirmation_timed_out = true;
            break;
        }

        pollfd descriptor{exec_error_pipe[0], POLLIN, 0};
        const int poll_result = poll(&descriptor, 1, poll_timeout);
        if (poll_result < 0) {
            if (errno == EINTR) {
                continue;
            }
            exec_error = errno;
            pipe_read_failed = true;
            break;
        }
        if (poll_result == 0) {
            confirmation_timed_out = true;
            break;
        }
        if ((descriptor.revents & POLLNVAL) != 0) {
            exec_error = EBADF;
            pipe_read_failed = true;
            break;
        }

        if ((descriptor.revents & (POLLIN | POLLHUP)) != 0) {
            const ssize_t count = read(
                exec_error_pipe[0],
                error_bytes + received,
                sizeof(exec_error) - received
            );
            if (count > 0) {
                received += static_cast<std::size_t>(count);
                continue;
            }
            if (count == 0) {
                pipe_eof = true;
                break;
            }
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            exec_error = errno;
            pipe_read_failed = true;
            break;
        }

        if ((descriptor.revents & POLLERR) != 0) {
            exec_error = EIO;
            pipe_read_failed = true;
            break;
        }
    }

    if (confirmation_timed_out) {
        if (logger_) {
            logger_->error("Timed out waiting for exec confirmation for {}", app.getPath());
        }
        kill(child_pid, SIGKILL);
        int child_status = 0;
        while (waitpid(child_pid, &child_status, 0) < 0 && errno == EINTR) {
        }
        close(exec_error_pipe[0]);
        return {-1, ETIMEDOUT};
    }
    close(exec_error_pipe[0]);

    if (received > 0 || pipe_read_failed || !pipe_eof) {
        if (received != sizeof(exec_error) && !pipe_read_failed) {
            exec_error = EIO;
        }
        if (pipe_read_failed) {
            kill(child_pid, SIGKILL);
        }
        int child_status = 0;
        while (waitpid(child_pid, &child_status, 0) < 0 && errno == EINTR) {
        }
        if (logger_) {
            logger_->error("execvp failed for {}: {}", app.getPath(), strerror(exec_error));
        }
        return {-1, exec_error};
    }

    if (logger_) {
        logger_->info("App {} started successfully", app.getId());
        logger_->info("PID={}", child_pid);
    }

    return {child_pid, 0};
}

void ProcessManager::terminateStartedProcess(pid_t child_pid) {
    if (kill(child_pid, SIGTERM) != 0 && errno != ESRCH && logger_) {
        logger_->warn("SIGTERM failed for undelivered PID={}: {}", child_pid, strerror(errno));
    }

    const auto deadline = std::chrono::steady_clock::now() + kChildTerminationTimeout;
    int status = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        const pid_t wait_result = waitpid(child_pid, &status, WNOHANG);
        if (wait_result == child_pid || (wait_result < 0 && errno == ECHILD)) {
            return;
        }
        if (wait_result < 0 && errno != EINTR) {
            break;
        }
        poll(nullptr, 0, 10);
    }

    if (kill(child_pid, SIGKILL) != 0 && errno != ESRCH && logger_) {
        logger_->warn("SIGKILL failed for undelivered PID={}: {}", child_pid, strerror(errno));
    }
    while (waitpid(child_pid, &status, 0) < 0 && errno == EINTR) {
    }
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
        if (socket_path_.empty() || socket_path_.size() >= sizeof(addr.sun_path)) {
            close(server_socket_fd_);
            throw std::runtime_error("Invalid Process Manager socket path");
        }
        strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);
    
        unlink(socket_path_.c_str());
    
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
    
        if (logger_) logger_->info("Server socket setup successfully at {}", socket_path_);
    }
    
void ProcessManager::handleNewConnection() {
    const int client_fd = accept4(
        server_socket_fd_,
        nullptr,
        nullptr,
        SOCK_NONBLOCK | SOCK_CLOEXEC
    );
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

    const std::uint64_t generation = ++connection_seq_;
    connected_clients_.emplace(client_fd, generation);
    client_frame_readers_.emplace(client_fd, ipc::FrameReader{});
    if (logger_) {
        logger_->info(
            "New client connection accepted (FD: {}, generation: {})",
            client_fd,
            generation
        );
    }
}

void ProcessManager::handleClientMessage(int client_fd) {
    const auto client_it = connected_clients_.find(client_fd);
    if (client_it == connected_clients_.end()) {
        return;
    }
    const ClientConnection client{client_fd, client_it->second};
    bool should_disconnect = false;
    char buffer[4096];

    while (!should_disconnect) {
        const ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);

        if (bytes_read > 0) {
            auto reader_it = client_frame_readers_.find(client_fd);
            if (reader_it == client_frame_readers_.end()) {
                should_disconnect = true;
                break;
            }

            ipc::FrameReader& frame_reader = reader_it->second;
            frame_reader.feed(buffer, static_cast<std::size_t>(bytes_read));
            while (auto message = frame_reader.next()) {
                if (!message->empty() && !processClientMessage(client, *message)) {
                    should_disconnect = true;
                    break;
                }
            }

            if (!should_disconnect && frame_reader.overflowed()) {
                if (logger_) {
                    logger_->error(
                        "IPC message exceeded {} bytes (FD: {})",
                        ipc::kMaxFrameBytes,
                        client_fd
                    );
                }
                should_disconnect = true;
            }
            continue;
        }

        if (bytes_read == 0) {
            if (logger_) logger_->info("Client disconnected (FD: {})", client_fd);
            should_disconnect = true;
            break;
        }

        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            break;
        }

        if (logger_) logger_->error("recv failed (FD: {}): {}", client_fd, strerror(errno));
        should_disconnect = true;
    }

    if (should_disconnect) {
        disconnectClient(client_fd);
    }
}

ipc::SendStatus ProcessManager::sendClientMessage(
    int client_fd,
    const std::string& message
) {
    const ipc::SendResult send_result = ipc::sendMessage(
        client_fd,
        message,
        kClientSendTimeout
    );
    if (send_result.status != ipc::SendStatus::Ok && logger_) {
        logger_->warn(
            "Failed to send IPC message to FD={} (status={}, bytes={}, errno={})",
            client_fd,
            static_cast<int>(send_result.status),
            send_result.bytes_sent,
            send_result.err
        );
    }
    return send_result.status;
}

bool ProcessManager::processClientMessage(
    const ClientConnection& client,
    const std::string& message
) {
    if (logger_) logger_->info("Received message from client: {}", message);

    json request;
    try {
        request = json::parse(message);
    } catch (const json::exception& e) {
        if (logger_) logger_->error("Invalid IPC JSON message: {}", e.what());
        return true;
    }

    if (!request.is_object() ||
        !request.contains("request_id") ||
        !request["request_id"].is_string() ||
        request["request_id"].get<std::string>().empty()) {
        if (logger_) logger_->warn("Discarding IPC command without request_id");
        return true;
    }

    const std::string request_id = request["request_id"].get<std::string>();
    if (!request.contains("action") ||
        !request["action"].is_string() ||
        request["action"].get<std::string>() != "start") {
        if (logger_) logger_->warn("Ignoring unsupported IPC action for request_id={}", request_id);
        return true;
    }

    const std::string game_id =
        request.contains("game_id") && request["game_id"].is_string()
            ? request["game_id"].get<std::string>()
            : std::string{};
    const std::string path =
        request.contains("path") && request["path"].is_string()
            ? request["path"].get<std::string>()
            : std::string{};

    auto send_failure = [&](int error_code, const std::string& error_message) {
        const json response{
            {"event", "game_start_failed"},
            {"request_id", request_id},
            {"game_id", game_id},
            {"error_code", error_code},
            {"message", error_message}
        };
        return sendClientMessage(client.fd, response.dump());
    };

    if (game_id.empty() || path.empty()) {
        if (logger_) {
            logger_->error("Invalid start request for request_id={}", request_id);
        }
        return !requiresDisconnect(send_failure(EINVAL, "game_id and path are required"));
    }

    ApplicationDefinition app(game_id, path.c_str(), "game");
    try {
        if (request.contains("graphics")) {
            const auto& graphics = request["graphics"];
            app.useGamescope = graphics.value("use_gamescope", false);
            app.renderWidth = graphics.value("width", 1280);
            app.renderHeight = graphics.value("height", 720);
            app.refreshRate = graphics.value("fps", 60);
        }
    } catch (const json::exception& e) {
        if (logger_) logger_->error("Invalid graphics config: {}", e.what());
        return !requiresDisconnect(send_failure(EINVAL, "Invalid start request"));
    }

    const ProcessStartResult start_result = startApplication(app);
    if (!start_result.ok()) {
        return !requiresDisconnect(send_failure(
            start_result.error_code,
            std::string(strerror(start_result.error_code))
        ));
    }

    {
        std::lock_guard<std::mutex> lock(processes_mutex_);
        running_processes_[start_result.pid] = RunningProcess{
            std::chrono::steady_clock::now(),
            request_id,
            game_id,
            client
        };
    }

    const json response{
        {"event", "game_started"},
        {"request_id", request_id},
        {"game_id", game_id},
        {"pid", start_result.pid}
    };
    const ipc::SendStatus send_status = sendClientMessage(client.fd, response.dump());
    if (send_status == ipc::SendStatus::Ok) {
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(processes_mutex_);
        running_processes_.erase(start_result.pid);
    }
    if (logger_) {
        logger_->error(
            "Terminating PID={} because game_started could not be delivered",
            start_result.pid
        );
    }
    terminateStartedProcess(start_result.pid);
    return !requiresDisconnect(send_status);
}

void ProcessManager::disconnectClient(int client_fd) {
    const auto client = connected_clients_.find(client_fd);
    if (client == connected_clients_.end()) {
        return;
    }
    const std::uint64_t generation = client->second;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr) == -1 &&
        errno != ENOENT && logger_) {
        logger_->warn("Failed to remove client FD={} from epoll: {}", client_fd, strerror(errno));
    }

    std::vector<pid_t> orphaned_processes;
    {
        std::lock_guard<std::mutex> lock(processes_mutex_);
        for (auto process = running_processes_.begin();
             process != running_processes_.end();) {
            const ClientConnection& owner = process->second.client;
            if (owner.fd == client_fd && owner.generation == generation) {
                orphaned_processes.push_back(process->first);
                process = running_processes_.erase(process);
            } else {
                ++process;
            }
        }
    }

    connected_clients_.erase(client);
    client_frame_readers_.erase(client_fd);
    close(client_fd);

    for (pid_t child_pid : orphaned_processes) {
        if (logger_) {
            logger_->warn(
                "Terminating PID={}, owner client disconnected",
                child_pid
            );
        }
        terminateStartedProcess(child_pid);
    }
}
