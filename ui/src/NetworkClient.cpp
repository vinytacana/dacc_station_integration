/**
 * @file NetworkClient.cpp
 * @brief Implementação do cliente de comunicação com o Process Manager via Unix Socket.
 * 
 * Gerencia a conexão com o daemon de gerenciamento de processos, enviando comandos
 * de inicialização de jogos e recebendo eventos sobre o estado dos processos.
 */

#include "NetworkClient.hpp"
#include "Utils.hpp"
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include "LogManager.hpp"

/// Caminho do socket Unix para comunicação IPC com o Process Manager
#define SOCKET_PATH "/tmp/gameman.sock"

namespace {

constexpr std::size_t MAX_IPC_MESSAGE_SIZE = 64 * 1024;

} // namespace

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
        receiveBuffer_.clear();
        
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
 * Implementa "Optimistic Update": assume sucesso imediato para manter UI responsiva.
 * 
 * Formato da mensagem JSON:
 * @code{.json}
 * {
 *   "action": "start",
 *   "id": "game_id",
 *   "path": "/path/to/executable"
 * }
 * @endcode
 * 
 * @param id Identificador único do jogo no sistema.
 * @param path Caminho completo para o executável do jogo.
 * 
 * @note Se não houver conexão ativa, tenta reconectar automaticamente.
 * @note O envio usa mutex internamente (UnixSocketClient) para thread-safety.
 */
bool NetworkClient::sendStartGame(const std::string& id, const std::string& path) {
    if (id.empty()) {
        LOG_ERROR("Cannot start game: empty id.");
        return false;
    }

    std::string absolutePath = MeuProjeto::caminho_absoluto_projeto(path);
    if (absolutePath.empty() || access(absolutePath.c_str(), F_OK) != 0) {
        LOG_ERROR("Cannot start game: executable path not found: " + absolutePath);
        return false;
    }

    /// Tenta reconectar se não houver conexão ativa
    if (!connectToManager()) {
        LOG_WARNING("Cannot start game: Not connected.");
        launchPending_ = false;
        return false;
    }

    /// Constrói mensagem JSON com dados do jogo
    json j;
    j["action"] = "start";
    j["id"] = id;
    j["path"] = absolutePath;

    /// Serializa para string e envia via socket
    std::string msg = j.dump() + '\n';
    client_->send(msg); // UnixSocketClient::send uses mutex and handles writing
    
    LOG_INFO("Start command sent for: " + id);
    
    /**
     * Mantem um estado pendente ate o Process Manager confirmar ou encerrar o processo.
     * O daemon atual pode nao enviar game_started; nesse caso, game_finished ainda limpa o estado.
     */
    launchPending_ = true;
    runningGameId_ = id;
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
 * - **process_exited**: Processo do jogo terminou (com ou sem erro)
 * - **game_started**: Confirmação de que o jogo iniciou com sucesso
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
            receiveBuffer_.append(buffer, static_cast<std::size_t>(bytes_read));

            std::size_t delimiterPosition = std::string::npos;
            while ((delimiterPosition = receiveBuffer_.find('\n')) != std::string::npos) {
                std::string message = receiveBuffer_.substr(0, delimiterPosition);
                receiveBuffer_.erase(0, delimiterPosition + 1);

                if (!message.empty() && message.back() == '\r') {
                    message.pop_back();
                }
                if (!message.empty()) {
                    processMessage(message);
                }
            }

            if (receiveBuffer_.size() > MAX_IPC_MESSAGE_SIZE) {
                LOG_ERROR(
                    "IPC message exceeded " + std::to_string(MAX_IPC_MESSAGE_SIZE) + " bytes."
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
        if (!j.contains("event")) {
            return;
        }

        std::string evt = j["event"];

        if (evt == "game_finished" || evt == "process_exited") {
            LOG_INFO(">>> JOGO TERMINOU! <<<");
            isRunning_ = false;
            launchPending_ = false;
            runningGameId_.clear();
        } else if (evt == "game_started") {
            isRunning_ = true;
            launchPending_ = false;
            if (j.contains("id")) {
                runningGameId_ = j["id"];
            }
        } else if (evt == "game_start_failed") {
            LOG_ERROR("Game start failed event received.");
            isRunning_ = false;
            launchPending_ = false;
            runningGameId_.clear();
        }
    } catch (const json::exception& e) {
        LOG_ERROR("Invalid IPC JSON message: " + std::string(e.what()));
    }
}

void NetworkClient::resetConnectionState() {
    isRunning_ = false;
    launchPending_ = false;
    runningGameId_.clear();
    receiveBuffer_.clear();
    client_.reset();
}
