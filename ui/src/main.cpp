#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>

// Headers do Projeto
#include "GerenciadorImagens.hpp"
#include "GerenciadorJogos.hpp"
#include "GerenciadorFontes.hpp"
#include "Arquivos.hpp"
#include "Utils.hpp"
#include "Theme.hpp"
#include "JanelaJogo.hpp"
#include "NetworkClient.hpp"
#include "SystemStatus.hpp"

using namespace MeuProjeto;
using namespace std;

// Constantes de Tela
const int LARGURA_TELA = 1920;
const int ALTURA_TELA = 1080;

// Estado Global (Simplificado)
struct AppState {
    bool running = true;
};

AppState appState;

// Referências Globais para o Projeto
GerenciadorImagens gerImg;
GerenciadorJogos gerenciadorJogos;

/**
 * @brief Carrega os metadados dos jogos a partir de arquivos texto.
 */
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
}

int main(int argc, char* argv[]) {
    // 1. Inicialização SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    TTF_Init();

    // 2. Janela & Renderizador (Full Screen)
    SDL_Window* window = SDL_CreateWindow("DACC Station - Hub", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        LARGURA_TELA, ALTURA_TELA, 
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS);
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // 3. Inicialização de Dados e Serviços
    carregarDadosJogos();
    NetworkClient::getInstance().connectToManager();

    // 4. Criação da Janela Hub Principal
    // Refatorada para gerenciar a navegação entre todos os jogos
    JanelaJogo hubPrincipal(renderer);

    // 5. Loop Principal da Aplicação
    SDL_Event e;
    while (appState.running) {
        
        // --- A. TRATAMENTO DE EVENTOS ---
        while (SDL_PollEvent(&e)) {
            // Eventos do Sistema
            if (e.type == SDL_QUIT) {
                appState.running = false;
            }

            // Delegar eventos para o Hub Principal
            // Se tratarEvento retornar true, o Hub sinalizou saída (ESC)
            if (hubPrincipal.tratarEvento(e, 0, 0)) {
                appState.running = false;
            }
        }

        // --- B. ATUALIZAÇÃO E COMUNICAÇÃO ---
        NetworkClient::getInstance().checkEvents();
        SystemStatus::getInstance().update();

        // --- C. RENDERIZAÇÃO ---
        SDL_RenderClear(renderer);

        // O Hub desenha o fundo, informações do jogo selecionado, 
        // o carrossel de capas e os overlays do sistema (Top/Bottom Bar).
        hubPrincipal.desenhar(0, 0);

        SDL_RenderPresent(renderer);
        
        // Delay para controle de framerate (~60 FPS)
        SDL_Delay(16);
    }

    // 6. Limpeza de Recursos
    gerImg.liberarTudo();
    TTF_Quit();
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    cout << "[EXIT] DACC Station encerrada com sucesso." << endl;
    return 0;
}