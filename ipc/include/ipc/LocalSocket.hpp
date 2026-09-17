#pragma once

#include <string>
#include <sys/types.h>

namespace ipc {

// XDG_RUNTIME_DIR/dacc-station, or explicit private DACC_RUNTIME_DIR in development.
std::string runtimeDirectory();
std::string runtimeDirectory(const std::string& xdg, const std::string& development);
std::string processManagerSocketPath();
std::string logSocketPath();

enum class PeerStatus { Authorized, Unauthorized, CredentialsUnavailable };
PeerStatus authorizePeer(int fd, uid_t expected_uid, pid_t* pid = nullptr);
const char* peerErrorCode(PeerStatus status);
int connectLocalSocket(const std::string& path);

// Owns the listener and a lifetime flock; destruction only removes its own inode.
class LocalSocketServer {
public:
    explicit LocalSocketServer(const std::string& path);
    ~LocalSocketServer();
    LocalSocketServer(const LocalSocketServer&) = delete;
    LocalSocketServer& operator=(const LocalSocketServer&) = delete;
    int fd() const { return socket_fd_; }

private:
    void cleanup() noexcept;
    int directory_fd_{-1};
    int lock_fd_{-1};
    int socket_fd_{-1};
    std::string name_;
    dev_t device_{0};
    ino_t inode_{0};
    bool owns_socket_{false};
};

} // namespace ipc
