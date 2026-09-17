#include "ipc/LocalSocket.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <vector>

template<class F> void rejected(F action) {
    bool failed = false;
    try { action(); } catch (const std::exception&) { failed = true; }
    assert(failed);
}

int main() {
    char pattern[] = "/tmp/dacc-socket-test-XXXXXX";
    const char* created = mkdtemp(pattern);
    assert(created);
    const std::string base(created);
    const auto runtime = ipc::runtimeDirectory(base, "");
    assert(runtime == base + "/dacc-station");
    struct stat runtime_stat{};
    assert(stat(runtime.c_str(), &runtime_stat) == 0);
    assert((runtime_stat.st_mode & 0777) == 0700);
    assert(ipc::runtimeDirectory("", base) == base);
    rejected([] { ipc::runtimeDirectory("", ""); });
    rejected([&] { ipc::runtimeDirectory("/missing/runtime", base); });
    assert(chmod(base.c_str(), 0755) == 0);
    rejected([&] { ipc::runtimeDirectory(base, ""); });
    assert(chmod(base.c_str(), 0700) == 0);
    const std::string path = runtime + "/test.sock";
    {
        ipc::LocalSocketServer first(path);
        struct stat before{}, after{};
        assert(lstat(path.c_str(), &before) == 0);
        assert((before.st_mode & 0777) == 0600);
        rejected([&] { ipc::LocalSocketServer second(path); });
        assert(lstat(path.c_str(), &after) == 0 && before.st_ino == after.st_ino);
        int client = ipc::connectLocalSocket(path);
        int peer = accept4(first.fd(), nullptr, nullptr, SOCK_CLOEXEC);
        assert(peer >= 0);
        pid_t pid = -1;
        assert(ipc::authorizePeer(peer, geteuid(), &pid) == ipc::PeerStatus::Authorized);
        assert(pid == getpid());
        assert(ipc::authorizePeer(peer, geteuid() + 1) == ipc::PeerStatus::Unauthorized);
        assert(ipc::authorizePeer(-1, geteuid()) == ipc::PeerStatus::CredentialsUnavailable);
        close(peer);
        close(client);

        assert(chmod(path.c_str(), 0660) == 0);
        rejected([&] { (void)ipc::connectLocalSocket(path); });
        assert(chmod(path.c_str(), 0600) == 0);
    }
    assert(access(path.c_str(), F_OK) != 0);
    { ipc::LocalSocketServer restart(path); }
    // A socket left by a dead server is recoverable, but a live legacy server is not.
    int stale = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    assert(stale >= 0);
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strcpy(addr.sun_path, path.c_str());
    assert(bind(stale, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    assert(listen(stale, 1) == 0);
    struct stat active_before{}, active_after{};
    assert(lstat(path.c_str(), &active_before) == 0);
    std::vector<int> backlog;
    for (int attempt = 0; attempt < 32; ++attempt) {
        int client = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        assert(client >= 0);
        if (connect(client, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
            backlog.push_back(client);
            continue;
        }
        const int error = errno;
        close(client);
        assert(error == EAGAIN || error == EINPROGRESS);
        break;
    }
    assert(!backlog.empty());
    rejected([&] { ipc::LocalSocketServer active(path); });
    assert(lstat(path.c_str(), &active_after) == 0);
    assert(active_before.st_ino == active_after.st_ino);
    for (int client : backlog) close(client);
    close(stale);
    { ipc::LocalSocketServer recovered(path); }
    { std::ofstream file(path); file << "preserve"; }
    rejected([&] { ipc::LocalSocketServer regular(path); });
    assert(std::filesystem::is_regular_file(path));
    assert(unlink(path.c_str()) == 0);
    assert(symlink("/not/a/socket", path.c_str()) == 0);
    rejected([&] { ipc::LocalSocketServer symlinked(path); });
    assert(std::filesystem::is_symlink(path));
    assert(unlink(path.c_str()) == 0);
    // Destructor must not remove a replacement inode.
    {
        ipc::LocalSocketServer server(path);
        assert(unlink(path.c_str()) == 0);
        std::ofstream replacement(path);
        replacement << "preserve";
    }
    assert(std::filesystem::is_regular_file(path));

    if (geteuid() == 0) {
        const std::string wrong_owner = base + "/wrong-owner";
        assert(mkdir(wrong_owner.c_str(), 0700) == 0);
        assert(chown(wrong_owner.c_str(), 1, 1) == 0);
        rejected([&] { ipc::runtimeDirectory("", wrong_owner); });
        assert(chown(wrong_owner.c_str(), 0, 0) == 0);
    }

    const std::string umask_base = base + "/umask-base";
    assert(mkdir(umask_base.c_str(), 0700) == 0);
    const mode_t old_umask = umask(0777);
    const auto umask_runtime = ipc::runtimeDirectory(umask_base, "");
    umask(old_umask);
    assert(stat(umask_runtime.c_str(), &runtime_stat) == 0);
    assert((runtime_stat.st_mode & 0777) == 0700);
    assert(symlink(runtime.c_str(), (base + "/link").c_str()) == 0);
    rejected([&] { ipc::runtimeDirectory("", base + "/link"); });
    std::filesystem::remove_all(base);
    std::cout << "[PASS] secure runtime, socket lifecycle and peer credentials\n";
}
