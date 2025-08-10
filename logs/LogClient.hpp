#pragma once

#include "LogManager.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <string>
#include <mutex>

class LogClient {

public:
    LogClient();
    ~LogClient(){ if (fd_ >= 0) close(fd_);}
    
    void log(const std::string &msg);

private:
    int fd_;
    std::mutex mtx_;
};
