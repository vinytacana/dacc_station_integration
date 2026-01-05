/**
 * @file JanelaJogo.hpp
 * @brief Definição da classe JanelaJogo, que exibe os detalhes e opções de um título específico.
 * 
 * Este arquivo contém a interface para a visualização detalhada de um jogo,
 * apresentando sua descrição, galeria de imagens (capturas de tela) e
 * opções de interação como "Jogar" ou "Fechar".
 */

#ifndef JANELA_JOGO_HPP
#define JANELA_JOGO_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include "Botao.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class JanelaJogo
 * @brief Gerencia a interface de detalhes de um jogo selecionado.
 * 
 * Esta classe é responsável por carregar e exibir as informações completas de um jogo.
 * Ela suporta renderização de múltiplas texturas (capas e capturas), navegação 
 * por botões interativos e um sistema de carrossel de imagens com animações.
 */
class JanelaJogo {
public:
    /**
     * @brief Construtor da classe JanelaJogo.
     * 
     * Inicializa os elementos visuais, carrega as texturas das capturas de tela
     * e configura as áreas de layout da janela.
     * 
     * @param renderer O renderizador SDL utilizado para criar as texturas.
     * @param nomeJogo Título do jogo a ser exibido.
     * @param imagemPath Caminho para a imagem principal (capa interna).
     * @param descricao Texto descritivo/sinopse do jogo.
     * @param capturasPaths Vetor de strings com os caminhos para as screenshots do jogo.
     */
    JanelaJogo(SDL_Renderer* renderer, const std::string& nomeJogo, const std::string& imagemPath,
               const std::string& descricao, const std::vector<std::string>& capturasPaths);

    /**
     * @brief Destrutor da classe JanelaJogo.
     * 
     * Libera as texturas carregadas para a imagem principal e para todas as capturas de tela.
     */
    ~JanelaJogo();

    /**
     * @brief Renderiza todos os elementos da janela de detalhes na tela.
     * 
     * Chama internamente as funções de desenho do fundo, topo, descrição e carrossel.
     * 
     * @param offsetX Deslocamento horizontal opcional.
     * @param offsetY Deslocamento vertical opcional.
     */
    void desenhar(int offsetX = 0, int offsetY = 0);

    /**
     * @brief Processa eventos de entrada do mouse e teclado comuns.
     * 
     * @param evento Referência ao evento SDL capturado.
     * @param offsetX Deslocamento X da janela no momento do evento.
     * @param offsetY Deslocamento Y da janela no momento do evento.
     * @return true se o evento resultou em uma ação (ex: fechar a janela).
     * @return false caso contrário.
     */
    bool tratarEvento(SDL_Event& evento, int offsetX, int offsetY);
    
    /**
     * @brief Aciona a lógica de execução do jogo (launcher).
     */
    void executarJogo();

    /**
     * @brief Processa entradas específicas de controles (Gamepads/Joysticks).
     * 
     * Lida com a navegação entre os botões "Jogar" e "Fechar", além de trocar 
     * as capturas de tela utilizando os botões de ombro (Shoulders).
     * 
     * @param evento Referência ao evento SDL de controle.
     * @return true se o comando de fechar foi acionado via controle.
     */
    bool tratarEventoControle(SDL_Event& evento);

private:
    SDL_Renderer* renderer;                 /**< Ponteiro para o renderizador SDL. */
    SDL_Texture* imagemJogo;                /**< Textura da imagem de destaque do jogo. */
    std::vector<SDL_Texture*> capturasTexturas; /**< Vetor de texturas das capturas de tela. */
    std::string nomeJogo;                   /**< Título armazenado. */
    std::string descricao;                  /**< Texto descritivo armazenado. */
    
    Botao botaoJogar;                       /**< Objeto do botão para iniciar o jogo. */
    Botao botaoFechar;                      /**< Objeto do botão para sair da visualização. */
    
    // Áreas de Layout (Geometria)
    SDL_Rect areaJanela;                    /**< Retângulo total da janela de detalhes. */
    SDL_Rect areaTopo;                      /**< Área superior (geralmente para o título). */
    SDL_Rect areaFundo;                     /**< Área de fundo geral. */
    SDL_Rect areaImagem;                    /**< Retângulo onde a imagem principal é desenhada. */
    SDL_Rect areaDescricao;                 /**< Retângulo destinado ao bloco de texto. */
    SDL_Rect areaFundoCapturas;             /**< Área que abriga o carrossel de screenshots. */
    
    Botao setaEsquerda;                     /**< Botão visual para navegar para a captura anterior. */
    Botao setaDireita;                      /**< Botão visual para navegar para a próxima captura. */
    
    int capturaIndex;                       /**< Índice da captura de tela atualmente em foco no carrossel. */
    
    // Controle de foco e animação
    bool botaoJogarFocado;                  /**< Indica se o foco (para controle) está no botão de play. */
    Uint32 ultimoTempoLB;                   /**< Timestamp do último input no botão L1/LB (Debounce). */
    Uint32 ultimoTempoRB;                   /**< Timestamp do último input no botão R1/RB (Debounce). */
    
    bool animacaoSetaEsquerda;              /**< Flag para efeito visual de clique na seta esquerda. */
    bool animacaoSetaDireita;               /**< Flag para efeito visual de clique na seta direita. */
    Uint32 tempoAnimacaoEsquerda;           /**< Temporizador para duração do efeito da seta esquerda. */
    Uint32 tempoAnimacaoDireita;            /**< Temporizador para duração do efeito da seta direita. */
    
    // Constantes de Layout e Comportamento
    static constexpr int CAPTURA_LARGURA = 480;      /**< Largura padrão de cada screenshot. */
    static constexpr int CAPTURA_ALTURA = 270;       /**< Altura padrão de cada screenshot. */
    static constexpr int CAPTURA_POS_Y = 790;        /**< Posição vertical das screenshots na janela. */
    static constexpr int CAPTURA_ESPACAMENTO = 520;  /**< Distância horizontal entre screenshots. */
    static constexpr Uint32 DEBOUNCE_SHOULDER = 300; /**< Tempo de espera em ms entre trocas de imagem via controle. */
    static constexpr Uint32 DURACAO_ANIMACAO = 200;  /**< Duração em ms do efeito visual de clique nas setas. */

    // Métodos privados de auxílio à renderização
    void desenharFundo();
    void desenharTopo();
    void desenharAreaFundo();
    void desenharImagem(int offsetX, int offsetY);
    void desenharDescricao(int offsetX, int offsetY);
    void desenharCapturas(int offsetX, int offsetY);
    void desenharBotoes(int offsetX, int offsetY);

    /**
     * @brief Atualiza o estado das animações de feedback das setas baseado no tempo.
     */
    void atualizarAnimacoes();
};

} // namespace MeuProjeto

#endif // JANELA_JOGO_HPP