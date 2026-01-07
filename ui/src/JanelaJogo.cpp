#include "JanelaJogo.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorImagens.hpp"
#include "GerenciadorJogos.hpp" 
#include "GerenciadorAudio.hpp"
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
      botaoJogar(1450, 880, 400, 100, "JOGAR"), // Texto simplificado
      areaJanela{0, 0, 1920, 1080},
      areaFundo{100, 150, 1000, 600}, 
      areaDescricao{100, 320, 800, 350}
{
    // Configura Botão Jogar (Estilo Steam Deck)
    botaoJogar.setCor(Theme::ACCENT_COLOR, 
                      {Theme::ACCENT_COLOR.r, Theme::ACCENT_COLOR.g, Theme::ACCENT_COLOR.b, 200},
                      {Theme::ACCENT_COLOR.r, Theme::ACCENT_COLOR.g, Theme::ACCENT_COLOR.b, 150});
    botaoJogar.setFonte(TipoFonte::NEGRITO);
    botaoJogar.setTamanhoFonte(30); // Fonte levemente menor para acomodar ícone
    botaoJogar.setIcon("A"); // Define o ícone de input

    // Inicia música ambiente
    gerAudio.tocarMusica("bgm.mp3");
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
                    gerAudio.tocarSom("nav.wav");
                }
                break;
            case SDLK_LEFT:
                if (focoAtual == Foco::CARROSSEL) {
                    indiceSelecionado = (indiceSelecionado - 1 + totalJogos) % totalJogos;
                    gerAudio.tocarSom("nav.wav");
                }
                break;
            case SDLK_UP:
                // Sobe para ações (Botão Jogar)
                if (focoAtual != Foco::ACOES) gerAudio.tocarSom("nav.wav");
                focoAtual = Foco::ACOES;
                botaoJogar.setSelecionado(true);
                break;
            case SDLK_DOWN:
                // Desce para carrossel
                if (focoAtual != Foco::CARROSSEL) gerAudio.tocarSom("nav.wav");
                focoAtual = Foco::CARROSSEL;
                botaoJogar.setSelecionado(false);
                break;
            case SDLK_RETURN: 
            case SDLK_KP_ENTER:
                if (focoAtual == Foco::CARROSSEL || (focoAtual == Foco::ACOES && botaoJogar.estaSelecionado())) {
                     gerAudio.tocarSom("select.wav");
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
        1280, 720, 60, true // Gamescope ENABLED
    );
}

void JanelaJogo::desenhar(int offsetX, int offsetY) {
    // Obter dimensões reais da janela de renderização
    int screenW, screenH;
    SDL_GetRendererOutputSize(renderer, &screenW, &screenH);

    auto listaJogos = gerenciadorJogos.listarJogos();
    
    // Fallback se não houver jogos
    if (listaJogos.empty()) {
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);
        MeuProjeto::desenharTexto(renderer, "Nenhum jogo encontrado!", screenW/2 - 200, screenH/2, {255,255,255,255}, 40, TipoFonte::NEGRITO);
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
    
    // Reposiciona Botão Jogar (TODO 02)
    botaoJogar.area.x = areaDescricao.x;
    botaoJogar.area.y = 520; // Abaixo da descrição
    botaoJogar.area.w = 300;
    botaoJogar.area.h = 60;
    botaoJogar.setCornerRadius(30); // Pill shape

    // Atualizar Estado do Botão (TODO 09)
    if (NetworkClient::getInstance().isGameRunning()) {
        // Se há um jogo rodando...
        std::string runningId = NetworkClient::getInstance().getRunningGameId();
        
        // Verifica se é o jogo selecionado
        if (jogoAtual->getId() == runningId) {
            botaoJogar.setTexto("EM EXECUÇÃO"); 
            botaoJogar.setCor({50, 150, 50, 255}, {70, 170, 70, 255}, {40, 120, 40, 255}); // Verde
        } else {
            botaoJogar.setTexto("BLOQUEADO"); // Ou "JÁ EXISTE UM JOGO"
             botaoJogar.setCor({100, 50, 50, 255}, {100, 50, 50, 255}, {100, 50, 50, 255}); // Vermelho
        }
    } else {
        botaoJogar.setTexto("JOGAR"); // Simplificado
        // Azul Steam (Default)
        botaoJogar.setCor(Theme::ACCENT_COLOR, 
                      {Theme::ACCENT_COLOR.r, Theme::ACCENT_COLOR.g, Theme::ACCENT_COLOR.b, 200},
                      {Theme::ACCENT_COLOR.r, Theme::ACCENT_COLOR.g, Theme::ACCENT_COLOR.b, 150});
    }
    
    botaoJogar.desenhar(renderer, offsetX, offsetY);

    // --- 5. SYSTEM UI (Top/Bottom Bars) ---
    TTF_Font* fontUI = fontManager.carregar(ConfigFontes::obterCaminho(TipoFonte::NORMAL), 24);
    TTF_Font* fontUINegrito = fontManager.carregar(ConfigFontes::obterCaminho(TipoFonte::NEGRITO), 28);

    // Usar dimensões dinâmicas (screenW, screenH) garante que a barra cole no fundo real
    GraphicsUtils::drawSystemTopBar(renderer, screenW, fontUINegrito);
    GraphicsUtils::drawBottomNavHints(renderer, screenW, screenH, fontUI);
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
    // Título (Logo ou Texto)
    SDL_Texture* logo = gerImg.getLogo(renderer, jogo.getId());
    
    if (logo) {
        // Renderizar Logo (TODO 06)
        int logoW, logoH;
        SDL_QueryTexture(logo, NULL, NULL, &logoW, &logoH);
        
        // Target: Altura máx 120, Largura máx 500
        float scale = 1.0f;
        if (logoH > 140) scale = 140.0f / logoH;
        if (logoW * scale > 500) scale = 500.0f / logoW;
        
        int finalW = (int)(logoW * scale);
        int finalH = (int)(logoH * scale);
        
        SDL_Rect rectLogo = {areaFundo.x, areaFundo.y - 10, finalW, finalH};
        
        // Sombra do logo (Opcional, preta com baixa opacidade)
        SDL_SetTextureColorMod(logo, 0, 0, 0);
        SDL_SetTextureAlphaMod(logo, 100);
        SDL_Rect rectSombra = {rectLogo.x + 3, rectLogo.y + 3, rectLogo.w, rectLogo.h};
        SDL_RenderCopy(renderer, logo, NULL, &rectSombra);
        
        // Logo Real
        SDL_SetTextureColorMod(logo, 255, 255, 255);
        SDL_SetTextureAlphaMod(logo, 255);
        SDL_RenderCopy(renderer, logo, NULL, &rectLogo);

    } else {
        // Título Texto (Fallback)
        MeuProjeto::desenharTexto(renderer, jogo.getNome(), areaFundo.x + 3, areaFundo.y + 3, {0,0,0,150}, 72, TipoFonte::NEGRITO);
        MeuProjeto::desenharTexto(renderer, jogo.getNome(), areaFundo.x, areaFundo.y, Theme::TEXT_PRIMARY, 72, TipoFonte::NEGRITO);
    }

    // Metadata Row (TODO 03 & TODO 07)
    std::string metaGeneros = jogo.getGenero();
    if (metaGeneros.empty()) metaGeneros = jogo.getCodigo(); // Fallback para código se gênero não existir
    if (metaGeneros.empty()) metaGeneros = "GERAL";

    // Formatação Visual Dinâmica
    std::string metaDisplay = metaGeneros;
    
    // Adicionar Ano se válido
    if (jogo.getAno() > 0) {
        metaDisplay += "  •  " + std::to_string(jogo.getAno());
    }

    // Adicionar Players
    metaDisplay += "  •  " + std::to_string(jogo.getPlayers());
    if (jogo.getPlayers() == 1) metaDisplay += " JOGADOR";
    else metaDisplay += " JOGADORES";
    
    MeuProjeto::desenharTexto(renderer, metaDisplay, areaFundo.x, areaFundo.y + 90, {200, 200, 200, 255}, 24, TipoFonte::NORMAL);

    // Descrição
    std::string desc = jogo.getDescricao();
    if (!desc.empty()) {
        MeuProjeto::desenharTexto(
            renderer, 
            desc, 
            areaDescricao.x, 
            areaDescricao.y, 
            {220, 220, 220, 255}, // Mais claro para leitura
            24, 
            TipoFonte::NORMAL
        );
    }
}

void JanelaJogo::desenharCarrossel(const std::vector<Jogo>& listaJogos) {
    int totalJogos = static_cast<int>(listaJogos.size());
    
    // Configurações de Zoom (TODO 01)
    const int ZOOM_W = (int)(COVER_WIDTH * 1.25); // 25% maior
    const int ZOOM_H = (int)(COVER_HEIGHT * 1.25);
    const int ZOOM_OFFSET_Y = (ZOOM_H - COVER_HEIGHT) / 2; // Sobe metade do crescimento

    // Camera: Focada no item selecionado (X = 100)
    int targetX = 100;
    // O offset da câmera é calculado como se todos fossem normais para manter a grade consistente
    int cameraOffsetX = targetX - (indiceSelecionado * (COVER_WIDTH + COVER_SPACING));

    // Lambda para desenhar um item individual
    auto desenharItem = [&](int index, bool isSelected) {
        int w = isSelected ? ZOOM_W : COVER_WIDTH;
        int h = isSelected ? ZOOM_H : COVER_HEIGHT;
        
        // Posição base (Grid normal)
        int baseX = (index * (COVER_WIDTH + COVER_SPACING)) + cameraOffsetX;
        
        // Ajuste de centralização se estiver selecionado
        // Se cresceu 30px, move 15px pra esquerda e 15px pra cima
        int drawX = isSelected ? (baseX - (w - COVER_WIDTH)/2) : baseX;
        int drawY = CAROUSEL_Y_START - (isSelected ? ZOOM_OFFSET_Y : 0);

        SDL_Rect destRect = {drawX, drawY, w, h};

        // Culling (Simplificado)
        if (drawX + w < 0 || drawX > SCREEN_WIDTH) return;

        // Capa
        SDL_Texture* capa = gerImg.getCapa(renderer, listaJogos[index].getId());
        
        if (capa) {
             SDL_RenderCopy(renderer, capa, NULL, &destRect);
        } else {
             // Placeholder simples se falhar tudo
             SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
             SDL_RenderFillRect(renderer, &destRect);
        }

        if (isSelected) {
             // GLOW / BORDA DESTAQUE SUAVE (TODO 05)
             Uint32 ticks = SDL_GetTicks();
             // Pulso de Alpha entre 100 e 180
             int alphaPulse = 100 + (int)(40 * (1.0 + sin(ticks * 0.005))); 
             
             SDL_Color glowColor = {255, 255, 255, (Uint8)alphaPulse};
             if (focoAtual == Foco::ACOES) {
                 glowColor = {150, 150, 150, (Uint8)(alphaPulse / 2)}; // Mais apagado se foco estiver no botão
             }

             // Desenha brilho arredondado atrás (levemente maior)
             int glowPadding = 8;
             roundedBoxRGBA(renderer, 
                            destRect.x - glowPadding, destRect.y - glowPadding, 
                            destRect.x + destRect.w + glowPadding, destRect.y + destRect.h + glowPadding, 
                            12, // Radius
                            glowColor.r, glowColor.g, glowColor.b, glowColor.a);
             
             // Borda branca sólida fina para definição
             roundedRectangleRGBA(renderer, 
                                  destRect.x - 2, destRect.y - 2, 
                                  destRect.x + destRect.w + 2, destRect.y + destRect.h + 2, 
                                  4, // Radius menor
                                  255, 255, 255, 255);

        } else {
             // DIMMING (Não selecionados mais escuros)
             SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
             SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
             SDL_RenderFillRect(renderer, &destRect);
        }
    };

    // 1. Passada de Fundo: Itens NÃO selecionados
    for (int i = 0; i < totalJogos; i++) {
        if (i == indiceSelecionado) continue;
        desenharItem(i, false);
    }

    // 2. Passada de Frente: Item SELECIONADO (Z-Index Top)
    if (indiceSelecionado >= 0 && indiceSelecionado < totalJogos) {
        desenharItem(indiceSelecionado, true);
    }
}
