#include "LogProtocol.hpp"
#include "ipc/LocalSocket.hpp"

LogAdmission LogProtocol::admit(int fd, uid_t expected_uid) {
    if (readers_.size() >= kMaxClients) return LogAdmission::LimitReached;
    const auto peer = ipc::authorizePeer(fd, expected_uid);
    if (peer == ipc::PeerStatus::Unauthorized) return LogAdmission::Unauthorized;
    if (peer == ipc::PeerStatus::CredentialsUnavailable) return LogAdmission::CredentialsUnavailable;
    readers_.try_emplace(fd);
    return LogAdmission::Accepted;
}

LogFeedResult LogProtocol::feed(int fd, const char* data, std::size_t size) {
    LogFeedResult result;
    auto found = readers_.find(fd);
    if (found == readers_.end()) {
        result.overflowed = true;
        return result;
    }
    found->second.feed(data, size);
    while (auto message = found->second.next()) result.messages.push_back(std::move(*message));
    result.overflowed = found->second.overflowed();
    return result;
}

void LogProtocol::remove(int fd) { readers_.erase(fd); }
