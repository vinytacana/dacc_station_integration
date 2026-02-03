#ifndef SYSTEM_STATUS_HPP
#define SYSTEM_STATUS_HPP

#include <string>

namespace MeuProjeto {

/**
 * @struct SystemData
 * @brief Estrutura contendo dados do status atual do sistema
 * 
 * Esta estrutura encapsula informações em tempo real sobre o estado do sistema,
 * incluindo hora atual, status de conectividade e nível de bateria. É utilizada
 * para exibir informações do sistema na interface do usuário, tipicamente em
 * uma barra de status.
 */
struct SystemData {
    /**
     * @brief Hora atual do sistema no formato "HH:MM"
     * 
     * String formatada com a hora no formato de 24 horas com dois dígitos
     * para hora e minuto, separados por dois pontos (exemplo: "14:30", "09:05").
     */
    std::string currentTime;
    
    /**
     * @brief Status da conexão WiFi
     * 
     * Indica se o dispositivo está conectado a uma rede WiFi.
     * true = conectado, false = desconectado.
     */
    bool wifiConnected;
    
    /**
     * @brief Nível de carga da bateria em porcentagem
     * 
     * Valor inteiro representando a porcentagem de carga da bateria,
     * variando de 0 (completamente descarregada) a 100 (completamente carregada).
     */
    int batteryLevel;
};

/**
 * @class SystemStatus
 * @brief Classe singleton para monitoramento e cache do status do sistema
 * 
 * Esta classe implementa o padrão Singleton para fornecer acesso global ao status
 * do sistema operacional/dispositivo. Ela mantém um cache dos dados do sistema
 * (hora, WiFi, bateria) e atualiza essas informações periodicamente, evitando
 * consultas excessivas ao sistema operacional que poderiam impactar a performance.
 * 
 * O cache é atualizado apenas quando necessário através do método update(), e os
 * dados em cache podem ser consultados rapidamente através de getCachedData().
 */
class SystemStatus {
public:
    /**
     * @brief Obtém a instância única do SystemStatus (padrão Singleton)
     * 
     * Retorna uma referência para a única instância do SystemStatus existente
     * na aplicação. A instância é criada estaticamente na primeira chamada
     * (Meyer's Singleton), garantindo thread-safety em C++11 e posteriores.
     * 
     * @return Referência para a instância singleton do SystemStatus
     */
    static SystemStatus& getInstance() {
        static SystemStatus instance;
        return instance;
    }

    /**
     * @brief Atualiza os dados do sistema em cache
     * 
     * Consulta o sistema operacional para obter informações atualizadas sobre
     * hora, conectividade WiFi e nível de bateria, atualizando o cache interno.
     * Este método deve ser chamado periodicamente (por exemplo, a cada segundo
     * ou poucos segundos) para manter os dados sincronizados, mas não precisa
     * ser chamado a cada frame para evitar overhead desnecessário.
     * 
     * A implementação pode incluir lógica de throttling para evitar atualizações
     * muito frequentes baseando-se no timestamp da última atualização.
     */
    void update();
    
    /**
     * @brief Obtém os dados do sistema armazenados em cache
     * 
     * Retorna uma cópia da estrutura SystemData com os valores mais recentes
     * obtidos pela última chamada de update(). Este método é extremamente rápido
     * pois apenas retorna dados já armazenados em memória, sem fazer novas
     * consultas ao sistema operacional.
     * 
     * @return Estrutura SystemData contendo hora atual, status WiFi e nível de bateria
     */
    SystemData getCachedData() const;

private:
    /**
     * @brief Construtor privado (padrão Singleton)
     * 
     * Inicializa o SystemStatus com valores padrão. Privado para garantir
     * que apenas uma instância seja criada através do método getInstance().
     * Pode realizar configurações iniciais e primeira consulta dos dados do sistema.
     */
    SystemStatus();
    
    /**
     * @brief Cache dos dados do sistema
     * 
     * Armazena a última versão conhecida dos dados do sistema (hora, WiFi, bateria).
     * Este cache é atualizado pelo método update() e consultado por getCachedData().
     */
    SystemData cache;
    
    /**
     * @brief Timestamp da última atualização
     * 
     * Armazena o tempo (tipicamente em milissegundos desde epoch ou desde
     * início da aplicação) da última vez que o cache foi atualizado. Utilizado
     * para implementar throttling e evitar atualizações excessivamente frequentes.
     */
    unsigned int lastUpdate;
};

} // namespace MeuProjeto

#endif // SYSTEM_STATUS_HPP