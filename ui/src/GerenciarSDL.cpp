/**
 * @file GerenciarSDL.cpp
 * @brief Implementação da classe GerenciarSDL para controle de subsistemas e hardware.
 * 
 * Este arquivo contém a lógica de inicialização da biblioteca SDL2 e suas extensões
 * (Image e TTF), além de gerenciar a criação da janela principal e a verificação
 * de integridade dos periféricos de entrada.
 */

#include "GerenciarSDL.hpp"
#include "Utils.hpp" 
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Inicializa os subsistemas da SDL2, extensões e cria a infraestrutura de vídeo.
 * 
 * O processo segue as seguintes etapas:
 * 1. Inicializa o núcleo da SDL com suporte a Vídeo e GameController.
 * 2. Inicializa o suporte a imagens PNG via SDL_image.
 * 3. Inicializa o suporte a fontes TrueType via SDL_ttf.
 * 4. Habilita o sistema de eventos de controle.
 * 5. Cria uma janela centralizada, sem bordas e em modo tela cheia desktop.
 * 6. Cria um renderizador acelerado por hardware com VSync ativado para fluidez visual.
 * 
 * @param janela Referência de saída para o ponteiro da janela criada.
 * @param renderer Referência de saída para o ponteiro do renderizador criado.
 * @param largura Largura base da janela.
 * @param altura Altura base da janela.
 * @return true se todas as etapas de inicialização foram bem-sucedidas.
 * @return false se ocorreu um erro fatal em qualquer subsistema.
 */
bool GerenciarSDL::inicializar(SDL_Window*& janela, SDL_Renderer*& renderer, int largura, int altura) {
    cout << "INICIALIZANDO SDL..." << endl;

    // Define a qualidade da escala:
    // "0" = Nearest (Pixelado, bom para pixel art)
    // "1" = Linear (Suave, bom para imagens HD/Fotos)
    // "2" = Best (Anisotrópica, melhor qualidade possível)

    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2")) {
        cerr << "[AVISO] Não foi possível definir a qualidade de escala linear." << endl;
    }
    
    // 1. Inicializar SDL Geral e Controle
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        cerr << "[ERRO FATAL] Falha ao inicializar SDL: " << SDL_GetError() << endl;
        return false;
    }
    cout << "[OK] SDL inicializado com sucesso" << endl;
    
    // 2. Inicializar Imagens (PNG)
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        cerr << "[ERRO FATAL] Falha ao inicializar SDL_image: " << IMG_GetError() << endl;
        SDL_Quit();
        return false;
    }
    cout << "[OK] SDL_image inicializado" << endl;
    
    // 3. Inicializar Fontes (TTF)
    if (TTF_Init() == -1) {
        cerr << "[AVISO] SDL_ttf não pôde ser inicializado: " << TTF_GetError() << endl;
        // Não é fatal, mas textos não funcionarão corretamente se as funções de desenho forem chamadas
    } else {
        cout << "[OK] SDL_ttf inicializado" << endl;
    }
    
    // Prepara o sistema para captura de eventos de entrada de texto (útil para busca)
    SDL_StartTextInput();

    // 4. Habilitar eventos de controle
    SDL_GameControllerEventState(SDL_ENABLE);
    cout << "[OK] Eventos de GameController habilitados" << endl;

    // 5. Criar Janela
    cout << "\nCriando janela..." << endl;
    // Configurada como BORDERLESS e FULLSCREEN_DESKTOP para imersão total
    janela = SDL_CreateWindow("Gerenciador de Jogos",
                             SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED,
                             largura,
                             altura,
                             SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS);
    if (!janela) {
        cerr << "[ERRO FATAL] Erro ao criar janela: " << SDL_GetError() << endl;
        return false;
    }
    cout << "[OK] Janela criada" << endl;
    
    // 6. Criar Renderizador
    // Utiliza ACCELERATED para usar a GPU e PRESENTVSYNC para evitar Screen Tearing
    renderer = SDL_CreateRenderer(janela, -1,
                                 SDL_RENDERER_ACCELERATED |
                                 SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        cerr << "[ERRO FATAL] Erro ao criar renderizador: " << SDL_GetError() << endl;
        return false;
    }
    cout << "[OK] Renderizador criado" << endl;
    
    return true;
}

/**
 * @brief Realiza a verificação de conectividade de joysticks e fornece feedback visual.
 * 
 * Se nenhum controle for detectado, o sistema exibe uma tela de erro vermelha por 3 segundos.
 * Se um controle for detectado mas não for compatível com o mapeamento GameController,
 * a inicialização prossegue com um aviso. Caso um controle válido seja encontrado,
 * exibe uma mensagem de sucesso verde.
 * 
 * @param janela Ponteiro para a janela para obter dimensões de desenho.
 * @param renderer Ponteiro para o renderizador onde as mensagens serão desenhadas.
 * @return true se pelo menos um controle compatível foi encontrado.
 * @return false se não há joysticks ou se são incompatíveis.
 */
bool GerenciarSDL::verificarControle(SDL_Window* janela, SDL_Renderer* renderer) {
    cout << "VERIFICANDO CONTROLE..." << endl;
    
    // Verifica conexão física imediata
    int numJoysticks = SDL_NumJoysticks();
    cout << "Joysticks detectados: " << numJoysticks << endl;
    
    // Caso de erro: Nenhum dispositivo de entrada de jogo detectado
    if (numJoysticks < 1) {
        cerr << "[ERRO] Nenhum controle conectado!" << endl;
        
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
        SDL_Delay(3000); // Bloqueia por 3 segundos para leitura do usuário
        
        return false;
    }
    
    // Tenta validar a compatibilidade com a API GameController
    bool controleValido = false;
    SDL_GameController* tempController = nullptr;
    
    for (int i = 0; i < numJoysticks; i++) {
        if (SDL_IsGameController(i)) {
            tempController = SDL_GameControllerOpen(i);
            if (tempController) {
                cout << "[OK] Controle compatível encontrado: " << SDL_GameControllerName(tempController) << endl;
                controleValido = true;
                // Fecha a instância temporária; o gerenciamento definitivo será feito no GerenciarInputs
                SDL_GameControllerClose(tempController); 
                break;
            }
        }
    }
    
    if (!controleValido) {
        cerr << "[ERRO] Controle não compatível!" << endl;
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
    SDL_Delay(1000); // Aguarda um breve momento antes de entrar na interface principal
    
    return true;
}

/**
 * @brief Encerra todos os subsistemas abertos e libera a memória de vídeo.
 * 
 * Deve ser chamado no encerramento da aplicação para garantir que a janela
 * e o renderizador sejam destruídos antes do fechamento das bibliotecas (TTF, IMG, SDL).
 * 
 * @param janela Ponteiro para a janela a ser destruída.
 * @param renderer Ponteiro para o renderizador a ser destruído.
 */
void GerenciarSDL::limpar(SDL_Window* janela, SDL_Renderer* renderer) {
    cout << "ENCERRANDO SISTEMA..." << endl;

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        cout << "Renderizador destruído." << endl;
    }
    if (janela) {
        SDL_DestroyWindow(janela);
        cout << "Janela destruída." << endl;
    }

    // Encerramento das extensões de forma segura
    if (TTF_WasInit()) TTF_Quit();
    IMG_Quit();
    
    // Finalização do sistema de input de texto e núcleo SDL
    SDL_StopTextInput();
    SDL_Quit();
    
    cout << "SISTEMA ENCERRADO COM SUCESSO!" << endl;
}