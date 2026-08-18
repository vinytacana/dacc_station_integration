#include "UnixSocketClient.hpp"
#include "UnixSocketUtils.hpp"

#include <unistd.h> 
#include <sys/un.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>

UnixSocketClient::UnixSocketClient(const std::string& path){
    fd_  = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd_ < 0) { 
        throw std::runtime_error("UnixSocketClient: socket failed");
    }

    sockaddr_un addr{};
    UnixSocketUtils::prepareAddress(addr, path);
    
    int ret = connect(fd_, (sockaddr*)&addr, sizeof(addr));
    
    if( ret < 0 ){
        std::string error_msg = strerror(errno);
        close(fd_);
        throw std::runtime_error("UnixSocketClient: connect() failed for path: " + path + " (" + error_msg + ")");
    }

}

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
