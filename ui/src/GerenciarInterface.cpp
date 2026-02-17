/**
 * @file GerenciarInterface.cpp
 * @brief Implementação da classe GerenciarInterface responsável por gerenciar todos os elementos visuais da interface do usuário
 *
 * Este arquivo contém a implementação completa do gerenciador de interface, incluindo:
 * - Inicialização de botões, categorias e elementos visuais
 * - Carregamento de recursos gráficos (ícones, imagens, texturas)
 * - Gerenciamento de layout e posicionamento de elementos
 * - Atualização dinâmica da interface baseada em categorias
 * - Sistema de scroll e navegação entre categorias
 */

#include "GerenciarInterface.hpp"
#include "GerenciadorJogos.hpp"
#include "GerenciadorImagens.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include "Utils.hpp"
#include "GerenciadorFontes.hpp" 
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <algorithm>

using namespace MeuProjeto;
using namespace std;

/** @brief Gerenciador global de jogos */
extern GerenciadorJogos gerenciadorJogos;

/** @brief Gerenciador global de imagens */
extern GerenciadorImagens gerImg;

/**
 * @brief Construtor padrão da classe GerenciarInterface
 *
 * Inicializa uma nova instância do gerenciador de interface.
 * Os ponteiros de elementos visuais são inicializados como nullptr implicitamente.
 */
GerenciarInterface::GerenciarInterface() {}

/**
 * @brief Destrutor da classe GerenciarInterface
 *
 * Libera toda a memória alocada dinamicamente para os elementos da interface:
 * - Botões de configuração, tema, aleatório e pesquisa
 * - Setas de navegação de categorias
 * - Fundo de categorias
 * - Janela popup
 * - Teclado virtual (se existir)
 */
GerenciarInterface::~GerenciarInterface() {
    delete btnConfig; 
    delete btnMudarTema; 
    delete btnAleatorio; 
    delete btnPesquisa;
    delete setaEsquerda; 
    delete setaDireita; 
    delete fundoCategorias; 
    delete popupJanela;
    if (tecladoVirtual) delete tecladoVirtual;
}

/**
 * @brief Inicializa todos os elementos da interface gráfica
 *
 * Esta função realiza a inicialização completa da interface, incluindo:
 * 1. Criação de janelas popup e teclado virtual
 * 2. Carregamento de ícones para temas claro e escuro
 * 3. Criação de botões de configuração, tema e pesquisa
 * 4. Carregamento de ícones de lupa e imagens explicativas
 * 5. Inicialização do sistema de categorias
 * 6. Criação do painel de categorias com setas de navegação
 * 7. Criação do botão de jogo aleatório
 * 8. Inicialização dos destaques (mini-capas de jogos)
 * 9. Atualização inicial da lista de jogos
 *
 * @param renderer Ponteiro para o renderizador SDL2 usado para criar texturas
 * @return true se todos os elementos foram inicializados com sucesso
 * @return false se algum elemento crítico falhou ao carregar (com mensagem de erro no stderr)
 */
bool GerenciarInterface::inicializar(SDL_Renderer* renderer) {
    // Validação do renderizador
    if (!renderer) return false;
    
    // Criação da janela popup para exibir informações dos jogos
    popupJanela = new Janela(renderer);
    
    // Criação do teclado virtual para entrada de texto
    tecladoVirtual = new TecladoVirtual(); 

    // CARREGAMENTO DE ÍCONES DE CONFIGURAÇÃO
    
    // Carrega e Verifica: Ícone de configuração para tema claro
    iconeConfigClaro = gerImg.carregar(renderer, "assets/images/light/ConfigClaro.png");
    if (!iconeConfigClaro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/light/ConfigClaro.png" << endl;
        return false;
    }

    // Carrega e Verifica: Ícone de configuração para tema escuro
    iconeConfigEscuro = gerImg.carregar(renderer, "assets/images/dark/ConfigEscuro.png");
    if (!iconeConfigEscuro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/dark/ConfigEscuro.png" << endl;
        return false;
    }

    // CRIAÇÃO DOS BOTÕES PRINCIPAIS
    
    /**
     * Botão de Configurações:
     * - Posicionado usando ConfigLayout para responsividade
     * - Formato redondo (setIsRound)
     * - Sem imagem inicial (definida dinamicamente baseada no tema)
     */
    btnConfig = new Botao(renderer, 
        ConfigLayout::X(ConfigLayout::BOTAO_CONFIG_POS_X), 
        ConfigLayout::Y(ConfigLayout::BOTAO_CONFIG_POS_Y), 
        ConfigLayout::F(ConfigLayout::BOTAO_CONFIG_LARGURA), 
        ConfigLayout::F(ConfigLayout::BOTAO_CONFIG_ALTURA), 
        "", "Configurações", "", false);
    btnConfig->setIsRound(true);

    /**
     * Botão de Mudar Tema:
     * - Alterna entre tema claro e escuro
     * - Possui ícone específico (mudarTema.png)
     * - Formato redondo
     */
    btnMudarTema = new Botao(renderer, 
        ConfigLayout::X(ConfigLayout::BOTAO_TEMA_POS_X), 
        ConfigLayout::Y(ConfigLayout::BOTAO_TEMA_POS_Y), 
        ConfigLayout::F(ConfigLayout::BOTAO_TEMA_LARGURA), 
        ConfigLayout::F(ConfigLayout::BOTAO_TEMA_ALTURA), 
        "assets/images/icons/mudarTema.png", "Mudar Tema", "", false);
    btnMudarTema->setIsRound(true);

    // Verifica se a imagem interna do botão Mudar Tema foi carregada corretamente
    if (!btnMudarTema->getTexturaImagem()) {
        cerr << "[ERRO] Falha ao carregar imagem do botão: assets/images/icons/mudarTema.png" << endl;
        return false;
    }

    /**
     * Botão de Pesquisa:
     * - Permite buscar jogos por nome
     * - Possui placeholder customizável
     * - Exibe resultados de pesquisa em tempo real
     */
    btnPesquisa = new BotaoPesquisa(renderer, 
        ConfigLayout::X(ConfigLayout::PESQUISA_POS_X), 
        ConfigLayout::Y(ConfigLayout::PESQUISA_POS_Y), 
        ConfigLayout::F(ConfigLayout::PESQUISA_LARGURA), 
        ConfigLayout::F(ConfigLayout::PESQUISA_ALTURA), 
        "Barra de Pesquisa");
    
    // Configuração das fontes do campo de pesquisa
    btnPesquisa->setFontePlaceholder(TipoFonte::NORMAL);
    btnPesquisa->setTamanhoFontePlaceholder(ConfigLayout::F(24));
    btnPesquisa->setFonteResultados(TipoFonte::NEGRITO);
    btnPesquisa->setTamanhoFonteResultados(ConfigLayout::F(32));

    // CARREGAMENTO DE ÍCONES DE LUPA
    
    // Carrega e Verifica: Ícone de lupa para tema claro
    iconeLupaClaro = gerImg.carregar(renderer, "assets/images/light/lupa_PesquisaClaro.png");
    if (!iconeLupaClaro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/light/lupa_PesquisaClaro.png" << endl;
        return false;
    }

    // Carrega e Verifica: Ícone de lupa para tema escuro
    iconeLupaEscuro = gerImg.carregar(renderer, "assets/images/dark/lupa_PesquisaEscuro.png");
    if (!iconeLupaEscuro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/dark/lupa_PesquisaEscuro.png" << endl;
        return false;
    }
    
    // CARREGAMENTO DE IMAGENS EXPLICATIVAS
    
    // Carrega e Verifica: Imagem explicativa dos botões para tema claro
    explicacaoClaro = gerImg.carregar(renderer, "assets/images/light/explicacaoBotoesClaro.jpg");
    if (!explicacaoClaro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/light/explicacaoBotoesClaro.jpg" << endl;
        return false;
    }

    // Carrega e Verifica: Imagem explicativa dos botões para tema escuro
    explicacaoEscuro = gerImg.carregar(renderer, "assets/images/dark/explicacaoBotoesEscuro.jpg");
    if (!explicacaoEscuro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/dark/explicacaoBotoesEscuro.jpg" << endl;
        return false;
    }

    // INICIALIZAÇÃO DAS CATEGORIAS
    
    /**
     * Criação das categorias de jogos:
     * - "Todas": Exibe todos os jogos disponíveis
     * - "Aventura", "Casual", "Ação", "Rpg", "Estrategia", "Esportes": Categorias específicas
     */
    vector<string> nomesCategorias = {"Todas", "Aventura", "Casual", "Ação", "Rpg", "Estrategia", "Esportes"};
    
    // Cria um botão para cada categoria com dimensões configuradas
    for (const auto& nome : nomesCategorias) {
        Botao btn(0, 0,
                  ConfigLayout::F(ConfigLayout::ITEM_CATEGORIA_LARGURA),
                  ConfigLayout::F(ConfigLayout::ITEM_CATEGORIA_ALTURA),
                  nome);

        // Define bordas arredondadas para visual mais suave
        btn.setRetanguloBordasArredondadas(18); 
        categorias.push_back(btn);
    }   
    
    // Ajusta tamanho de fonte para categorias específicas (para melhor legibilidade)
    if (categorias.size() > 0) categorias[1].setTamanhoFonte(ConfigLayout::F(24)); // Aventura
    if (categorias.size() > 4) categorias[3].setTamanhoFonte(ConfigLayout::F(24)); // Ação
    if (categorias.size() > 5) categorias[2].setTamanhoFonte(ConfigLayout::F(24)); // Casual

    // CRIAÇÃO DO PAINEL DE CATEGORIAS
    
    /**
     * Fundo visual para o painel de categorias:
     * - Forma retangular com cor personalizada
     * - Serve como container visual para as categorias
     */
    fundoCategorias = new Forma(
        ConfigLayout::X(ConfigLayout::PAINEL_CATEGORIA_POS_X), 
        ConfigLayout::Y(ConfigLayout::PAINEL_CATEGORIA_POS_Y), 
        ConfigLayout::F(ConfigLayout::PAINEL_CATEGORIA_LARGURA), 
        ConfigLayout::F(ConfigLayout::PAINEL_CATEGORIA_ALTURA), 
        {50, 50, 60, 255}, "retangulo");

    // CRIAÇÃO DAS SETAS DE NAVEGAÇÃO
    
    /**
     * Seta Esquerda:
     * - Navega para categorias anteriores
     * - Formato arredondado e fonte em negrito
     */
    setaEsquerda = new Botao(
        ConfigLayout::X(ConfigLayout::SETA_CATEGORIA_ESQ_POS_X),  
        ConfigLayout::Y(ConfigLayout::SETA_CATEGORIA_POS_Y), 
        ConfigLayout::F(ConfigLayout::SETA_CATEGORIA_LARGURA), 
        ConfigLayout::F(ConfigLayout::SETA_CATEGORIA_ALTURA), 
        "<"
    );

    /**
     * Seta Direita:
     * - Navega para categorias seguintes
     * - Formato arredondado e fonte em negrito
     */
    setaDireita = new Botao(
        ConfigLayout::X(ConfigLayout::SETA_CATEGORIA_DIR_POS_X), 
        ConfigLayout::Y(ConfigLayout::SETA_CATEGORIA_POS_Y), 
        ConfigLayout::F(ConfigLayout::SETA_CATEGORIA_LARGURA), 
        ConfigLayout::F(ConfigLayout::SETA_CATEGORIA_ALTURA), 
        ">"
    );

    // Configuração visual das setas
    setaEsquerda->setFonte(TipoFonte::NEGRITO);
    setaDireita->setFonte(TipoFonte::NEGRITO);
    setaEsquerda->setRetanguloBordasArredondadas(20);
    setaDireita->setRetanguloBordasArredondadas(20);

    // CÁLCULO DE POSICIONAMENTO DO BOTÃO ALEATÓRIO
    
    /**
     * Calcula a posição centralizada do botão de jogo aleatório:
     * - Baseado na largura total da grade de jogos
     * - Centralizado horizontalmente na tela
     */
    int larguraItem   = ConfigLayout::F(ConfigLayout::GRADE_JOGOS_LARGURA_ITEM);
    int espacoX       = ConfigLayout::F(ConfigLayout::GRADE_JOGOS_ESPACO_X);
    const int COLUNAS = ConfigLayout::GRADE_JOGOS_COLUNAS;
    int colunas       = COLUNAS;

    // Largura total da grade = (largura dos itens) + (espaços entre itens)
    int larguraTotalGrade =
        (colunas * larguraItem) +
        ((colunas - 1) * espacoX);

    int posXAleatorio = (ConfigLayout::larguraTela - larguraTotalGrade) / 2;
    int posYAleatorio = ConfigLayout::Y(ConfigLayout::GRADE_JOGOS_Y_INICIAL);
    int largAleatorio = ConfigLayout::F(ConfigLayout::BOTAO_ALEATORIO_LARGURA);
    int altAleatorio  = ConfigLayout::F(ConfigLayout::BOTAO_ALEATORIO_ALTURA);

    /**
     * Botão de Jogo Aleatório:
     * - Criado uma única vez e fixo na primeira posição da grade
     * - Ocupa a posição (0,0) do grid (índice 0)
     * - Usa o campo de descrição curta do popup para exibir "Jogo Aleatório"
     * - Imagem e nome inicialmente vazios (preenchidos dinamicamente)
     */
    btnAleatorio = new Botao(renderer, posXAleatorio, posYAleatorio, largAleatorio, altAleatorio,
                         "", "", "", false);
    btnAleatorio->setParentJanela(popupJanela);

    // Carrega e Verifica: Ícone do dado para o botão aleatório
    iconeDado = gerImg.carregar(renderer, "assets/images/jogoAleatorio.png");
    if (!iconeDado) {
        cerr << "[ERRO] Falha ao carregar: assets/images/jogoAleatorio.png" << endl;
        return false;
    }

    // INICIALIZAÇÃO DOS DESTAQUES (MINI-CAPAS)
   
    /**
     * Configuração de posicionamento dos destaques:
     * - Exibidos horizontalmente no topo da interface
     * - Espaçamento uniforme entre eles
     * - Máximo de 5 destaques
     */
    int startX = ConfigLayout::X(ConfigLayout::DESTAQUE_MINI_POS_X_INICIAL);
    int startY = ConfigLayout::Y(ConfigLayout::DESTAQUE_MINI_POS_Y);
    int tamanho = ConfigLayout::F(ConfigLayout::DESTAQUE_MINI_TAMANHO);
    int espacamento = ConfigLayout::F(ConfigLayout::DESTAQUE_MINI_PASSO_TOTAL);

    // Obtém todos os jogos disponíveis
    vector<Jogo> todosJogos = gerenciadorJogos.listarJogos();
    
    destaques.clear(); 
    
    /**
     * Cria botões de destaque para os primeiros 5 jogos:
     * - Cada destaque exibe a capa do jogo em miniatura
     * - Conectado à janela popup para exibir detalhes ao clicar
     */
    for (int i = 0; i < std::min(5, (int)todosJogos.size()); i++) {
        Jogo& jogo = todosJogos[i];
        destaques.push_back(Botao(renderer, 
                                  startX + (i * espacamento), 
                                  startY, 
                                  tamanho, 
                                  tamanho,
                                  jogo.getCapaTelaPrincipal(), 
                                  jogo.getNome(), 
                                  jogo.getDescricaoCurta(), 
                                  true));
        
        // Verifica se a capa do jogo (destaque) carregou corretamente
        if (!destaques.back().getTexturaImagem()) {
            cerr << "[ERRO] Falha ao carregar capa do destaque: " << jogo.getNome() << endl;
            return false;
        }

        // Conecta o destaque à janela popup
        destaques.back().setParentJanela(popupJanela);
    }
    
    // ATUALIZAÇÃO INICIAL DA LISTA DE JOGOS
    
    /**
     * Carrega a categoria "Todas" por padrão na inicialização
     * Popula a grade de jogos com todos os jogos disponíveis
     */
    atualizarJogosPorCategoria("Todas", renderer);

    return true; // Sucesso - todos os elementos foram inicializados
}

/**
 * @brief Atualiza a lista de jogos exibidos baseada na categoria selecionada
 *
 * Esta função realiza as seguintes operações:
 * 1. Limpa a lista atual de jogos
 * 2. Busca jogos da categoria especificada no gerenciador
 * 3. Calcula posicionamento em grade (COLUNAS x LINHAS)
 * 4. Cria botões para cada jogo com suas capas e informações
 * 5. Respeita o botão aleatório na posição 0 (jogos começam do índice 1)
 *
 * Layout da Grade:
 * - Posição 0: Botão Aleatório (fixo)
 * - Posições 1-14: Jogos da categoria (máximo 14 jogos visíveis)
 * - Total: 15 itens na tela (3 colunas x 5 linhas)
 *
 * @param categoria Nome da categoria a filtrar (ex: "Todas", "Aventura", "Ação")
 * @param renderer Ponteiro para o renderizador SDL2 usado para criar texturas dos botões
 */
void GerenciarInterface::atualizarJogosPorCategoria(const std::string& categoria, SDL_Renderer* renderer) {
    // Limpa a lista atual de jogos para reconstruir com a nova categoria
    jogos.clear();
    
    // Busca jogos filtrados pela categoria no gerenciador de jogos
    vector<Jogo*> jogosFiltrados = gerenciadorJogos.buscarPorCategoria(categoria);
    
    // CONFIGURAÇÕES DE LAYOUT DA GRADE
    
    const int COLUNAS = ConfigLayout::GRADE_JOGOS_COLUNAS; // Número de colunas na grade
    int larguraItem = ConfigLayout::F(ConfigLayout::GRADE_JOGOS_LARGURA_ITEM);   // Largura de cada item
    int alturaItem = ConfigLayout::F(ConfigLayout::GRADE_JOGOS_ALTURA_ITEM);     // Altura de cada item
    int espacoX = ConfigLayout::F(ConfigLayout::GRADE_JOGOS_ESPACO_X);           // Espaço horizontal entre itens
    int alturaLinha = alturaItem + ConfigLayout::F(ConfigLayout::GRADE_JOGOS_ESPACO_Y); // Altura total de uma linha
    int yInicial = ConfigLayout::Y(ConfigLayout::GRADE_JOGOS_Y_INICIAL);         // Posição Y inicial da grade

    /**
     * IMPORTANTE - Sistema de Indexação:
     * - O Botão Aleatório ocupa a posição (0,0) da grade (índice 0)
     * - Os jogos começam a ser renderizados a partir do índice 1
     * - Isso garante que o botão aleatório sempre fique visível no topo esquerdo
     */
    
    // Determina o número máximo de jogos a exibir
    int maxJogos = 14; // Para completar 15 itens na tela (1 aleatório + 14 jogos)
    if (jogosFiltrados.empty()) maxJogos = 0; // Se não há jogos, não cria botões
    
    // CRIAÇÃO DOS BOTÕES DE JOGOS
    
    for (int i = 0; i < std::min((int)jogosFiltrados.size(), maxJogos); i++) { 
        Jogo* jogo = jogosFiltrados[i];
        
        /**
         * Cálculo de posição na grade:
         * - indiceVisual = i + 1 (pois índice 0 é o botão aleatório)
         * - col = posição na coluna (0 a COLUNAS-1)
         * - linha = número da linha (0, 1, 2, ...)
         */
        int indiceVisual = i + 1;
        
        int col = indiceVisual % COLUNAS;        // Posição na coluna
        int linha = indiceVisual / COLUNAS;      // Número da linha

        /**
         * Centralização da grade na tela:
         * - Calcula largura total ocupada pela grade
         * - Centraliza horizontalmente subtraindo da largura da tela
         */
        int larguraTotalGrade =
            (COLUNAS * larguraItem) +
            ((COLUNAS - 1) * espacoX);

        int xInicialGrade =
            (ConfigLayout::larguraTela - larguraTotalGrade) / 2;

        // Posição X absoluta do item = início da grade + (coluna × passo)
        int posXAbs = xInicialGrade + (col * (larguraItem + espacoX));

        // Posição Y = posição inicial + (linha × altura da linha)
        int posY = yInicial + (linha * alturaLinha); 
        
        /**
         * Criação do botão do jogo:
         * - Posição calculada na grade
         * - Dimensões configuradas
         * - Capa do jogo como imagem
         * - Nome e descrição curta para popup
         */
        jogos.push_back(Botao(renderer, posXAbs, posY, 
                              larguraItem, alturaItem,
                              jogo->getCapaTelaPrincipal(), 
                              jogo->getNome(), 
                              jogo->getDescricaoCurta(), 
                              false));
                              
        // Conecta o botão do jogo à janela popup para exibir detalhes
        jogos.back().setParentJanela(popupJanela);
    }
}

/**
 * @brief Atualiza as posições visuais das categorias baseado no estado de scroll
 *
 * Esta função implementa um sistema de carrossel circular para navegação de categorias:
 * 
 * Sistema de Slots:
 * - 4 slots visíveis na tela (posições 0, 1, 2, 3)
 * - "Todas" sempre fixo na posição central (slot 4)
 * - Demais categorias rolam ciclicamente nos 4 slots visíveis
 *
 * Modos de Navegação:
 * 1. MODO CONTROLE (Joystick/Gamepad conectado):
 *    - Centraliza a visão na categoria ATIVA (selecionada)
 *    - A categoria selecionada fica sempre no slot 2 (centro)
 * 
 * 2. MODO MOUSE (Sem controles conectados):
 *    - Segue o índice alterado pelas setas da tela
 *    - Scroll manual através das setas esquerda/direita
 *
 * Matemática Circular:
 * - Usa módulo para criar efeito de rolagem infinita
 * - Categorias "voltam ao início" ao chegar no final
 * - Evita índices negativos com normalização
 *
 * @param estado Referência ao estado do gerenciador de scroll contendo:
 *               - categoriaIndex: índice atual do scroll (modo mouse)
 *               - categoriaAtivaIndex: índice da categoria selecionada (modo controle)
 */
void GerenciarInterface::atualizarPosicoes(GerenciarScroll& estado) {
    // RESET INICIAL
    
    /**
     * Reseta todas as categorias para fora da tela visível
     * Isso garante que apenas as categorias calculadas apareçam
     */
    for (auto& cat : categorias) { 
        cat.area.x = -5000; 
        cat.area.y = -5000; 
    }
    
    // CONFIGURAÇÕES DE LAYOUT
    
    int baseX = ConfigLayout::X(ConfigLayout::ITEM_CATEGORIA_POS_X_BASE);  // Posição X base do primeiro slot
    int largura = ConfigLayout::F(ConfigLayout::ITEM_CATEGORIA_LARGURA);   // Largura de cada categoria
    int espaco = ConfigLayout::F(ConfigLayout::ITEM_CATEGORIA_ESPACO);     // Espaço entre categorias
    int passo = largura + espaco;                                           // Distância total entre slots
    int posY = ConfigLayout::Y(ConfigLayout::ITEM_CATEGORIA_POS_Y);        // Posição Y fixa de todas categorias

    /**
     * Posicionamento da categoria "Todas":
     * - Sempre fixo no slot 4 (posição central à direita)
     * - Não participa do sistema de scroll
     */
    categorias[0].area.x = baseX + (4 * passo);
    categorias[0].area.y = posY;

    // CÁLCULO DE ITENS DINÂMICOS
    
    int numCategorias = categorias.size();      // Total de categorias
    int slotsDisponiveis = 4;                   // Slots 0, 1, 2, 3 (4 slots visíveis)
    int itensDinamicos = numCategorias - 1;     // Exclui "Todas" do scroll

    if (itensDinamicos > 0) {
        
        int indiceBaseScroll;

        // LÓGICA HÍBRIDA DE NAVEGAÇÃO
        
        /**
         * Detecta o modo de entrada baseado na presença de controles:
         * - SDL_NumJoysticks() > 0: Há controles conectados
         */
        if (SDL_NumJoysticks() > 0) {
            // MODO CONTROLE
            /**
             * Lógica Original - Centralização na Categoria Ativa:
             * - Calcula scroll para manter a categoria selecionada visível
             * - Se categoria ativa > 0 (não é "Todas"):
             *   - Converte para índice dinâmico (subtrai 1)
             *   - Subtrai 2 para centralizar no slot 2
             * - Se categoria ativa = 0 ("Todas"):
             *   - Define indiceBaseScroll = -1 (mostra primeiras categorias)
             */
            if (estado.categoriaAtivaIndex > 0) {
                int indiceDinamicoDoSelecionado = estado.categoriaAtivaIndex - 1;
                indiceBaseScroll = indiceDinamicoDoSelecionado - 2;
            } else {
                indiceBaseScroll = -1; 
            }
        } else {
            // MODO MOUSE
            /**
             * Lógica Nova - Scroll Manual:
             * - Segue estritamente o índice controlado pelas setas
             * - estado.categoriaIndex é incrementado/decrementado pelas setas
             * - Subtrai 1 para converter de índice global para dinâmico
             */
            indiceBaseScroll = estado.categoriaIndex - 1; 
        }

        // NORMALIZAÇÃO CIRCULAR
        
        /**
         * Garante matemática circular correta:
         * - Módulo (%) para voltar ao início ao ultrapassar o fim
         * - Soma itensDinamicos para evitar valores negativos
         * - Segundo módulo garante resultado sempre positivo
         * 
         * Exemplo com 6 itens dinâmicos:
         * - indiceBaseScroll = -1 → ((-1 % 6) + 6) % 6 = 5
         * - indiceBaseScroll = 7  → ((7 % 6) + 6) % 6 = 1
         */
        indiceBaseScroll = (indiceBaseScroll % itensDinamicos + itensDinamicos) % itensDinamicos;

        // PREENCHIMENTO DOS SLOTS VISÍVEIS
        
        /**
         * Distribui categorias pelos 4 slots visíveis:
         * - Para cada slot (0 a 3):
         *   - Calcula qual item dinâmico deve ocupar o slot
         *   - Converte para índice real no vetor (soma 1 para pular "Todas")
         *   - Posiciona o botão no slot correspondente
         */
        for (int i = 0; i < slotsDisponiveis; i++) {
            // Calcula item dinâmico para o slot 'i' usando scroll circular
            int indiceVirtual = (indiceBaseScroll + i) % itensDinamicos;
            
            // Converte para índice real no vetor (pula índice 0 que é "Todas")
            int indiceRealVector = indiceVirtual + 1;

            if (indiceRealVector < numCategorias) {
                Botao* btn = &categorias[indiceRealVector];
                btn->area.x = baseX + (i * passo);  // Posição X baseada no slot
                btn->area.y = posY;                  // Posição Y fixa
            }
        }
    }
  
    // ATUALIZAÇÃO DAS SETAS DE NAVEGAÇÃO
    
    /**
     * Atualiza posição Y das setas para manter responsividade:
     * - Útil quando há redimensionamento de tela
     * - Mantém setas alinhadas verticalmente com as categorias
     */
    if (setaEsquerda)
        setaEsquerda->area.y = ConfigLayout::Y(ConfigLayout::SETA_CATEGORIA_POS_Y);

    if (setaDireita)
        setaDireita->area.y = ConfigLayout::Y(ConfigLayout::SETA_CATEGORIA_POS_Y);
}

/**
 * @brief Obtém a lista ordenada de todos os botões navegáveis da interface
 *
 * Esta função constrói dinamicamente uma lista de ponteiros para botões que podem ser
 * navegados usando controles ou teclado. A ordem de adição determina a sequência de navegação.
 *
 * Ordem de Navegação:
 * 1. Destaques (mini-capas no topo)
 * 2. Botão de Configurações
 * 3. Botão de Pesquisa
 * 4. Botão de Mudar Tema
 * 5. Categorias visíveis (ordenadas da esquerda para direita)
 * 6. Botão Aleatório
 * 7. Jogos da grade
 *
 * Sistema de Filtragem de Categorias:
 * - Apenas categorias com y > 0 são consideradas visíveis
 * - Isso previne a seleção de categorias que estão fora da tela (PONTO 3)
 * - As categorias visíveis são ordenadas por posição X (esquerda → direita)
 * - Isso resolve a navegação confusa entre categorias (PONTO 2)
 *
 * @return Referência para o vetor de ponteiros de botões navegáveis
 */
std::vector<Botao*>& GerenciarInterface::obterBotoesNavegaveis() {
    // Limpa a lista anterior para reconstruir com estado atual
    botoesNavegaveis.clear();
    
    // CAMADA 1: DESTAQUES
    /**
     * Adiciona os 5 destaques (mini-capas) do topo da interface
     * Primeira área de navegação acessível ao usuário
     */
    for (auto& destaque : destaques) 
        botoesNavegaveis.push_back(&destaque);
    
    // CAMADA 2: BOTÕES DO TOPO
    /**
     * Adiciona botões fixos do cabeçalho da interface:
     * - Configurações: Acesso a opções do sistema
     * - Pesquisa: Campo de busca de jogos
     * - Mudar Tema: Alternância entre tema claro/escuro
     */
    botoesNavegaveis.push_back(btnConfig);
    botoesNavegaveis.push_back(btnPesquisa);
    botoesNavegaveis.push_back(btnMudarTema);
    
    // CAMADA 3: CATEGORIAS (COM ORDENAÇÃO)
    
    /**
     * Vetor temporário para armazenar categorias visíveis com suas posições X
     * Estrutura: par<posição_x, ponteiro_para_botão>
     * Permite ordenação espacial antes de adicionar à lista final
     */
    std::vector<std::pair<int, Botao*>> catsVisiveis;
    
    /**
     * Filtragem de Categorias Visíveis:
     * - Percorre todas as categorias
     * - Adiciona apenas aquelas com y > 0 (visíveis na tela)
     * - Armazena junto com sua posição X para ordenação posterior
     * 
     * IMPORTANTE: Isso resolve o PONTO 3 (seleção de itens ocultos)
     * Categorias fora da tela (y <= 0) não são navegáveis
     */
    for (auto& cat : categorias) {
        if (cat.area.y > 0) {
            catsVisiveis.push_back({cat.area.x, &cat});
        }
    }
    
    /**
     * Ordenação Espacial (Esquerda → Direita):
     * - Usa std::sort com lambda comparadora
     * - Compara posições X dos botões
     * - Garante navegação natural da esquerda para direita
     * 
     * IMPORTANTE: Isso resolve o PONTO 2 (navegação confusa)
     * A ordem visual na tela agora corresponde à ordem de navegação
     */
    std::sort(catsVisiveis.begin(), catsVisiveis.end(), 
        [](const std::pair<int, Botao*>& a, const std::pair<int, Botao*>& b) {
            return a.first < b.first; // Ordena por posição X crescente
        });

    /**
     * Adiciona categorias ordenadas à lista final
     * Agora estão em sequência espacial correta (esquerda → direita)
     */
    for (auto& par : catsVisiveis) {
        botoesNavegaveis.push_back(par.second);
    }

    // CAMADA 4: BOTÃO ALEATÓRIO
    /**
     * Adiciona botão de jogo aleatório
     * Posicionado no início da grade de jogos (posição 0,0)
     */
    if (btnAleatorio) 
        botoesNavegaveis.push_back(btnAleatorio);
    
    // CAMADA 5: GRADE DE JOGOS
    /**
     * Adiciona todos os jogos visíveis na grade
     * Ordem natural: esquerda → direita, topo → base
     */
    for (auto& jogo : jogos) 
        botoesNavegaveis.push_back(&jogo);
    
    return botoesNavegaveis;
}

/**
 * @brief Obtém um botão de categoria específico pelo seu índice
 *
 * Função auxiliar para acesso direto a categorias individuais.
 * Útil para navegação programática ou atalhos.
 *
 * @param indice Índice da categoria no vetor (0 = "Todas", 1 = "Aventura", etc.)
 * @return Ponteiro para o botão da categoria, ou nullptr se índice inválido
 */
Botao* GerenciarInterface::getBotaoCategoria(int indice) {
    if (indice >= 0 && indice < (int)categorias.size()) 
        return &categorias[indice];
    return nullptr;
}

/**
 * @brief Renderiza todos os elementos visuais da interface na tela
 *
 * Esta é a função principal de renderização que desenha todos os componentes da interface
 * em camadas específicas, respeitando a ordem de profundidade (z-order).
 *
 * Ordem de Renderização (do fundo para frente):
 * 1. Background de destaque principal
 * 2. Painel de fundo das categorias
 * 3. Botão aleatório
 * 4. Título "Categorias"
 * 5. Botões de categorias
 * 6. Setas de navegação de categorias
 * 7. Ícone do dado (sobreposto ao botão aleatório)
 * 8. Texto "Jogo Aleatório" (condicional)
 * 9. Grade de jogos
 * 10. Destaques (mini-capas)
 * 11. Popup (se aberto)
 * 12. Barra superior (overlay fixo)
 * 13. Botões do topo (Config, Pesquisa, Tema)
 * 14. Ícone de lupa
 * 15. Imagem explicativa
 * 16. Teclado virtual (se ativo)
 *
 * Sistema de Scroll:
 * - estado.scrollX e scrollY controlam o deslocamento da câmera
 * - Elementos fixos (topo) não são afetados pelo scroll
 * - Elementos do mundo são deslocados por -scrollX e -scrollY
 *
 * @param renderer Ponteiro para o renderizador SDL2
 * @param estado Referência ao estado de scroll e navegação da interface
 */
void GerenciarInterface::desenhar(SDL_Renderer* renderer, GerenciarScroll& estado) {
    // Validação do renderizador
    if (!renderer) return;

    // OBTENÇÃO DE CORES DO TEMA
    
    /**
     * Obtém referência ao gerenciador de temas (singleton)
     * Carrega paleta de cores do tema atual (claro/escuro)
     */
    auto& tema = GerenciadorTemas::getInstance();
    
    SDL_Color btnNorm = tema.getCorBotaoNormal();        // Cor padrão dos botões
    SDL_Color btnHover = tema.getCorBotaoHover();        // Cor ao passar o mouse
    SDL_Color btnPress = tema.getCorBotaoPressionado();  // Cor ao pressionar

    // CONFIGURAÇÃO DOS BOTÕES DO TOPO
    
    /**
     * Botões Config e MudarTema:
     * - Cor normal transparente {0,0,0,0} para mostrar apenas o ícone
     * - Mantém feedback visual em hover e press
     */
    btnConfig->setCor({0,0,0,0}, btnHover, btnPress);
    btnMudarTema->setCor({0,0,0,0}, btnHover, btnPress);
    
    /**
     * Botão Aleatório:
     * - Usa cores padrão do tema
     * - Tem fundo visível (não transparente)
     */
    btnAleatorio->setCor(btnNorm, btnHover, btnPress);

    /**
     * Atualização dinâmica do ícone de configuração:
     * - Seleciona textura apropriada baseada no tema atual
     * - Tema CLARO → iconeConfigClaro
     * - Tema ESCURO → iconeConfigEscuro
     */
    if (btnConfig) {
        SDL_Texture* configTex = (tema.getTemaAtual() == TipoTema::CLARO) ? 
                                  iconeConfigClaro : iconeConfigEscuro;
        btnConfig->setTexturaImagem(configTex);
    }
    
    /**
     * Reset de modulação de cor do botão de tema:
     * - Garante que o ícone apareça nas cores originais
     * - RGB(255,255,255) = sem modificação de cor
     */
    if (btnMudarTema->getTexturaImagem()) {
        SDL_SetTextureColorMod(btnMudarTema->getTexturaImagem(), 255, 255, 255);
    }

    // RENDERIZAÇÃO DO BACKGROUND DE DESTAQUE
    
    /**
     * Sistema Dual de Background:
     * 
     * 1. PRIORITÁRIO - Background direto por path:
     *    - Verifica se há um caminho específico definido
     *    - Carrega e renderiza a imagem diretamente
     * 
     * 2. FALLBACK - Sistema de índices:
     *    - Se path estiver vazio, usa comportamento antigo
     *    - Seleciona imagem baseada em currentDestaqueIndex
     *    - Mantém compatibilidade com código legado
     */
    if (!estado.backgroundImagePath.empty()) {
        // Modo 1: Path direto
        SDL_Texture* bg = gerImg.carregar(renderer, estado.backgroundImagePath);
        if (bg) {
            SDL_Rect fundoRect = {
                ConfigLayout::X(ConfigLayout::DESTAQUE_PRINCIPAL_POS_X) - estado.scrollX, 
                ConfigLayout::Y(ConfigLayout::DESTAQUE_PRINCIPAL_POS_Y) - estado.scrollY, 
                ConfigLayout::F(ConfigLayout::DESTAQUE_PRINCIPAL_LARGURA), 
                ConfigLayout::F(ConfigLayout::DESTAQUE_PRINCIPAL_ALTURA)
            };
            SDL_RenderCopy(renderer, bg, NULL, &fundoRect);
        }
    } else if (estado.imagensDestaques.size() > 0) {
        // Modo 2: Sistema de índices (fallback)
        if (estado.currentDestaqueIndex >= 0 && 
            estado.currentDestaqueIndex < (int)estado.imagensDestaques.size()) {
            SDL_Texture* bg = gerImg.carregar(renderer, 
                                               estado.imagensDestaques[estado.currentDestaqueIndex]);
            if (bg) {
                SDL_Rect fundoRect = {
                    ConfigLayout::X(ConfigLayout::DESTAQUE_PRINCIPAL_POS_X) - estado.scrollX, 
                    ConfigLayout::Y(ConfigLayout::DESTAQUE_PRINCIPAL_POS_Y) - estado.scrollY, 
                    ConfigLayout::F(ConfigLayout::DESTAQUE_PRINCIPAL_LARGURA), 
                    ConfigLayout::F(ConfigLayout::DESTAQUE_PRINCIPAL_ALTURA)
                };
                SDL_RenderCopy(renderer, bg, NULL, &fundoRect);
            }
        }
    }

    // FUNDO DO PAINEL DE CATEGORIAS
    
    /**
     * Renderiza o retângulo de fundo atrás das categorias:
     * - Atualiza cor baseada no tema atual
     * - Deslocado pelo scroll (parte do mundo rolável)
     */
    if (fundoCategorias) {
        fundoCategorias->mudarCor(tema.getCorRetangulos());
        fundoCategorias->desenhar(renderer, -estado.scrollX, -estado.scrollY);
    }
    
    // BOTÃO ALEATÓRIO (PRIMEIRA PASSADA)
    
    /**
     * Desenha o botão base do jogo aleatório
     * O ícone do dado será sobreposto posteriormente
     */
    if (btnAleatorio) 
        btnAleatorio->desenhar(renderer, -estado.scrollX, -estado.scrollY);
    
    // TÍTULO "CATEGORIAS"
    
    /**
     * Renderiza o texto "Categorias" acima do painel de categorias
     * - Cor do tema para texto em negrito
     * - Tamanho de fonte: 38 (escalado)
     * - Afetado pelo scroll
     */
    desenharTexto(renderer, "Categorias", 
        ConfigLayout::X(ConfigLayout::TITULO_CATEGORIA_POS_X) - estado.scrollX, 
        ConfigLayout::Y(ConfigLayout::TITULO_CATEGORIA_POS_Y) - estado.scrollY, 
        tema.getCorTextoNegrito(), ConfigLayout::F(38), TipoFonte::NEGRITO);

    // RENDERIZAÇÃO DAS CATEGORIAS
    
    int numCategorias = categorias.size();
    if (numCategorias > 0) {
        
        /**
         * Percorre todas as categorias e desenha apenas as visíveis
         * Sistema de coloração baseado em estado:
         */
        for (int i = 0; i < (int)categorias.size(); ++i) {
            // Verifica se a categoria está visível (y > 0)
            if (categorias[i].area.y > 0) {
                
                // --- LÓGICA DE COR (CORREÇÃO 2: SOMBRA BRANCA) ---
                
                bool isAtivo = false;

                /**
                 * Determina se a categoria está "ativa" (selecionada):
                 * 
                 * Caso 1: Categoria explicitamente selecionada
                 * - O índice atual coincide com categoriaAtivaIndex
                 * 
                 * Caso 2: Estado padrão
                 * - Nenhuma categoria selecionada (índice -1)
                 * - E estamos desenhando "Todas" (índice 0)
                 * - "Todas" é o padrão quando nada está selecionado
                 */
                if (i == estado.categoriaAtivaIndex) {
                    isAtivo = true;
                }
                else if (estado.categoriaAtivaIndex == -1 && i == 0) {
                    isAtivo = true;
                }

                /**
                 * Aplicação de Cores por Estado:
                 * 
                 * 1. ATIVA (Selecionada):
                 *    - Cor pressionada (roxo/cor do tema)
                 *    - Indica seleção atual
                 * 
                 * 2. FOCO TEMPORÁRIO (Por Gatilho):
                 *    - Cor de destaque especial
                 *    - Indica preview antes de selecionar
                 * 
                 * 3. NORMAL:
                 *    - Cor padrão de botão
                 *    - Estado não-selecionado
                 */
                if (isAtivo) {
                    // Categoria Ativa → Roxo/Cor do Tema
                    categorias[i].setCor(btnPress, btnHover, btnPress);
                } 
                else if (i == estado.categoriaFocadaPorGatilho) {
                    // Foco Temporário → Destaque
                    categorias[i].setCor(tema.getCorDestaque(), btnHover, btnPress);
                } 
                else {
                    // Normal
                    categorias[i].setCor(btnNorm, btnHover, btnPress);
                }

                // Renderiza a categoria com scroll aplicado
                categorias[i].desenhar(renderer, -estado.scrollX, -estado.scrollY);
            }
        }
    }

    // SETAS DE NAVEGAÇÃO DE CATEGORIAS
   
    /**
     * Renderiza as setas esquerda e direita para navegar entre categorias:
     * - Cores padrão do tema
     * - Cor de texto específica para melhor contraste
     */
    SDL_Color corTextoSeta = tema.getCorTextoNegrito();
    
    if (setaEsquerda) {
        setaEsquerda->setCor(btnNorm, btnHover, btnPress);
        setaEsquerda->setCorTexto(corTextoSeta);
        setaEsquerda->desenhar(renderer, -estado.scrollX, -estado.scrollY);
    }

    if (setaDireita) {
        setaDireita->setCor(btnNorm, btnHover, btnPress);
        setaDireita->setCorTexto(corTextoSeta);
        setaDireita->desenhar(renderer, -estado.scrollX, -estado.scrollY);
    }

    // BOTÃO ALEATÓRIO (SEGUNDA PASSADA - OVERLAY)
    
    if (btnAleatorio) {
        // Redesenha o botão base
        btnAleatorio->desenhar(renderer, -estado.scrollX, -estado.scrollY);
        
        SDL_Rect areaBtn = btnAleatorio->getArea();
        int tamanhoDado = ConfigLayout::F(150);
        
        /**
         * Renderização do Ícone do Dado:
         * - Centralizado sobre o botão aleatório
         * - Tamanho: 150px (escalado)
         * - Posição calculada relativa ao centro do botão
         */
        if (iconeDado) {
            SDL_Rect rectDado = {
                (areaBtn.x + areaBtn.w / 2) - (tamanhoDado / 2) - estado.scrollX,
                (areaBtn.y + areaBtn.h / 2) - (tamanhoDado / 2) - estado.scrollY,
                tamanhoDado,
                tamanhoDado
            };
            SDL_RenderCopy(renderer, iconeDado, NULL, &rectDado);
        }

        /**
         * Sistema de Texto Condicional "Jogo Aleatório":
         * 
         * O texto só aparece quando o usuário está interagindo com o botão.
         * Condições para mostrar:
         * 1. Botão está focado (navegação por controle/teclado)
         * 2. Mouse está sobre o botão (hover)
         * 3. Mouse está na área do botão (verificação por coordenadas)
         */
        bool deveMostrarTexto = false;

        // Verifica foco ou hover via métodos do botão
        if (btnAleatorio->isFocado() || btnAleatorio->isHover()) {
            deveMostrarTexto = true;
        }

        /**
         * Verificação adicional de posição do mouse:
         * - Obtém coordenadas do mouse na tela
         * - Converte para coordenadas do mundo (adiciona scroll)
         * - Verifica se o ponto está dentro do botão
         */
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        
        int mouseWorldX = mx + estado.scrollX;
        int mouseWorldY = my + estado.scrollY;

        if (btnAleatorio->contemPonto(mouseWorldX, mouseWorldY)) {
            deveMostrarTexto = true;
        }

        /**
         * Renderização do Texto "Jogo Aleatório":
         * - Apenas se deveMostrarTexto == true
         * - Posicionado abaixo do ícone do dado
         * - Centralizado horizontalmente no botão
         */
        if (deveMostrarTexto) {
            static GerenciadorFontes localFontes; 
            int tamanhoFonte = ConfigLayout::F(30); 
            std::string textoAleatorio = "Jogo Aleatório";

            TTF_Font* font = localFontes.carregar(
                ConfigFontes::obterCaminho(TipoFonte::NEGRITO), 
                tamanhoFonte
            );
            
            if (font) {
                // Calcula dimensões do texto
                int wText, hText;
                TTF_SizeUTF8(font, textoAleatorio.c_str(), &wText, &hText);

                // Centraliza horizontalmente no botão
                int xTexto = areaBtn.x + (areaBtn.w - wText) / 2 - estado.scrollX;
                
                // Posiciona abaixo do dado (centro + metade do dado + margem)
                int yTexto = (areaBtn.y + areaBtn.h / 2) + (tamanhoDado / 2) + 
                             ConfigLayout::Y(10) - estado.scrollY;

                desenharTexto(renderer, textoAleatorio, xTexto, yTexto, 
                             tema.getCorTextoNegrito(), tamanhoFonte, TipoFonte::NEGRITO);
            }
        }
    }    

    // GRADE DE JOGOS E DESTAQUES

    /**
     * Renderiza todos os jogos da grade atual
     * Afetados pelo scroll (parte do mundo rolável)
     */
    for (auto& jogo : jogos) 
        jogo.desenhar(renderer, -estado.scrollX, -estado.scrollY);
    
    /**
     * Renderiza os destaques (mini-capas no topo)
     * Afetados pelo scroll
     */
    for (auto& dest : destaques) 
        dest.desenhar(renderer, -estado.scrollX, -estado.scrollY);

    // JANELA POPUP
    
    /**
     * Renderiza popup se estiver aberto:
     * - Exibe informações detalhadas do jogo
     * - Sobrepõe todos os elementos anteriores
     * - Afetado pelo scroll
     */
    if (popupJanela && popupJanela->isPopupOpen()) {
        popupJanela->desenharPopup(-estado.scrollX, -estado.scrollY);
    }

    // BARRA SUPERIOR (OVERLAY FIXO)

    /**
     * Renderiza retângulo de fundo do topo da tela:
     * - Cor baseada no tema (cor de painéis)
     * - NÃO afetado pelo scroll (fixo na tela)
     * - Serve como base para botões do cabeçalho
     */
    SDL_Color corTopo = tema.getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, corTopo.r, corTopo.g, corTopo.b, corTopo.a);
    
    SDL_Rect rectTopo = {
        0, 
        0, 
        ConfigLayout::X(ConfigLayout::FUNDO_TOPO_LARGURA), 
        ConfigLayout::Y(ConfigLayout::FUNDO_TOPO_ALTURA)
    };
    SDL_RenderFillRect(renderer, &rectTopo);

    // BOTÕES DO TOPO (FIXOS)
    
    /**
     * Renderiza botões do cabeçalho:
     * - NÃO recebem offset de scroll (0, 0)
     * - Permanecem fixos no topo mesmo ao rolar a tela
     */
    if (btnConfig) 
        btnConfig->desenhar(renderer, 0, 0);
    if (btnPesquisa) 
        btnPesquisa->desenhar(renderer, 0, 0); 
    if (btnMudarTema) 
        btnMudarTema->desenhar(renderer, 0, 0);

    // ÍCONE DE LUPA
    
    /**
     * Renderiza ícone de lupa ao lado do campo de pesquisa:
     * - Seleciona ícone apropriado baseado no tema
     * - Posição fixa (não afetada por scroll)
     */
    SDL_Texture* lupaAtual = (tema.getTemaAtual() == TipoTema::CLARO) ? 
                              iconeLupaClaro : iconeLupaEscuro;
    if (lupaAtual) {
        SDL_Rect rectLupa = {
            ConfigLayout::X(ConfigLayout::ICONE_LUPA_POS_X), 
            ConfigLayout::Y(ConfigLayout::ICONE_LUPA_POS_Y), 
            ConfigLayout::F(ConfigLayout::ICONE_LUPA_TAMANHO), 
            ConfigLayout::F(ConfigLayout::ICONE_LUPA_TAMANHO)
        }; 
        SDL_RenderCopy(renderer, lupaAtual, NULL, &rectLupa);
    }

    // IMAGEM EXPLICATIVA DOS BOTÕES
    
    /**
     * Renderiza imagem explicativa no rodapé:
     * - Seleciona versão apropriada do tema (claro/escuro)
     * - Centralizada horizontalmente
     * - Alinhada ao fundo da tela
     * - Explica controles e funções dos botões
     */
    SDL_Texture* explicacaoAtual = (tema.getTemaAtual() == TipoTema::CLARO) ? 
                                    explicacaoClaro : explicacaoEscuro;
    
    if (explicacaoAtual) {
        // Obtém dimensões originais da imagem (esperado 1920x140 ou similar)
        int w = 0, h = 0;
        SDL_QueryTexture(explicacaoAtual, NULL, NULL, &w, &h);
        
        // Escala as dimensões
        int scaledW = ConfigLayout::F(w);
        int scaledH = ConfigLayout::F(h);
        
        // Posiciona no fundo do design nativo (Y=1280)
        // O topo da imagem será 1280 - h_nativo
        int posY = ConfigLayout::Y(1280 - h);
        
        SDL_Rect rectExplicacao = { 
            ConfigLayout::X(0),  // Alinhado ao início do canvas virtual
            posY,
            scaledW, 
            scaledH 
        };
        
        SDL_RenderCopy(renderer, explicacaoAtual, NULL, &rectExplicacao);
    }
    
    // TECLADO VIRTUAL
    
    /**
     * Renderiza teclado virtual se estiver ativo:
     * - Última camada (sobrepõe tudo)
     * - Usado para entrada de texto em pesquisas
     * - Geralmente ativado em dispositivos touch
     */
    if (tecladoVirtual) 
        tecladoVirtual->desenhar(renderer);
}