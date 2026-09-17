#pragma once

#include "ipc/FramedSocket.hpp"
#include <cstddef>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

enum class LogAdmission { Accepted, LimitReached, Unauthorized, CredentialsUnavailable };

struct LogFeedResult {
    std::vector<std::string> messages;
    bool overflowed{false};
};

class LogProtocol {
public:
    static constexpr std::size_t kMaxClients = 64;
    LogAdmission admit(int fd, uid_t expected_uid);
    LogFeedResult feed(int fd, const char* data, std::size_t size);
    void remove(int fd);
    std::size_t size() const { return readers_.size(); }
private:
    std::unordered_map<int, ipc::FrameReader> readers_;
};
