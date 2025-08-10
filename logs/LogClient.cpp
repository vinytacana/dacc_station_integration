#include <string>
#include <mutex>
#include <iostream>

#include "LogClient.hpp"

LogClient::LogClient(){
    fd_ = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd_ < 0) { LOG_ERROR("socket()"); return; }
    
    const SockAddr client_socket{};
    
    if (connect(fd_,
        (sockaddr*)&client_socket.addr, 
        sizeof(client_socket.addr)) < 0) {
            
        LOG_ERROR("connect()"); 
        close(fd_); 
        fd_ = -1;
    }

    std::cout << ("connected.");
}

void LogClient::log(const std::string& msg) {
    if (fd_ < 0) {
        LOG_ERROR("file descriptor on LogClient");
        return; 
    };

    std::lock_guard<std::mutex> lk(mtx_);
    std::string out = msg + "\n";

    ssize_t n = write(fd_, out.data(), out.size());
    if (n < 0) {
        LOG_ERROR("write()");
    }
}