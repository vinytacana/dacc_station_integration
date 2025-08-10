#ifndef __GAMEMAN_H__
#define __GAMEMAN_H__

#include <sys/epoll.h> 
#include <sys/signalfd.h>

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <future> 

#include "./logs/LogClient.hpp"

#define MAX_EVENTS 10

class Game;

using TimePoint = std::chrono::steady_clock::time_point;

class ProcessManager{
private:
    pid_t createProcess();
    LogClient logger;   

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
    ProcessManager();
    ~ProcessManager(); 
    bool startGame(Game* game);
    bool isGameValid(Game* game);
    bool startProcess(Game* game);
    // bool stopProcess(pid_t processId);
    // bool isProcessRunning(pid_t processId);
    // int getProcessExitCode(pid_t processId);
    // std::vector<char*> constructArgList(Game* game);
    // guardar tempo rodado
};

class Game
{
    private:
    const char* path;
    
    public:
    std::string filename;
    std::string type;
    Game(std::string filename, const char* path, std::string type) : 
        filename{std::move(filename)}, path{path}, type{std::move(type)} { }

    // ~Game();
    const char* getPath() const { return path; }
};
#endif

