#pragma once

#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <mutex>

extern const char* const SOCKET_PATH;

struct SockAddr{
    sockaddr_un addr;

    SockAddr() {
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);
    }
};

class LogManager{
public:
    static void logError(const std::string& context, bool includeErrno = true);   
};

#define LOG_ERROR(what) \
    LogManager::logError(std::string(__func__) + " -> " + what)

