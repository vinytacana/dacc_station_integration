#ifndef JANELA_JOGO_HPP
#define JANELA_JOGO_HPP

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "Botao.hpp"
#include "GerenciadorJogos.hpp"
#include "Jogo.hpp"

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
    int indiceSelecionado; 

    // ELEMENTOS DE UI
    Botao botaoJogar;

    // Layout Areas
    SDL_Rect areaJanela;
    SDL_Rect areaFundo;     
    SDL_Rect areaDescricao; 

    // Métodos Auxiliares de Desenho
    void desenharHero(const Jogo& jogo);       
    void desenharInfo(const Jogo& jogo);       
    void desenharCarrossel(const std::vector<Jogo>& listaJogos); 

    // Constantes de Layout (Escaladas para 1080p)
    static constexpr int CARD_LARGURA = 300;
    static constexpr int CARD_ALTURA = 420;
    static constexpr int CARD_SPACING = 30;
    static constexpr int CARROSSEL_Y = 700;
};

} // namespace MeuProjeto

#endif // JANELA_JOGO_HPP
