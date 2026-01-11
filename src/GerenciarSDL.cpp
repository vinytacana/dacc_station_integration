/**
 * @file GerenciarSDL.cpp
 * @brief Implementação da classe GerenciarSDL para controle de subsistemas e hardware.
 */

#include "GerenciarSDL.hpp"
#include "Utils.hpp" // Para desenharTexto
#include "LogManager.hpp" // Integração com o sistema de Logs
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

using namespace MeuProjeto;
using namespace std;

bool GerenciarSDL::inicializar(SDL_Window*& janela, SDL_Renderer*& renderer, int largura, int altura) {
    LOG_INFO("UI: INICIALIZANDO SDL...");
    
    // 1. Inicializar SDL Geral e Controle
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        LOG_ERROR("UI: [FATAL] Falha ao inicializar SDL: " + string(SDL_GetError()));
        return false;
    }
    LOG_INFO("UI: SDL inicializado com sucesso");
    
    // 2. Inicializar Imagens (PNG)
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        LOG_ERROR("UI: [FATAL] Falha ao inicializar SDL_image: " + string(IMG_GetError()));
        SDL_Quit();
        return false;
    }
    LOG_INFO("UI: SDL_image inicializado");
    
    // 3. Inicializar Fontes (TTF)
    if (TTF_Init() == -1) {
        LOG_WARNING("UI: SDL_ttf não pôde ser inicializado: " + string(TTF_GetError()));
    } else {
        LOG_INFO("UI: SDL_ttf inicializado");
    }
    
    // Prepara o sistema para captura de eventos de entrada de texto
    SDL_StartTextInput();

    // 4. Habilitar eventos de controle
    SDL_GameControllerEventState(SDL_ENABLE);
    LOG_INFO("UI: Eventos de GameController habilitados");

    // 5. Criar Janela
    LOG_INFO("UI: Criando janela...");
    janela = SDL_CreateWindow("Gerenciador de Jogos",
                             SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED,
                             largura,
                             altura,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS);
    if (!janela) {
        LOG_ERROR("UI: [FATAL] Erro ao criar janela: " + string(SDL_GetError()));
        return false;
    }
    LOG_INFO("UI: Janela criada");
    
    // 6. Criar Renderizador
    renderer = SDL_CreateRenderer(janela, -1,
                                 SDL_RENDERER_ACCELERATED |
                                 SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        LOG_ERROR("UI: [FATAL] Erro ao criar renderizador: " + string(SDL_GetError()));
        return false;
    }
    LOG_INFO("UI: Renderizador criado");
    
    return true;
}

bool GerenciarSDL::verificarControle(SDL_Window* janela, SDL_Renderer* renderer) {
    LOG_INFO("UI: VERIFICANDO CONTROLE...");
    
    // Verifica conexão física imediata
    int numJoysticks = SDL_NumJoysticks();
    LOG_INFO("UI: Joysticks detectados: " + to_string(numJoysticks));
    
    // Caso de erro: Nenhum dispositivo de entrada de jogo detectado
    if (numJoysticks < 1) {
        LOG_WARNING("UI: Nenhum controle conectado!");
        
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        
        int w, h;
        SDL_GetWindowSize(janela, &w, &h);
        
        // Desenha feedback visual de erro na tela
        desenharTexto(renderer, "ERRO: Nenhum controle conectado!", 
                     w/2 - 400, h/2 - 50, 
                     {255, 50, 50, 255}, 36, TipoFonte::NEGRITO);
        desenharTexto(renderer, "Conecte um controle e reinicie o programa", 
                     w/2 - 380, h/2 + 20, 
                     {200, 200, 200, 255}, 28, TipoFonte::NORMAL);
        
        SDL_RenderPresent(renderer);
        SDL_Delay(3000); 
        
        return false;
    }
    
    // Tenta validar a compatibilidade com a API GameController
    bool controleValido = false;
    SDL_GameController* tempController = nullptr;
    
    for (int i = 0; i < numJoysticks; i++) {
        if (SDL_IsGameController(i)) {
            tempController = SDL_GameControllerOpen(i);
            if (tempController) {
                LOG_INFO("UI: Controle compatível encontrado: " + string(SDL_GameControllerName(tempController)));
                controleValido = true;
                // Fecha a instância temporária
                SDL_GameControllerClose(tempController); 
                break;
            }
        }
    }
    
    if (!controleValido) {
        LOG_ERROR("UI: Controle não compatível!");
        return false;
    }
    
    // Feedback visual de sucesso
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);
    
    int w, h;
    SDL_GetWindowSize(janela, &w, &h);
    
    desenharTexto(renderer, "CONTROLE CONECTADO!", 
                 w/2 - 280, h/2 - 80, 
                 {50, 255, 50, 255}, 48, TipoFonte::NEGRITO);
    desenharTexto(renderer, "Iniciando em 1 segundo...", 
                 w/2 - 220, h/2 + 60, 
                 {150, 150, 150, 255}, 28, TipoFonte::NORMAL);
                 
    SDL_RenderPresent(renderer);
    SDL_Delay(1000); 
    
    return true;
}

void GerenciarSDL::limpar(SDL_Window* janela, SDL_Renderer* renderer) {
    LOG_INFO("UI: ENCERRANDO SISTEMA...");

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        LOG_INFO("UI: Renderizador destruído.");
    }
    if (janela) {
        SDL_DestroyWindow(janela);
        LOG_INFO("UI: Janela destruída.");
    }

    // Encerramento das extensões de forma segura
    if (TTF_WasInit()) TTF_Quit();
    IMG_Quit();
    
    // Finalização do sistema de input de texto e núcleo SDL
    SDL_StopTextInput();
    SDL_Quit();
    
    LOG_INFO("UI: SISTEMA ENCERRADO COM SUCESSO!");
}
