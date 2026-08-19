/**
 * @file NetworkClient.cpp
 * @brief Implementação do cliente de comunicação com o Process Manager via Unix Socket.
 * 
 * Gerencia a conexão com o daemon de gerenciamento de processos, enviando comandos
 * de inicialização de jogos e recebendo eventos sobre o estado dos processos.
 */

#include "NetworkClient.hpp"
#include "Utils.hpp"
#include <chrono>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include "LogManager.hpp"

/// Caminho do socket Unix para comunicação IPC com o Process Manager
#define SOCKET_PATH "/tmp/gameman.sock"

/**
 * @brief Retorna a instância única do NetworkClient (Singleton).
 * 
 * Garante que apenas uma conexão com o Process Manager exista em toda a aplicação.
 * 
 * @return NetworkClient& Referência para a instância singleton.
 */
NetworkClient& NetworkClient::getInstance() {
    static NetworkClient instance;
    return instance;
}

/**
 * @brief Construtor privado do NetworkClient.
 * 
 * Não realiza conexão automática; use connectToManager() explicitamente.
 */
NetworkClient::NetworkClient() {}

/**
 * @brief Destrutor do NetworkClient.
 * 
 * A limpeza da conexão é gerenciada automaticamente pelo destrutor de UnixSocketClient.
 */
NetworkClient::~NetworkClient() {
    // UnixSocketClient destructor handles cleanup
}

/**
 * @brief Estabelece conexão com o Process Manager via Unix Socket.
 * 
 * Verifica se já existe uma conexão ativa antes de tentar criar uma nova.
 * Configura o socket em modo não-bloqueante para evitar travamentos na UI.
 * 
 * @return true Se a conexão foi estabelecida ou já estava ativa.
 * @return false Se houve falha na tentativa de conexão.
 * 
 * @note Em caso de erro, o cliente é resetado e a mensagem é registrada no log.
 */
bool NetworkClient::connectToManager() {
    /// Retorna imediatamente se já existe conexão válida
    if (client_ && client_->isConnected()) return true;

    try {
        /// Cria nova instância do cliente Unix Socket apontando para SOCKET_PATH
        client_ = std::make_unique<UnixSocketClient>(SOCKET_PATH);
        
        /// Configura modo não-bloqueante para leitura assíncrona de eventos
        client_->setNonBlocking(true);
        frameReader_.reset();
        
        LOG_INFO("Connected to Process Manager.");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to connect to Process Manager: " + std::string(e.what()));
        resetConnectionState();
        return false;
    }
}

/**
 * @brief Envia comando de inicialização de jogo para o Process Manager.
 * 
 * Serializa os dados em formato JSON e transmite via socket Unix.
 * Mantém o lançamento pendente até receber uma resposta correlacionada do daemon.
 * 
 * Formato da mensagem JSON:
 * @code{.json}
 * {
 *   "action": "start",
 *   "request_id": "ui-...",
 *   "game_id": "game_id",
 *   "path": "/path/to/executable"
 * }
 * @endcode
 * 
 * @param id Identificador único do jogo no sistema.
 * @param path Caminho completo para o executável do jogo.
 * 
 * @note Se não houver conexão ativa, tenta reconectar automaticamente.
 * @note Este método e checkEvents() são chamados pela mesma thread da UI.
 */
bool NetworkClient::sendStartGame(const std::string& id, const std::string& path) {
    if (id.empty()) {
        lastLaunchError_ = "Identificador do jogo vazio.";
        LOG_ERROR("Cannot start game: empty id.");
        return false;
    }

    std::string absolutePath = MeuProjeto::caminho_absoluto_projeto(path);
    if (absolutePath.empty() || access(absolutePath.c_str(), F_OK) != 0) {
        lastLaunchError_ = "Executavel do jogo nao encontrado.";
        LOG_ERROR("Cannot start game: executable path not found: " + absolutePath);
        return false;
    }

    /// Tenta reconectar se não houver conexão ativa
    if (!connectToManager()) {
        lastLaunchError_ = "Process Manager indisponivel.";
        LOG_WARNING("Cannot start game: Not connected.");
        recomputeDerivedState();
        return false;
    }

    /// Constrói mensagem JSON com dados do jogo
    const std::string request_id = createRequestId();
    json j;
    j["action"] = "start";
    j["game_id"] = id;
    j["request_id"] = request_id;
    j["path"] = absolutePath;

    /// Serializa para string e envia via socket
    const ipc::SendResult send_result = client_->send(
        j.dump(),
        std::chrono::milliseconds(250)
    );
    if (send_result.status != ipc::SendStatus::Ok) {
        lastLaunchError_ = "Falha imediata ao enviar comando ao Process Manager.";
        LOG_ERROR(
            "Failed to send start command (status=" +
            std::to_string(static_cast<int>(send_result.status)) +
            ", errno=" + std::to_string(send_result.err) + ")."
        );
        recomputeDerivedState();
        if (send_result.status == ipc::SendStatus::Disconnected ||
            send_result.status == ipc::SendStatus::Desynced) {
            resetConnectionState();
        }
        return false;
    }
    
    LOG_INFO("Start command sent for: " + id);

    pendingRequests_[request_id] = id;
    recomputeDerivedState();
    lastLaunchError_.clear();
    return true;
}

/**
 * @brief Verifica e processa eventos recebidos do Process Manager.
 * 
 * Realiza leitura não-bloqueante do socket, decodifica mensagens JSON e
 * atualiza o estado interno baseado nos eventos recebidos.
 * 
 * Eventos processados:
 * - **game_finished**: Jogo foi encerrado normalmente
 * - **game_started**: Confirmação de que o jogo iniciou com sucesso
 * - **game_start_failed**: Falha confirmada antes da execução do jogo
 * 
 * @note Esta função deve ser chamada periodicamente no loop principal da aplicação.
 * @note Erros EAGAIN/EWOULDBLOCK são ignorados (normal em sockets não-bloqueantes).
 */
void NetworkClient::checkEvents() {
    if (!client_) return;

    char buffer[4096];

    while (client_) {
        ssize_t bytes_read = client_->receive(buffer, sizeof(buffer));

        if (bytes_read > 0) {
            frameReader_.feed(buffer, static_cast<std::size_t>(bytes_read));
            while (auto message = frameReader_.next()) {
                if (!message->empty()) {
                    processMessage(*message);
                }
            }

            if (frameReader_.overflowed()) {
                LOG_ERROR(
                    "IPC message exceeded " + std::to_string(ipc::kMaxFrameBytes) + " bytes."
                );
                resetConnectionState();
                return;
            }

            continue;
        }

        if (bytes_read == 0) {
            LOG_INFO("Server disconnected.");
            resetConnectionState();
            return;
        }

        if (errno == EINTR) {
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }

        LOG_ERROR("Recv error: " + std::string(strerror(errno)));
        resetConnectionState();
        return;
    }
}

void NetworkClient::processMessage(const std::string& message) {
    LOG_DEBUG("Received: " + message);

    try {
        auto j = json::parse(message);
        if (!j.is_object() || !j.contains("event") || !j["event"].is_string()) {
            return;
        }

        const std::string evt = j["event"].get<std::string>();
        if (!j.contains("request_id") ||
            !j["request_id"].is_string() ||
            j["request_id"].get<std::string>().empty()) {
            LOG_WARNING("Ignoring IPC event without request_id: " + evt);
            return;
        }

        const std::string request_id = j["request_id"].get<std::string>();
        const std::string event_game_id =
            j.contains("game_id") && j["game_id"].is_string()
                ? j["game_id"].get<std::string>()
                : std::string{};

        if (event_game_id.empty()) {
            LOG_WARNING("Ignoring IPC event without game_id for request_id: " + request_id);
            return;
        }

        if (evt == "game_finished") {
            auto active = activeRequests_.find(request_id);
            if (active == activeRequests_.end()) {
                LOG_WARNING("Ignoring finish event for unknown request_id: " + request_id);
                return;
            }
            if (active->second != event_game_id) {
                LOG_ERROR("Protocol mismatch in game_finished for request_id: " + request_id);
                return;
            }
            activeRequests_.erase(active);
            recomputeDerivedState();
            LOG_INFO(">>> JOGO TERMINOU! <<<");
        } else if (evt == "game_started") {
            auto pending = pendingRequests_.find(request_id);
            if (pending == pendingRequests_.end()) {
                LOG_WARNING("Ignoring start event for unknown request_id: " + request_id);
                return;
            }
            if (pending->second != event_game_id) {
                LOG_ERROR("Protocol mismatch in game_started for request_id: " + request_id);
                return;
            }
            activeRequests_.emplace(request_id, pending->second);
            pendingRequests_.erase(pending);
            lastLaunchError_.clear();
            recomputeDerivedState();
        } else if (evt == "game_start_failed") {
            auto pending = pendingRequests_.find(request_id);
            if (pending == pendingRequests_.end()) {
                LOG_WARNING("Ignoring failure event for unknown request_id: " + request_id);
                return;
            }
            if (pending->second != event_game_id) {
                LOG_ERROR("Protocol mismatch in game_start_failed for request_id: " + request_id);
                return;
            }
            pendingRequests_.erase(pending);
            lastLaunchError_ = j.value("message", "Falha ao iniciar o jogo.");
            recomputeDerivedState();
            LOG_ERROR("Game start failed: " + lastLaunchError_);
        } else {
            LOG_WARNING("Ignoring unknown IPC event: " + evt);
        }
    } catch (const json::exception& e) {
        LOG_ERROR("Invalid IPC JSON message: " + std::string(e.what()));
    }
}

std::string NetworkClient::createRequestId() {
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    ++requestSequence_;
    return "ui-" + std::to_string(getpid()) + "-" + std::to_string(timestamp) + "-" +
        std::to_string(requestSequence_);
}

void NetworkClient::recomputeDerivedState() {
    launchPending_ = !pendingRequests_.empty();
    isRunning_ = !activeRequests_.empty();
    currentRequestId_.clear();
    runningGameId_.clear();

    if (activeRequests_.size() == 1) {
        const auto& active = *activeRequests_.begin();
        currentRequestId_ = active.first;
        runningGameId_ = active.second;
        return;
    }
    if (activeRequests_.size() > 1) {
        LOG_ERROR("Protocol inconsistency: more than one game is active.");
        return;
    }

    if (pendingRequests_.size() == 1) {
        const auto& pending = *pendingRequests_.begin();
        currentRequestId_ = pending.first;
        runningGameId_ = pending.second;
    } else if (pendingRequests_.size() > 1) {
        LOG_ERROR("Protocol inconsistency: more than one launch is pending.");
    }
}

void NetworkClient::resetConnectionState() {
    pendingRequests_.clear();
    activeRequests_.clear();
    recomputeDerivedState();
    frameReader_.reset();
    client_.reset();
}
