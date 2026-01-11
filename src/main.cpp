//Compilar: g++ implementacao/*.cpp mains/main.cpp -I cabecalhos -o DACCStation -lSDL2 -lSDL2_image -lSDL2_gfx -lSDL2_ttf
//Executar: ./DACCStation


/**
 * @file main.cpp
 * @brief Ponto de entrada principal da aplicação de biblioteca de jogos (Launcher).
 * 
 * Este arquivo coordena a integração de todos os sistemas: inicialização da SDL2,
 * carregamento da base de dados de jogos, gerenciamento de interface, processamento
 * de inputs (teclado, mouse e gamepad) e o loop de renderização principal.
 */

#include <iostream>
#include <vector>
#include <string>
#include <ctime>

#include "GerenciarSDL.hpp"
#include "GerenciarInputs.hpp"
#include "GerenciarInterface.hpp"
#include "GerenciarScroll.hpp"
#include "JanelaConfiguracao.hpp" 
#include "GerenciadorJogos.hpp"
#include "GerenciadorImagens.hpp"
#include "Arquivos.hpp"
#include "GerenciadorTemas.hpp"
#include "LogManager.hpp"
#include "NetworkClient.hpp"

using namespace std;
using namespace MeuProjeto;

/** 
 * @brief Instância global do gerenciador de imagens para cache de texturas. 
 */
MeuProjeto::GerenciadorImagens gerImg;

/** 
 * @brief Instância global que armazena a base de dados de todos os jogos carregados. 
 */
MeuProjeto::GerenciadorJogos gerenciadorJogos;

/** @brief Largura padrão da janela principal da aplicação. */
const int LARGURA_JANELA = 1920;

/** @brief Altura padrão da janela principal da aplicação. */
const int ALTURA_JANELA = 1280;

/**
 * @brief Carrega os dados dos jogos a partir de arquivos de texto no disco.
 * 
 * Percorre uma lista pré-definida de caminhos de arquivos e utiliza a classe
 * Arquivos para desserializar os dados e popular o gerenciadorJogos.
 */
void carregarJogosDoDisco() {
    LOG_INFO("UI: Carregando biblioteca de jogos...");
    vector<string> arquivos = {
        "assets/data/jogo1.txt", "assets/data/jogo2.txt", "assets/data/jogo3.txt",
        "assets/data/jogo4.txt", "assets/data/jogo5.txt"
    };
    for (const auto& arquivo : arquivos) {
        Arquivos::carregarJogos(arquivo, gerenciadorJogos);
    }
    LOG_INFO("UI: Total de jogos carregados: " + to_string(gerenciadorJogos.listarJogos().size()));
}

/**
 * @brief Função principal (Entry Point).
 * 
 * @param argc Quantidade de argumentos de linha de comando.
 * @param argv Vetor de argumentos de linha de comando.
 * @return int Status de saída do programa (0 para sucesso, 1 para erro).
 */
int main(int argc, char* argv[]) {
    // Inicializa Logging
    LogManager::getInstance().initialize("UI", "/tmp/dacc-station.sock");
    LOG_INFO("UI: Inicializando DACC Station UI...");

    // Tenta conectar ao Process Manager
    if (NetworkClient::getInstance().connectToManager()) {
        LOG_INFO("UI: Conexão com PM estabelecida.");
    } else {
        LOG_WARNING("UI: Não foi possível conectar ao PM. A UI funcionará offline.");
    }

    // Inicializa semente para funções aleatórias
    srand(static_cast<unsigned int>(time(NULL)));

    SDL_Window* janela = nullptr;
    SDL_Renderer* renderer = nullptr;

    // 1. Inicialização do Hardware e SDL
    if (!GerenciarSDL::inicializar(janela, renderer, LARGURA_JANELA, ALTURA_JANELA)) {
        LOG_ERROR("UI: Falha crítica no SDL.");
        return 1;
    }

    // Verifica a presença de periféricos (Controles)
    if (!GerenciarSDL::verificarControle(janela, renderer)) {
        LOG_WARNING("UI: Nenhum controle detectado. Modo Mouse/Teclado.");
    }

    // 2. Carregamento de Dados
    carregarJogosDoDisco();

    // 3. Inicialização dos Módulos de Interface e Input
    GerenciarScroll estado;           /**< Estado de navegação e scroll. */
    GerenciarInterface interface;     /**< Orquestrador visual. */
    interface.inicializar(renderer);
    
    GerenciarInputs inputs;           /**< Processador de periféricos. */
    inputs.inicializarControle();
    
    JanelaConfiguracao janelaConfig; /**< Janela secundária de configurações. */

    // Configuração inicial do carrossel de destaques (Hero Banners)
    estado.imagensDestaques.clear();
    vector<Jogo> todosJogos = gerenciadorJogos.listarJogos();
    for (int i = 0; i < std::min(5, (int)todosJogos.size()); i++) {
        // Popula os destaques com a imagem de fundo dos primeiros 5 jogos
        estado.imagensDestaques.push_back(todosJogos[i].getFundoDestaque());
    }
    
    Uint32 lastBackgroundChange = SDL_GetTicks();
    const Uint32 CHANGE_INTERVAL = 60000; /**< Intervalo de 60 segundos para trocar o fundo. */

    SDL_Event evento;
    LOG_INFO("UI: SISTEMA INICIADO! Entrando no loop principal.");

    // 4. Loop Principal (Game Loop)
    while (estado.rodando) {
        
        // Verifica mensagens do Backend (Ex: Jogo fechou)
        NetworkClient::getInstance().checkEvents();

        // Gerenciamento automático da janela de configuração
        if (estado.configAberta && !janelaConfig.estaAberta()) janelaConfig.abrir();
        else if (!estado.configAberta && janelaConfig.estaAberta()) janelaConfig.fechar();
        else if (!janelaConfig.estaAberta() && estado.configAberta) estado.configAberta = false; 

        // 4.1 Processamento de Eventos (Event Polling)
        while (SDL_PollEvent(&evento)) {
            
            // Prioridade 1: Eventos da Janela de Configuração (Modo Modal)
            if (janelaConfig.estaAberta()) {
                if (janelaConfig.processarEvento(evento)) {
                    if (!janelaConfig.estaAberta()) estado.configAberta = false;
                    continue; 
                }
            }

            // Evento de fechamento total
            if (evento.type == SDL_QUIT) estado.rodando = false;

            // Prioridade 2: Eventos da Janela de Detalhes do Jogo
            if (estado.janelaJogoAtual) {
                bool fecharJanela = false;
                if (evento.type == SDL_CONTROLLERBUTTONDOWN || evento.type == SDL_CONTROLLERBUTTONUP) {
                    if (estado.janelaJogoAtual->tratarEventoControle(evento)) fecharJanela = true;
                } else {
                    if (estado.janelaJogoAtual->tratarEvento(evento, 0, 0)) fecharJanela = true;
                }

                if (fecharJanela) {
                    delete estado.janelaJogoAtual;
                    estado.janelaJogoAtual = nullptr;
                }
                continue; // Bloqueia inputs da interface principal enquanto a janela do jogo está aberta
            }

            // Prioridade 3: Inputs da Interface Principal (Navegação/Grid)
            if (!janelaConfig.estaAberta()) {
                inputs.atualizar(evento, estado, interface, renderer);
            }
        }

        // 4.2 Lógica de Atualização (Update)
        // Rotação automática das imagens de destaque no fundo
        if (!estado.imagensDestaques.empty() && !estado.janelaJogoAtual && !estado.configAberta) {
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime - lastBackgroundChange >= CHANGE_INTERVAL) {
                estado.currentDestaqueIndex = (estado.currentDestaqueIndex + 1) % estado.imagensDestaques.size();
                lastBackgroundChange = currentTime;
            }
        }

        // 4.3 Renderização (Draw)
        // Define a cor de fundo com base no tema selecionado
        SDL_Color corFundo = GerenciadorTemas::getInstance().getCorFundo();
        SDL_SetRenderDrawColor(renderer, corFundo.r, corFundo.g, corFundo.b, corFundo.a);
        SDL_RenderClear(renderer);
        
        // Desenha os elementos da interface (Grid, Categorias, Busca)
        interface.desenhar(renderer, estado);
        
        // Se houver uma janela de detalhes de jogo aberta, desenha por cima (Overlay)
        if (estado.janelaJogoAtual) {
            estado.janelaJogoAtual->desenhar(0, 0);
        }

        // Atualiza a tela principal
        SDL_RenderPresent(renderer);

        // Renderização da Janela de Configuração (se estiver aberta)
        if (janelaConfig.estaAberta()) {
            janelaConfig.desenhar();
        }
        
        // Controle de taxa de quadros (aprox. 60 FPS)
        SDL_Delay(16);
    }

    // 5. Finalização e Limpeza de Recursos
    inputs.fecharControle();
    janelaConfig.fechar();
    if (estado.janelaJogoAtual) delete estado.janelaJogoAtual;
    GerenciarSDL::limpar(janela, renderer);
    
    return 0;
}