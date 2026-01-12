#include "NetworkClient.hpp"
#include <cstring>
#include <cerrno>
#include "LogManager.hpp"

#define SOCKET_PATH "/tmp/gameman.sock"

NetworkClient& NetworkClient::getInstance() {
    static NetworkClient instance;
    return instance;
}

NetworkClient::NetworkClient() {}

NetworkClient::~NetworkClient() {
    // UnixSocketClient destructor handles cleanup
}

bool NetworkClient::connectToManager() {
    if (client_ && client_->isConnected()) return true;

    try {
        client_ = std::make_unique<UnixSocketClient>(SOCKET_PATH);
        client_->setNonBlocking(true);
        LOG_INFO("Connected to Process Manager.");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to connect to Process Manager: " + std::string(e.what()));
        client_.reset();
        return false;
    }
}

void NetworkClient::sendStartGame(const std::string& id, const std::string& path) {
    if (!connectToManager()) {
        LOG_WARNING("Cannot start game: Not connected.");
        return;
    }

    json j;
    j["action"] = "start";
    j["id"] = id;
    j["path"] = path;

    std::string msg = j.dump();
    client_->send(msg); // UnixSocketClient::send uses mutex and handles writing
    
    LOG_INFO("Start command sent for: " + id);
    // Assume sucesso imediato para a UI (Optimistic Update)
    isRunning_ = true;
    runningGameId_ = id;
}

void NetworkClient::checkEvents() {
    if (!client_) return;

    char buffer[1024];
    ssize_t bytes_read = client_->receive(buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        LOG_DEBUG("Received: " + std::string(buffer));
        
        try {
            auto j = json::parse(buffer);
            if (j.contains("event")) {
                std::string evt = j["event"];
                
                if (evt == "game_finished" || evt == "process_exited") {
                    LOG_INFO(">>> JOGO TERMINOU! <<<");
                    isRunning_ = false;
                    runningGameId_ = "";
                }
                else if (evt == "game_started") {
                     isRunning_ = true;
                     if(j.contains("id")) runningGameId_ = j["id"];
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("JSON parse error: " + std::string(e.what()));
        }

    } else if (bytes_read == 0) {
        LOG_INFO("Server disconnected.");
        client_.reset();
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("Recv error: " + std::string(strerror(errno)));
            client_.reset();
        }
    }
}