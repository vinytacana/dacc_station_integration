// Compilar: //g++ implementacao/*.cpp mains/main.cpp -I cabecalhos -o gerenciador -lSDL2 -lSDL2_image -lSDL2_gfx -lSDL2_ttf
// Executar //./gerenciador

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream> 
#include <cstdlib> 
#include <ctime>   

#include "Botao.hpp"
#include "Forma.hpp"
#include "GerenciadorImagens.hpp"
#include "Janela.hpp"
#include "Utils.hpp"
#include "JanelaJogo.hpp"
#include "BotaoPesquisa.hpp"
#include "GerenciadorJogos.hpp" 
#include "Arquivos.hpp" 
#include "NetworkClient.hpp" 
#include "Theme.hpp"        // Added
#include "GraphicsUtils.hpp" // Added

using namespace MeuProjeto;
using namespace std;

// ============================================================================
// DEFINIÇÕES GLOBAIS
// ============================================================================
/**
 * @brief Instância global do gerenciador de imagens (cache de texturas).
 * * Usado por toda a aplicação para carregar e gerenciar texturas (SDL_Texture*).
 */
GerenciadorImagens gerImg;

/**
 * @brief Instância global do gerenciador de jogos (coleção de dados de jogos).
 * * Contém a lista mestra de todos os objetos @c Jogo carregados.
 */
GerenciadorJogos gerenciadorJogos;

//  Constantes Globais de Layout 
const int LARGURA_JANELA = 1920; ///< Largura da área visível da janela.
const int ALTURA_JANELA = 1280; ///< Altura da área visível da janela.
const int WORLD_WIDTH = 1920; ///< Largura total do mundo (para scroll X).
const int WORLD_HEIGHT = 2560; ///< Altura total do mundo (para scroll Y).
const SDL_Color COR_FUNDO = {30, 30, 30, 255}; ///< Cor de fundo principal.
const SDL_Color COR_RETANGULO = {50, 50, 60, 255}; ///< Cor de retângulos de painel/fundo.
const SDL_Color COR_TEXTO = {255, 255, 255, 255}; ///< Cor padrão para textos.

// Constantes de Layout do Carrossel de Categorias
const int CATEGORIA_POS_Y = 930;
const int CATEGORIA_LARGURA = 140;
const int CATEGORIA_ALTURA = 80;
const int CATEGORIA_ESPACAMENTO = 146; 
const int CATEGORIA_AREA_VISIVEL_INICIO = 275;
const int CATEGORIA_AREA_VISIVEL_FIM = 915;
const int POSICAO_BASE_X = 465;

// Constantes de Layout dos Jogos Filtrados
const int JOGO_Y = 1080;
const int JOGO_LARGURA = 384;
const int JOGO_ALTURA = 328; 
const int JOGO_GAP = 76;

// ============================================================================
// FUNÇÃO: Carregar jogos de arquivos
// ============================================================================
/**
 * @brief Carrega os dados de todos os jogos de arquivos de texto predefinidos.
 *
 * Utiliza a classe @c Arquivos e o @c GerenciadorJogos global.
 */
void carregarTodosJogos() {
    cout << "A carregar dados dos jogos..." << endl;
    
    vector<string> arquivos = {
        "assets/data/jogo1.txt",
        "assets/data/jogo2.txt",
        "assets/data/jogo3.txt",
        "assets/data/jogo4.txt",
        "assets/data/jogo5.txt"
    };
    
    for (const auto& arquivo : arquivos) {
        Arquivos::carregarJogos(arquivo, gerenciadorJogos);
    }
    
    cout << "Jogos carregados: " << gerenciadorJogos.listarJogos().size() << endl;
}

// ============================================================================
// FUNÇÃO: Criar JanelaJogo a partir de um Jogo real
// ============================================================================
/**
 * @brief Cria uma nova instância de @c JanelaJogo com base em um objeto @c Jogo existente.
 *
 * @param renderer O renderizador SDL.
 * @param jogo O ponteiro para o objeto @c Jogo de onde os dados serão extraídos.
 * @return Um ponteiro para a nova @c JanelaJogo (tela de detalhes), ou @c nullptr se o jogo for nulo.
 */
JanelaJogo* criarJanelaDeJogoReal(SDL_Renderer* renderer, Jogo* jogo) {
    if (!jogo) return nullptr;
    
    string nome = jogo->getNome();
    string desc = jogo->getDescricao();
    string categorias = jogo->getCodigo(); // Ex: "Aventura, RPG"
    string capa = jogo->getCapa();         // Imagem principal (campo *)
    string exec = jogo->getExecutavel();   // Caminho do executável (campo &)
    vector<string> capturas = jogo->getCapturas(); // Capturas de tela (campo %)
    
    // Formatar descrição para o popup
    stringstream ss;
    ss << "Título do Jogo: [" << nome << "]\n"
       << "Gênero: [" << categorias << "]\n"
       << "Sinopse:" << desc << "]\n";
    
    // Usar capa como imagem principal, capturas no carrossel, e passar o executável
    return new JanelaJogo(renderer, nome, capa, ss.str(), capturas, exec);
}

// ============================================================================
// FUNÇÃO: Atualizar jogos exibidos com base na categoria
// ============================================================================
/**
 * @brief Filtra a lista de jogos por categoria e recria o conjunto de botões exibidos na UI.
 *
 * @param jogos Referência ao vetor de botões de jogos a ser atualizado.
 * @param categoria A categoria a ser filtrada (e.g., "Aventura" ou "Todas").
 * @param renderer O renderizador SDL.
 * @param popupJanela A instância de @c Janela para gerenciar popups nos novos botões.
 */
void atualizarJogosExibidos(vector<Botao>& jogos, 
                            const string& categoria, 
                            SDL_Renderer* renderer,
                            Janela* popupJanela) {
    jogos.clear();
    
    vector<Jogo*> jogosFiltrados = gerenciadorJogos.buscarPorCategoria(categoria);
    
    // Mostrar até 4 jogos na área de jogos filtrados
    int maxJogos = min(4, (int)jogosFiltrados.size());
    
    for (int i = 0; i < maxJogos; i++) {
        Jogo* jogo = jogosFiltrados[i];
        string imgPath = jogo->getCapa();
        
        int posX = JOGO_GAP + (i * (JOGO_LARGURA + JOGO_GAP));
        
        // Cria um novo botão de jogo
        jogos.push_back(Botao(renderer, posX, JOGO_Y, JOGO_LARGURA, JOGO_ALTURA,
                              imgPath, jogo->getNome(), jogo->getDescricao(), false));
        jogos.back().setParentJanela(popupJanela);
    }
}

// ============================================================================
// FUNÇÕES DE CALLBACK PARA OS BOTÕES DE CONFIGURAÇÃO
// ============================================================================
/**
 * @brief Enumeração para identificar o submenu de configurações ativo.
 */
enum class SubmenuConfig {
    NENHUM = -1,
    ALTERAR_CONTROLE = 0,
    RESTAURAR_PADROES = 1,
    SUPORTE_AJUDA = 2,
    CONFIG_AVANCADAS = 3
};

/**
 * @brief Variável de estado para o submenu de configurações ativo.
 */
SubmenuConfig submenuAtivo = SubmenuConfig::NENHUM;


/**
 * @brief Callback para abrir a tela de Alterar Controle.
 * @param configRenderer O renderizador da janela de configuração (não usado no corpo atual).
 */
void abrirAlterarControle(SDL_Renderer* configRenderer) {
    std::cout << "[CONFIG] Alterar Controle - INICIADO" << std::endl;
    submenuAtivo = SubmenuConfig::ALTERAR_CONTROLE;
}

/**
 * @brief Callback para abrir a tela de Suporte/Ajuda.
 * @param configRenderer O renderizador da janela de configuração (não usado no corpo atual).
 */
void abrirSuporteAjuda(SDL_Renderer* configRenderer) {
    std::cout << "[CONFIG] Suporte/Ajuda - INICIADO" << std::endl;
    submenuAtivo = SubmenuConfig::SUPORTE_AJUDA;
}

/**
 * @brief Callback para abrir a tela de Configurações Avançadas.
 * @param configRenderer O renderizador da janela de configuração (não usado no corpo atual).
 */
void abrirConfiguracoesAvancadas(SDL_Renderer* configRenderer) {
    std::cout << "[CONFIG] Configurações Avançadas - INICIADO" << std::endl;
    submenuAtivo = SubmenuConfig::CONFIG_AVANCADAS;
}

/**
 * @brief Executa a ação de configuração associada a um botão.
 *
 * @param indice O índice do botão de configuração clicado.
 * @param configRenderer O renderizador da janela de configuração.
 */
void executarAcaoConfig(int indice, SDL_Renderer* configRenderer) {
    switch (indice) {
        case 0:
            abrirAlterarControle(configRenderer);
            break;
        case 1:
            abrirSuporteAjuda(configRenderer);
            break;
        case 2:
            abrirConfiguracoesAvancadas(configRenderer);
            break;
    }
}

// ============================================================================
// MAIN
// ============================================================================
/**
 * @brief Função principal do programa. Inicializa o SDL e contém o loop principal de eventos e desenho.
 */
int main(int argc, char* argv[]) {
    srand(static_cast<unsigned int>(time(NULL)));

    // =========================================================================
    // 1. INICIALIZAÇÃO SDL
    // =========================================================================
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        cerr << "Erro ao iniciar SDL: " << SDL_GetError() << endl;
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        cerr << "Erro ao iniciar SDL_image: " << IMG_GetError() << endl;
        SDL_Quit();
        return 1;
    }
    bool ttf_disponivel = (TTF_Init() != -1);
    if (!ttf_disponivel) {
        cerr << "Aviso: SDL_ttf não pôde ser inicializado." << endl;
    }
    SDL_StartTextInput();

    // =========================================================================
    // 2. CRIAÇÃO DA JANELA PRINCIPAL
    // =========================================================================
    SDL_Window* janela = SDL_CreateWindow("Gerenciador de Jogos",
                                         SDL_WINDOWPOS_CENTERED,
                                         SDL_WINDOWPOS_CENTERED,
                                         LARGURA_JANELA,
                                         ALTURA_JANELA,
                                         SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS);
    if (!janela) {
        cerr << "Erro ao criar janela" << endl;
        if (ttf_disponivel) TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    SDL_Renderer* renderizador = SDL_CreateRenderer(janela, -1,
                                                   SDL_RENDERER_ACCELERATED |
                                                   SDL_RENDERER_PRESENTVSYNC);
    if (!renderizador) {
        cerr << "Erro ao criar renderizador" << endl;
        SDL_DestroyWindow(janela);
        if (ttf_disponivel) TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // =========================================================================
    // 3. CARREGAR DADOS
    // =========================================================================
    carregarTodosJogos();
    
    // =========================================================================
    // 4. INSTANCIAÇÃO DOS ELEMENTOS DA UI
    // =========================================================================
    
    //  UI de Configurações (Elementos para a Janela Secundária) 
    Botao btnFecharConfig(1465, 10, 50, 50, "X");
    btnFecharConfig.setCor({200, 20, 20, 255}, {250, 50, 50, 255}, {180, 0, 0, 255});

    Botao btnVoltar(50, 10, 150, 50, "< Voltar");
    btnVoltar.setCor({100, 100, 100, 255}, {150, 150, 150, 255}, {80, 80, 80, 255});
    
    vector<Botao> btnsConfig = {
        Botao(217, 274, 1101, 122, "Alterar Controle"),
        Botao(217, 550, 1101, 122, "Suporte/Ajuda"),
        Botao(217, 826, 1101, 122, "Configurações Avançadas")
    };

    //  Barra Superior (Configurações, Pesquisa e Tema) 
    Botao config(renderizador, 35, 5, 130, 180, "assets/textures/configs2.png", "Configurações", "", false);
    Botao mudarTema(renderizador, 1730, 27, 120, 120, "assets/textures/mudarTema.png", "Mudar Tema", "", false);
    config.setIsRound(true);
    mudarTema.setIsRound(true);
    config.setCor({0, 0, 0, 0}, {81, 78, 78, 150}, {74, 72, 73, 150}); 
    mudarTema.setCor({0, 0, 0, 0}, {90, 88, 88, 180}, {75, 74, 74, 180});
    
    BotaoPesquisa pesquisa(renderizador, 456, 37, 1008, 98, "Barra de Pesquisa");
    pesquisa.setFontePlaceholder(TipoFonte::NORMAL);
    pesquisa.setTamanhoFontePlaceholder(24);
    pesquisa.setFonteResultados(TipoFonte::NEGRITO);
    pesquisa.setTamanhoFonteResultados(32);
    pesquisa.setCor({69, 67, 67, 255}, {90, 88, 88, 255}, {75, 74, 74, 255});
    
    SDL_Texture* lupa = gerImg.carregar(renderizador, "assets/textures/lupa_Pesquisa.png");
    
    //  Categorias (Carrossel) 
    vector<string> nomesCategorias = {"Aventura", "Casual", "Acao", "RPG", "Estrategia", "Esportes"};
    vector<Botao> categorias;
    for (const auto& nome : nomesCategorias) {
        categorias.push_back(Botao(0, 0, CATEGORIA_LARGURA, CATEGORIA_ALTURA, nome));
        categorias.back().setCor({59, 59, 61, 255}, {124, 124, 127, 255}, {104, 104, 106, 255});
    }
    categorias[0].setTamanhoFonte(24);
    categorias[4].setTamanhoFonte(22);
    categorias[5].setTamanhoFonte(24);
    int categoriaAtivaIndex = -1; // -1 indica que nenhuma categoria está ativa/selecionada
    
    Botao setaEsquerda(220, CATEGORIA_POS_Y, 40, 80, "<");
    Botao setaDireita(960, CATEGORIA_POS_Y, 40, 80, ">");
    setaEsquerda.setCor({119, 118, 123, 255}, {96, 97, 99, 255}, {131, 131, 133, 255});
    setaDireita.setCor({119, 118, 123, 255}, {96, 97, 99, 255}, {131, 131, 133, 255});
    setaEsquerda.setFonte(TipoFonte::NORMAL);
    setaDireita.setFonte(TipoFonte::NORMAL);
    // Forma fundoCategorias(...) removido, desenhado agora via GraphicsUtils
    
    //  Botão Aleatório 
    Botao aleatorio(1200, 920, 600, 100, "Escolher aleatoriamente");
    aleatorio.setCor({59, 59, 61, 255}, {124, 124, 127, 255}, {104, 104, 106, 255});
    aleatorio.setTamanhoFonte(24);
    SDL_Texture* dado = gerImg.carregar(renderizador, "assets/textures/jogoAleatorio.png");
    
    //  Destaques (Carrossel de Capas) 
    vector<Botao> destaques;
    int startX = 70, startY = 597, tamanho = 300, espacamento = 372;
    
    for (int i = 0; i < min(5, (int)gerenciadorJogos.listarJogos().size()); i++) {
        Jogo* jogo = gerenciadorJogos.obterJogoPorIndice(i);
        if (jogo) {
            string imgPath = jogo->getCapa();
            // isDestaque=true para ativar a transparência especial
            destaques.push_back(Botao(renderizador, startX + (i * espacamento), startY, tamanho, tamanho,
                                      imgPath, jogo->getNome(), jogo->getDescricao(), true));
        }
    }
    
    //  Gerenciamento de Popups e Jogos Iniciais 
    Janela popupJanela(renderizador);
    for (auto& dest : destaques) dest.setParentJanela(&popupJanela); // Associa popups aos destaques
    
    vector<Botao> jogos;
    string categoriaAtual = "Todas";
    atualizarJogosExibidos(jogos, categoriaAtual, renderizador, &popupJanela); // Carrega jogos iniciais
    
    //  Fundo Rotativo dos Destaques 
    vector<string> imagensDestaques;
    for (int i = 0; i < min(5, (int)gerenciadorJogos.listarJogos().size()); i++) {
        Jogo* jogo = gerenciadorJogos.obterJogoPorIndice(i);
        if (jogo && !jogo->getCapa().empty()) {
           imagensDestaques.push_back(jogo->getCapa());
        }
    }
    SDL_Texture* fundoDestaques = imagensDestaques.empty() ? nullptr : gerImg.carregar(renderizador, imagensDestaques[0]);
    int currentDestaqueIndex = 0;
    Uint32 lastChangeTime = SDL_GetTicks();
    const Uint32 CHANGE_INTERVAL = 60000; // Intervalo de 60 segundos (60000 ms)

    // =========================================================================
    // 5. VARIÁVEIS DE ESTADO
    // =========================================================================
    SDL_Window* configJanela = nullptr; ///< Ponteiro para a janela secundária de configurações.
    SDL_Renderer* configRenderer = nullptr; ///< Renderizador da janela de configurações.
    bool configAberta = false; ///< Flag que indica se a janela de configurações está ativa.
    bool rodando = true; ///< Flag para o loop principal.
    SDL_Event evento; ///< Estrutura de evento do SDL.
    int scrollX = 0; ///< Offset de rolagem horizontal.
    int scrollY = 0; ///< Offset de rolagem vertical.
    int categoriaIndex = 0; ///< Índice da categoria atualmente centralizada no carrossel.
    JanelaJogo* janelaJogo = nullptr; ///< Ponteiro para a tela de detalhes do jogo (quando aberta).

    bool mouseEmDestaque = false; ///< Indica se o mouse está sobre uma das áreas de destaque.
    int destaqueHoverIndex = -1; ///< Índice do destaque sob o mouse.

    // =========================================================================
    // 6. LOOP PRINCIPAL
    // =========================================================================
    
    // Tenta conectar ao ProcessManager na inicialização (opcional)
    NetworkClient::getInstance().connectToManager();

    while (rodando) {
        // Verificar eventos de rede (IPC com ProcessManager)
        NetworkClient::getInstance().checkEvents();

        //  Atualização de Posição das Categorias (Carrossel Circular) 
        int numCategorias = static_cast<int>(categorias.size());
        int totalWidthCategorias = numCategorias * CATEGORIA_ESPACAMENTO;

        for (int i = 0; i < numCategorias; ++i) {
            // Calcula a posição X do botão baseada no índice central ('categoriaIndex')
            int posX = POSICAO_BASE_X + (i - categoriaIndex) * CATEGORIA_ESPACAMENTO;
            categorias[i].area.x = posX; 
            categorias[i].area.y = CATEGORIA_POS_Y;
        }
        
        // *** LÓGICA: DETECTAR HOVER NOS DESTAQUES (Troca Fundo Rotativo) ***
        int mouseX_atual, mouseY_atual;
        SDL_GetMouseState(&mouseX_atual, &mouseY_atual);
        // Ajusta as coordenadas do mouse para o mundo com scroll (se necessário)
        mouseX_atual += scrollX;
        mouseY_atual += scrollY;
        
        bool hoverAnterior = mouseEmDestaque;
        mouseEmDestaque = false;
        destaqueHoverIndex = -1;
        
        for (size_t i = 0; i < destaques.size(); i++) {
            if (destaques[i].contemPonto(mouseX_atual, mouseY_atual)) {
                mouseEmDestaque = true;
                destaqueHoverIndex = i;
                
                // Trocar fundo se mudou de destaque
                if (!hoverAnterior || destaqueHoverIndex != i) {
                    if (i < imagensDestaques.size()) {
                        fundoDestaques = gerImg.carregar(renderizador, imagensDestaques[i]);
                    }
                }
                break;
            }
        }
        
        // Se acabou de sair do hover, reseta o timer para o carrossel automático
        if (hoverAnterior && !mouseEmDestaque) {
            lastChangeTime = SDL_GetTicks();
        }
        
        //  Tratamento de Eventos (Poll Event) 
        while (SDL_PollEvent(&evento)) {
            Uint32 windowID = evento.window.windowID;

            if (configAberta && configJanela && windowID == SDL_GetWindowID(configJanela)) {
                // ============== EVENTOS DA JANELA DE CONFIGURAÇÕES ==============
                
                // Fechar janela com X do sistema ou minimizar
                if(evento.type == SDL_WINDOWEVENT) {
                    if (evento.window.event == SDL_WINDOWEVENT_CLOSE || 
                        evento.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                        SDL_DestroyRenderer(configRenderer);
                        SDL_DestroyWindow(configJanela);
                        configJanela = nullptr;
                        configRenderer = nullptr;
                        configAberta = false;
                    }
                }

                if (submenuAtivo != SubmenuConfig::NENHUM) {
                    // Botão Voltar do submenu
                    if (btnVoltar.tratarEvento(evento, 0, 0) && 
                        evento.type == SDL_MOUSEBUTTONUP) {
                        submenuAtivo = SubmenuConfig::NENHUM;
                    }
                }
                
                // Tratamento de cliques nos botões de configuração
                for (size_t i = 0; i < btnsConfig.size(); i++) {
                    if (btnsConfig[i].tratarEvento(evento, 0, 0) && 
                        evento.type == SDL_MOUSEBUTTONUP) {
                          executarAcaoConfig(i, configRenderer);
                    }
                }
                
                // Botão X (Fechar janela)
                if (btnFecharConfig.tratarEvento(evento, 0, 0) && 
                    evento.type == SDL_MOUSEBUTTONUP) {
                    SDL_DestroyRenderer(configRenderer);
                    SDL_DestroyWindow(configJanela);
                    configJanela = nullptr;
                    configRenderer = nullptr;
                    configAberta = false;
                    submenuAtivo = SubmenuConfig::NENHUM;
                }
            } 
            else if (windowID == SDL_GetWindowID(janela)) {
                // ============== EVENTOS DA JANELA PRINCIPAL ==============

                if (janelaJogo) {
                    // Se a tela de detalhes está aberta, trata o evento nela
                    if (janelaJogo->tratarEvento(evento, 0, 0)) {
                        delete janelaJogo; // Retorna true no JanelaJogo se for para fechar
                        janelaJogo = nullptr;
                    }
                    continue; // Pula o processamento do restante da UI principal
                }
                
                if (evento.type == SDL_QUIT) {
                    rodando = false;
                }
                
                // Tratamento de eventos de minimização/restauração
                if (evento.type == SDL_WINDOWEVENT) {
                    if (evento.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                        if (configJanela) SDL_HideWindow(configJanela); 
                    } else if (evento.window.event == SDL_WINDOWEVENT_RESTORED) {
                        if (configJanela) SDL_ShowWindow(configJanela); 
                    }
                }
                
                // Trata eventos nos botões da barra superior e jogos (com offset de scroll)
                config.tratarEvento(evento, -scrollX, -scrollY);
                mudarTema.tratarEvento(evento, -scrollX, -scrollY);
                pesquisa.tratarEvento(evento, -scrollX, -scrollY); 
                setaEsquerda.tratarEvento(evento, -scrollX, -scrollY);
                setaDireita.tratarEvento(evento, -scrollX, -scrollY);
                aleatorio.tratarEvento(evento, -scrollX, -scrollY);
                
                for (auto& dest : destaques) dest.tratarEvento(evento, -scrollX, -scrollY);
                for (auto& jogo : jogos) jogo.tratarEvento(evento, -scrollX, -scrollY);
                
                // Tratamento de eventos nas Categorias Visíveis (Carrossel Circular)
                for (int i = 0; i < numCategorias; ++i) {
                    int posX_base = categorias[i].area.x; 
                    // Considera as posições adjacentes (circular) para o tratamento de hover/clique
                    int posicoes_x[] = {posX_base, posX_base - totalWidthCategorias, posX_base + totalWidthCategorias};
                    
                    for (int posX : posicoes_x) {
                        if (posX >= CATEGORIA_AREA_VISIVEL_INICIO && posX + CATEGORIA_LARGURA <= CATEGORIA_AREA_VISIVEL_FIM) {
                            int posX_original = categorias[i].area.x; 
                            categorias[i].area.x = posX; // Ajusta temporariamente a posição
                            categorias[i].tratarEvento(evento, -scrollX, -scrollY);
                            categorias[i].area.x = posX_original; // Restaura a posição
                        }
                    }
                }

                if (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT) {
                    int mouseX = evento.button.x + scrollX;
                    int mouseY = evento.button.y + scrollY;

                    bool clicouNaSeta = false;

                    // Lógica de clique nas setas do carrossel de categorias
                    if (setaEsquerda.contemPonto(mouseX, mouseY)) {
                        categoriaIndex = (categoriaIndex - 1 + numCategorias) % numCategorias;
                        clicouNaSeta = true;
                    } else if (setaDireita.contemPonto(mouseX, mouseY)) {
                        categoriaIndex = (categoriaIndex + 1) % numCategorias;
                        clicouNaSeta = true;
                    }

                    // ===== LÓGICA DE TOGGLE DE CATEGORIA (Cliques) =====
                    if (!clicouNaSeta) {
                        bool clicouEmCategoria = false;
                        
                        // Verifica clique em categorias visíveis
                        for (size_t i = 0; i < categorias.size(); i++) {
                            int posX_base = categorias[i].area.x;
                            int posicoes_x[] = {posX_base, posX_base - totalWidthCategorias, posX_base + totalWidthCategorias};
                            
                            for (int posX : posicoes_x) {
                                SDL_Rect areaTemp = {posX, CATEGORIA_POS_Y, CATEGORIA_LARGURA, CATEGORIA_ALTURA};
                                
                                // Verifica se o clique está dentro da área visível E no botão
                                if (posX >= CATEGORIA_AREA_VISIVEL_INICIO && 
                                    posX + CATEGORIA_LARGURA <= CATEGORIA_AREA_VISIVEL_FIM &&
                                    mouseX >= areaTemp.x && mouseX <= areaTemp.x + areaTemp.w &&
                                    mouseY >= areaTemp.y && mouseY <= areaTemp.y + areaTemp.h) {
                                    
                                    clicouEmCategoria = true;
                                    
                                    // Lógica de Ativação/Desativação (Toggle)
                                    if (categoriaAtivaIndex == static_cast<int>(i)) {
                                        cout << "Desmarcando categoria: " << nomesCategorias[i] << endl;
                                        categoriaAtual = "Todas";
                                        categoriaAtivaIndex = -1;
                                        atualizarJogosExibidos(jogos, categoriaAtual, renderizador, &popupJanela);
                                    } 
                                    else {
                                        cout << "Categoria selecionada: " << nomesCategorias[i] << endl;
                                        categoriaAtual = nomesCategorias[i];
                                        categoriaAtivaIndex = i;
                                        atualizarJogosExibidos(jogos, categoriaAtual, renderizador, &popupJanela);
                                    }
                                    
                                    break; 
                                }
                            }
                            
                            if (clicouEmCategoria) break; 
                        }
                    }  
                    // Cliques nos Destaques, Jogos e Botão Aleatório (Abre JanelaJogo)
                    
                    // Destaques
                    for (size_t i = 0; i < destaques.size() && i < gerenciadorJogos.listarJogos().size(); i++) {
                        if (destaques[i].contemPonto(mouseX, mouseY)) {
                            Jogo* jogoReal = gerenciadorJogos.obterJogoPorIndice(i);
                            janelaJogo = criarJanelaDeJogoReal(renderizador, jogoReal);
                            break;
                        }
                    }
                    
                    // Jogos Filtrados
                    if (!janelaJogo) {
                        vector<Jogo*> jogosFiltrados = gerenciadorJogos.buscarPorCategoria(categoriaAtual);
                        for (size_t i = 0; i < jogos.size() && i < jogosFiltrados.size(); i++) {
                            if (jogos[i].contemPonto(mouseX, mouseY)) {
                                janelaJogo = criarJanelaDeJogoReal(renderizador, jogosFiltrados[i]);
                                break;
                            }
                        }
                    }
                    
                    // Botão Aleatório
                    if (!janelaJogo && aleatorio.contemPonto(mouseX, mouseY)) {
                        if (!gerenciadorJogos.listarJogos().empty()) {
                            vector<Jogo> todosJogos = gerenciadorJogos.listarJogos();
                            Jogo* jogoAleatorio = gerenciadorJogos.obterJogoPorIndice(rand() % todosJogos.size());
                            janelaJogo = criarJanelaDeJogoReal(renderizador, jogoAleatorio);
                        }
                    }
                    
                    // Clique no fundo dos destaques (troca o fundo rotativo)
                    if (!janelaJogo && !imagensDestaques.empty() && !mouseEmDestaque) {
                        SDL_Rect areaFundoDestaques = {30, 180, 1862, 597}; 
                        if (mouseX >= areaFundoDestaques.x && mouseX <= areaFundoDestaques.x + areaFundoDestaques.w &&
                            mouseY >= areaFundoDestaques.y && mouseY <= areaFundoDestaques.y + areaFundoDestaques.h) {
                            currentDestaqueIndex = (currentDestaqueIndex + 1) % imagensDestaques.size();
                            fundoDestaques = gerImg.carregar(renderizador, imagensDestaques[currentDestaqueIndex]);
                            lastChangeTime = SDL_GetTicks();
                        }
                    }
                }
                
                // Tratamento de teclas (Scroll e ESC)
                if (evento.type == SDL_KEYDOWN) {
                    switch (evento.key.keysym.sym) {
                        case SDLK_LEFT: scrollX -= 20; break;
                        case SDLK_RIGHT: scrollX += 20; break;
                        case SDLK_UP: scrollY -= 20; break;
                        case SDLK_DOWN: scrollY += 20; break;
                        case SDLK_ESCAPE:
                            if (configAberta && configJanela) { 
                                // Fecha janela de config com ESC
                                SDL_DestroyRenderer(configRenderer);
                                SDL_DestroyWindow(configJanela);
                                configJanela = nullptr;
                                configRenderer = nullptr;
                                configAberta = false;
                            } else {
                                rodando = false;
                            }
                            break;
                    }
                }
                
                // Tratamento de scroll do mouse
                if (evento.type == SDL_MOUSEWHEEL) {
                    scrollY -= evento.wheel.y * 20;
                    scrollX -= evento.wheel.x * 20;
                }
                
                // Clique para abrir a Janela de Configurações
                if (evento.type == SDL_MOUSEBUTTONDOWN && evento.button.button == SDL_BUTTON_LEFT && !configAberta) {
                    int mouseX_fixed = evento.button.x + scrollX;
                    int mouseY_fixed = evento.button.y + scrollY;
                    
                    if (config.contemPonto(mouseX_fixed, mouseY_fixed)) {
                        // Cria a janela secundária de configurações
                        configJanela = SDL_CreateWindow("Configurações",
                                                       SDL_WINDOWPOS_CENTERED,
                                                       SDL_WINDOWPOS_CENTERED,
                                                       1525, 1116,
                                                       SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
                        if (configJanela) {
                            configRenderer = SDL_CreateRenderer(configJanela, -1, SDL_RENDERER_ACCELERATED);
                            if (configRenderer) {
                                configAberta = true;
                                SDL_RaiseWindow(configJanela);
                            }
                        }
                    }
                }
            }
        }
        
        //  Lógica Pós-Evento / Pré-Renderização 
        
        // Pesquisa: Abre JanelaJogo se um resultado foi clicado
        Jogo* jogoClicado = pesquisa.getJogoClicado();
        if (jogoClicado && !janelaJogo) {
            janelaJogo = criarJanelaDeJogoReal(renderizador, jogoClicado);
            pesquisa.resetarJogoClicado();
        }

        // Limitar scroll
        scrollX = max(0, min(scrollX, WORLD_WIDTH - LARGURA_JANELA));
        scrollY = max(0, min(scrollY, WORLD_HEIGHT - ALTURA_JANELA));
        if (janelaJogo) {
            scrollX = 0;
            scrollY = 0;
        }
        
        // Trocar fundo automaticamente (rotativo)
        if (!imagensDestaques.empty() && !mouseEmDestaque) {
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime - lastChangeTime >= CHANGE_INTERVAL) {
                currentDestaqueIndex = (currentDestaqueIndex + 1) % imagensDestaques.size();
                fundoDestaques = gerImg.carregar(renderizador, imagensDestaques[currentDestaqueIndex]);
                lastChangeTime = currentTime;
            }
        }
        // =========================================================================
        // 7. RENDERIZAÇÃO
        // =========================================================================
        
        //  Renderização da Janela Principal 
        
        // 7.1. Limpa a tela com a cor base do tema
        SDL_SetRenderDrawColor(renderizador, Theme::BG_PRIMARY.r, Theme::BG_PRIMARY.g, Theme::BG_PRIMARY.b, 255);
        SDL_RenderClear(renderizador);
        
        // 7.2. Fundo dos Destaques (Rotativo e Ambiental)
        if (fundoDestaques) {
            // Desenha a imagem cobrindo toda a tela (respeitando aspect ratio 'cover')
            SDL_Rect screenRect = {0, 0, LARGURA_JANELA, ALTURA_JANELA};
            // Se tiver scroll, a imagem de fundo fica fixa (efeito parallax zero) ou move junto?
            // Vamos deixar fixa para "ancorar" a experiência visual.
            GraphicsUtils::drawTextureCover(renderizador, fundoDestaques, screenRect);
            
            // Aplica o overlay escuro (Vidro fumê) para garantir legibilidade
            GraphicsUtils::drawOverlay(renderizador, LARGURA_JANELA, ALTURA_JANELA, 220); // 220/255 = bem escuro
        }
        
        // 7.3. Elementos estáticos/base (Scroll aplicado)
        
        // Fundo das Categorias (Painel Arredondado Moderno)
        SDL_Rect rectFundoCategorias = {30 - scrollX, 920 - scrollY, 980, 100};
        GraphicsUtils::drawRoundedRect(renderizador, rectFundoCategorias, Theme::RADIUS_MEDIUM, Theme::BG_PANEL);

        // Botão Aleatório (Estilo Card)
        // aleatorio.desenhar() já usa GraphicsUtils internamente agora, mas podemos desenhar um fundo extra se quiser
        aleatorio.desenhar(renderizador, -scrollX, -scrollY);
        
        // Título "Categorias"
        desenharTexto(renderizador, "Categorias", 50 - scrollX, 935 - scrollY, Theme::TEXT_SECONDARY, 20, TipoFonte::NEGRITO);
        
        if (dado) {
            SDL_Rect rectDado = {1200 - scrollX, 900 - scrollY, 150, 150};
            SDL_RenderCopy(renderizador, dado, NULL, &rectDado);
        }

        setaEsquerda.desenhar(renderizador, -scrollX, -scrollY);
        setaDireita.desenhar(renderizador, -scrollX, -scrollY);
        
        // 7.4. Desenha jogos e destaques
        // Destaques (Hero Section)
        if (!destaques.empty()) {
             // Título da seção Hero
             // desenharTexto(renderizador, "Destaques", 70 - scrollX, 550 - scrollY, Theme::TEXT_PRIMARY, 28, TipoFonte::NEGRITO);
        }
        
        for (auto& jogo : jogos) jogo.desenhar(renderizador, -scrollX, -scrollY);
        for (auto& dest : destaques) dest.desenhar(renderizador, -scrollX, -scrollY);

        // 7.5. Desenha categorias (Carrossel Circular)
        for (int i = 0; i < numCategorias; ++i) {
            int posX_base = categorias[i].area.x; 
            int posicoes_x[] = {posX_base, posX_base - totalWidthCategorias, posX_base + totalWidthCategorias};

            for (int posX : posicoes_x) {
                // Atualiza a cor para o estado Ativo/Inativo usando o Tema
                if (i == categoriaAtivaIndex) {
                    categorias[i].setCor(Theme::ACCENT_COLOR, Theme::BTN_HOVER, Theme::BTN_ACTIVE);
                } else {
                    categorias[i].setCor(Theme::BTN_NORMAL, Theme::BTN_HOVER, Theme::BTN_ACTIVE);
                }
                
                // Força botões redondos/arredondados nas categorias
                // categorias[i].setIsRound(false); // Já são retangulares, mas desenhados com drawRoundedRect no Botao.cpp

                // Desenha apenas as categorias dentro da área visível (clip manual)
                if (posX >= CATEGORIA_AREA_VISIVEL_INICIO && posX + CATEGORIA_LARGURA <= CATEGORIA_AREA_VISIVEL_FIM) {
                    int posX_original = categorias[i].area.x; 
                    categorias[i].area.x = posX; 
                    categorias[i].desenhar(renderizador, -scrollX, -scrollY);
                    categorias[i].area.x = posX_original; 
                }
            }
        }

        // 7.6. Elementos da Barra Superior (Header - Sem Scroll)
        // Desenha um painel de fundo para o Header (Vidro jateado no topo)
        SDL_Rect headerRect = {0, 0, LARGURA_JANELA, 160};
        GraphicsUtils::drawRoundedRect(renderizador, headerRect, 0, Theme::BG_PANEL); // Radius 0 = retângulo puro
        // Linha sutil separando o header
        // boxRGBA(renderizador, 0, 160, LARGURA_JANELA, 161, 255, 255, 255, 20);

        config.desenhar(renderizador, -scrollX, -scrollY);
        pesquisa.desenhar(renderizador, -scrollX, -scrollY);
        mudarTema.desenhar(renderizador, -scrollX, -scrollY);
        
        if (lupa) {
            SDL_Rect rectLupa = {1350 - scrollX, 40 - scrollY, 100, 100}; 
            SDL_RenderCopy(renderizador, lupa, NULL, &rectLupa);
        }

        // 7.7. Desenha Popup (Hover)
        if (popupJanela.isPopupOpen()) {
            popupJanela.desenharPopup(-scrollX, -scrollY);
        }
        
        // 7.8. Desenha Janela de Detalhes do Jogo (Sobrepõe tudo, sem scroll)
        if (janelaJogo) {
            janelaJogo->desenhar(0, 0); 
        }
        
        // Apresenta o renderizador principal
        SDL_RenderPresent(renderizador);
        
        //  Renderização da Janela de Configurações (Secundária) 
        if (configAberta && configRenderer) {
            SDL_RaiseWindow(configJanela); 

            SDL_SetRenderDrawColor(configRenderer, 42, 44, 48, 255);
            SDL_RenderClear(configRenderer);
            
            // Desenha o conteúdo do submenu ou o menu principal
            if (submenuAtivo != SubmenuConfig::NENHUM) {
                // Conteúdo do submenu (quadrado azul como placeholder)
                const int tamanho = 200;
                SDL_Rect quadrado;
                quadrado.w = tamanho;
                quadrado.h = tamanho;
                quadrado.x = (1525 - tamanho) / 2;  
                quadrado.y = (1116 - tamanho) / 2; 
                
                SDL_SetRenderDrawColor(configRenderer, 0, 0, 255, 255); 
                SDL_RenderFillRect(configRenderer, &quadrado);
                
                btnVoltar.desenhar(configRenderer);
            } else {
                // Menu principal de configurações
                desenharTexto(configRenderer, "Configurações", 535, 90, COR_TEXTO, 60);
                
                for (auto& btn : btnsConfig) btn.desenhar(configRenderer);
            }

            btnFecharConfig.desenhar(configRenderer); 
            // Apresenta o renderizador secundário
            SDL_RenderPresent(configRenderer);
        }
        
        SDL_Delay(16); // Limita a 60 FPS (1000ms / 60 frames ≈ 16ms)
    }
    
    // =========================================================================
    // 8. LIMPEZA 
    // =========================================================================
    /**
     * @brief Bloco de código responsável por liberar todos os recursos do SDL e memória alocada dinamicamente.
     */
    if (configJanela) {
        SDL_DestroyRenderer(configRenderer);
        SDL_DestroyWindow(configJanela);
    }
    if (janelaJogo) {
        delete janelaJogo;
    }
    // Libera texturas do cache global
    gerImg.liberarTudo();
    
    // Destrói os recursos principais
    SDL_DestroyRenderer(renderizador);
    SDL_DestroyWindow(janela);
    
    // Encerra os subsistemas
    if (ttf_disponivel) {
        TTF_Quit();
       }
    IMG_Quit();
    SDL_StopTextInput();
    SDL_Quit();
    
    return 0;
}