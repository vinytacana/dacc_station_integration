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
    
    // New methods for generalized client usage (e.g. UI)
    void setNonBlocking(bool enable);
    ssize_t receive(void* buffer, size_t size);
    bool isConnected() const;

private:
    int fd_ {-1};
    std::mutex mtx_;
};