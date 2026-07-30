#pragma once

#include <SDL2/SDL.h>

namespace MeuProjeto {

/**
 * @brief Componente reutilizável para sliders segmentados.
 *
 * Mantém a hitbox, converte a posição do mouse em valor e desenha os
 * segmentos preenchidos e vazios, incluindo o estado desabilitado.
 */
class ControleSlider {
public:
    ControleSlider(double valorMinimo, double valorMaximo, int totalSegmentos);

    /**
     * @brief Atualiza a área interativa e desenha o slider.
     */
    void desenhar(
        SDL_Renderer* renderer,
        const SDL_Rect& area,
        double valorAtual,
        bool habilitado,
        const SDL_Color& corPreenchida,
        const SDL_Color& corVazia
    );

    /**
     * @brief Indica se o ponto está dentro da hitbox atual.
     */
    bool contemPonto(int x, int y) const;

    /**
     * @brief Converte a coordenada X em um valor alinhado aos segmentos.
     */
    double calcularValor(int mouseX) const;

private:
    double valorMinimo_;
    double valorMaximo_;
    int totalSegmentos_;
    SDL_Rect area_{0, 0, 0, 0};

    int calcularSegmento(int mouseX) const;
    int calcularSegmentosPreenchidos(double valorAtual) const;
    void desenharBarraProgresso(
        SDL_Renderer* renderer,
        int segmentosPreenchidos,
        const SDL_Color& corPreenchida,
        const SDL_Color& corVazia
    ) const;
};

} // namespace MeuProjeto
