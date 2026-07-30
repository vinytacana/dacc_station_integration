/**
 * @file main.cpp
 * @brief Ponto de entrada principal da aplicação de biblioteca de jogos (Launcher).
 * 
 * Compilar: make
 * Executar: scripts/start_station.sh
 * 
 * Este arquivo coordena a integração de todos os sistemas: inicialização da SDL2,
 * carregamento da base de dados de jogos, gerenciamento de interface, processamento
 * de inputs (teclado, mouse e gamepad) e o loop de renderização principal.
 * 
 * **Arquitetura do Sistema:**
 * O programa segue o padrão clássico de Game Loop com três fases principais:
 * 1. **Inicialização**: Setup de hardware, carregamento de recursos e dados
 * 2. **Loop Principal**: Event Polling → Update → Render (60 FPS)
 * 3. **Finalização**: Limpeza de recursos e encerramento gracioso
 * 
 * **Hierarquia de Prioridade de Eventos:**
 * 1. Janela de Configuração (modal, bloqueia tudo)
 * 2. Janela de Detalhes do Jogo (overlay, bloqueia navegação)
 * 3. Interface Principal (grid de jogos, navegação)
 */

#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <filesystem>

#include "GerenciarSDL.hpp"
#include "GerenciarInputs.hpp"
#include "GerenciarInterface.hpp"
#include "GerenciarScroll.hpp"
#include "JanelaConfiguracao.hpp" 
#include "GerenciadorJogos.hpp"
#include "GerenciadorImagens.hpp"
#include "GerenciadorAudio.hpp"
#include "Arquivos.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include "NetworkClient.hpp"
#include "LogManager.hpp"
#include "SystemStatus.hpp"
#include "Utils.hpp"
#include "json.hpp"

using namespace std;
using namespace MeuProjeto;

/**
 * @brief Instância global do gerenciador de imagens para cache de texturas.
 * 
 * Mantém em memória todas as texturas SDL carregadas (capas, fundos, screenshots)
 * para evitar recarregamento contínuo do disco e melhorar performance de renderização.
 */
MeuProjeto::GerenciadorImagens gerImg;

/**
 * @brief Instância global que armazena a base de dados de todos os jogos carregados.
 * 
 * Contém o catálogo completo de jogos com metadados (nome, descrição, caminhos)
 * e fornece métodos de busca, filtragem e acesso aos objetos Jogo.
 */
MeuProjeto::GerenciadorJogos gerenciadorJogos;

/**
 * @brief Definição da janela de jogo atual (overlay de detalhes).
 * 
 * Resolve problema de múltiplas definições ao declarar como unique_ptr global.
 * Apenas uma janela de detalhes pode estar aberta por vez (comportamento modal).
 */
std::unique_ptr<MeuProjeto::JanelaJogo> MeuProjeto::janelaJogoAtual = nullptr; 

/**
 * @brief Largura padrão da janela principal da aplicação em pixels.
 * 
 * Valor obtido de ConfigLayout::LARGURA_NATIVA, tipicamente 1920px para Full HD.
 */
const int LARGURA_JANELA = ConfigLayout::LARGURA_NATIVA;

/**
 * @brief Altura padrão da janela principal da aplicação em pixels.
 * 
 * Valor obtido de ConfigLayout::ALTURA_NATIVA, tipicamente 1080px para Full HD.
 */
const int ALTURA_JANELA = ConfigLayout::ALTURA_NATIVA;

/**
 * @brief Carrega os dados dos jogos a partir de arquivos de texto no disco.
 * 
 * Percorre uma lista pré-definida de caminhos de arquivos .txt contendo metadados
 * dos jogos em formato estruturado. Utiliza a classe Arquivos para desserializar
 * os dados e popular o gerenciadorJogos com objetos Jogo completos.
 * 
 * **Formato esperado dos arquivos:**
 * Cada arquivo .txt contém campos separados por delimitadores específicos,
 * incluindo: nome, descrições, código, executável, imagens (capas, fundos, screenshots).
 * 
 * @note Se um arquivo não existir ou estiver corrompido, apenas esse arquivo é ignorado.
 * @note A função exibe no console o total de jogos carregados com sucesso.
 * 
 * @see Arquivos::carregarJogos() para detalhes do formato de serialização.
 */
void carregarJogosDoDisco() {
    cout << "Carregando biblioteca de jogos..." << endl;
    
    vector<string> arquivos;
    std::filesystem::path diretorioDados = caminho_absoluto_projeto("assets/data");
    if (std::filesystem::exists(diretorioDados)) {
        for (const auto& entrada : std::filesystem::directory_iterator(diretorioDados)) {
            if (entrada.is_regular_file() && entrada.path().extension() == ".txt") {
                arquivos.push_back(entrada.path().string());
            }
        }
    }
    std::sort(arquivos.begin(), arquivos.end());
    
    /// Itera sobre cada arquivo e tenta carregar os dados
    for (const auto& arquivo : arquivos) {
        Arquivos::carregarJogos(arquivo, gerenciadorJogos);
    }
    
    /// Exibe estatística de carregamento para debug
    cout << "Total de jogos carregados: " << gerenciadorJogos.listarJogos().size() << endl;
}

/**
 * @brief Função principal (Entry Point) da aplicação.
 * 
 * Executa a seguinte sequência de operações:
 * 
 * **Fase 1 - Inicialização (Startup):**
 * 1. Configura semente de números aleatórios
 * 2. Inicializa sistema de logging com conexão ao Process Manager
 * 3. Inicializa SDL2 e cria janela/renderizador
 * 4. Reproduz vídeo de introdução (splash screen)
 * 5. Verifica conectividade de controles/gamepads
 * 6. Carrega banco de dados de jogos do disco
 * 7. Conecta ao Process Manager via Unix Socket
 * 8. Inicializa módulos de interface e input
 * 9. Configura carrossel de destaques inicial
 * 
 * **Fase 2 - Loop Principal (Runtime):**
 * - Atualiza status de rede e sistema
 * - Processa eventos SDL (input, janelas, quit)
 * - Atualiza lógica (rotação de backgrounds, animações)
 * - Renderiza interface completa (60 FPS)
 * 
 * **Fase 3 - Finalização (Shutdown):**
 * - Fecha controles e janelas auxiliares
 * - Libera cache de texturas e fontes
 * - Destrói janela e renderizador SDL
 * - Finaliza subsistemas SDL
 * 
 * @param argc Quantidade de argumentos de linha de comando (não utilizado).
 * @param argv Vetor de argumentos de linha de comando (não utilizado).
 * 
 * @return int Status de saída do programa:
 *         - 0: Execução bem-sucedida
 *         - 1: Erro fatal durante inicialização
 * 
 * @note O loop principal roda a aproximadamente 60 FPS (16ms por frame).
 * @note Todos os recursos são liberados automaticamente na finalização.
 */
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    // FASE 1: INICIALIZAÇÃO DO SISTEMA
    
    /**
     * Inicializa gerador de números aleatórios com timestamp atual.
     * Usado para seleção aleatória de elementos da UI (ex: destaque inicial).
     */
    srand(static_cast<unsigned int>(time(NULL)));

    ConfigFontes::configurarFontes(
        caminho_absoluto_projeto("assets/fonts/Garet-Book.ttf"),
        caminho_absoluto_projeto("assets/fonts/Garet-Heavy.ttf")
    );

    /**
     * Inicialização do sistema de logging integrado.
     * Tenta carregar configuração do Process Manager do arquivo JSON,
     * caso contrário usa path padrão do socket Unix.
     */
    try {
        std::ifstream config_file(caminho_absoluto_projeto("process-manager/config.json"));
        std::string socket_path = "/tmp/dacc-station.sock"; ///< Path padrão
        
        if (config_file.is_open()) {
             nlohmann::json config = nlohmann::json::parse(config_file);
             /// Extrai path customizado se disponível no JSON
             if(config.contains("server") && config["server"].contains("socket_path"))
                socket_path = config["server"]["socket_path"];
        }
        
        /// Inicializa logger com identificador "UI" e path do socket
        LogManager::getInstance().initialize("UI", socket_path);
    } catch (...) {
        /// Fallback para configuração padrão em caso de erro
        LogManager::getInstance().initialize("UI", "/tmp/dacc-station.sock");
    }

    /// Ponteiros para janela e renderizador SDL (gerenciados por GerenciarSDL)
    SDL_Window* janela = nullptr;
    SDL_Renderer* renderer = nullptr;

    /**
     * Inicializa SDL2, cria janela em tela cheia e renderizador acelerado.
     * Retorna false em caso de erro crítico (driver de vídeo, memória, etc).
     */
    if (!GerenciarSDL::inicializar(janela, renderer, LARGURA_JANELA, ALTURA_JANELA)) {
        return 1; ///< Encerra com código de erro
    }

    /**
     * Reproduz vídeo de introdução usando MPV externo.
     * Bloqueia execução até término ou fechamento manual pelo usuário.
     */
    GerenciarSDL::tocarVideoIntro(caminho_absoluto_projeto("ui/assets/images/TelaInicalDACCC.mp4"));

    /**
     * Verifica presença física de controles/gamepads conectados.
     * Exibe feedback visual (verde=OK, vermelho=erro) por alguns segundos.
     * Se falhar, continua em modo mouse/teclado.
     */
    if (!GerenciarSDL::verificarControle(janela, renderer)) {
        cout << "[AVISO] Nenhum controle detectado." << endl;
        cout << "[SISTEMA] Iniciando em modo Mouse/Teclado..." << endl;
    }

    /// Carrega catálogo de jogos dos arquivos .txt no disco
    carregarJogosDoDisco();
    
    /**
     * Estabelece conexão com o Process Manager daemon.
     * Usado para iniciar jogos como processos filhos gerenciados externamente.
     */
    NetworkClient::getInstance().connectToManager();

    // INICIALIZAÇÃO DOS MÓDULOS DE INTERFACE
    
    GerenciarScroll estado;      ///< Estado global de navegação, scroll e flags
    GerenciarInterface interface; ///< Orquestrador visual (grid, categorias, busca)
    
    /**
     * Inicializa recursos visuais da interface (carrega imagens de UI).
     * Se falhar, encerra o programa pois interface não pode ser renderizada.
     */
    if (!interface.inicializar(renderer)) {
        cerr << "Erro Fatal: Falha ao carregar imagens da interface. Encerrando sistema." << endl;
        GerenciarSDL::limpar(janela, renderer);
        return 1;
    }
    
    GerenciarInputs inputs;      ///< Processador de eventos de periféricos
    inputs.inicializarControle(); ///< Abre conexão com gamepad detectado
    
    JanelaConfiguracao janelaConfig(janela); ///< Janela modal de configurações/opções

    /**
     * Configuração inicial do carrossel de destaques (Hero Banners).
     * Popula array com imagens de fundo dos primeiros 5 jogos do catálogo.
     */
    estado.imagensDestaques.clear();
    vector<Jogo> todosJogos = gerenciadorJogos.listarJogos();
    
    for (int i = 0; i < std::min(5, (int)todosJogos.size()); i++) {
        /// Adiciona caminho da imagem de fundo de cada jogo ao carrossel
        estado.imagensDestaques.push_back(todosJogos[i].getFundoDestaque());
    }
    
    /**
     * Define o background inicial como primeira imagem do carrossel.
     * Será rotacionado automaticamente a cada 60 segundos.
     */
    if (!estado.imagensDestaques.empty()) {
        estado.backgroundImagePath = estado.imagensDestaques[0];
    }
    
    /// Controle de tempo para rotação automática de backgrounds
    Uint32 lastBackgroundChange = SDL_GetTicks();
    const Uint32 CHANGE_INTERVAL = 60000; ///< Intervalo de 60 segundos (60000ms)

    SDL_Event evento; ///< Estrutura para captura de eventos SDL
    cout << "SISTEMA INICIADO!" << endl;

    // FASE 2: LOOP PRINCIPAL (GAME LOOP)
    
    while (estado.rodando) {
        
        /**
         * Atualização de sistemas em tempo real:
         * - NetworkClient: Verifica eventos do Process Manager (jogo terminou, etc)
         * - SystemStatus: Atualiza cache de horário, WiFi, bateria (throttled 1s)
         */
        NetworkClient::getInstance().checkEvents();
        SystemStatus::getInstance().update();
        
        /**
         * Gerenciamento automático do estado da janela de configuração.
         * Sincroniza flag estado.configAberta com estado real da janela.
         */
        if (estado.configAberta && !janelaConfig.estaAberta()) 
            janelaConfig.abrir();
        else if (!estado.configAberta && janelaConfig.estaAberta()) 
            janelaConfig.fechar();
        else if (!janelaConfig.estaAberta() && estado.configAberta) 
            estado.configAberta = false; 

        // 4.1 PROCESSAMENTO DE EVENTOS (EVENT POLLING)
        
        /**
         * Loop de polling de eventos SDL.
         * Processa todos os eventos acumulados na fila antes de renderizar.
         */
        while (SDL_PollEvent(&evento)) {
            
            /**
             * PRIORIDADE 1: Janela de Configuração (Modo Modal)
             * 
             * Quando aberta, captura TODOS os eventos e bloqueia interface principal.
             * O continue garante que nenhum outro sistema processe o evento.
             */
            if (janelaConfig.estaAberta()) {
                if (janelaConfig.processarEvento(evento)) {
                    /// Se processarEvento retornar true, janela foi fechada
                    if (!janelaConfig.estaAberta()) estado.configAberta = false;
                    continue; ///< Bloqueia propagação do evento
                }
            }

            /**
             * Evento global de fechamento (Alt+F4, botão X, etc).
             * Encerra o loop principal e inicia shutdown.
             */
            if (evento.type == SDL_QUIT) estado.rodando = false;

            /**
             * PRIORIDADE 2: Janela de Detalhes do Jogo (Overlay)
             * 
             * Quando aberta, captura eventos mas permite visualização da interface.
             * Bloqueia apenas a navegação do grid principal.
             */
            if (estado.janelaJogoAtual) {
                bool fecharJanela = false;
                
                /**
                 * Processa eventos de controle (gamepad) separadamente
                 * devido a diferenças na estrutura SDL_Event.
                 */
                if (evento.type == SDL_CONTROLLERBUTTONDOWN || evento.type == SDL_CONTROLLERBUTTONUP) {
                    if (estado.janelaJogoAtual->tratarEventoControle(evento)) 
                        fecharJanela = true;
                } else {
                    /// Eventos de mouse e teclado
                    if (estado.janelaJogoAtual->tratarEvento(evento, 0, 0)) 
                        fecharJanela = true;
                }

                /// Se tratamento retornou true, usuário fechou a janela
                if (fecharJanela) {
                    MeuProjeto::janelaJogoAtual.reset();
                    estado.janelaJogoAtual = nullptr;
                }
                
                /// Bloqueia inputs da interface principal
                continue;
            }

            /**
             * PRIORIDADE 3: Interface Principal (Navegação/Grid)
             * 
             * Apenas processa se nenhuma janela modal/overlay estiver aberta.
             * Gerencia navegação do grid, busca, seleção de jogos, etc.
             */
            if (!janelaConfig.estaAberta()) {
                inputs.atualizar(evento, estado, interface, renderer);
            }
        }

        // 4.2 LÓGICA DE ATUALIZAÇÃO (UPDATE)
        
        /**
         * Rotação automática das imagens de destaque no fundo.
         * Apenas ativa quando:
         * - Há imagens no carrossel
         * - Nenhuma janela de jogo está aberta
         * - Janela de configuração está fechada
         */
        if (!estado.imagensDestaques.empty() && !estado.janelaJogoAtual && !estado.configAberta) {
            Uint32 currentTime = SDL_GetTicks();
            
            /// Verifica se passou tempo suficiente desde última troca
            if (currentTime - lastBackgroundChange >= CHANGE_INTERVAL) {
                /**
                 * Avança para próxima imagem com wraparound (0 após última).
                 * Operador % garante que nunca exceda tamanho do array.
                 */
                estado.currentDestaqueIndex = (estado.currentDestaqueIndex + 1) % estado.imagensDestaques.size();
                lastBackgroundChange = currentTime; ///< Reseta timer
            }
        }

        // 4.3 RENDERIZAÇÃO (DRAW)
        
        /**
         * Define cor de fundo baseada no tema ativo.
         * Limpa buffer de renderização com essa cor antes de desenhar elementos.
         */
        SDL_Color corFundo = GerenciadorTemas::getInstance().getCorFundo();
        SDL_SetRenderDrawColor(renderer, corFundo.r, corFundo.g, corFundo.b, corFundo.a);
        SDL_RenderClear(renderer);
        
        /**
         * Renderiza todos os elementos da interface principal:
         * - Background animado (destaque atual)
         * - Grid de jogos com scroll
         * - Barra de categorias
         * - Campo de busca
         * - Barra superior (relógio, bateria, WiFi)
         * - Barra inferior (dicas de navegação)
         */
        interface.desenhar(renderer, estado);
        
        /**
         * Se houver janela de detalhes aberta, renderiza como overlay.
         * Desenha por cima da interface principal (sistema de camadas).
         */
        if (estado.janelaJogoAtual) {
            estado.janelaJogoAtual->desenhar(0, 0);
        }

        /**
         * Apresenta buffer de renderização na tela (swap de buffers).
         * Com VSync ativo, aguarda próximo refresh do monitor.
         */
        SDL_RenderPresent(renderer);

        /**
         * Renderização da janela de configuração (se estiver aberta).
         * Usa janela SDL separada, não afeta renderização principal.
         */
        if (janelaConfig.estaAberta()) {
            janelaConfig.desenhar();
        }
        
        /**
         * Controle de taxa de quadros.
         * Aguarda 16ms (~62.5 FPS) para não sobrecarregar CPU.
         * VSync já limita a 60 FPS, mas isso garante mínimo de CPU usage.
         */
        SDL_Delay(16);
    }

    // FASE 3: FINALIZAÇÃO E LIMPEZA DE RECURSOS
    
    /**
     * Encerra controles de forma segura, fechando conexão com gamepad.
     * Previne memory leaks de handles de dispositivos.
     */
    inputs.fecharControle();
    
    /// Fecha janela de configuração se ainda estiver aberta
    janelaConfig.fechar();
    
    /**
     * Libera janela de jogo se houver instância ativa.
     * unique_ptr::reset() chama destrutor automaticamente.
     */
    if (estado.janelaJogoAtual) MeuProjeto::janelaJogoAtual.reset();
    
    /**
     * Libera cache de texturas ANTES de destruir o renderer.
     * Ordem crítica: texturas devem ser destruídas enquanto renderer existe.
     */
    gerImg.liberarTudo();
    MeuProjeto::limparCacheTexto();
    gerAudio.liberarTudo();
    
    /**
     * Destrói janela, renderizador e finaliza todos os subsistemas SDL.
     * Deve ser a última chamada relacionada à SDL no programa.
     */
    GerenciarSDL::limpar(janela, renderer);
    
    return 0; ///< Retorna sucesso para o sistema operacional
}
