//Compilar:  g++ main/main.cpp \
    implementacao/Principais/*.cpp \
    implementacao/Arquivos/*.cpp \
    implementacao/Gerenciadores/*.cpp \
    implementacao/GerenciadoresJogos/*.cpp \
    implementacao/Interface/Componentes/*.cpp \
    implementacao/Interface/Janelas/*.cpp \
    -I cabecalhos \
    -o DACCStation \
    -lSDL2 -lSDL2_image -lSDL2_gfx -lSDL2_ttf
//Executar: ./DACCStation


/**
 * @file main.cpp
 * @brief Ponto de entrada principal da aplicação de biblioteca de jogos (Launcher).
 * * Este arquivo coordena a integração de todos os sistemas: inicialização da SDL2,
 * carregamento da base de dados de jogos, gerenciamento de interface, processamento
 * de inputs (teclado, mouse e gamepad) e o loop de renderização principal.
 */

#include <iostream>
#include <vector>
#include <string>
#include <ctime>

#include "Principais/GerenciarSDL.hpp"
#include "Gerenciadores/GerenciarInputs.hpp"
#include "Interface/Janelas/GerenciarInterface.hpp"
#include "Interface/Janelas/GerenciarScroll.hpp"
#include "Interface/Janelas/JanelaConfiguracao.hpp" 
#include "GerenciadoresJogos/GerenciadorJogos.hpp"
#include "Gerenciadores/GerenciadorImagens.hpp"
#include "Arquivos/Arquivos.hpp"
#include "Gerenciadores/GerenciadorTemas.hpp"
#include "Principais/ConfigLayout.hpp"

using namespace std;
using namespace MeuProjeto;

/** * @brief Instância global do gerenciador de imagens para cache de texturas. 
 */
MeuProjeto::GerenciadorImagens gerImg;

/** * @brief Instância global que armazena a base de dados de todos os jogos carregados. 
 */
MeuProjeto::GerenciadorJogos gerenciadorJogos;

/**
 * @brief Definição da janela de jogo atual (Resolvendo o erro de múltiplas definições).
 */
std::unique_ptr<MeuProjeto::JanelaJogo> MeuProjeto::janelaJogoAtual = nullptr; 

/** @brief Largura padrão da janela principal da aplicação. */
const int LARGURA_JANELA =  ConfigLayout::LARGURA_NATIVA;

/** @brief Altura padrão da janela principal da aplicação. */
const int ALTURA_JANELA = ConfigLayout::ALTURA_NATIVA;

/**
 * @brief Carrega os dados dos jogos a partir de arquivos de texto no disco.
 * * Percorre uma lista pré-definida de caminhos de arquivos e utiliza a classe
 * Arquivos para desserializar os dados e popular o gerenciadorJogos.
 */
void carregarJogosDoDisco() {
    cout << "Carregando biblioteca de jogos..." << endl;
    vector<string> arquivos = {
        "arquivos/Jogos/Jogo1/jogo1.txt", "arquivos/Jogos/Jogo2/jogo2.txt", "arquivos/Jogos/Jogo3/jogo3.txt",
        "arquivos/Jogos/Jogo4/jogo4.txt", "arquivos/Jogos/Jogo5/jogo5.txt"
    };
    for (const auto& arquivo : arquivos) {
        Arquivos::carregarJogos(arquivo, gerenciadorJogos);
    }
    cout << "Total de jogos carregados: " << gerenciadorJogos.listarJogos().size() << endl;
}

/**
 * @brief Função principal (Entry Point).
 * * @param argc Quantidade de argumentos de linha de comando.
 * @param argv Vetor de argumentos de linha de comando.
 * @return int Status de saída do programa (0 para sucesso, 1 para erro).
 */
int main(int argc, char* argv[]) {
    // Inicializa semente para funções aleatórias
    srand(static_cast<unsigned int>(time(NULL)));

    SDL_Window* janela = nullptr;
    SDL_Renderer* renderer = nullptr;

    // 1. Inicialização do Hardware e SDL
    if (!GerenciarSDL::inicializar(janela, renderer, LARGURA_JANELA, ALTURA_JANELA)) {
        return 1;
    }

    // Verifica a presença de periféricos (Controles)
    if (!GerenciarSDL::verificarControle(janela, renderer)) {
        cout << "[AVISO] Nenhum controle detectado." << endl;
        cout << "[SISTEMA] Iniciando em modo Mouse/Teclado..." << endl;
    }

    // 2. Carregamento de Dados
    carregarJogosDoDisco();

    // 3. Inicialização dos Módulos de Interface e Input
    GerenciarScroll estado;           /**< Estado de navegação e scroll. */
    GerenciarInterface interface;     /**< Orquestrador visual. */
    
    // --- VERIFICAÇÃO ADICIONADA AQUI ---
    if (!interface.inicializar(renderer)) {
        cerr << "Erro Fatal: Falha ao carregar imagens da interface. Encerrando sistema." << endl;
        GerenciarSDL::limpar(janela, renderer);
        return 1;
    }
    
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
    cout << "SISTEMA INICIADO!" << endl;

    // 4. Loop Principal (Game Loop)
    while (estado.rodando) {
        
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
                    MeuProjeto::janelaJogoAtual.reset();
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
    if (estado.janelaJogoAtual) MeuProjeto::janelaJogoAtual.reset();;
    GerenciarSDL::limpar(janela, renderer);
    
    return 0;
}