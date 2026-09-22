#include "UnixSocketClient.hpp"
#include "ipc/LocalSocket.hpp"

#include <unistd.h> 
#include <sys/un.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>

UnixSocketClient::UnixSocketClient(const std::string& path)
    : fd_(ipc::connectLocalSocket(path)) {}

UnixSocketClient::~UnixSocketClient(){
    if (fd_ >= 0) close(fd_);
}

ipc::SendResult UnixSocketClient::send(
    const std::string& data,
    std::chrono::milliseconds timeout
) {
    if (fd_ < 0) {
        return {ipc::SendStatus::Disconnected, 0, EBADF};
    }

    std::lock_guard<std::mutex> lock(mtx_);
    return ipc::sendMessage(fd_, data, timeout);
}

void UnixSocketClient::setNonBlocking(bool enable) {
    if (fd_ < 0) return;
    int flags = fcntl(fd_, F_GETFL, 0);
    if (enable) {
        fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
    } else {
        fcntl(fd_, F_SETFL, flags & ~O_NONBLOCK);
    }
}

ssize_t UnixSocketClient::receive(void* buffer, size_t size) {
    if (fd_ < 0) return -1;
    return recv(fd_, buffer, size, 0);
}

bool UnixSocketClient::isConnected() const {
    return fd_ >= 0;
}
