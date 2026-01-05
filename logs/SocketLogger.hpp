#pragma once 

#include "ILogger.hpp"

#include <string>
#include <memory>

class UnixSocketClient;

class SocketLogger : public ILogger {
public:
    explicit SocketLogger(const std::string& socket_path);
    ~SocketLogger() override;
    
    void log(const std::string& message) override;

private:
    std::unique_ptr<UnixSocketClient> _socket_client_;
};
