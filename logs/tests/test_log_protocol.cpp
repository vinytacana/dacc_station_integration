#include "LogProtocol.hpp"
#include "UnixSocketClient.hpp"
#include "ipc/LocalSocket.hpp"

#include <array>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

int main() {
    LogProtocol protocol;
    std::vector<std::array<int, 2>> pairs;
    for (std::size_t i = 0; i < LogProtocol::kMaxClients; ++i) {
        std::array<int, 2> pair{};
        assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, pair.data()) == 0);
        assert(protocol.admit(pair[0], geteuid()) == LogAdmission::Accepted);
        pairs.push_back(pair);
    }
    std::array<int, 2> excess{};
    assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, excess.data()) == 0);
    assert(protocol.admit(excess[0], geteuid()) == LogAdmission::LimitReached);
    for (auto pair : pairs) { protocol.remove(pair[0]); close(pair[0]); close(pair[1]); }
    close(excess[0]); close(excess[1]);

    std::array<int, 2> peer{};
    assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, peer.data()) == 0);
    assert(protocol.admit(peer[0], geteuid() + 1) == LogAdmission::Unauthorized);
    assert(protocol.admit(-1, geteuid()) == LogAdmission::CredentialsUnavailable);
    assert(protocol.admit(peer[0], geteuid()) == LogAdmission::Accepted);
    auto partial = protocol.feed(peer[0], "partial", 7);
    assert(partial.messages.empty() && !partial.overflowed);
    auto completed = protocol.feed(peer[0], " frame\none\ntwo\n", 15);
    assert(completed.messages.size() == 3);
    assert(completed.messages[0] == "partial frame");
    assert(completed.messages[1] == "one" && completed.messages[2] == "two");
    std::string oversized(ipc::kMaxFrameBytes + 1, 'x');
    auto overflow = protocol.feed(peer[0], oversized.data(), oversized.size());
    assert(overflow.overflowed);
    protocol.remove(peer[0]);
    close(peer[0]); close(peer[1]);

    std::array<int, 2> abrupt{};
    assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, abrupt.data()) == 0);
    assert(protocol.admit(abrupt[0], geteuid()) == LogAdmission::Accepted);
    close(abrupt[1]);
    char closed_byte{};
    assert(recv(abrupt[0], &closed_byte, 1, 0) == 0);
    protocol.remove(abrupt[0]);
    close(abrupt[0]);

    char pattern[] = "/tmp/dacc-log-protocol-XXXXXX";
    const char* root = mkdtemp(pattern);
    assert(root);
    const std::string path = std::string(root) + "/log.sock";
    ipc::LocalSocketServer server(path);
    {
        UnixSocketClient client(path);
        int accepted = accept4(server.fd(), nullptr, nullptr, SOCK_CLOEXEC);
        assert(accepted >= 0);
        assert(protocol.admit(accepted, geteuid()) == LogAdmission::Accepted);
        assert(client.send("wire-format", std::chrono::milliseconds(250)).status == ipc::SendStatus::Ok);
        char data[64]{};
        const ssize_t count = recv(accepted, data, sizeof(data), 0);
        assert(count > 0);
        auto wire = protocol.feed(accepted, data, static_cast<std::size_t>(count));
        assert(wire.messages.size() == 1 && wire.messages[0] == "wire-format");
        protocol.remove(accepted);
        close(accepted);
    }
    std::filesystem::remove_all(root);
    std::cout << "Log protocol tests passed.\n";
}
