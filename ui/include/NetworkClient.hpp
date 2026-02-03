#pragma once

#include <string>
#include <functional>
#include <iostream>
#include <memory>
#include "json.hpp"
#include "UnixSocketClient.hpp"

using json = nlohmann::json;

/**
 * @class NetworkClient
 * @brief Cliente de rede singleton para comunicação com o gerenciador de jogos
 * 
 * Esta classe implementa o padrão Singleton para gerenciar a comunicação de rede
 * entre a aplicação e um gerenciador externo de jogos através de Unix Domain Sockets.
 * É responsável por enviar comandos para iniciar jogos, monitorar o estado de execução
 * e processar eventos recebidos do gerenciador.
 * 
 * O NetworkClient mantém o controle do estado atual dos jogos (em execução ou parado)
 * e fornece uma interface simplificada para interação com o sistema de gerenciamento
 * de jogos externo.
 */
class NetworkClient {
public:
    /**
     * @brief Obtém a instância única do NetworkClient (padrão Singleton)
     * 
     * Retorna uma referência para a única instância do NetworkClient existente
     * na aplicação. Se a instância ainda não foi criada, ela será criada na
     * primeira chamada deste método.
     * 
     * @return Referência para a instância singleton do NetworkClient
     */
    static NetworkClient& getInstance();

    /**
     * @brief Estabelece conexão com o gerenciador de jogos
     * 
     * Tenta conectar ao servidor gerenciador através de Unix Domain Socket.
     * Este método deve ser chamado antes de qualquer tentativa de enviar comandos
     * ou verificar eventos. A conexão é mantida durante toda a vida útil da aplicação.
     * 
     * @return true se a conexão foi estabelecida com sucesso, false caso contrário
     */
    bool connectToManager();
    
    /**
     * @brief Envia comando para iniciar um jogo específico
     * 
     * Transmite uma requisição ao gerenciador para iniciar a execução de um jogo.
     * O comando inclui tanto o identificador único do jogo quanto o caminho completo
     * para seu executável. O gerenciador será responsável por lançar o processo do jogo
     * e notificar quando o jogo for encerrado.
     * 
     * @param id Identificador único do jogo a ser iniciado
     * @param path Caminho completo do arquivo executável do jogo
     */
    void sendStartGame(const std::string& id, const std::string& path);
                      
    /**
     * @brief Verifica e processa eventos recebidos do gerenciador
     * 
     * Este método deve ser chamado regularmente no loop principal da aplicação
     * (tipicamente a cada frame). Ele verifica se há mensagens ou eventos pendentes
     * do gerenciador de jogos (como notificações de jogo iniciado ou encerrado) e
     * processa essas mensagens, atualizando o estado interno do cliente conforme necessário.
     * 
     * É essencial para manter a sincronização entre o estado da aplicação e o
     * estado real dos jogos gerenciados externamente.
     */
    void checkEvents();
    
    // State Getters
    
    /**
     * @brief Verifica se há algum jogo em execução no momento
     * 
     * Consulta o estado interno para determinar se existe um jogo atualmente
     * sendo executado através do gerenciador.
     * 
     * @return true se há um jogo em execução, false caso contrário
     */
    bool isGameRunning() const { return isRunning_; }
    
    /**
     * @brief Obtém o identificador do jogo atualmente em execução
     * 
     * Retorna o ID do jogo que está sendo executado no momento. Se nenhum jogo
     * estiver em execução, retorna uma string vazia.
     * 
     * @return String contendo o ID do jogo em execução, ou string vazia se nenhum jogo está rodando
     */
    std::string getRunningGameId() const { return runningGameId_; }

private:
    /**
     * @brief Construtor privado (padrão Singleton)
     * 
     * Inicializa o NetworkClient com estado padrão. Privado para garantir
     * que apenas uma instância seja criada através do método getInstance().
     */
    NetworkClient();
    
    /**
     * @brief Destrutor privado
     * 
     * Libera recursos e fecha conexões quando o NetworkClient é destruído.
     * Privado como parte da implementação do padrão Singleton.
     */
    ~NetworkClient();
    
    /**
     * @brief Ponteiro único para o cliente de Unix Socket
     * 
     * Gerencia a comunicação de baixo nível através de Unix Domain Sockets.
     * Utiliza smart pointer (unique_ptr) para gerenciamento automático de memória.
     */
    std::unique_ptr<UnixSocketClient> client_;
    
    // Game State
    
    /**
     * @brief Flag indicando se há um jogo atualmente em execução
     * 
     * Mantém o estado de execução: true quando um jogo está rodando,
     * false quando nenhum jogo está ativo. Inicializado como false.
     */
    bool isRunning_ = false;
    
    /**
     * @brief Identificador do jogo atualmente em execução
     * 
     * Armazena o ID do jogo que está sendo executado no momento.
     * String vazia quando nenhum jogo está rodando.
     */
    std::string runningGameId_;

    // Prevent copying
    
    /**
     * @brief Construtor de cópia deletado (padrão Singleton)
     * 
     * Previne a criação de cópias da instância singleton, garantindo
     * que apenas uma única instância exista na aplicação.
     */
    NetworkClient(const NetworkClient&) = delete;
    
    /**
     * @brief Operador de atribuição deletado (padrão Singleton)
     * 
     * Previne a atribuição entre instâncias, reforçando a unicidade
     * da instância singleton.
     */
    NetworkClient& operator=(const NetworkClient&) = delete;
};