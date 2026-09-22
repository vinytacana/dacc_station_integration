#include "ipc/FramedSocket.hpp"
#include "ipc/LocalSocket.hpp"
#include "json.hpp"

#include <cassert>
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <poll.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

using json = nlohmann::json;
using namespace std::chrono_literals;

pid_t startDaemon(const std::string& runtime, const std::string& catalog) {
    const pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) {
        unsetenv("XDG_RUNTIME_DIR");
        setenv("DACC_RUNTIME_DIR", runtime.c_str(), 1);
        setenv("DACC_GAME_CATALOG", catalog.c_str(), 1);
        const int null_fd = open("/dev/null", O_WRONLY | O_CLOEXEC);
        if (null_fd >= 0) { dup2(null_fd, STDOUT_FILENO); dup2(null_fd, STDERR_FILENO); }
        execl("../bin/process-manager", "process-manager", nullptr);
        _exit(127);
    }
    return pid;
}

int connectEventually(const std::string& socket_path) {
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    while (std::chrono::steady_clock::now() < deadline) {
        try { return ipc::connectLocalSocket(socket_path); }
        catch (...) { std::this_thread::sleep_for(10ms); }
    }
    assert(false && "daemon socket should become available");
    return -1;
}

json receiveEvent(int fd, ipc::FrameReader& reader) {
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    char data[4096];
    while (std::chrono::steady_clock::now() < deadline) {
        if (auto message = reader.next()) return json::parse(*message);
        pollfd descriptor{fd, POLLIN, 0};
        const int result = poll(&descriptor, 1, 50);
        if (result < 0 && errno == EINTR) continue;
        assert(result >= 0);
        if (result == 0) continue;
        const ssize_t count = recv(fd, data, sizeof(data), 0);
        assert(count > 0);
        reader.feed(data, static_cast<std::size_t>(count));
    }
    assert(false && "daemon should emit event");
    return {};
}

void waitDaemon(pid_t pid, int expected_status) {
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    int status = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        const pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) {
            assert(WIFEXITED(status) && WEXITSTATUS(status) == expected_status);
            return;
        }
        assert(result == 0 || (result < 0 && errno == EINTR));
        std::this_thread::sleep_for(10ms);
    }
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);
    assert(false && "daemon shutdown must be bounded");
}

void processGone(pid_t pid) {
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline) {
        errno = 0;
        if (kill(pid, 0) < 0 && errno == ESRCH) return;
        std::this_thread::sleep_for(10ms);
    }
    assert(false && "game descendant survived daemon shutdown");
}

int main() {
    char pattern[] = "/tmp/dacc-daemon-test-XXXXXX";
    const char* made = mkdtemp(pattern);
    assert(made);
    const std::string root(made);
    const std::string child_file = root + "/child.pid";
    const std::string script = root + "/tree.sh";
    const std::string catalog = root + "/games.json";
    const std::string socket_path = root + "/process-manager.sock";
    {
        std::ofstream output(script);
        output << "#!/bin/sh\ntrap '' TERM\n(trap '' TERM; while :; do sleep 1; done) &\n"
               << "echo $! > '" << child_file << "'\nwhile :; do sleep 1; done\n";
    }
    assert(chmod(script.c_str(), 0700) == 0);
    {
        std::ofstream output(catalog);
        output << json{{"games", json::array({{{"id", "tree"}, {"argv", json::array({"tree.sh"})}, {"cwd", "."}}})}};
    }
    assert(chmod(catalog.c_str(), 0600) == 0);

    pid_t daemon = startDaemon(root, catalog);
    int client = connectEventually(socket_path);
    const auto command = json{{"action", "start"}, {"request_id", "sigterm-daemon"}, {"game_id", "tree"}};
    assert(ipc::sendMessage(client, command.dump(), 500ms).status == ipc::SendStatus::Ok);
    ipc::FrameReader reader;
    const pid_t leader = receiveEvent(client, reader).value("pid", -1);
    pid_t child = -1;
    for (int attempt = 0; attempt < 200 && child < 1; ++attempt) {
        std::ifstream input(child_file);
        input >> child;
        if (child < 1) std::this_thread::sleep_for(10ms);
    }
    assert(leader > 1 && child > 1);
    assert(kill(daemon, SIGTERM) == 0);
    waitDaemon(daemon, 0);
    close(client);
    processGone(leader);
    processGone(child);
    assert(access(socket_path.c_str(), F_OK) != 0);

    daemon = startDaemon(root, catalog);
    client = connectEventually(socket_path);
    close(client);
    assert(kill(daemon, SIGKILL) == 0);
    int crash_status = 0;
    assert(waitpid(daemon, &crash_status, 0) == daemon);
    assert(WIFSIGNALED(crash_status) && WTERMSIG(crash_status) == SIGKILL);
    assert(access(socket_path.c_str(), F_OK) == 0);

    daemon = startDaemon(root, catalog);
    client = connectEventually(socket_path);
    close(client);
    assert(kill(daemon, SIGTERM) == 0);
    waitDaemon(daemon, 0);
    assert(access(socket_path.c_str(), F_OK) != 0);
    std::filesystem::remove_all(root);
}
