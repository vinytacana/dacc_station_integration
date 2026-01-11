#pragma once

#include <string>
#include <functional>
#include <iostream>
#include <memory>
#include "json.hpp"
#include "UnixSocketClient.hpp"

using json = nlohmann::json;

class NetworkClient {
public:
    static NetworkClient& getInstance();

    bool connectToManager();
    // Updated signature to support graphics options
    void sendStartGame(const std::string& id, const std::string& path, 
                      int width = 1280, int height = 720, int fps = 60, bool useGamescope = true);
                      
    void checkEvents(); // Call this in the main loop
    
    // State Getters
    bool isGameRunning() const { return isRunning_; }
    std::string getRunningGameId() const { return runningGameId_; }

private:
    NetworkClient();
    ~NetworkClient();
    
    std::unique_ptr<UnixSocketClient> client_;
    
    // Game State
    bool isRunning_ = false;
    std::string runningGameId_;

    // Prevent copying
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;
};