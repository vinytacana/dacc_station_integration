#pragma once

#include <sys/epoll.h> 
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <set>
#include <string>
#include <future> 
#include <memory>
#include <mutex>

#include <spdlog/spdlog.h>

#define MAX_EVENTS 10
#define SOCKET_PATH "/tmp/gameman.sock"

class ApplicationDefinition;

using TimePoint = std::chrono::steady_clock::time_point;

class ProcessManager{
private:
    pid_t createProcess();
    std::shared_ptr<spdlog::logger> logger_;   

    std::unordered_map<pid_t, TimePoint> running_processes_;
    std::set<int> connected_clients_;
    std::mutex processes_mutex_;

    std::promise<void> ready_promise;
    std::thread event_loop_thread_;
    std::atomic<bool> stop_loop_{false};
    
    int epoll_fd_;
    int signal_fd_;
    int server_socket_fd_;

    void eventLoop();
    void handleChildSignal();
    std::vector<std::string> buildCommand(const ApplicationDefinition& app);
    
    // Novas funções para IPC
    void setupServerSocket();
    void handleNewConnection();
    void handleClientMessage(int client_fd);

public:
    explicit ProcessManager(std::shared_ptr<spdlog::logger> logger);
    ~ProcessManager(); 
    pid_t startApplication(ApplicationDefinition& app);
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