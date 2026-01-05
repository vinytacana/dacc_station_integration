#pragma once

#include <string>
#include <mutex>

class UnixSocketClient {
public:
    explicit UnixSocketClient(const std::string& path);
    ~UnixSocketClient();

    UnixSocketClient(const UnixSocketClient&) = delete;
    UnixSocketClient& operator=(const UnixSocketClient&) = delete;

    void send(const std::string& data);

private:
    int fd_ {-1};
    std::mutex mtx_;
};