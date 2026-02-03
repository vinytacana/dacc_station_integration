/**
 * @file GerenciarInputs.hpp
 * @brief Definição da classe GerenciarInputs responsável pelo processamento de entradas de periféricos.
 * 
 * Este arquivo contém a lógica que unifica e gerencia as entradas de teclado, mouse 
 * e controles (gamepads) via SDL_GameController, permitindo a navegação na interface
 * e a interação com os elementos gráficos.
 */

#ifndef GERENCIAR_INPUTS_HPP
#define GERENCIAR_INPUTS_HPP

#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include "GerenciarInterface.hpp"
#include "GerenciarScroll.hpp"
#include "Botao.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class GerenciarInputs
 * @brief Gerencia a captura e o tratamento de eventos de hardware.
 * 
 * A classe GerenciarInputs centraliza o processamento de eventos da SDL. Ela 
 * mantém o estado dos controles conectados, gerencia o foco visual para navegação
 * por botões (estilo console) e implementa filtros de tempo (debounce) para 
 * garantir uma navegação fluida e sem erros de input duplo.
 */
class GerenciarInputs {
public:
    /**
     * @brief Construtor da classe GerenciarInputs.
     * Inicializa os estados internos, ponteiros de controle e variáveis de tempo.
     */
    GerenciarInputs();

    /**
     * @brief Destrutor da classe GerenciarInputs.
     * Garante que o controle seja fechado corretamente ao encerrar o objeto.
     */
    ~GerenciarInputs();

    /**
     * @brief Detecta e inicializa um controle (Gamepad) conectado ao sistema.
     * 
     * Procura por joysticks compatíveis e abre o primeiro dispositivo encontrado
     * utilizando a API SDL_GameController para mapeamento automático de botões.
     */
    void inicializarControle();

    /**
     * @brief Fecha a conexão com o controle e libera os recursos da SDL.
     */
    void fecharControle();

    /**
     * @brief Ponto de entrada principal para o processamento de eventos.
     * 
     * Este método deve ser chamado dentro do loop principal do programa. Ele 
     * distribui o evento recebido para os métodos específicos (Mouse, Teclado ou Gamepad)
     * baseando-se no tipo do evento.
     * 
     * @param evento Referência ao evento SDL capturado no loop.
     * @param estado Referência ao estado de scroll e navegação global.
     * @param interface Referência ao gerenciador de elementos da interface gráfica.
     * @param renderer Ponteiro para o renderizador SDL (necessário para abrir janelas de config).
     */
    void atualizar(SDL_Event& evento, GerenciarScroll& estado, GerenciarInterface& interface, SDL_Renderer* renderer);

private:
    SDL_GameController* gameController = nullptr; /**< Ponteiro para a instância do controle SDL ativo. */
    
    int botaoFocadoIndex = -1; /**< Índice do botão que detém o foco atual na navegação por controle. */
    std::vector<Botao*> botoesNavegaveis; /**< Coleção de botões que podem receber foco via D-Pad ou Analógico. */
    
    // Constantes de Tempo (ms)
    const Uint32 INTERVALO_INPUT_CONTROLE = 200; /**< Cooldown para navegação direcional. */
    const Uint32 INTERVALO_DUPLO_CLIQUE = 300;  /**< Janela de tempo para detectar cliques duplos. */
    const Uint32 DEBOUNCE_GATILHOS = 200;       /**< Filtro para evitar disparos repetidos nos gatilhos (L2/R2). */
    
    /** 
     * @brief Zona morta para o analógico.
     * Define o limiar mínimo de inclinação (aprox. 50%) para considerar um movimento, 
     * evitando o fenômeno de "stick drift".
     */
    const int DEADZONE = 16000;
    
    // Timestamps para controle de debounce de botões específicos
    Uint32 ultimoTempoLB = 0; /**< Último clique no botão superior esquerdo (L1). */
    Uint32 ultimoTempoRB = 0; /**< Último clique no botão superior direito (R1). */
    Uint32 ultimoTempoLT = 0; /**< Último acionamento do gatilho esquerdo (L2). */
    Uint32 ultimoTempoRT = 0; /**< Último acionamento do gatilho direito (R2). */
    
    // Controle de Cliques e Estado de Confirmação
    bool cliquePendente = false;        /**< Indica se um clique foi iniciado mas não finalizado. */
    Uint32 tempoCliquePendente = 0;     /**< Timestamp de quando o clique foi iniciado. */
    Botao* botaoPendente = nullptr;     /**< Ponteiro para o botão que está aguardando confirmação. */
    bool ignorarProximoUpA = false;     /**< Flag técnica para sincronia de eventos de soltar botão do controle. */

    /**
     * @brief Processa entradas exclusivamente do teclado.
     * @param evento Evento SDL.
     * @param estado Estado global de scroll.
     */
    void processarTeclado(SDL_Event& evento, GerenciarScroll& estado);

    /**
     * @brief Processa interações via mouse (cliques, movimento e rolagem).
     * @param evento Evento SDL.
     * @param estado Estado global de scroll.
     * @param interface Gerenciador de interface.
     * @param renderer Renderizador.
     */
    void processarMouse(SDL_Event& evento, GerenciarScroll& estado, GerenciarInterface& interface, SDL_Renderer* renderer);

    /**
     * @brief Processa entradas de Gamepad (Botões, D-Pad e Analógicos).
     * @param evento Evento SDL.
     * @param estado Estado global de scroll.
     * @param interface Gerenciador de interface.
     * @param renderer Renderizador.
     */
    void processarGamepad(SDL_Event& evento, GerenciarScroll& estado, GerenciarInterface& interface, SDL_Renderer* renderer);
    
    /**
     * @brief Lógica de movimentação do foco entre botões usando o controle.
     * 
     * @param direcaoX Movimento horizontal (-1 esquerda, 1 direita).
     * @param direcaoY Movimento vertical (-1 cima, 1 baixo).
     * @param estado Estado global de scroll (para ajustar a visualização ao mover o foco).
     * @param interface Gerenciador de interface para buscar botões visíveis.
     */
    void navegarComControle(int direcaoX, int direcaoY, GerenciarScroll& estado, GerenciarInterface& interface);
    
    /**
     * @brief Executa a função associada a um botão.
     * 
     * @param botao Ponteiro para o botão acionado.
     * @param estado Estado global de scroll.
     * @param interface Gerenciador de interface.
     * @param renderer Renderizador.
     * @param executarDireto Se true, ignora timers e executa a ação imediatamente.
     */
    void executarAcaoBotao(Botao* botao, GerenciarScroll& estado, GerenciarInterface& interface, SDL_Renderer* renderer, bool executarDireto = false);

    /**
     * @brief Atalho para disparar a abertura da janela de configurações do sistema.
     * @param estado Referência ao estado global.
     * @param renderer Renderizador para a nova janela.
     */
    void abrirConfiguracoes(GerenciarScroll& estado);
};

} // namespace MeuProjeto

#endif // GERENCIAR_INPUTS_HPP