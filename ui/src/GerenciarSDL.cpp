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
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <cstdlib> 
#include <unistd.h>
#include <limits.h> 
#include <stdlib.h> 
#include <unistd.h>

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

    /**
     * Define a qualidade da escala de renderização:
     * - "0" = Nearest (Pixelado, ideal para pixel art e sprites de baixa resolução)
     * - "1" = Linear (Suavização linear, adequado para imagens HD e fotografias)
     * - "2" = Best (Filtragem anisotrópica, máxima qualidade visual possível)
     * 
     * O sistema tenta usar a melhor qualidade disponível, com fallback automático
     * para qualidades inferiores caso o hardware não suporte.
     */
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2")) {
        std::cerr << "[AVISO] Qualidade '2' não suportada, tentando '1'..." << std::endl;
            if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
                std::cerr << "[AVISO] Usando qualidade padrão (nearest neighbor)." << std::endl;
            }
    }
    
    /**
     * Inicializa o subsistema de vídeo da SDL e o suporte a Game Controllers.
     * SDL_INIT_VIDEO: Habilita funcionalidades de janela, renderização e display.
     * SDL_INIT_GAMECONTROLLER: Ativa o mapeamento padronizado de controles de jogo.
     * Retorna 0 em caso de sucesso, valor negativo em caso de falha.
     */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        cerr << "[ERRO FATAL] Falha ao inicializar SDL: " << SDL_GetError() << endl;
        return false;
    }
    cout << "[OK] SDL inicializado com sucesso" << endl;
    
    /**
     * Inicializa a biblioteca SDL_image com suporte a arquivos PNG.
     * A verificação bitwise garante que especificamente o formato PNG foi habilitado.
     * Caso falhe, o sistema é encerrado pois imagens são essenciais para a interface.
     */
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        cerr << "[ERRO FATAL] Falha ao inicializar SDL_image: " << IMG_GetError() << endl;
        SDL_Quit();
        return false;
    }
    cout << "[OK] SDL_image inicializado" << endl;
    
    /**
     * Inicializa a biblioteca SDL_ttf para renderização de fontes TrueType.
     * A falha não é considerada fatal pois o sistema pode funcionar sem texto,
     * porém funcionalidades de interface que dependem de texto ficarão inoperantes.
     */
    if (TTF_Init() == -1) {
        cerr << "[AVISO] SDL_ttf não pôde ser inicializado: " << TTF_GetError() << endl;
        // Não é fatal, mas textos não funcionarão corretamente se as funções de desenho forem chamadas
    } else {
        cout << "[OK] SDL_ttf inicializado" << endl;
    }

    /**
     * Abre o dispositivo de áudio com SDL_mixer para reprodução de sons e música.
     * Parâmetros:
     * - 44100: Frequência de amostragem em Hz (qualidade CD)
     * - MIX_DEFAULT_FORMAT: Formato de áudio nativo da plataforma
     * - 2: Número de canais (Stereo)
     * - 2048: Tamanho do buffer em bytes (afeta latência)
     */
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        cout << "Erro ao inicializar SDL_mixer (Audio): " << Mix_GetError() << endl;
        return false;
    }

    /**
     * Aloca 16 canais de mixagem simultânea para efeitos sonoros.
     * Permite reproduzir até 16 sons diferentes ao mesmo tempo.
     */
    Mix_AllocateChannels(16);
    
    /**
     * Habilita a captura de entrada de texto do usuário.
     * Essencial para funcionalidades de busca e campos de entrada de texto.
     */
    SDL_StartTextInput();

    /**
     * Ativa o processamento de eventos de Game Controller.
     * Garante que eventos de botões, analógicos e gatilhos sejam capturados.
     */
    SDL_GameControllerEventState(SDL_ENABLE);
    cout << "[OK] Eventos de GameController habilitados" << endl;

    /**
     * Cria a janela principal da aplicação com as seguintes características:
     * - SDL_WINDOW_SHOWN: Janela visível imediatamente após criação
     * - SDL_WINDOW_FULLSCREEN_DESKTOP: Tela cheia usando resolução nativa do desktop
     * - SDL_WINDOW_BORDERLESS: Remove bordas e barra de título para imersão total
     * 
     * A janela é posicionada centralmente usando SDL_WINDOWPOS_CENTERED.
     */
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
    
    /**
     * Cria o renderizador associado à janela com aceleração por hardware:
     * - SDL_RENDERER_ACCELERATED: Usa GPU para renderização (melhor performance)
     * - SDL_RENDERER_PRESENTVSYNC: Sincroniza com taxa de atualização do monitor
     *   (elimina screen tearing e garante fluidez visual de 60fps ou mais)
     * 
     * O índice -1 seleciona automaticamente o driver de renderização mais adequado.
     */
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
 * @brief Reproduz um vídeo de introdução em tela cheia usando o reprodutor MPV externo.
 * 
 * Esta função procura o arquivo de vídeo no caminho especificado e, se não encontrado,
 * tenta no diretório pai. Ao localizar o arquivo, converte para caminho absoluto
 * e executa o MPV com configurações otimizadas para tela cheia sem controles visíveis.
 * 
 * @param caminhoVideo Caminho relativo ou absoluto do arquivo de vídeo (.mp4, .mkv, etc).
 * 
 * @note O MPV deve estar instalado no sistema para esta funcionalidade operar.
 * @note A função bloqueia a execução até que o vídeo termine ou seja fechado pelo usuário.
 * 
 * Configurações do MPV utilizadas:
 * - --fs: Modo tela cheia
 * - --ontop: Janela sempre acima de outras aplicações
 * - --no-border: Remove bordas da janela
 * - --no-osc: Desabilita controles na tela
 * - --no-osd-bar: Remove barra de progresso
 * - --no-input-cursor: Oculta cursor do mouse
 */
void GerenciarSDL::tocarVideoIntro(const std::string& caminhoVideo) {
    std::string caminhoFinal = caminhoVideo;
    bool encontrado = false;

    /**
     * Verifica se o arquivo existe no caminho fornecido usando access().
     * F_OK testa apenas a existência do arquivo, não permissões.
     */
    if (access(caminhoFinal.c_str(), F_OK) != -1) {
        encontrado = true;
    } 
    else {
        /**
         * Caso não encontre, tenta localizar no diretório pai (../).
         * Útil quando o executável está em subpasta como bin/ ou build/.
         */
        std::string tentativa = "../" + caminhoVideo;
        if (access(tentativa.c_str(), F_OK) != -1) {
            caminhoFinal = tentativa;
            encontrado = true;
        }
    }

    if (encontrado) {
        /**
         * Converte o caminho relativo para caminho absoluto usando realpath().
         * Isso evita problemas com o MPV ao interpretar caminhos relativos,
         * especialmente quando executado via system() que pode ter working directory diferente.
         * 
         * PATH_MAX define o tamanho máximo do buffer de caminho (geralmente 4096 bytes).
         */
        char absolutePath[PATH_MAX];
        if (realpath(caminhoFinal.c_str(), absolutePath)) {
            caminhoFinal = std::string(absolutePath);
        }

        std::cout << "[INTRO] Video encontrado! Reproduzindo: " << caminhoFinal << std::endl;
        
        /**
         * Monta o comando MPV com parâmetros otimizados:
         * --fs: Tela Cheia
         * --ontop: Ficar por cima de outras janelas
         * --no-border: Sem bordas de janela
         * --no-osc: Desabilita controles visuais na tela
         * --no-osd-bar: Remove barra de progresso
         * --no-input-cursor: Esconde o cursor do mouse
         * 
         * O caminho é envolto em aspas duplas para suportar nomes com espaços.
         */
        std::string comando = "mpv --fs --ontop --no-border --no-osc --no-osd-bar --no-input-cursor \"" + caminhoFinal + "\"";
        
        /**
         * Executa o comando usando system().
         * Bloqueia até que o MPV seja fechado.
         * Retorna 0 se MPV executou normalmente, valor diferente em caso de erro.
         */
        int resultado = system(comando.c_str());
        
        if (resultado != 0) {
            std::cerr << "[ERRO] MPV falhou ou foi fechado com erro. Codigo: " << resultado << std::endl;
        }
    } else {
        /**
         * Exibe mensagem de erro detalhada caso o vídeo não seja encontrado.
         * Fornece os caminhos testados para facilitar debugging.
         */
        std::cerr << "==================================================" << std::endl;
        std::cerr << "[ERRO CRITICO] Video de Intro NAO ENCONTRADO!" << std::endl;
        std::cerr << "Procurado em: " << caminhoVideo << std::endl;
        std::cerr << "E tambem em: ../" << caminhoVideo << std::endl;
        std::cerr << "Verifique se o arquivo .mp4 esta na pasta correta." << std::endl;
        std::cerr << "==================================================" << std::endl;
    }
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
    
    /**
     * SDL_NumJoysticks() retorna o número total de dispositivos de entrada do tipo
     * joystick conectados ao sistema, independente de serem compatíveis com GameController.
     */
    int numJoysticks = SDL_NumJoysticks();
    cout << "Joysticks detectados: " << numJoysticks << endl;
    
    /**
     * Caso de erro crítico: Nenhum dispositivo de entrada detectado.
     * Exibe feedback visual vermelho na tela por 3 segundos antes de retornar falha.
     */
    if (numJoysticks < 1) {
        cerr << "[ERRO] Nenhum controle conectado!" << endl;
        
        /// Define cor de fundo escura (RGB: 30, 30, 30)
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        
        /// Obtém as dimensões atuais da janela para centralizar o texto
        int w, h;
        SDL_GetWindowSize(janela, &w, &h);
        
        /**
         * Desenha mensagem de erro em vermelho vibrante (255, 50, 50).
         * Posicionamento centralizado com deslocamento calculado baseado na largura estimada do texto.
         */
        desenharTexto(renderer, "ERRO: Nenhum controle conectado!", 
                     w/2 - 400, h/2 - 50, 
                     {255, 50, 50, 255}, 36, TipoFonte::NEGRITO);
        
        /// Mensagem secundária em cinza claro com instruções ao usuário
        desenharTexto(renderer, "Conecte um controle e reinicie o programa", 
                     w/2 - 380, h/2 + 20, 
                     {200, 200, 200, 255}, 28, TipoFonte::NORMAL);
        
        /// Apresenta o conteúdo renderizado na tela
        SDL_RenderPresent(renderer);
        
        /// Bloqueia por 3 segundos para permitir leitura da mensagem pelo usuário
        SDL_Delay(3000);
        
        return false;
    }
    
    /**
     * Itera sobre todos os joysticks detectados para verificar compatibilidade
     * com a API GameController (mapeamento padronizado de botões).
     */
    bool controleValido = false;
    SDL_GameController* tempController = nullptr;
    
    for (int i = 0; i < numJoysticks; i++) {
        /**
         * SDL_IsGameController() verifica se o joystick no índice i possui
         * um mapeamento conhecido de botões (Xbox, PlayStation, etc).
         */
        if (SDL_IsGameController(i)) {
            /// Tenta abrir o controle para validar funcionalidade completa
            tempController = SDL_GameControllerOpen(i);
            if (tempController) {
                cout << "[OK] Controle compatível encontrado: " << SDL_GameControllerName(tempController) << endl;
                controleValido = true;
                
                /**
                 * Fecha a instância temporária do controle.
                 * O gerenciamento definitivo será feito pela classe GerenciarInputs
                 * para evitar múltiplas instâncias abertas simultaneamente.
                 */
                SDL_GameControllerClose(tempController); 
                break;
            }
        }
    }
    
    /**
     * Se nenhum controle compatível foi encontrado após iterar todos os dispositivos,
     * retorna falha sem feedback visual adicional.
     */
    if (!controleValido) {
        cerr << "[ERRO] Controle não compatível!" << endl;
        return false;
    }
    
    /**
     * Feedback visual de sucesso em verde (50, 255, 50).
     * Informa ao usuário que o sistema detectou e validou o controle corretamente.
     */
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);
    
    /// Obtém dimensões da janela para centralização
    int w, h;
    SDL_GetWindowSize(janela, &w, &h);
    
    /// Mensagem principal em verde brilhante com fonte em negrito
    desenharTexto(renderer, "CONTROLE CONECTADO!", 
                 w/2 - 280, h/2 - 80, 
                 {50, 255, 50, 255}, 48, TipoFonte::NEGRITO);
    
    /// Mensagem secundária em cinza claro informando início iminente
    desenharTexto(renderer, "Iniciando em 1 segundo...", 
                 w/2 - 220, h/2 + 60, 
                 {150, 150, 150, 255}, 28, TipoFonte::NORMAL);
                 
    SDL_RenderPresent(renderer);
    
    /// Aguarda 1 segundo antes de prosseguir para a interface principal
    SDL_Delay(1000);
    
    return true;
}

/**
 * @brief Encerra todos os subsistemas abertos e libera a memória de vídeo.
 * 
 * Deve ser chamado no encerramento da aplicação para garantir que a janela
 * e o renderizador sejam destruídos antes do fechamento das bibliotecas (TTF, IMG, SDL).
 * 
 * A ordem de destruição é crítica para evitar vazamentos de memória e crashes:
 * 1. Fecha dispositivo de áudio (Mix_CloseAudio)
 * 2. Destrói renderizador (libera buffers de GPU)
 * 3. Destrói janela (libera recursos do sistema operacional)
 * 4. Finaliza bibliotecas de extensão (TTF, IMG)
 * 5. Para captura de texto
 * 6. Finaliza núcleo SDL
 * 
 * @param janela Ponteiro para a janela a ser destruída.
 * @param renderer Ponteiro para o renderizador a ser destruído.
 */
void GerenciarSDL::limpar(SDL_Window* janela, SDL_Renderer* renderer) {
    cout << "ENCERRANDO SISTEMA..." << endl;

    /**
     * Fecha o dispositivo de áudio e libera recursos alocados pelo SDL_mixer.
     * Mix_CloseAudio() encerra streams ativos e libera buffers de mixagem.
     * Mix_Quit() finaliza a biblioteca completamente.
     */
    Mix_CloseAudio();
    Mix_Quit();

    /**
     * Destrói o renderizador e libera todos os recursos de GPU associados.
     * Inclui texturas, buffers de vértices e estados de renderização.
     */
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        cout << "Renderizador destruído." << endl;
    }
    
    /**
     * Destrói a janela e libera recursos do sistema operacional.
     * Fecha o contexto gráfico e libera memória alocada para o frame buffer.
     */
    if (janela) {
        SDL_DestroyWindow(janela);
        cout << "Janela destruída." << endl;
    }

    /**
     * Verifica se SDL_ttf foi inicializado antes de tentar finalizá-lo.
     * TTF_WasInit() retorna true se a biblioteca está ativa.
     * Previne chamadas duplas de TTF_Quit() que causariam erro.
     */
    if (TTF_WasInit()) TTF_Quit();
    
    /// Finaliza SDL_image e libera recursos de decodificação de imagens
    IMG_Quit();
    
    /**
     * Para o sistema de captura de entrada de texto.
     * Desabilita eventos de composição de caracteres e IME.
     */
    SDL_StopTextInput();
    
    /**
     * Finaliza completamente a SDL e todos os subsistemas inicializados.
     * Deve ser a última chamada relacionada à SDL no programa.
     */
    SDL_Quit();
    
    cout << "SISTEMA ENCERRADO COM SUCESSO!" << endl;
}