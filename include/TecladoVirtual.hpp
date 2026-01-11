/**
 * @file TecladoVirtual.hpp
 * @brief Definição da classe TecladoVirtual para entrada de texto via interface gráfica.
 * 
 * Este arquivo contém a lógica de um teclado alfanumérico renderizado em tela,
 * projetado para ser operado via controle (joystick) ou mouse, facilitando a
 * busca de jogos na biblioteca.
 */

#ifndef TECLADO_VIRTUAL_HPP
#define TECLADO_VIRTUAL_HPP

#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include "BotaoPesquisa.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class TecladoVirtual
 * @brief Gerencia e renderiza um teclado virtual na interface.
 * 
 * A classe controla uma grade de teclas alfanuméricas e funções especiais (espaço, apagar, etc.).
 * Ela suporta navegação por índices (para controles) e detecção de coordenadas (para mouse),
 * enviando os caracteres selecionados diretamente para um objeto BotaoPesquisa.
 */
class TecladoVirtual {
public:
    /**
     * @brief Construtor que inicializa o estado do teclado e as configurações de layout.
     */
    TecladoVirtual();

    /**
     * @brief Destrutor da classe TecladoVirtual.
     */
    ~TecladoVirtual();

    /**
     * @brief Renderiza o teclado completo na tela.
     * 
     * Desenha as linhas de caracteres, as teclas especiais e o cursor de seleção
     * (highlight) sobre a tecla atualmente focada.
     * 
     * @param renderer O renderizador SDL onde o teclado será desenhado.
     */
    void desenhar(SDL_Renderer* renderer);
    
    /**
     * @brief Processa entradas vindas de um controle (Gamepad/Joystick).
     * 
     * Lida com o movimento do D-Pad ou Analógico para mover o cursor entre as teclas
     * e processa o botão de confirmação para inserir o caractere no alvo.
     * 
     * @param evento O evento SDL capturado (SDL_JOYBUTTONDOWN, SDL_JOYAXISMOTION, etc).
     * @param alvo Ponteiro para o BotaoPesquisa que receberá o texto digitado.
     * @return true se o evento foi processado pelo teclado, false caso contrário.
     */
    bool processarControle(SDL_Event& evento, BotaoPesquisa* alvo);
    
    /**
     * @brief Processa entradas vindas do mouse.
     * 
     * Verifica se o clique ocorreu sobre uma tecla específica ou fora da área do teclado.
     * 
     * @param evento O evento SDL de mouse (SDL_MOUSEBUTTONDOWN).
     * @param alvo Ponteiro para o BotaoPesquisa que receberá o texto digitado.
     * @return true Se o evento foi consumido pelo teclado (impede que o clique atinja elementos atrás do teclado).
     * @return false Se o clique foi fora da área de interação do teclado.
     */
    bool processarMouse(SDL_Event& evento, BotaoPesquisa* alvo);

    /**
     * @brief Torna o teclado visível e reinicia o cursor para a posição inicial.
     */
    void abrir() { visivel = true; indiceX = 0; indiceY = 0; }

    /**
     * @brief Oculta o teclado da interface.
     */
    void fechar() { visivel = false; }

    /**
     * @brief Verifica se o teclado está ativo/visível na tela.
     * @return true se visível, false caso contrário.
     */
    bool estaVisivel() const { return visivel; }

private:
    bool visivel = false; /**< Estado de exibição do teclado. */
    
    // Layout do teclado dividido por linhas
    std::vector<std::string> linha1 = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    std::vector<std::string> linha2 = {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"};
    std::vector<std::string> linha3 = {"A", "S", "D", "F", "G", "H", "J", "K", "L"};
    std::vector<std::string> linha4 = {"Z", "X", "C", "V", "B", "N", "M"};
    
    /**
     * @enum TeclaEspecial
     * @brief Identificadores para teclas que não são caracteres simples.
     */
    enum class TeclaEspecial { NENHUMA, ESPACO, LIMPAR, APAGAR, ENTER };
    
    int indiceX = 0; /**< Posição horizontal atual do cursor no grid. */
    int indiceY = 0; /**< Posição vertical atual do cursor no grid. */
    
    // Configurações de layout (posicionamento e tamanho)
    const int BASE_X = 460;     /**< Posição X inicial do desenho do teclado. */
    const int BASE_Y = 600;     /**< Posição Y inicial do desenho do teclado. */
    const int TECLA_SIZE = 60;  /**< Tamanho em pixels de cada tecla quadrada. */
    const int GAP = 10;         /**< Espaçamento entre as teclas. */
    
    // Controle de tempo para evitar entradas duplicadas (debounce)
    Uint32 ultimoInput = 0;
    const Uint32 INTERVALO_INPUT = 150;

    // Configurações para sticks analógicos
    Uint32 ultimoInputAnalogico = 0;
    const Uint32 INTERVALO_ANALOGICO = 200;
    const int DEADZONE = 16000; /**< Limiar de sensibilidade para ignorar movimentos leves no analógico. */

    /**
     * @brief Desenha uma tecla individual.
     * @param renderer Renderizador SDL.
     * @param x Coordenada X.
     * @param y Coordenada Y.
     * @param letra Caractere a ser exibido.
     * @param focado Define se a tecla deve ser desenhada com destaque de seleção.
     */
    void desenharTecla(SDL_Renderer* renderer, int x, int y, const std::string& letra, bool focado);
    
    /**
     * @brief Desenha a linha de teclas especiais (Espaço, Apagar, etc).
     * @param renderer Renderizador SDL.
     */
    void desenharEspeciais(SDL_Renderer* renderer);

    /**
     * @brief Altera a posição do cursor no grid, tratando limites de borda.
     * @param deltaX Mudança em X (-1, 0, 1).
     * @param deltaY Mudança em Y (-1, 0, 1).
     */
    void moverCursor(int deltaX, int deltaY);
    
    /**
     * @brief Auxiliar para detecção de colisão AABB (caixa delimitadora).
     * 
     * Verifica se as coordenadas do mouse estão dentro do retângulo de uma tecla.
     * 
     * @param mouseX Posição X do mouse.
     * @param mouseY Posição Y do mouse.
     * @param x X da tecla.
     * @param y Y da tecla.
     * @param w Largura da tecla.
     * @param h Altura da tecla.
     * @return true se houver colisão.
     */
    bool checarCliqueTecla(int mouseX, int mouseY, int x, int y, int w, int h);
};

} // namespace MeuProjeto

#endif // TECLADO_VIRTUAL_HPP