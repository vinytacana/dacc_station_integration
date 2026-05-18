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
        
        LOG_INFO("Connected to Process Manager.");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to connect to Process Manager: " + std::string(e.what()));
        client_.reset();
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
    std::string msg = j.dump();
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

    /// Buffer para recepção de dados (máximo 1024 bytes por iteração)
    char buffer[1024];
    ssize_t bytes_read = client_->receive(buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {
        /// Adiciona terminador nulo para segurança ao processar string
        buffer[bytes_read] = '\0';
        LOG_DEBUG("Received: " + std::string(buffer));
        
        try {
            /// Tenta parsear resposta como JSON
            auto j = json::parse(buffer);
            if (j.contains("event")) {
                std::string evt = j["event"];
                
                /// Evento de término de jogo
                if (evt == "game_finished" || evt == "process_exited") {
                    LOG_INFO(">>> JOGO TERMINOU! <<<");
                    isRunning_ = false;
                    launchPending_ = false;
                    runningGameId_ = "";
                }
                /// Evento de confirmação de inicialização
                else if (evt == "game_started") {
                    isRunning_ = true;
                    launchPending_ = false;
                    if(j.contains("id")) runningGameId_ = j["id"];
                }
                else if (evt == "game_start_failed") {
                    LOG_ERROR("Game start failed event received.");
                    isRunning_ = false;
                    launchPending_ = false;
                    runningGameId_ = "";
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("JSON parse error: " + std::string(e.what()));
        }

    } else if (bytes_read == 0) {
        /// Socket fechado pelo servidor
        LOG_INFO("Server disconnected.");
        isRunning_ = false;
        launchPending_ = false;
        runningGameId_.clear();
        client_.reset();
    } else {
        /**
         * Erros de leitura são diferenciados:
         * - EAGAIN/EWOULDBLOCK: Normal em sockets não-bloqueantes (sem dados disponíveis)
         * - Outros erros: Indicam problema real na conexão
         */
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("Recv error: " + std::string(strerror(errno)));
            isRunning_ = false;
            launchPending_ = false;
            runningGameId_.clear();
            client_.reset();
        }
    }
}
