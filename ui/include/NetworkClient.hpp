#pragma once

#include <string>
#include <functional>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

class NetworkClient {
public:
    static NetworkClient& getInstance();

    bool connectToManager();
    void sendStartGame(const std::string& id, const std::string& path, 
                      int width, int height, int fps, bool useGamescope);
    void checkEvents(); // Call this in the main loop
    
    // State Getters
    bool isGameRunning() const { return isRunning_; }
    std::string getRunningGameId() const { return runningGameId_; }

private:
    NetworkClient();
    ~NetworkClient();
    
    int socket_fd_ = -1;
    bool connected_ = false;
    
    // Game State
    bool isRunning_ = false;
    std::string runningGameId_;

    // Prevent copying
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;
};
