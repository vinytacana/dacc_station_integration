#pragma once

#include <sys/epoll.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <iostream>
#include <vector>
#include <chrono>
#include <cstdint>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <string>
#include <future> 
#include <memory>

#include <spdlog/spdlog.h>

#include "ipc/FramedSocket.hpp"
#include "ipc/LocalSocket.hpp"
#include "GameCatalog.hpp"
#include <unordered_set>

#define MAX_EVENTS 10

using TimePoint = std::chrono::steady_clock::time_point;

struct ProcessStartResult {
    pid_t pid{-1};
    int error_code{0};

    bool ok() const noexcept { return pid > 0 && error_code == 0; }
};

struct ClientConnection {
    int fd{-1};
    std::uint64_t generation{0};
};

struct RunningProcess {
    TimePoint start_time;
    std::string request_id;
    std::string game_id;
    ClientConnection client;
};

class ProcessManager{
private:
    GameCatalog catalog_;
    std::unordered_map<int, std::unordered_set<std::string>> seen_requests_;
    std::shared_ptr<spdlog::logger> logger_;   

    std::unordered_map<pid_t, RunningProcess> running_processes_;
    std::unordered_map<int, std::uint64_t> connected_clients_;
    std::unordered_map<int, ipc::FrameReader> client_frame_readers_;
    std::uint64_t connection_seq_{0};

    std::promise<void> ready_promise;
    std::thread event_loop_thread_;
    std::atomic<bool> stop_loop_{false};
    
    int epoll_fd_{-1};
    int wake_fd_{-1};
    int server_socket_fd_{-1};
    int previous_subreaper_{0};
    bool owns_supervision_{false};
    std::atomic<bool> cleanup_incomplete_{false};
#ifdef PROCESS_MANAGER_TESTING
    std::atomic<bool> force_registration_failure_{false};
#endif
    std::string socket_path_;
    std::unique_ptr<ipc::LocalSocketServer> server_;

    void eventLoop();
    void reapFinishedGame();
    void finishGame(pid_t pid, bool leader_already_exited);
    void cleanupResources() noexcept;
    ProcessStartResult startApplication(const GameCommand& game);
    
    // Novas funções para IPC
    void setupServerSocket();
    void handleNewConnection();
    void handleClientMessage(int client_fd);
    bool processClientMessage(
        const ClientConnection& client,
        const std::string& message
    );
    ipc::SendStatus sendClientMessage(int client_fd, const std::string& message);
    void disconnectClient(int client_fd);
    int terminateStartedProcess(pid_t child_pid, bool leader_already_exited = false);

public:
    explicit ProcessManager(
        std::shared_ptr<spdlog::logger> logger,
        std::string socket_path = ipc::processManagerSocketPath(),
        std::string catalog_path = defaultGameCatalogPath()
    );
    ~ProcessManager(); 

#ifdef PROCESS_MANAGER_TESTING
    void forceCleanupIncompleteForTest() { cleanup_incomplete_ = true; }
    void forceRegistrationFailureForTest() { force_registration_failure_ = true; }
#endif

};
