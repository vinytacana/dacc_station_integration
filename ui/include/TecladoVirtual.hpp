/**
 * @file TecladoVirtual.hpp
 * @brief Definição da classe TecladoVirtual.
 *
 * Este arquivo especifica a interface de um teclado virtual projetado para permitir
 * a entrada de texto em sistemas que utilizam SDL2, com suporte nativo para 
 * navegação via controle (gamepad) e interação direta via mouse.
 * 
 * MODIFICAÇÃO: Adicionado construtor customizado para permitir centralização
 * em telas de diferentes tamanhos.
 */

#ifndef TECLADO_VIRTUAL_HPP
#define TECLADO_VIRTUAL_HPP

#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include "BotaoPesquisa.hpp"

namespace MeuProjeto {

/**
 * @class TecladoVirtual
 * @brief Gerencia e renderiza uma interface de teclado na tela para entrada de dados.
 *
 * A classe TecladoVirtual provê uma solução de acessibilidade e entrada de dados
 * para interfaces de usuário, permitindo que caracteres sejam enviados para um
 * componente BotaoPesquisa. Inclui lógica de navegação por grade, tratamento de
 * deadzone para analogicos e suporte a layouts responsivos.
 */
class TecladoVirtual {
public:
    /**
     * @brief Construtor padrão da classe TecladoVirtual.
     * 
     * Inicializa o estado interno do teclado, definindo as configurações padrão
     * de visibilidade e posicionamento dos índices de navegação.
     * Posição padrão: X=460, Y=600 (para tela de 1920px)
     */
    TecladoVirtual();

    /**
     * @brief Construtor customizado com posição específica.
     * 
     * Permite criar um teclado virtual em uma posição customizada,
     * útil para centralização em telas de tamanhos diferentes.
     * 
     * @param customBaseX Posição X base personalizada.
     * @param customBaseY Posição Y base personalizada.
     */
    TecladoVirtual(int customBaseX, int customBaseY);

    /**
     * @brief Destrutor da classe TecladoVirtual.
     * 
     * Responsável pela liberação de recursos associados à instância do teclado virtual.
     */
    ~TecladoVirtual();

    /**
     * @brief Renderiza a interface do teclado no renderer especificado.
     * 
     * Este método processa o desenho do painel de fundo, das teclas de caracteres
     * e das teclas especiais, considerando o estado atual de foco e visibilidade.
     * 
     * @param renderer Ponteiro para o SDL_Renderer onde o teclado será desenhado.
     */
    void desenhar(SDL_Renderer* renderer);

    /**
     * @brief Processa entradas provenientes de controles (gamepads).
     * 
     * Gerencia a navegação entre teclas e a seleção de caracteres utilizando
     * eventos de botões ou eixos analógicos do SDL_Event.
     * 
     * @param evento Referência para o evento SDL capturado.
     * @param alvo Ponteiro para o objeto BotaoPesquisa que receberá o texto inserido.
     * @return true se o evento foi processado com sucesso, false caso contrário.
     */
    bool processarControle(SDL_Event& evento, BotaoPesquisa* alvo);

    /**
     * @brief Processa entradas provenientes do mouse.
     * 
     * Trata o posicionamento do cursor e o clique sobre as teclas virtuais.
     * 
     * @param evento Referência para o evento SDL capturado.
     * @param alvo Ponteiro para o objeto BotaoPesquisa que receberá o texto inserido.
     * @return true se o clique foi realizado sobre uma tecla, false caso contrário.
     */
    bool processarMouse(SDL_Event& evento, BotaoPesquisa* alvo);

    /**
     * @brief Torna o teclado visível e reseta os índices de navegação.
     */
    void abrir() { visivel = true; indiceX = 0; indiceY = 0; }

    /**
     * @brief Oculta a interface do teclado virtual.
     */
    void fechar() { visivel = false; }

    /**
     * @brief Verifica se o teclado está sendo exibido no momento.
     * 
     * @return true se visível, false caso contrário.
     */
    bool estaVisivel() const { return visivel; }

private:
    bool visivel = false; /**< Estado de exibição do teclado. */
    
    std::vector<std::string> linha1 = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"}; /**< Primeira linha de caracteres (numérica). */
    std::vector<std::string> linha2 = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"}; /**< Segunda linha de caracteres (alfabética). */
    std::vector<std::string> linha3 = {"A", "S", "D", "F", "G", "H", "J", "K", "L"};      /**< Terceira linha de caracteres (alfabética). */
    std::vector<std::string> linha4 = {"Z", "X", "C", "V", "B", "N", "M"};               /**< Quarta linha de caracteres (alfabética). */
    
    int indiceX = 0; /**< Índice horizontal atual do foco no teclado. */
    int indiceY = 0; /**< Índice vertical atual do foco no teclado. */
    
    int baseX; /**< Coordenada X base para o início da renderização do teclado. */
    int baseY; /**< Coordenada Y base para o início da renderização do teclado. */
    int teclaLargura; /**< Largura individual de cada tecla alfanumérica. */
    int teclaAltura;  /**< Altura individual de cada tecla alfanumérica. */
    int gapX; /**< Espaçamento horizontal entre as teclas. */
    int gapY; /**< Espaçamento vertical entre as linhas de teclas. */
    
    int painelPadding; /**< Margem interna do painel de fundo. */
    int painelLargura; /**< Largura total do painel de fundo do teclado. */
    int painelAltura;  /**< Altura total do painel de fundo do teclado. */

    int offsetEspacoX;     /**< Deslocamento horizontal inicial para o botão de espaço. */
    int larguraBotaoEsp;   /**< Largura padrão para os botões de funções especiais (Espaço, Limpar). */
    int larguraBotaoOk;    /**< Largura específica para o botão de confirmação (OK). */
    int gapBotoesEsp;      /**< Espaçamento entre os botões da linha especial. */
    int offsetTextoY;      /**< Deslocamento vertical para centralização de texto nos botões especiais. */

    int fonteTamanhoTecla; /**< Tamanho da fonte utilizada nas teclas alfanuméricas. */
    int fonteTamanhoEsp;   /**< Tamanho da fonte utilizada nos botões de funções especiais. */
    int offsetCharX;       /**< Ajuste fino horizontal para centralização de caracteres. */
    int offsetCharY;       /**< Ajuste fino vertical para centralização de caracteres. */
    
    Uint32 ultimoInput = 0; /**< Marca temporal do último input processado via botões. */
    const Uint32 INTERVALO_INPUT = 150; /**< Período mínimo de espera entre inputs de botões (ms). */
    Uint32 ultimoInputAnalogico = 0; /**< Marca temporal do último input processado via analógico. */
    const Uint32 INTERVALO_ANALOGICO = 200; /**< Período mínimo de espera entre inputs analógicos (ms). */
    const int DEADZONE = 16000; /**< Limite de sensibilidade para ativação dos eixos analógicos. */

    /**
     * @brief Inicializa variáveis comuns de layout (usado por ambos construtores).
     */
    void inicializarLayout();

    /**
     * @brief Desenha uma tecla individual no renderer.
     * 
     * @param renderer Ponteiro para o SDL_Renderer.
     * @param x Posição horizontal da tecla.
     * @param y Posição vertical da tecla.
     * @param letra Caractere ou string a ser exibido na tecla.
     * @param focado Indica se a tecla deve ser renderizada com destaque de foco.
     */
    void desenharTecla(SDL_Renderer* renderer, int x, int y, const std::string& letra, bool focado);

    /**
     * @brief Desenha a linha inferior de botões especiais (Espaço, Limpar, OK).
     * 
     * @param renderer Ponteiro para o SDL_Renderer.
     */
    void desenharEspeciais(SDL_Renderer* renderer);

    /**
     * @brief Atualiza a posição do cursor/foco no teclado virtual.
     * 
     * @param deltaX Alteração no índice horizontal.
     * @param deltaY Alteração no índice vertical.
     */
    void moverCursor(int deltaX, int deltaY);

    /**
     * @brief Verifica se uma coordenada de mouse está dentro dos limites de uma tecla.
     * 
     * @param mouseX Posição X do mouse.
     * @param mouseY Posição Y do mouse.
     * @param x Posição X da tecla.
     * @param y Posição Y da tecla.
     * @param w Largura da tecla.
     * @param h Altura da tecla.
     * @return true se o ponto estiver contido na área, false caso contrário.
     */
    bool checarCliqueTecla(int mouseX, int mouseY, int x, int y, int w, int h);
};

} // namespace MeuProjeto

#endif // TECLADO_VIRTUAL_HPP