// Compilar: g++ src/*.cpp -I include -o gerenciador -lSDL2 -lSDL2_image -lSDL2_gfx -lSDL2_ttf
// Executar: ./gerenciador

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <cmath> // Para std::abs, std::min, std::max

// Headers do Projeto
#include "GerenciadorImagens.hpp"
#include "GerenciadorJogos.hpp"
#include "GerenciadorFontes.hpp"
#include "Arquivos.hpp"
#include "Utils.hpp"
#include "Theme.hpp"
#include "GraphicsUtils.hpp"
#include "JanelaJogo.hpp"
#include "NetworkClient.hpp"

using namespace MeuProjeto;
using namespace std;

// ============================================================================
// CONFIGURAÇÕES GLOBAIS & CONSTANTES
// ============================================================================
const int LARGURA_TELA = 1920;
const int ALTURA_TELA = 1080;

// Layout Hero
const int MARGEM_ESQUERDA = 100;
const int HERO_TITLE_Y = 150;
const int HERO_META_Y = 250;
const int HERO_DESC_Y = 320;
const int HERO_ACTION_Y = 550;

// Layout Carrossel
const int CARROSSEL_Y = 700;
const int CARD_WIDTH = 300;
const int CARD_HEIGHT = 420; // Aspect ratio capa DVD/Bluray
const int CARD_GAP = 30;

// Cores
const SDL_Color COR_BRANCO = {255, 255, 255, 255};
const SDL_Color COR_CINZA = {200, 200, 200, 255};
const SDL_Color COR_DESTAQUE = {255, 255, 255, 255}; // Branco puro para seleção

// ============================================================================
// SINGLETONS E ESTADO GLOBAL
// ============================================================================
GerenciadorImagens gerImg;
GerenciadorJogos gerenciadorJogos;
// GerenciadorFontes poderia ser global também, mas usamos Utils::getFonte por enquanto
// ou implementamos se necessário. Assumindo uso de Utils::desenharTexto que usa TTF_OpenFont internamente
// mas para performance no loop, ideal seria manter fontes abertas.
// Vamos usar uma abordagem RAII local ou um map estático se Utils não for otimizado.
// Verificando Utils.hpp... O Utils geralmente abre e fecha fonte. Isso é lento para render loop.
// Vamos criar um cache de fontes simples aqui para o loop principal.

TTF_Font* fontHeroTitle = nullptr;
TTF_Font* fontHeroMeta = nullptr;
TTF_Font* fontHeroDesc = nullptr;

// Estado da Aplicação
struct AppState {
    int selectedIndex = 0;
    bool running = true;
    JanelaJogo* janelaDetalhes = nullptr; // Se != nullptr, estamos na view de detalhes
    
    // Animação (Futuro)
    float scrollX = 0.0f;
    float targetScrollX = 0.0f;
};

AppState appState;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void carregarDadosJogos() {
    cout << "[INIT] Carregando biblioteca de jogos..." << endl;
    vector<string> arquivos = {
        "assets/data/jogo1.txt", "assets/data/jogo2.txt", 
        "assets/data/jogo3.txt", "assets/data/jogo4.txt", 
        "assets/data/jogo5.txt"
    };
    
    for (const auto& arq : arquivos) {
        Arquivos::carregarJogos(arq, gerenciadorJogos);
    }
    
    // Adicionar placeholder se vazio para não crashar
    if (gerenciadorJogos.listarJogos().empty()) {
        cout << "[WARN] Nenhum jogo encontrado. Criando placeholder." << endl;
        // Jogo placeholder = Jogo("Sem Jogos", "Verifique a pasta assets/data", "System", "assets/textures/placeholder.png", {});
        // gerenciadorJogos.adicionarJogo(placeholder);
    }
}

void initFonts() {
    // Carrega fontes persistentes para evitar I/O a cada frame
    fontHeroTitle = TTF_OpenFont("assets/fonts/Garet-Heavy.ttf", 72);
    fontHeroMeta = TTF_OpenFont("assets/fonts/Garet-Book.ttf", 28);
    fontHeroDesc = TTF_OpenFont("assets/fonts/Garet-Book.ttf", 24);

    if (!fontHeroTitle || !fontHeroMeta || !fontHeroDesc) {
        cerr << "[ERROR] Falha ao carregar fontes: " << TTF_GetError() << endl;
        // Tenta fallback
        fontHeroTitle = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 72);
    }
}

void closeFonts() {
    if (fontHeroTitle) TTF_CloseFont(fontHeroTitle);
    if (fontHeroMeta) TTF_CloseFont(fontHeroMeta);
    if (fontHeroDesc) TTF_CloseFont(fontHeroDesc);
}

// Renderiza texto usando fonte pré-carregada para performance
void renderTextFast(SDL_Renderer* renderer, TTF_Font* font, const string& text, int x, int y, SDL_Color color, int maxWidth = 0) {
    if (!font || text.empty()) return;

    SDL_Surface* surface = nullptr;
    if (maxWidth > 0) {
        surface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), color, maxWidth);
    } else {
        surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    }

    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_Rect dst = {x, y, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &dst);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

// Cria a Janela de Detalhes (mantida para compatibilidade com lógica existente)
JanelaJogo* criarJanelaDetalhes(SDL_Renderer* renderer, Jogo* jogo) {
    if (!jogo) return nullptr;
    
    stringstream ss;
    ss << "Título: " << jogo->getNome() << "\n"
       << "Gênero: " << jogo->getCodigo() << "\n\n"
       << jogo->getDescricao();
    
    return new JanelaJogo(renderer, jogo->getNome(), jogo->getCapa(), ss.str(), jogo->getCapturas(), jogo->getExecutavel());
}

// ============================================================================
// MAIN LOOP
// ============================================================================

int main(int argc, char* argv[]) {
    // 1. Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    TTF_Init();

    // 2. Janela & Renderizador
    SDL_Window* window = SDL_CreateWindow("DACC Station - Hero UI", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        LARGURA_TELA, ALTURA_TELA, 
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS);
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // 3. Inicialização de Dados
    carregarDadosJogos();
    initFonts();
    NetworkClient::getInstance().connectToManager(); // IPC Start

    // Validação inicial
    int totalJogos = gerenciadorJogos.listarJogos().size();
    if (totalJogos == 0) {
        // Fallback mode se não houver jogos
    }

    // 4. Loop Principal
    SDL_Event e;
    while (appState.running) {
        
        // -------------------------------------------------
        // A. INPUT HANDLING
        // -------------------------------------------------
        while (SDL_PollEvent(&e)) {
            // IPC Events
            NetworkClient::getInstance().checkEvents();

            if (e.type == SDL_QUIT) {
                appState.running = false;
            }
            
            // Se estiver na janela de detalhes, delega o input para ela
            if (appState.janelaDetalhes) {
                if (appState.janelaDetalhes->tratarEvento(e, 0, 0)) {
                    // Se tratarEvento retornar true, fecha a janela de detalhes
                    delete appState.janelaDetalhes;
                    appState.janelaDetalhes = nullptr;
                    // Limpa buffer de entrada para evitar ações duplas
                }
                continue; // Pula lógica do menu principal
            }

            // Input do Menu Principal
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_RIGHT:
                        appState.selectedIndex = (appState.selectedIndex + 1) % totalJogos;
                        break;
                    case SDLK_LEFT:
                        appState.selectedIndex = (appState.selectedIndex - 1 + totalJogos) % totalJogos;
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                    case SDLK_SPACE:
                        // Abrir Detalhes / Jogar
                        if (totalJogos > 0) {
                            Jogo* j = gerenciadorJogos.obterJogoPorIndice(appState.selectedIndex);
                            appState.janelaDetalhes = criarJanelaDetalhes(renderer, j);
                        }
                        break;
                    case SDLK_ESCAPE:
                        appState.running = false; // Ou abrir menu de sair
                        break;
                }
            }
        }

        // Se estiver na janela de detalhes, renderiza ela e pula o resto
        if (appState.janelaDetalhes) {
            appState.janelaDetalhes->desenhar(0, 0); // X, Y offset
            SDL_RenderPresent(renderer);
            continue;
        }

        // -------------------------------------------------
        // B. RENDER PIPELINE (Z-ORDER)
        // -------------------------------------------------
        SDL_RenderClear(renderer);

        Jogo* jogoAtual = nullptr;
        if (totalJogos > 0) {
            jogoAtual = gerenciadorJogos.obterJogoPorIndice(appState.selectedIndex);
        }

        if (jogoAtual) {
            // LAYER 0: BACKGROUND ATMOSFÉRICO
            // -------------------------------------------------
            SDL_Texture* bgTex = gerImg.carregar(renderer, jogoAtual->getCapa());
            if (bgTex) {
                // Desenha em tela cheia (Stretch ou Cover)
                SDL_Rect screenRect = {0, 0, LARGURA_TELA, ALTURA_TELA};
                GraphicsUtils::drawTextureCover(renderer, bgTex, screenRect);
            }

            // LAYER 1: DIMMER / OVERLAY
            // -------------------------------------------------
            // Retângulo preto com transparência para garantir leitura do texto
            GraphicsUtils::drawOverlay(renderer, LARGURA_TELA, ALTURA_TELA, 210); // Alpha 210/255

            // LAYER 2: HERO CONTENT (TEXTOS)
            // -------------------------------------------------
            // Título
            renderTextFast(renderer, fontHeroTitle, jogoAtual->getNome(), MARGEM_ESQUERDA, HERO_TITLE_Y, COR_BRANCO);
            
            // Metadados (Categoria | Genero)
            string meta = jogoAtual->getCodigo(); // Assumindo que codigo guarda info curta. Se não, ajustar.
            if (meta.empty()) meta = "Jogo DACC";
            renderTextFast(renderer, fontHeroMeta, meta, MARGEM_ESQUERDA, HERO_META_Y, COR_CINZA);

            // Sinopse (Com quebra de linha)
            string desc = jogoAtual->getDescricao();
            // Limitar texto se muito longo visualmente? O TTF_RenderUTF8_Blended_Wrapped cuida da quebra.
            renderTextFast(renderer, fontHeroDesc, desc, MARGEM_ESQUERDA, HERO_DESC_Y, COR_CINZA, 800); // Max Width 800px

            // Ação CTA
            renderTextFast(renderer, fontHeroMeta, "[ENTER] Jogar  [ESC] Sair", MARGEM_ESQUERDA, HERO_ACTION_Y, COR_BRANCO);
        }

        // LAYER 3: CARROSSEL (NAVEGAÇÃO)
        // -------------------------------------------------
        // Renderiza lista horizontal na parte inferior
        
        // Posição X fixa para o item selecionado (ex: 100px da esquerda)
        // Ou centralizado? Vamos manter o selecionado à esquerda alinhado com o texto (MARGEM_ESQUERDA).
        
        int startX = MARGEM_ESQUERDA; 
        
        // Queremos desenhar uma janela de jogos ao redor do índice atual
        // Vamos desenhar do [selected] até [selected + N] que couber na tela
        // E talvez [selected - 1] parcialmente fora se quisermos efeito de continuidade
        
        for (int offset = 0; offset < 6; offset++) {
            int idx = (appState.selectedIndex + offset) % totalJogos;
            if (idx < 0) idx += totalJogos; // Safety
            
            Jogo* j = gerenciadorJogos.obterJogoPorIndice(idx);
            if (!j) continue;

            // Posição Visual
            int x = startX + (offset * (CARD_WIDTH + CARD_GAP));
            int y = CARROSSEL_Y;
            
            // Se passar da tela, para de desenhar
            if (x > LARGURA_TELA) break;

            // Desenha Capa
            SDL_Rect cardRect = {x, y, CARD_WIDTH, CARD_HEIGHT};
            SDL_Texture* coverTex = gerImg.carregar(renderer, j->getCapa());
            
            if (coverTex) {
                SDL_RenderCopy(renderer, coverTex, NULL, &cardRect);
            } else {
                // Placeholder retângulo cinza
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                SDL_RenderFillRect(renderer, &cardRect);
            }

            // Destaque (Focus Ring) apenas para o offset 0 (o selecionado)
            if (offset == 0) {
                // Borda Branca Espessa
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                GraphicsUtils::drawRoundedRectOutline(renderer, cardRect, 0, COR_BRANCO);
                // Desenhar mais uma vez para espessura (hack simples de espessura 2px)
                SDL_Rect r2 = {x-1, y-1, CARD_WIDTH+2, CARD_HEIGHT+2};
                SDL_RenderDrawRect(renderer, &r2);
                SDL_Rect r3 = {x-2, y-2, CARD_WIDTH+4, CARD_HEIGHT+4};
                SDL_RenderDrawRect(renderer, &r3);
            } else {
                // Escurecer itens não selecionados
                GraphicsUtils::drawOverlay(renderer, LARGURA_TELA, ALTURA_TELA, 0); // Reset blend mode check? 
                // Não, precisamos desenhar um rect preto semi-transparente SOBRE este card específico
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100); // Alpha 100
                SDL_RenderFillRect(renderer, &cardRect);
            }
        }

        // Finaliza Frame
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // 5. Cleanup
    closeFonts();
    gerImg.liberarTudo(); // Libera texturas
    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
