#include "SocketLogger.hpp"
#include "UnixSocketClient.hpp"

SocketLogger::SocketLogger(const std::string &path) : _socket_client_{std::make_unique<UnixSocketClient>(path)}{ }

SocketLogger::~SocketLogger() = default;

void SocketLogger::log(const std::string& message) {
    if(_socket_client_){
        _socket_client_->send(message + "\n");
    }
}