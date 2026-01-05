#pragma once

#include <sys/epoll.h> 
#include <sys/signalfd.h>

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <string>
#include <future> 
#include <memory>

#include "logs/ILogger.hpp"

#define MAX_EVENTS 10

class ApplicationDefinition;

using TimePoint = std::chrono::steady_clock::time_point;

class ProcessManager{
private:
    pid_t createProcess();
    std::unique_ptr<ILogger> logger_;   

    std::unordered_map<pid_t, TimePoint> running_processes_;
    std::mutex processes_mutex_;

    std::promise<void> ready_promise;
    std::thread event_loop_thread_;
    std::atomic<bool> stop_loop_{false};
    
    int epoll_fd_;
    int signal_fd_;

    void eventLoop();
    void handleChildSignal();

public:
    explicit ProcessManager(std::unique_ptr<ILogger> logger);
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
    ApplicationDefinition(std::string id, const char* path, std::string type) : 
        id_{std::move(id)}, executable_path_{path}, type_{std::move(type)} { }

    const std::string& getId() const { return id_; }
    const std::string& getPath() const { return executable_path_; }
    const std::string& getType() const { return type_; }
};

