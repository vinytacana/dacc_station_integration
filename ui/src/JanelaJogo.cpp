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

    // 1. Navegação por Teclado
    if (evento.type == SDL_KEYDOWN) {
        switch (evento.key.keysym.sym) {
            case SDLK_RIGHT:
                if (focoAtual == Foco::CARROSSEL) {
                    indiceSelecionado = (indiceSelecionado + 1) % totalJogos;
                }
                break;
            case SDLK_LEFT:
                if (focoAtual == Foco::CARROSSEL) {
                    indiceSelecionado = (indiceSelecionado - 1 + totalJogos) % totalJogos;
                }
                break;
            case SDLK_UP:
                // Sobe para ações (Botão Jogar)
                focoAtual = Foco::ACOES;
                botaoJogar.setSelecionado(true);
                break;
            case SDLK_DOWN:
                // Desce para carrossel
                focoAtual = Foco::CARROSSEL;
                botaoJogar.setSelecionado(false);
                break;
            case SDLK_RETURN: 
            case SDLK_KP_ENTER:
                if (focoAtual == Foco::CARROSSEL || (focoAtual == Foco::ACOES && botaoJogar.estaSelecionado())) {
                     executarJogo();
                }
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
    // Bloquear se já houver algo rodando
    if (NetworkClient::getInstance().isGameRunning()) {
        std::cout << "[LAUNCHER] Bloqueado: Já existe um processo ativo." << std::endl;
        return;
    }

    Jogo* jogoAtual = gerenciadorJogos.obterJogoPorIndice(indiceSelecionado);
    if (!jogoAtual) return;

    std::cout << "[LAUNCHER] Iniciando: " << jogoAtual->getNome() << " (ID: " << jogoAtual->getId() << ")" << std::endl;

    // Envia comando via NetworkClient para o Process Manager
    NetworkClient::getInstance().sendStartGame(
        jogoAtual->getId(), // Passar ID (TODO 09)
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
    
    // Atualizar Estado do Botão (TODO 09)
    if (NetworkClient::getInstance().isGameRunning()) {
        // Se há um jogo rodando...
        std::string runningId = NetworkClient::getInstance().getRunningGameId();
        
        // Verifica se é o jogo selecionado
        if (jogoAtual->getId() == runningId) {
            botaoJogar.setTexto("EM EXECUÇÃO"); 
        } else {
            botaoJogar.setTexto("BLOQUEADO"); // Ou "JÁ EXISTE UM JOGO"
        }
    } else {
        botaoJogar.setTexto("JOGAR (Enter)");
    }
    
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
    // Detectar mudança de seleção para iniciar transição
    if (indiceSelecionado != lastIndice) {
        lastIndice = indiceSelecionado;
        
        // O antigo "Proximo" vira o "Atual" (fundo)
        if (bgProximo) {
            bgAtual = bgProximo;
        }
        
        // Carrega o novo "Proximo"
        bgProximo = gerImg.getWallpaper(renderer, jogo.getId());
        
        // Se não tinha nada antes (primeiro frame), define imediatamente
        if (!bgAtual) bgAtual = bgProximo;
        
        // Reseta transição
        transitionAlpha = 0.0f;
    }

    // Atualizar Alpha (Animação) - Aprox. 60 FPS
    if (transitionAlpha < 255.0f) {
        transitionAlpha += 15.0f; // Velocidade do fade
        if (transitionAlpha > 255.0f) transitionAlpha = 255.0f;
    }

    // 1. Desenhar Fundo (Anterior/Base)
    if (bgAtual) {
        SDL_SetTextureAlphaMod(bgAtual, 255);
        SDL_RenderCopy(renderer, bgAtual, NULL, NULL);
    } else {
        // Fallback cor sólida se não houver imagem nenhuma
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderFillRect(renderer, &areaJanela);
    }

    // 2. Desenhar Novo Fundo (Fade In)
    if (bgProximo && bgProximo != bgAtual) {
        SDL_SetTextureAlphaMod(bgProximo, (Uint8)transitionAlpha);
        SDL_RenderCopy(renderer, bgProximo, NULL, NULL);
        // Resetar alpha para não afetar outros usos
        SDL_SetTextureAlphaMod(bgProximo, 255); 
    }

    // Overlay Escuro para legibilidade (Alpha 200 conforme TODO 02)
    GraphicsUtils::drawOverlay(renderer, areaJanela.w, areaJanela.h, 200); 
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
    if (!desc.empty()) {
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
}

void JanelaJogo::desenharCarrossel(const std::vector<Jogo>& listaJogos) {
    int totalJogos = static_cast<int>(listaJogos.size());
    
    // Lógica de Câmera (TODO 01)
    // Queremos que o item selecionado fique na posição X = 100
    int targetX = 100;
    int cameraOffsetX = targetX - (indiceSelecionado * (COVER_WIDTH + COVER_SPACING));

    for (int i = 0; i < totalJogos; i++) {
        // Calcular posição absoluta X deste item baseado na câmera
        int drawX = (i * (COVER_WIDTH + COVER_SPACING)) + cameraOffsetX;

        // Culling: Se estiver totalmente fora da tela, pule
        if (drawX + COVER_WIDTH < 0 || drawX > SCREEN_WIDTH) {
            continue;
        }

        SDL_Rect destRect = {drawX, CAROUSEL_Y_START, COVER_WIDTH, COVER_HEIGHT};

        // Usar capa oficial ou fallback
        SDL_Texture* capa = gerImg.getCapa(renderer, listaJogos[i].getId());
        
        if (capa) {
            SDL_RenderCopy(renderer, capa, NULL, &destRect);
        } else {
            // Placeholder simples se falhar tudo
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            SDL_RenderFillRect(renderer, &destRect);
            
            // Borda interna para distinguir placeholders
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_Rect rBorder = {destRect.x+2, destRect.y+2, destRect.w-4, destRect.h-4};
            SDL_RenderDrawRect(renderer, &rBorder);
        }

        // Highlight Básico (TODO 04)
        if (i == indiceSelecionado) {
             // Se o foco estiver nas ações (botão Jogar), o carrossel fica "inativo" (cinza)
             // Se o foco estiver no carrossel, borda branca brilhante
             if (focoAtual == Foco::CARROSSEL) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Branco ativo
             } else {
                SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255); // Cinza inativo
             }

             // Borda grossa (4px)
             for(int w=0; w<4; w++) {
                 SDL_Rect outline = {destRect.x - w, destRect.y - w, destRect.w + (w*2), destRect.h + (w*2)};
                 SDL_RenderDrawRect(renderer, &outline);
             }
        } else {
             // Dimming para não selecionados
             SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
             SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
             SDL_RenderFillRect(renderer, &destRect);
        }
    }
}
