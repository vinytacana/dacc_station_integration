/**
 * @file JanelaBluetooth.hpp
 * @brief Definição da classe JanelaBluetooth para gerenciamento de dispositivos Bluetooth.
 * 
 * Este arquivo contém a interface para o submenu de configurações Bluetooth,
 * incluindo toggle ON/OFF, listagem de dispositivos pareados e disponíveis,
 * status de conexão e funcionalidade de escaneamento.
 * 
 */

#ifndef JANELA_BLUETOOTH_HPP
#define JANELA_BLUETOOTH_HPP

#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include "Botao.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @enum TipoDispositivoBT
 * @brief Tipos de dispositivos Bluetooth para ícones apropriados.
 */
enum class TipoDispositivoBT {
    CONTROLE,       /**< Controles de jogo. */
    HEADPHONE,      /**< Fones de ouvido e caixas de som. */
    SMARTPHONE,     /**< Smartphones e tablets. */
    TV,             /**< TVs e monitores. */
    COMPUTADOR,     /**< Computadores e laptops. */
    OUTRO           /**< Dispositivo genérico. */
};

/**
 * @struct DispositivoBluetooth
 * @brief Estrutura para armazenar informações sobre um dispositivo Bluetooth.
 */
struct DispositivoBluetooth {
    std::string nome;           /**< Nome do dispositivo. */
    TipoDispositivoBT tipo;     /**< Tipo do dispositivo (para ícone). */
    bool pareado;               /**< Se o dispositivo está pareado. */
    bool conectado;             /**< Se o dispositivo está conectado. */
    std::string endereco;       /**< Endereço MAC do dispositivo. */
    
    /**
     * @brief Construtor do DispositivoBluetooth.
     * @param n Nome do dispositivo.
     * @param t Tipo do dispositivo.
     * @param p Se está pareado.
     * @param c Se está conectado.
     * @param e Endereço MAC.
     */
    DispositivoBluetooth(const std::string& n, TipoDispositivoBT t, 
                         bool p = false, bool c = false, 
                         const std::string& e = "")
        : nome(n), tipo(t), pareado(p), conectado(c), endereco(e) {}
};

/**
 * @class JanelaBluetooth
 * @brief Gerencia a interface de configurações Bluetooth.
 * 
 * Esta classe renderiza e controla a interação com o menu de configurações
 * Bluetooth, incluindo:
 * - Toggle de Bluetooth ON/OFF
 * - Lista de dispositivos pareados com status
 * - Lista de dispositivos disponíveis (escaneados)
 * - Função de escanear (acionada por botão Y)
 * - Conexão/desconexão de dispositivos
 * - Esquecimento de dispositivos (botão X)
 * - Navegação via controle ou mouse
 */
class JanelaBluetooth {
public:
    /**
     * @brief Construtor da classe JanelaBluetooth.
     */
    JanelaBluetooth();

    /**
     * @brief Destrutor da classe JanelaBluetooth.
     */
    ~JanelaBluetooth();

    /**
     * @brief Renderiza a interface de configurações Bluetooth.
     * @param renderer Ponteiro para o renderizador SDL onde desenhar.
     */
    void desenhar(SDL_Renderer* renderer);

    /**
     * @brief Processa eventos de input específicos para esta tela.
     * @param evento Referência ao evento SDL capturado.
     * @return true se o evento foi processado e causou mudança de estado.
     */
    bool processarEvento(SDL_Event& evento);

    /**
     * @brief Reseta o estado da janela para valores padrão.
     */
    void resetar();

    /**
     * @brief Carrega as texturas necessárias (imagem explicativa).
     * @param renderer Renderizador SDL.
     */
    void carregarTexturas(SDL_Renderer* renderer);

private:
    // ESTADO DO BLUETOOTH
    
    bool bluetoothAtivo = true;             /**< Estado do Bluetooth (ON/OFF). */
    std::atomic<bool> escaneando{false};    /**< Se está em processo de escaneamento. */
    std::mutex mutexBT;
    std::thread threadBT;
    
    // LISTAS DE DISPOSITIVOS
    
    std::vector<DispositivoBluetooth> dispositivosPareados;   /**< Dispositivos pareados. */
    std::vector<DispositivoBluetooth> dispositivosDisponiveis; /**< Dispositivos disponíveis. */
    
    // BOTÕES INTERATIVOS
    
    std::unique_ptr<Botao> btnToggleBluetooth;  /**< Botão toggle ON/OFF. */
    
    // NOTA: btnEscanear removido - agora é função acionada por Y
    // Botões dos dispositivos serão criados automaticamente quando
    // a integração real com Bluetooth for implementada
    std::vector<std::unique_ptr<Botao>> botoesPareados;      /**< Botões dos dispositivos pareados. */
    std::vector<std::unique_ptr<Botao>> botoesDisponiveis;   /**< Botões dos dispositivos disponíveis. */
    
    // TEXTURAS
    
    SDL_Texture* texturaExplicacao = nullptr; /**< Textura da imagem explicativa. */
    
    // CONTROLE DE NAVEGAÇÃO
    
    int indiceFocado = -1;                  /**< Índice do elemento focado (-1 = toggle). */
    int scrollOffsetPareados = 0;           /**< Offset de scroll para dispositivos pareados. */
    int scrollOffsetDisponiveis = 0;        /**< Offset de scroll para dispositivos disponíveis. */
    int maxDispositivosVisiveis = 3;        /**< Máximo de dispositivos visíveis por lista. */
    
    // Controle de Input
    Uint32 ultimoInputAnalogico = 0;        /**< Timestamp do último input analógico. */
    const Uint32 INTERVALO_ANALOGICO = 200; /**< Intervalo mínimo entre inputs (ms). */
    const int DEADZONE = 16000;             /**< Limiar de sensibilidade do analógico. */
    
    // ANIMAÇÃO DE ESCANEAMENTO
    
    Uint32 tempoInicioEscanear = 0;         /**< Timestamp do início do escaneamento. */
    const Uint32 DURACAO_ESCANEAMENTO = 2000; /**< Duração da animação (2 segundos). */
    
    /**
     * @brief Inicializa os botões e elementos interativos.
     */
    void inicializarBotoes();

    /**
     * @brief Inicializa a lista de dispositivos pareados (dados de exemplo).
     */
    void inicializarDispositivosPareados();

    /**
     * @brief Inicializa a lista de dispositivos disponíveis (dados de exemplo).
     */
    void inicializarDispositivosDisponiveis();

    /**
     * @brief Renderiza o toggle do Bluetooth.
     * @param renderer Renderizador SDL.
     */
    void desenharToggleBluetooth(SDL_Renderer* renderer);

    /**
     * @brief Renderiza a lista de dispositivos pareados.
     * @param renderer Renderizador SDL.
     */
    void desenharDispositivosPareados(SDL_Renderer* renderer);

    /**
     * @brief Renderiza a lista de dispositivos disponíveis.
     * @param renderer Renderizador SDL.
     */
    void desenharDispositivosDisponiveis(SDL_Renderer* renderer);

    /**
     * @brief Renderiza um único dispositivo da lista.
     * @param renderer Renderizador SDL.
     * @param dispositivo Dados do dispositivo.
     * @param x Posição X.
     * @param y Posição Y.
     * @param focado Se o dispositivo está focado.
     */
    void desenharDispositivo(SDL_Renderer* renderer, 
                             const DispositivoBluetooth& dispositivo,
                             int x, int y, bool focado);

    /**
     * @brief Desenha a mensagem de escaneamento com animação.
     * @param renderer Renderizador SDL.
     */
    void desenharMensagemEscaneamento(SDL_Renderer* renderer);

    /**
     * @brief Desenha a imagem explicativa no rodapé.
     * @param renderer Renderizador SDL.
     */
    void desenharImagemExplicativa(SDL_Renderer* renderer);

    /**
     * @brief Alterna o estado do Bluetooth (ON/OFF).
     */
    void toggleBluetooth();

    /**
     * @brief Inicia o escaneamento de dispositivos disponíveis.
     * NOTA: Função acionada por botão Y do controle.
     */
    void iniciarEscaneamento();

    /**
     * @brief Conecta ou desconecta um dispositivo pareado.
     * @param indice Índice do dispositivo na lista de pareados.
     */
    void toggleConexaoDispositivo(int indice);

    /**
     * @brief Pareia um dispositivo disponível.
     * @param indice Índice do dispositivo na lista de disponíveis.
     */
    void parearDispositivo(int indice);

    /**
     * @brief Esquece um dispositivo pareado e move para disponíveis.
     * NOTA: Função acionada por botão X do controle.
     * @param indice Índice do dispositivo na lista de pareados.
     */
    void esquecerDispositivo(int indice);

    /**
     * @brief Move o foco para o elemento anterior.
     */
    void navegarParaCima();

    /**
     * @brief Move o foco para o próximo elemento.
     */
    void navegarParaBaixo();

    /**
     * @brief Confirma a seleção do elemento focado.
     */
    void confirmarSelecao();

    /**
     * @brief Atualiza o scroll das listas baseado no foco.
     */
    void atualizarScroll();

    /**
     * @brief Calcula o índice total de elementos focáveis.
     * @return Número total de elementos que podem receber foco.
     */
    int calcularTotalElementosFocaveis();
};

} // namespace MeuProjeto

#endif // JANELA_BLUETOOTH_HPP