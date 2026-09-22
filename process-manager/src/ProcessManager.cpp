#include "ProcessManager.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/prctl.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <dirent.h>

#include <climits>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <new>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <thread>
#include <chrono>

#include "json.hpp"

using json = nlohmann::json;

// Only the event-loop thread owns lifecycle state and calls waitpid.
// SIGCHLD is intentionally left untouched: bounded polling also works in multithreaded hosts.
namespace {

std::atomic<bool> manager_present{false};

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
    return status != ipc::SendStatus::Ok;
}

enum class GroupState { NoLiveMembers, HasLiveMembers, Unknown };

GroupState groupState(pid_t process_group, pid_t leader) {
    DIR* proc = opendir("/proc");
    if (!proc) return GroupState::Unknown;
    GroupState result = GroupState::NoLiveMembers;
    while (const dirent* entry = readdir(proc)) {
        char* end = nullptr;
        errno = 0;
        const long value = std::strtol(entry->d_name, &end, 10);
        if (errno != 0 || !end || *end != '\0' || value <= 0) continue;
        std::ifstream stat_file(std::string("/proc/") + entry->d_name + "/stat");
        std::string stat;
        if (!std::getline(stat_file, stat)) {
            if (errno != ENOENT && errno != ESRCH) result = GroupState::Unknown;
            continue;
        }
        const auto close_paren = stat.rfind(')');
        if (close_paren == std::string::npos || close_paren + 2 >= stat.size()) {
            result = GroupState::Unknown;
            continue;
        }
        char state = 0;
        pid_t parent = -1;
        pid_t pgid = -1;
        std::istringstream fields(stat.substr(close_paren + 2));
        if (!(fields >> state >> parent >> pgid)) {
            result = GroupState::Unknown;
            continue;
        }
        if (pgid == process_group && value != leader && state != 'Z' && state != 'X') {
            result = GroupState::HasLiveMembers;
            break;
        }
        if (pgid == process_group && value == leader && state != 'Z' && state != 'X') {
            result = GroupState::HasLiveMembers;
            break;
        }
    }
    closedir(proc);
    return result;
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
    std::string socket_path,
    std::string catalog_path
) : catalog_(catalog_path), logger_{std::move(logger)}, socket_path_{std::move(socket_path)} {

    try {
        // Acquire the socket before changing any process-wide supervision state.
        epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd_ < 0) throw std::runtime_error("epoll_create1 failed");
        setupServerSocket();
        bool expected = false;
        if (!manager_present.compare_exchange_strong(expected, true)) {
            throw std::runtime_error("Only one ProcessManager per host process is supported");
        }
        owns_supervision_ = true;
        if (prctl(PR_GET_CHILD_SUBREAPER, &previous_subreaper_) != 0 ||
            prctl(PR_SET_CHILD_SUBREAPER, 1) != 0) {
            throw std::runtime_error("Failed to enable child subreaper");
        }
        struct sigaction child_action{};
        if (sigaction(SIGCHLD, nullptr, &child_action) != 0 ||
            child_action.sa_handler == SIG_IGN || (child_action.sa_flags & SA_NOCLDWAIT)) {
            throw std::runtime_error("SIGCHLD auto-reap is incompatible with ProcessManager");
        }
        wake_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (wake_fd_ < 0) throw std::runtime_error("eventfd failed");
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = wake_fd_;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, wake_fd_, &ev) != 0) {
            throw std::runtime_error("Failed to add shutdown wakeup");
        }
        auto ready = ready_promise.get_future();
        event_loop_thread_ = std::thread(&ProcessManager::eventLoop, this);
        ready.get();
    } catch (...) {
        cleanupResources();
        throw;
    }
}

void ProcessManager::cleanupResources() noexcept {
    for (const auto& client : connected_clients_) close(client.first);
    connected_clients_.clear();
    client_frame_readers_.clear();
    seen_requests_.clear();
    server_.reset();
    if (wake_fd_ >= 0) close(wake_fd_);
    if (epoll_fd_ >= 0) close(epoll_fd_);
    if (owns_supervision_) {
        prctl(PR_SET_CHILD_SUBREAPER, previous_subreaper_);
        manager_present = false;
    }
}

ProcessManager::~ProcessManager() {
    stop_loop_ = true;
    const std::uint64_t wake = 1;
    // EAGAIN means a wakeup is already pending.
    while (write(wake_fd_, &wake, sizeof(wake)) < 0 && errno == EINTR) {}
    if (event_loop_thread_.joinable()) event_loop_thread_.join();
    cleanupResources();
}

void ProcessManager::eventLoop() {
    ready_promise.set_value();
    try {
        epoll_event events[MAX_EVENTS];
        while (!stop_loop_) {
            // Do not rely on which host thread receives SIGCHLD, or reap unrelated children.
            reapFinishedGame();
            const int count = epoll_wait(epoll_fd_, events, MAX_EVENTS, 25);
            if (count < 0) {
                if (errno == EINTR) continue;
                throw std::runtime_error("epoll_wait failed");
            }
            for (int i = 0; i < count && !stop_loop_; ++i) {
                const int fd = events[i].data.fd;
                if (fd == wake_fd_) continue;
                if (fd == server_socket_fd_) handleNewConnection();
                else handleClientMessage(fd);
            }
        }
    } catch (const std::exception& e) {
        stop_loop_ = true;
        if (logger_) logger_->error("ProcessManager event loop failed: {}", e.what());
    }
    // Listener stays owned until all children are reaped; no new request is processed.
    while (!running_processes_.empty()) finishGame(running_processes_.begin()->first, false);
}

void ProcessManager::reapFinishedGame() {
    if (running_processes_.empty()) return;
    const pid_t pid = running_processes_.begin()->first;
    siginfo_t info{};
    const int result = waitid(P_PID, pid, &info, WEXITED | WNOHANG | WNOWAIT);
    if (result == 0 && info.si_pid == pid) finishGame(pid, true);
    else if (result < 0 && errno == ECHILD) {
        // A foreign reaper violates our ownership: never signal a possibly recycled PID.
        cleanup_incomplete_ = true;
        finishGame(pid, true);
    }
}

void ProcessManager::finishGame(pid_t pid, bool leader_already_exited) {
    const auto it = running_processes_.find(pid);
    if (it == running_processes_.end()) return;
    const RunningProcess process = it->second;
    const int status = terminateStartedProcess(pid, leader_already_exited);
    running_processes_.erase(it);
    const json response{
        {"event", "game_finished"}, {"request_id", process.request_id},
        {"game_id", process.game_id}, {"pid", pid}, {"exit_status", normalizedExitStatus(status)}
    };
    const auto client = connected_clients_.find(process.client.fd);
    if (client != connected_clients_.end() && client->second == process.client.generation) {
        if (requiresDisconnect(sendClientMessage(process.client.fd, response.dump()))) {
            disconnectClient(process.client.fd);
        }
    }
}

ProcessStartResult ProcessManager::startApplication(const GameCommand& game) {
    const int validation = GameCatalog::validateExecutable(game);
    if (validation) return {-1, validation};
    const int cwd_fd = open(
        game.working_directory.c_str(),
        O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW
    );
    if (cwd_fd < 0) return {-1, errno};
    std::vector<std::string> args_vector = game.argv;
    std::vector<char*> exec_args;
    exec_args.reserve(args_vector.size() + 1);
    for (auto& arg : args_vector) {
        exec_args.push_back(arg.data());
    }
    exec_args.push_back(nullptr);

    int exec_error_pipe[2];
    if (pipe2(exec_error_pipe, O_CLOEXEC | O_NONBLOCK) != 0) {
        const int pipe_error = errno;
        close(cwd_fd);
        if (logger_) logger_->error("pipe2() failed: {}", strerror(pipe_error));
        return {-1, pipe_error};
    }

    const pid_t child_pid = fork();
    if (child_pid < 0) {
        const int fork_error = errno;
        close(exec_error_pipe[0]);
        close(exec_error_pipe[1]);
        close(cwd_fd);
        if (logger_) logger_->error("fork() failed: {}", strerror(fork_error));
        return {-1, fork_error};
    }

    if (child_pid == 0) {
        close(exec_error_pipe[0]);
#ifdef PROCESS_MANAGER_TESTING
        const char* failure_stage = std::getenv("DACC_PM_TEST_CHILD_FAILURE");
        if (failure_stage && std::strcmp(failure_stage, "setpgid") == 0) {
            reportChildFailure(exec_error_pipe[1], EPERM);
        }
#endif
        if (setpgid(0, 0) != 0) reportChildFailure(exec_error_pipe[1], errno);
        prepareChildSignals(exec_error_pipe[1]);
#ifdef PROCESS_MANAGER_TESTING
        if (failure_stage && std::strcmp(failure_stage, "fchdir") == 0) {
            reportChildFailure(exec_error_pipe[1], ENOENT);
        }
#endif
        if (fchdir(cwd_fd) != 0) reportChildFailure(exec_error_pipe[1], errno);
        close(cwd_fd);
        execvp(exec_args[0], exec_args.data());
        reportChildFailure(exec_error_pipe[1], errno);
    }

    // Child also sets PGID before exec, closing both sides of the fork/exec race.
    if (setpgid(child_pid, child_pid) != 0 && errno != EACCES && errno != ESRCH) {
        if (logger_) logger_->warn("Parent setpgid failed: {}", strerror(errno));
    }
    close(cwd_fd);
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
        if (stop_loop_) { exec_error = ECANCELED; pipe_read_failed = true; break; }
        const int poll_timeout = remainingPollTimeout(confirmation_deadline);
        if (poll_timeout == 0) {
            confirmation_timed_out = true;
            break;
        }

        pollfd descriptor{exec_error_pipe[0], POLLIN, 0};
        const int poll_result = poll(&descriptor, 1, std::min(poll_timeout, 25));
        if (poll_result < 0) {
            if (errno == EINTR) {
                continue;
            }
            exec_error = errno;
            pipe_read_failed = true;
            break;
        }
        if (poll_result == 0) continue;
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
            logger_->error("Timed out waiting for exec confirmation for {}", game.argv[0]);
        }
        terminateStartedProcess(child_pid);
        close(exec_error_pipe[0]);
        return {-1, ETIMEDOUT};
    }
    close(exec_error_pipe[0]);

    if (received > 0 || pipe_read_failed || !pipe_eof) {
        if (received != sizeof(exec_error) && !pipe_read_failed) {
            exec_error = EIO;
        }
        terminateStartedProcess(child_pid);
        if (logger_) {
            logger_->error("execvp failed for {}: {}", game.argv[0], strerror(exec_error));
        }
        return {-1, exec_error};
    }

    if (logger_) {
        logger_->info("App {} started successfully", game.argv[0]);
        logger_->info("PID={}", child_pid);
    }

    return {child_pid, 0};
}

int ProcessManager::terminateStartedProcess(pid_t child_pid, bool leader_already_exited) {
    siginfo_t info{};
    int inspected;
    do { inspected = waitid(P_PID, child_pid, &info, WEXITED | WNOHANG | WNOWAIT); }
    while (inspected < 0 && errno == EINTR);
    if (child_pid <= 1 || child_pid == getpgrp() || inspected < 0) {
        cleanup_incomplete_ = true;
        return -1;
    }
    const pid_t process_group = getpgid(child_pid);
    if (process_group != child_pid) {
        // Without a verified, still-pinned group leader, negative-PID signaling
        // could target an unrelated group. Limit cleanup to the known child PID.
        cleanup_incomplete_ = true;
        kill(child_pid, SIGKILL);
        int status = -1;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(750);
        while (std::chrono::steady_clock::now() < deadline) {
            const pid_t result = waitpid(child_pid, &status, WNOHANG);
            if (result == child_pid || (result < 0 && errno == ECHILD)) break;
            if (result < 0 && errno != EINTR) break;
            poll(nullptr, 0, 5);
        }
        return status;
    }
    if (info.si_pid == child_pid) leader_already_exited = true;
    // Keep the leader unreaped (even if it already exited) until the LAST group signal.
    // This pins its PID/PGID and prevents signaling an unrelated reused process group.
    GroupState group_state = groupState(process_group, child_pid);
    if (!leader_already_exited || group_state != GroupState::NoLiveMembers) {
        kill(-process_group, SIGTERM);
        const auto grace = std::chrono::steady_clock::now() + kChildTerminationTimeout;
        do {
            poll(nullptr, 0, 5);
            group_state = groupState(process_group, child_pid);
        } while (group_state != GroupState::NoLiveMembers &&
                 std::chrono::steady_clock::now() < grace);
        if (group_state != GroupState::NoLiveMembers) {
            kill(-process_group, SIGKILL);
            // Also covers an unexpected group transition while the PID is pinned.
            kill(child_pid, SIGKILL);
        }
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(750);
    int leader_status = 0;
    bool leader_reaped = false;
    while (std::chrono::steady_clock::now() < deadline) {
        int status = 0;
        const pid_t reaped = waitpid(leader_reaped ? -child_pid : child_pid, &status, WNOHANG);
        if (reaped > 0) {
            if (reaped == child_pid) {
                leader_reaped = true;
                leader_status = status;
                if (logger_) logger_->info("Reaped terminated process PID={}", child_pid);
            }
            continue;
        }
        if (reaped < 0 && errno == ECHILD && leader_reaped) return leader_status;
        if (reaped < 0 && errno != EINTR && errno != ECHILD) break;
        poll(nullptr, 0, 5);
    }
    // An uninterruptible kernel task cannot be killed/reaped within a hard deadline.
    // Fail closed for subsequent launches instead of reusing the slot or hanging shutdown.
    cleanup_incomplete_ = true;
    if (logger_) logger_->critical("process_cleanup_incomplete: PGID={}", child_pid);
    return leader_status;
}

void ProcessManager::setupServerSocket() {
    server_ = std::make_unique<ipc::LocalSocketServer>(socket_path_);
    server_socket_fd_ = server_->fd();
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = server_socket_fd_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_socket_fd_, &ev) != 0) {
        throw std::runtime_error("Failed to add server socket to epoll");
    }
}

void ProcessManager::handleNewConnection() {
    if (stop_loop_) return;
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

    const auto peer = ipc::authorizePeer(client_fd, geteuid());
    if (peer != ipc::PeerStatus::Authorized || connected_clients_.size() >= 64) {
        if (logger_) logger_->warn("Rejected IPC client: {}", ipc::peerErrorCode(peer));
        close(client_fd);
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

    for (int batch = 0; batch < 16 && !should_disconnect && !stop_loop_; ++batch) {
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
    if (!validProtocolId(request_id)) return false;
    auto& seen = seen_requests_[client.fd];
    if (seen.count(request_id)) {
        return !requiresDisconnect(sendClientMessage(client.fd, json{
            {"event", "request_rejected"}, {"request_id", request_id},
            {"code", "duplicate_request"}
        }.dump()));
    }
    if (seen.size() >= 1024) return false;
    seen.insert(request_id);
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
    auto send_failure = [&](int error_code, const std::string& error_message, const std::string& code = "launch_failed") {
        const json response{
            {"event", "game_start_failed"},
            {"request_id", request_id},
            {"game_id", game_id},
            {"error_code", error_code},
            {"code", code},
            {"message", error_message}
        };
        return sendClientMessage(client.fd, response.dump());
    };

    if (!validProtocolId(game_id)) {
        return !requiresDisconnect(send_failure(EINVAL, "Invalid game_id", "invalid_game_id"));
    }
    if (request.contains("path") || request.contains("argv") || request.contains("graphics") ||
        request.contains("cwd")) {
        return !requiresDisconnect(send_failure(EINVAL, "Executable and options belong to server catalog", "client_command_forbidden"));
    }
    const auto* game = catalog_.find(game_id);
    if (!game) return !requiresDisconnect(send_failure(ENOENT, "Unknown game_id", "unknown_game"));
    if (stop_loop_) return !requiresDisconnect(send_failure(ECANCELED, "Server stopping", "server_stopping"));
    if (cleanup_incomplete_) return !requiresDisconnect(send_failure(EBUSY, "Previous cleanup incomplete", "process_cleanup_incomplete"));
    if (!running_processes_.empty()) {
        return !requiresDisconnect(send_failure(EBUSY, "A game is already active", "game_already_running"));
    }
    // This entire check/start/register sequence runs on the single event-loop thread.
    const ProcessStartResult start_result = startApplication(*game);
    if (!start_result.ok()) {
        return !requiresDisconnect(send_failure(
            start_result.error_code,
            std::string(strerror(start_result.error_code))
        ));
    }

    try {
#ifdef PROCESS_MANAGER_TESTING
        if (force_registration_failure_.exchange(false)) throw std::bad_alloc();
#endif
        running_processes_.emplace(start_result.pid, RunningProcess{
            std::chrono::steady_clock::now(),
            request_id,
            game_id,
            client
        });
    } catch (const std::exception&) {
        terminateStartedProcess(start_result.pid);
        return !requiresDisconnect(send_failure(
            ENOMEM,
            "Could not register launched process",
            "state_registration_failed"
        ));
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
    seen_requests_.erase(client_fd);
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
