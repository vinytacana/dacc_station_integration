#include "NetworkClient.hpp"
#include <cstring>
#include <fcntl.h>
#include <cerrno>

#define SOCKET_PATH "/tmp/gameman.sock"

NetworkClient& NetworkClient::getInstance() {
    static NetworkClient instance;
    return instance;
}

NetworkClient::NetworkClient() {}

NetworkClient::~NetworkClient() {
    if (socket_fd_ != -1) {
        close(socket_fd_);
    }
}

bool NetworkClient::connectToManager() {
    if (connected_) return true;

    socket_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd_ == -1) {
        std::cerr << "[NetworkClient] Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        // Don't log spam if just checking
        // std::cerr << "[NetworkClient] Failed to connect: " << strerror(errno) << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    // Set non-blocking mode
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK);

    connected_ = true;
    std::cout << "[NetworkClient] Connected to Process Manager." << std::endl;
    return true;
}

void NetworkClient::sendStartGame(const std::string& id, const std::string& path, 
                                 int width, int height, int fps, bool useGamescope) {
    if (!connectToManager()) {
        std::cerr << "[NetworkClient] Cannot start game: Not connected." << std::endl;
        return;
    }

    json j;
    j["action"] = "start";
    j["id"] = id;
    j["path"] = path;

    json graphics;
    graphics["use_gamescope"] = useGamescope;
    graphics["width"] = width;
    graphics["height"] = height;
    graphics["fps"] = fps;
    j["graphics"] = graphics;

    std::string msg = j.dump();

    if (send(socket_fd_, msg.c_str(), msg.size(), MSG_NOSIGNAL) == -1) {
        std::cerr << "[NetworkClient] Failed to send message: " << strerror(errno) << std::endl;
        connected_ = false;
        close(socket_fd_);
        socket_fd_ = -1;
    } else {
        std::cout << "[NetworkClient] Start command sent for: " << id << std::endl;
    }
}

void NetworkClient::checkEvents() {
    if (!connected_) return;

    char buffer[1024];
    ssize_t bytes_read = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::cout << "[NetworkClient] Received: " << buffer << std::endl;
        
        try {
            auto j = json::parse(buffer);
            if (j.contains("event") && j["event"] == "game_finished") {
                std::cout << ">>> JOGO TERMINOU! PID: " << j.value("pid", -1) << " <<<" << std::endl;
                // Aqui poderíamos disparar um callback para a UI atualizar
            }
        } catch (const std::exception& e) {
            std::cerr << "[NetworkClient] JSON parse error: " << e.what() << std::endl;
        }

    } else if (bytes_read == 0) {
        std::cout << "[NetworkClient] Server disconnected." << std::endl;
        connected_ = false;
        close(socket_fd_);
        socket_fd_ = -1;
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "[NetworkClient] Recv error: " << strerror(errno) << std::endl;
            connected_ = false;
            close(socket_fd_);
            socket_fd_ = -1;
        }
    }
}
