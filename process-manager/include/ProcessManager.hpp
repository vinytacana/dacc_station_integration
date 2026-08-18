#pragma once

#include <sys/epoll.h> 
#include <sys/signalfd.h>
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
#include <mutex>

#include <spdlog/spdlog.h>

#include "ipc/FramedSocket.hpp"

#define MAX_EVENTS 10
class ApplicationDefinition;

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
    std::shared_ptr<spdlog::logger> logger_;   

    std::unordered_map<pid_t, RunningProcess> running_processes_;
    std::unordered_map<int, std::uint64_t> connected_clients_;
    std::unordered_map<int, ipc::FrameReader> client_frame_readers_;
    std::mutex processes_mutex_;
    std::uint64_t connection_seq_{0};

    std::promise<void> ready_promise;
    std::thread event_loop_thread_;
    std::atomic<bool> stop_loop_{false};
    
    int epoll_fd_;
    int signal_fd_;
    int server_socket_fd_;
    std::string socket_path_;

    void eventLoop();
    void handleChildSignal();
    std::vector<std::string> buildCommand(const ApplicationDefinition& app);
    
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
    void terminateStartedProcess(pid_t child_pid);

public:
    explicit ProcessManager(
        std::shared_ptr<spdlog::logger> logger,
        std::string socket_path = "/tmp/gameman.sock"
    );
    ~ProcessManager(); 
    ProcessStartResult startApplication(ApplicationDefinition& app);
    bool isAppValid(const ApplicationDefinition& app) const;

};

class ApplicationDefinition
{
private:
    std::string id_;
    std::string executable_path_;
    std::string type_;
    
public:
    // Graphics Configuration
    int renderWidth = 1280;
    int renderHeight = 720;
    int outputWidth = 0;   // 0 = Auto
    int outputHeight = 0;  // 0 = Auto
    int refreshRate = 60;
    bool useGamescope = false;
    bool fullscreen = true;

    ApplicationDefinition(std::string id, const char* path, std::string type) : 
        id_{std::move(id)}, executable_path_{path}, type_{std::move(type)} { }

    const std::string& getId() const { return id_; }
    const std::string& getPath() const { return executable_path_; }
    const std::string& getType() const { return type_; }
};
