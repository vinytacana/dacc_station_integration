#include "ControleSlider.hpp"

#include <algorithm>

namespace MeuProjeto {

namespace {

constexpr int ESPACAMENTO_SEGMENTOS = 4;
constexpr SDL_Color COR_PREENCHIDA_DESABILITADA{120, 120, 120, 160};
constexpr SDL_Color COR_VAZIA_DESABILITADA{70, 70, 70, 130};

} // namespace

ControleSlider::ControleSlider(double valorMinimo, double valorMaximo, int totalSegmentos)
    : valorMinimo_(valorMinimo),
      valorMaximo_(valorMaximo > valorMinimo ? valorMaximo : valorMinimo + 1.0),
      totalSegmentos_(std::max(1, totalSegmentos)) {}

void ControleSlider::desenhar(
    SDL_Renderer* renderer,
    const SDL_Rect& area,
    double valorAtual,
    bool habilitado,
    const SDL_Color& corPreenchida,
    const SDL_Color& corVazia
) {
    area_ = area;

    const SDL_Color& preenchida = habilitado
        ? corPreenchida
        : COR_PREENCHIDA_DESABILITADA;
    const SDL_Color& vazia = habilitado
        ? corVazia
        : COR_VAZIA_DESABILITADA;

    desenharBarraProgresso(
        renderer,
        calcularSegmentosPreenchidos(valorAtual),
        preenchida,
        vazia
    );
}

bool ControleSlider::contemPonto(int x, int y) const {
    return area_.w > 0 && area_.h > 0 &&
           x >= area_.x && x <= area_.x + area_.w &&
           y >= area_.y && y <= area_.y + area_.h;
}

double ControleSlider::calcularValor(int mouseX) const {
    int segmento = calcularSegmento(mouseX);
    double passo = (valorMaximo_ - valorMinimo_) / totalSegmentos_;
    return valorMinimo_ + (segmento * passo);
}

int ControleSlider::calcularSegmento(int mouseX) const {
    if (area_.w <= 0) {
        return 0;
    }

    int posicao = std::clamp(mouseX - area_.x, 0, area_.w - 1);
    int segmento = (posicao * totalSegmentos_) / area_.w + 1;
    return std::clamp(segmento, 0, totalSegmentos_);
}

int ControleSlider::calcularSegmentosPreenchidos(double valorAtual) const {
    double proporcao = (valorAtual - valorMinimo_) / (valorMaximo_ - valorMinimo_);
    proporcao = std::clamp(proporcao, 0.0, 1.0);
    return static_cast<int>(proporcao * totalSegmentos_);
}

void ControleSlider::desenharBarraProgresso(
    SDL_Renderer* renderer,
    int segmentosPreenchidos,
    const SDL_Color& corPreenchida,
    const SDL_Color& corVazia
) const {
    if (!renderer || area_.w <= 0 || area_.h <= 0) {
        return;
    }

    int larguraSegmento =
        (area_.w - (ESPACAMENTO_SEGMENTOS * (totalSegmentos_ - 1))) / totalSegmentos_;
    if (larguraSegmento <= 0) {
        return;
    }

    for (int i = 0; i < totalSegmentos_; ++i) {
        int posicaoX = area_.x + i * (larguraSegmento + ESPACAMENTO_SEGMENTOS);
        SDL_Rect segmento{posicaoX, area_.y, larguraSegmento, area_.h};
        const SDL_Color& cor = i < segmentosPreenchidos ? corPreenchida : corVazia;

        SDL_SetRenderDrawColor(renderer, cor.r, cor.g, cor.b, cor.a);
        SDL_RenderFillRect(renderer, &segmento);
    }
}

} // namespace MeuProjeto
