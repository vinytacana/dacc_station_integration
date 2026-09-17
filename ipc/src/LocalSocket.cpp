#include "ipc/LocalSocket.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <poll.h>
#include <stdexcept>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

namespace ipc {
namespace {

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message + ": " + std::strerror(errno));
}

// Walk every component with O_NOFOLLOW, including ancestors.
int openDirectory(const std::string& path) {
    if (path.empty() || path[0] != '/') {
        throw std::runtime_error("runtime_directory_invalid: absolute directory required");
    }
    int fd = open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) fail("open root");
    for (const auto& part : std::filesystem::path(path).relative_path()) {
        if (part == "." || part == "..") {
            close(fd);
            throw std::runtime_error("runtime_directory_invalid: dot components forbidden");
        }
        int next = openat(fd, part.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        const int error = errno;
        close(fd);
        if (next < 0) { errno = error; fail("runtime_directory_unsafe"); }
        fd = next;
    }
    struct stat st{};
    if (fstat(fd, &st) != 0 || st.st_uid != geteuid() || (st.st_mode & 0777) != 0700) {
        close(fd);
        throw std::runtime_error("runtime_directory_unsafe: owner must be current UID and mode 0700");
    }
    return fd;
}

sockaddr_un address(const std::string& path) {
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (path.empty() || path.size() >= sizeof(addr.sun_path)) {
        throw std::runtime_error("socket_path_invalid: Unix socket path too long");
    }
    std::memcpy(addr.sun_path, path.c_str(), path.size() + 1);
    return addr;
}

std::string anchoredPath(int directory, const std::string& name) {
    const std::string anchor = "/proc/self/fd/" + std::to_string(directory);
    struct stat descriptor{}, proc_entry{};
    if (fstat(directory, &descriptor) != 0 || stat(anchor.c_str(), &proc_entry) != 0 ||
        descriptor.st_dev != proc_entry.st_dev || descriptor.st_ino != proc_entry.st_ino) {
        throw std::runtime_error("proc_fd_unavailable: /proc/self/fd is required for anchored Unix sockets");
    }
    return anchor + "/" + name;
}

int connectNonBlocking(int fd, const sockaddr_un& addr, int timeout_ms) {
    if (connect(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == 0) return 0;
    int error = errno;
    if (error != EINPROGRESS) return error;

    pollfd descriptor{fd, POLLOUT, 0};
    int result;
    do { result = poll(&descriptor, 1, timeout_ms); } while (result < 0 && errno == EINTR);
    if (result == 0) return ETIMEDOUT;
    if (result < 0) return errno;
    socklen_t size = sizeof(error);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &size) != 0) return errno;
    return error;
}

void validateSocket(const struct stat& st) {
    if (!S_ISSOCK(st.st_mode) || st.st_uid != geteuid()) {
        throw std::runtime_error("socket_path_unsafe: expected Unix socket owned by current UID");
    }
}

} // namespace

std::string runtimeDirectory(const std::string& xdg, const std::string& development) {
    if (xdg.empty() && development.empty()) {
        throw std::runtime_error("runtime_directory_missing: set XDG_RUNTIME_DIR or private DACC_RUNTIME_DIR");
    }
    const std::string base = xdg.empty() ? development : xdg;
    int fd = openDirectory(base);
    if (xdg.empty()) { close(fd); return base; }
    bool created = false;
    if (mkdirat(fd, "dacc-station", 0700) == 0) {
        created = true;
    } else if (errno != EEXIST) {
        int error = errno; close(fd); errno = error; fail("create runtime directory");
    }
    if (created && fchmodat(fd, "dacc-station", 0700, 0) != 0) {
        int error = errno; close(fd); errno = error; fail("chmod runtime directory");
    }
    close(fd);
    const std::string result = base + "/dacc-station";
    fd = openDirectory(result);
    close(fd);
    return result;
}

std::string runtimeDirectory() {
    const char* xdg = std::getenv("XDG_RUNTIME_DIR");
    const char* development = std::getenv("DACC_RUNTIME_DIR");
    return runtimeDirectory(xdg ? xdg : "", development ? development : "");
}
std::string processManagerSocketPath() { return runtimeDirectory() + "/process-manager.sock"; }
std::string logSocketPath() { return runtimeDirectory() + "/log-server.sock"; }

PeerStatus authorizePeer(int fd, uid_t expected_uid, pid_t* pid) {
    ucred credentials{};
    socklen_t size = sizeof(credentials);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &credentials, &size) != 0 ||
        size != sizeof(credentials) || credentials.pid <= 0) {
        return PeerStatus::CredentialsUnavailable;
    }
    if (pid) *pid = credentials.pid;
    return credentials.uid == expected_uid ? PeerStatus::Authorized : PeerStatus::Unauthorized;
}
const char* peerErrorCode(PeerStatus status) {
    switch (status) {
    case PeerStatus::Authorized: return "ok";
    case PeerStatus::Unauthorized: return "peer_unauthorized";
    default: return "peer_credentials_unavailable";
    }
}

int connectLocalSocket(const std::string& path) {
    const auto p = std::filesystem::path(path);
    int directory = openDirectory(p.parent_path().string());
    int fd = -1;
    try {
        struct stat st{};
        if (fstatat(directory, p.filename().c_str(), &st, AT_SYMLINK_NOFOLLOW) != 0) fail("socket unavailable");
        validateSocket(st);
        if ((st.st_mode & 0777) != 0600) throw std::runtime_error("socket_mode_unsafe");
        auto addr = address(anchoredPath(directory, p.filename().string()));
        fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        if (fd < 0) fail("socket");
        const int error = connectNonBlocking(fd, addr, 250);
        if (error != 0) { errno = error; fail("connect"); }
        const auto status = authorizePeer(fd, geteuid());
        if (status != PeerStatus::Authorized) throw std::runtime_error(peerErrorCode(status));
        close(directory);
        return fd;
    } catch (...) {
        if (fd >= 0) close(fd);
        close(directory);
        throw;
    }
}

LocalSocketServer::LocalSocketServer(const std::string& path) {
    try {
        (void)address(path);
        const auto p = std::filesystem::path(path);
        name_ = p.filename().string();
        if (name_.empty()) throw std::runtime_error("socket_path_invalid");
        directory_fd_ = openDirectory(p.parent_path().string());
        // Never unlink this lock: an unlinked flock would allow two owners.
        const std::string lock_name = name_ + ".lock";
        lock_fd_ = openat(directory_fd_, lock_name.c_str(), O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0600);
        if (lock_fd_ < 0) fail("instance_lock_unsafe");
        struct stat lock_stat{};
        if (fstat(lock_fd_, &lock_stat) != 0 || !S_ISREG(lock_stat.st_mode) ||
            lock_stat.st_uid != geteuid() || lock_stat.st_nlink != 1 || (lock_stat.st_mode & 0777) != 0600) {
            throw std::runtime_error("instance_lock_unsafe");
        }
        if (flock(lock_fd_, LOCK_EX | LOCK_NB) != 0) throw std::runtime_error("instance_already_running");
        struct stat existing{};
        const auto addr = address(anchoredPath(directory_fd_, name_));
        if (fstatat(directory_fd_, name_.c_str(), &existing, AT_SYMLINK_NOFOLLOW) == 0) {
            validateSocket(existing);
            int probe = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
            if (probe < 0) fail("probe socket");
            const int error = connectNonBlocking(probe, addr, 100);
            close(probe);
            if (error == 0) throw std::runtime_error("instance_already_running");
            // EAGAIN (full backlog), EACCES and other ambiguous errors are not stale evidence.
            if (error != ECONNREFUSED) throw std::runtime_error("socket_state_uncertain");
            struct stat now{};
            if (fstatat(directory_fd_, name_.c_str(), &now, AT_SYMLINK_NOFOLLOW) != 0 ||
                now.st_dev != existing.st_dev || now.st_ino != existing.st_ino) {
                throw std::runtime_error("socket_changed_during_probe");
            }
            if (unlinkat(directory_fd_, name_.c_str(), 0) != 0) fail("remove stale socket");
        } else if (errno != ENOENT) { fail("inspect socket"); }
        socket_fd_ = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (socket_fd_ < 0) fail("socket");
        if (bind(socket_fd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0) fail("bind");
        struct stat bound{};
        if (fstatat(directory_fd_, name_.c_str(), &bound, AT_SYMLINK_NOFOLLOW) != 0) fail("stat bound socket");
        device_ = bound.st_dev;
        inode_ = bound.st_ino;
        owns_socket_ = true;
        if (fchmodat(directory_fd_, name_.c_str(), 0600, 0) != 0) fail("chmod socket");
        if (listen(socket_fd_, 16) != 0) fail("listen");
    } catch (...) { cleanup(); throw; }
}

void LocalSocketServer::cleanup() noexcept {
    if (socket_fd_ >= 0) close(socket_fd_);
    if (owns_socket_) {
        struct stat st{};
        if (fstatat(directory_fd_, name_.c_str(), &st, AT_SYMLINK_NOFOLLOW) == 0 &&
            S_ISSOCK(st.st_mode) && st.st_uid == geteuid() && st.st_dev == device_ && st.st_ino == inode_) {
            unlinkat(directory_fd_, name_.c_str(), 0);
        }
    }
    if (lock_fd_ >= 0) close(lock_fd_);
    if (directory_fd_ >= 0) close(directory_fd_);
}
LocalSocketServer::~LocalSocketServer() { cleanup(); }

} // namespace ipc
