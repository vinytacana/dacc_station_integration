#include "JanelaJogo.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorImagens.hpp"
#include "GerenciadorJogos.hpp" 
#include "Utils.hpp"
#include "Theme.hpp"
#include "GraphicsUtils.hpp"
#include "NetworkClient.hpp"
#include <iostream>
#include <algorithm> 

using namespace MeuProjeto;

// Referências globais
extern GerenciadorImagens gerImg;
extern GerenciadorJogos gerenciadorJogos; 

JanelaJogo::JanelaJogo(SDL_Renderer* renderer)
    : renderer(renderer),
      indiceSelecionado(0),
      botaoJogar(1450, 880, 400, 100, "JOGAR (Enter)"),
      areaJanela{0, 0, 1920, 1080},
      areaFundo{100, 150, 1000, 600}, 
      areaDescricao{100, 320, 800, 350}
{
    // Configura Botão Jogar (Estilo Steam Deck)
    botaoJogar.setCor(Theme::ACCENT_COLOR, 
                      {Theme::ACCENT_COLOR.r, Theme::ACCENT_COLOR.g, Theme::ACCENT_COLOR.b, 200},
                      {Theme::ACCENT_COLOR.r, Theme::ACCENT_COLOR.g, Theme::ACCENT_COLOR.b, 150});
    botaoJogar.setFonte(TipoFonte::NEGRITO);
    botaoJogar.setTamanhoFonte(36);
}

JanelaJogo::~JanelaJogo() {
    std::cout << "[DEBUG] JanelaJogo destruída." << std::endl;
}

bool JanelaJogo::tratarEvento(SDL_Event& evento, int offsetX, int offsetY) {
    auto listaJogos = gerenciadorJogos.listarJogos();
    if (listaJogos.empty()) {
        if (evento.type == SDL_KEYDOWN && evento.key.keysym.sym == SDLK_ESCAPE) return true;
        return false;
    }

    int totalJogos = static_cast<int>(listaJogos.size());

    // 1. Navegação por Teclado (Setas)
    if (evento.type == SDL_KEYDOWN) {
        switch (evento.key.keysym.sym) {
            case SDLK_RIGHT:
                indiceSelecionado = (indiceSelecionado + 1) % totalJogos;
                break;
            case SDLK_LEFT:
                indiceSelecionado = (indiceSelecionado - 1 + totalJogos) % totalJogos;
                break;
            case SDLK_RETURN: 
            case SDLK_KP_ENTER:
                executarJogo();
                break;
            case SDLK_ESCAPE:
                return true; // Sinaliza para fechar a aplicação
        }
    }

    // 2. Mouse no Botão Jogar
    if (botaoJogar.tratarEvento(evento, offsetX, offsetY)) {
        if (evento.type == SDL_MOUSEBUTTONUP) {
            executarJogo();
        }
    }

    return false;
}

void JanelaJogo::executarJogo() {
    Jogo* jogoAtual = gerenciadorJogos.obterJogoPorIndice(indiceSelecionado);
    if (!jogoAtual) return;

    std::cout << "[LAUNCHER] Iniciando: " << jogoAtual->getNome() << std::endl;

    // Envia comando via NetworkClient para o Process Manager
    NetworkClient::getInstance().sendStartGame(
        jogoAtual->getNome(),
        jogoAtual->getExecutavel(), 
        1280, 720, 60, false
    );
}

void JanelaJogo::desenhar(int offsetX, int offsetY) {
    auto listaJogos = gerenciadorJogos.listarJogos();
    
    // Fallback se não houver jogos
    if (listaJogos.empty()) {
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
        MeuProjeto::desenharTexto(renderer, "Nenhum jogo encontrado!", 500, 500, {255,255,255,255}, 40, TipoFonte::NEGRITO);
        return;
    }

    // Garante índice seguro
    if (indiceSelecionado >= (int)listaJogos.size()) indiceSelecionado = (int)listaJogos.size() - 1;
    
    Jogo* jogoAtual = &listaJogos[indiceSelecionado];

    // --- 1. CAMADA HERO (Fundo) ---
    desenharHero(*jogoAtual);

    // --- 2. CAMADA INFO (Texto) ---
    desenharInfo(*jogoAtual);

    // --- 3. CAMADA CARROSSEL (Lista Horizontal) ---
    desenharCarrossel(listaJogos);

    // --- 4. BOTÕES ---
    botaoJogar.desenhar(renderer, offsetX, offsetY);

    // --- 5. SYSTEM UI (Top/Bottom Bars) ---
    // Usamos as fontes globais via ConfigFontes ou gerenciador se necessário.
    // Para simplificar e manter performance, JanelaJogo poderia ter suas próprias fontes abertas,
    // mas aqui usaremos o Utils::desenharTexto que já lida com isso.
    // No entanto, as funções de GraphicsUtils pedem TTF_Font*.
    // Vou carregar fontes locais na JanelaJogo para as barras.
    
    static GerenciadorFontes fontGen;
    TTF_Font* fontUI = fontGen.carregar(ConfigFontes::obterCaminho(TipoFonte::NORMAL), 24);
    TTF_Font* fontUINegrito = fontGen.carregar(ConfigFontes::obterCaminho(TipoFonte::NEGRITO), 28);

    GraphicsUtils::drawSystemTopBar(renderer, areaJanela.w, fontUINegrito);
    GraphicsUtils::drawBottomNavHints(renderer, areaJanela.w, areaJanela.h, fontUI);
}

void JanelaJogo::desenharHero(const Jogo& jogo) {
    SDL_Texture* tex = gerImg.carregar(renderer, jogo.getCapa().c_str());
    
    if (tex) {
        GraphicsUtils::drawTextureCover(renderer, tex, areaJanela, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderFillRect(renderer, &areaJanela);
    }

    // Overlay Escuro para legibilidade
    GraphicsUtils::drawOverlay(renderer, areaJanela.w, areaJanela.h, 210); 
}

void JanelaJogo::desenharInfo(const Jogo& jogo) {
    // Título
    MeuProjeto::desenharTexto(
        renderer, 
        jogo.getNome(), 
        areaFundo.x, 
        areaFundo.y, 
        Theme::TEXT_PRIMARY, 
        72, 
        TipoFonte::NEGRITO
    );

    // Categoria
    MeuProjeto::desenharTexto(
        renderer, 
        jogo.getCodigo(), 
        areaFundo.x, 
        areaFundo.y + 100, 
        Theme::TEXT_SECONDARY, 
        28, 
        TipoFonte::NORMAL
    );

    // Descrição
    std::string desc = jogo.getDescricao();
    // A função desenharTexto do Utils não faz wrap automático por padrão na assinatura simples.
    // Mas o GraphicsUtils::drawSystemTopBar etc usam TTF. 
    // Para a descrição, vamos usar um wrap simples ou apenas desenhar.
    MeuProjeto::desenharTexto(
        renderer, 
        desc, 
        areaDescricao.x, 
        areaDescricao.y, 
        Theme::TEXT_SECONDARY, 
        24, 
        TipoFonte::NORMAL
    );
}

void JanelaJogo::desenharCarrossel(const std::vector<Jogo>& listaJogos) {
    int startX = areaFundo.x; // Alinhado com o texto
    int yPos = CARROSSEL_Y;
    int total = static_cast<int>(listaJogos.size());

    // Desenha uma janela de jogos a partir do selecionado
    for (int offset = 0; offset < 5; offset++) {
        int idx = (indiceSelecionado + offset) % total;
        
        int xPos = startX + (offset * (CARD_LARGURA + CARD_SPACING));

        if (xPos > areaJanela.w) break;

        SDL_Rect destRect = {xPos, yPos, CARD_LARGURA, CARD_ALTURA};

        SDL_Texture* capa = gerImg.carregar(renderer, listaJogos[idx].getCapa().c_str());
        if (capa) {
            SDL_RenderCopy(renderer, capa, NULL, &destRect);
        } else {
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &destRect);
        }

        if (offset == 0) {
            // Selecionado
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            GraphicsUtils::drawRoundedRectOutline(renderer, destRect, 0, {255, 255, 255, 255});
            // Espessura extra
            SDL_Rect r2 = {destRect.x-1, destRect.y-1, destRect.w+2, destRect.h+2};
            SDL_RenderDrawRect(renderer, &r2);
            SDL_Rect r3 = {destRect.x-2, destRect.y-2, destRect.w+4, destRect.h+4};
            SDL_RenderDrawRect(renderer, &r3);
        } else {
            // Não selecionado (Escurecido)
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 120);
            SDL_RenderFillRect(renderer, &destRect);
        }
    }
}
