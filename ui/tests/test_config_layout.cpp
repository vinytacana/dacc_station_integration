#include "ConfigLayout.hpp"
#include "JanelaAudioEVideo.hpp"
#include "LayoutEvents.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void exigir(bool condicao, const std::string& mensagem) {
    if (!condicao) {
        std::cerr << "[FAIL] " << mensagem << std::endl;
        std::exit(1);
    }
}

} // namespace

int main() {
    MeuProjeto::ConfigLayout::inicializar(1280, 720);
    exigir(MeuProjeto::ConfigLayout::larguraTela == 1280, "deve guardar a largura real");
    exigir(MeuProjeto::ConfigLayout::alturaTela == 720, "deve guardar a altura real");
    exigir(
        std::fabs(MeuProjeto::ConfigLayout::escalaGlobal - 0.5625f) < 0.0001f,
        "deve escolher a escala que cabe na area real"
    );
    exigir(MeuProjeto::ConfigLayout::offsetX == 100, "deve centralizar horizontalmente");
    exigir(MeuProjeto::ConfigLayout::offsetY == 0, "nao deve deslocar verticalmente sem sobra");

    MeuProjeto::ConfigLayout::inicializar(0, 0);
    exigir(
        MeuProjeto::ConfigLayout::larguraTela == 1280 &&
        MeuProjeto::ConfigLayout::alturaTela == 720,
        "dimensoes transitorias invalidas nao devem destruir o layout valido"
    );

    SDL_Event evento{};
    evento.type = SDL_WINDOWEVENT;
    evento.window.windowID = 7;
    evento.window.event = SDL_WINDOWEVENT_SIZE_CHANGED;
    exigir(MeuProjeto::eventoAtualizaLayout(evento, 7), "resize da janela principal deve atualizar layout");
    exigir(!MeuProjeto::eventoAtualizaLayout(evento, 8), "resize de outra janela deve ser ignorado");
#if SDL_VERSION_ATLEAST(2, 0, 18)
    evento.window.event = SDL_WINDOWEVENT_DISPLAY_CHANGED;
    exigir(MeuProjeto::eventoAtualizaLayout(evento, 7), "mudanca de display deve atualizar layout");
#endif
    evento.window.event = SDL_WINDOWEVENT_FOCUS_GAINED;
    exigir(!MeuProjeto::eventoAtualizaLayout(evento, 7), "evento sem geometria nao deve recalcular layout");

    MeuProjeto::Resolucao resolucao(1920, 1080, 59.94f);
    exigir(
        std::fabs(resolucao.taxaAtualizacao - 59.94f) < 0.001f,
        "opcao da UI deve preservar a taxa informada pelo backend"
    );
    MeuProjeto::Resolucao resolucaoSemTaxa(1920, 1080);
    exigir(
        resolucaoSemTaxa.taxaAtualizacao == 0.0f,
        "resolucao sem taxa conhecida nao deve assumir 60 Hz"
    );

    std::cout << "Config layout tests passed." << std::endl;
    return 0;
}
