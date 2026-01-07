#ifndef JANELA_JOGO_HPP
#define JANELA_JOGO_HPP

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "Botao.hpp"
#include "GerenciadorJogos.hpp"
#include "Jogo.hpp"
#include "GerenciadorFontes.hpp"

namespace MeuProjeto {

class JanelaJogo {
public:
    /**
     * @brief Construtor da Janela de Jogo (Hub Principal).
     */
    JanelaJogo(SDL_Renderer* renderer);

    ~JanelaJogo();

    /**
     * @brief Desenha todos os elementos da janela (Hub).
     */
    void desenhar(int offsetX = 0, int offsetY = 0);

    /**
     * @brief Trata eventos de navegação e seleção.
     * @return true se a aplicação deve fechar.
     */
    bool tratarEvento(SDL_Event& evento, int offsetX, int offsetY);

    /**
     * @brief Inicia o jogo selecionado.
     */
    void executarJogo();

private:
    SDL_Renderer* renderer;

    // ESTADO DA UI
    enum class Foco { CARROSSEL, ACOES };
    Foco focoAtual = Foco::CARROSSEL;
    int indiceSelecionado; 

    // ELEMENTOS DE UI
    Botao botaoJogar;

    // Gerenciador de Fontes Local
    GerenciadorFontes fontManager;

    // Layout Areas
    SDL_Rect areaJanela;
    SDL_Rect areaFundo;     
    SDL_Rect areaDescricao; 

    // ATMOSFERA (Fundo Dinâmico)
    SDL_Texture* bgAtual = nullptr;
    SDL_Texture* bgProximo = nullptr;
    float transitionAlpha = 255.0f;
    int lastIndice = -1; // Para detectar mudança de seleção

    // Métodos Auxiliares de Desenho
    void desenharHero(const Jogo& jogo);       
    void desenharInfo(const Jogo& jogo);       
    void desenharCarrossel(const std::vector<Jogo>& listaJogos); 

    // Constantes de Layout (Escaladas para 1080p - TODO 01)
    static constexpr int HERO_SECTION_HEIGHT = 720;
    static constexpr int CAROUSEL_Y_START = 750;
    static constexpr int COVER_WIDTH = 200;
    static constexpr int COVER_HEIGHT = 300;
    static constexpr int COVER_SPACING = 20;
    static constexpr int SCREEN_WIDTH = 1920;
};

} // namespace MeuProjeto

#endif // JANELA_JOGO_HPP
