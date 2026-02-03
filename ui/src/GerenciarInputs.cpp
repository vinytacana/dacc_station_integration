/**
 * @file GerenciarInputs.cpp
 * @brief Implementação do núcleo de processamento de entradas (Mouse, Teclado e Gamepad).
 * 
 * Este arquivo contém a lógica que permite navegar pela biblioteca "Steam-like".
 * Ele gerencia o foco visual, a transição entre dispositivos de entrada, o sistema
 * de clique duplo em controles e o roteamento de ações para cada botão.
 * 
 * **Características principais:**
 * - Sistema de foco navegável para controles (D-Pad/Analógico)
 * - Detecção de clique duplo em gamepad
 * - Priorização hierárquica de inputs (Teclado Virtual > Busca > Interface)
 * - Suporte híbrido: Mouse/Teclado e Gamepad podem ser usados simultaneamente
 * - Sincronização automática de backgrounds com navegação
 */

#include "GerenciarInputs.hpp"
#include "GerenciadorJogos.hpp"
#include "GerenciadorImagens.hpp"
#include "JanelaJogo.hpp"
#include "Utils.hpp"
#include "TecladoVirtual.hpp"
#include "GerenciadorTemas.hpp"
#include "GerenciadorAudio.hpp"
#include "ConfigLayout.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Referência externa ao banco de dados de jogos carregado no main.
 * 
 * Permite acesso direto ao catálogo completo para busca e filtragem.
 */
extern GerenciadorJogos gerenciadorJogos;

/**
 * @brief Referência externa ao cache de imagens (texturas SDL).
 * 
 * Gerencia carregamento e liberação de capas, fundos e screenshots.
 */
extern GerenciadorImagens gerImg;

/** 
 * @brief Flag estática para controle de estado de navegação por gatilhos.
 * 
 * Indica se o usuário está navegando pela barra de categorias usando os
 * botões Shoulder (L1/R1 ou LB/RB). Quando ativa, D-Pad/Analógico navegam
 * horizontalmente nas categorias em vez do grid de jogos.
 */
static bool modoNavegacaoGatilho = false;

/**
 * @brief Construtor padrão da classe GerenciarInputs.
 * 
 * Inicializa membros com valores padrão. A abertura real do gamepad
 * ocorre em inicializarControle().
 */
GerenciarInputs::GerenciarInputs() {}

/**
 * @brief Destrutor que garante fechamento seguro do controle.
 * 
 * Chama fecharControle() para liberar recursos do SDL_GameController.
 */
GerenciarInputs::~GerenciarInputs() { 
    fecharControle(); 
}

/**
 * @brief Inicializa o primeiro Gamepad compatível encontrado pelo sistema.
 * 
 * Itera sobre todos os joysticks conectados e abre o primeiro que possua
 * mapeamento GameController (Xbox, PlayStation, Nintendo, etc).
 * 
 * @note Se nenhum controle for encontrado, gameController permanece nullptr
 *       e o sistema funciona apenas com mouse/teclado.
 */
void GerenciarInputs::inicializarControle() {
    int n = SDL_NumJoysticks();
    for(int i=0; i<n; i++) {
        /// Verifica se o dispositivo possui mapeamento padronizado
        if(SDL_IsGameController(i)) {
            gameController = SDL_GameControllerOpen(i);
            break; ///< Conecta apenas ao primeiro controle encontrado
        }
    }
}

/**
 * @brief Encerra a conexão com o controle e limpa o ponteiro.
 * 
 * Deve ser chamado antes de encerrar a SDL para evitar memory leaks
 * de handles de dispositivos de entrada.
 */
void GerenciarInputs::fecharControle() {
    if(gameController) { 
        SDL_GameControllerClose(gameController); 
        gameController = nullptr; 
    }
}

/**
 * @brief Método principal de atualização chamado a cada frame do loop principal.
 * 
 * **Fluxo de Processamento:**
 * 
 * 1. **Atualização de Estado:**
 *    - Recalcula posições dos botões baseadas no scroll atual
 *    - Reconstrói lista de botões navegáveis (jogos, categorias, UI)
 * 
 * 2. **Inicialização de Foco:**
 *    - Se nenhum botão está focado, foca automaticamente o primeiro
 *    - Sincroniza background com o primeiro jogo em destaque
 * 
 * 3. **Sistema de Clique Duplo (Gamepad):**
 *    - Gerencia timer para diferenciar clique simples (abrir detalhes) 
 *      de clique duplo (executar jogo diretamente)
 *    - Janela de 300ms (INTERVALO_DUPLO_CLIQUE) para segundo clique
 * 
 * 4. **Hierarquia de Prioridade de Inputs:**
 *    - **PRIORIDADE 1**: Teclado Virtual (overlay modal, consome todos eventos)
 *    - **PRIORIDADE 2**: Barra de Pesquisa (input de texto e lista de resultados)
 *    - **PRIORIDADE 3**: Distribuição padrão (Mouse, Teclado, Gamepad)
 * 
 * @param e Evento SDL capturado no frame atual.
 * @param s Estado global de scroll, navegação e flags.
 * @param i Interface gráfica (grid, categorias, UI).
 * @param r Renderizador SDL para desenho de elementos visuais.
 * 
 * @note Esta função é chamada uma vez por frame após SDL_PollEvent.
 */
void GerenciarInputs::atualizar(SDL_Event& e, GerenciarScroll& s, GerenciarInterface& i, SDL_Renderer* r) {
    /**
     * Atualiza coordenadas de todos os botões baseadas no scroll atual.
     * Necessário porque scroll move virtualmente todo o conteúdo.
     */
    i.atualizarPosicoes(s);
    
    /**
     * Reconstrói vetor de elementos navegáveis.
     * Garante que novos elementos (ex: jogos filtrados) sejam incluídos.
     */
    botoesNavegaveis = i.obterBotoesNavegaveis();

    /**
     * Inicialização automática de foco.
     * Se nenhum elemento está focado e há botões disponíveis,
     * foca o primeiro elemento automaticamente.
     */
    if (botaoFocadoIndex == -1 && !botoesNavegaveis.empty()) {
        botaoFocadoIndex = 0;
        botoesNavegaveis[0]->setFocado(true);
        
        /**
         * Sincroniza o background com o primeiro destaque.
         * Garante que interface inicie com visual coerente.
         */
        if (!s.imagensDestaques.empty()) {
            s.currentDestaqueIndex = 0;
            s.backgroundImagePath = s.imagensDestaques[0];
        }
    }

    // GERENCIADOR DE TEMPO: CLIQUE SIMPLES VS DUPLO (GAMEPAD)
    
    /**
     * Sistema de detecção de clique duplo para gamepad.
     * 
     * Quando o botão A é pressionado, inicia um timer:
     * - Se segundo clique ocorrer dentro de 300ms → Executa jogo
     * - Se timer expirar sem segundo clique → Abre janela de detalhes
     * 
     * Isso permite que usuários executem jogos rapidamente com duplo-clique,
     * mantendo acesso aos detalhes com clique único.
     */
    if (cliquePendente) {
        if (SDL_GetTicks() - tempoCliquePendente >= INTERVALO_DUPLO_CLIQUE) {
            /**
             * Timer expirou: executa ação padrão (abrir detalhes).
             * Parâmetro 'false' indica que NÃO é clique duplo.
             */
            executarAcaoBotao(botaoPendente, s, i, r, false);
            cliquePendente = false; 
            botaoPendente = nullptr;
        }
    }

    // PRIORIDADE 1: TECLADO VIRTUAL (OVERLAY MODAL)
    
    /**
     * Quando o teclado virtual está visível, ele consome TODOS os eventos.
     * Permite que usuário digite na busca usando controle ou mouse.
     * 
     * Sistema de bypass de eventos:
     * - Se teclado fechar durante processamento (OK/B pressionado),
     *   define flag ignorarProximoUpA para evitar duplo-trigger
     * - Isso previne que o BUTTONUP do OK dispare clique na busca
     */
    if (i.getTecladoVirtual() && i.getTecladoVirtual()->estaVisivel()) {
        /// Processa eventos de mouse
        if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION) {
            if (i.getTecladoVirtual()->processarMouse(e, i.getBotaoPesquisa())) return;
        }
        
        /// Processa eventos de controle
        if (e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERAXISMOTION) {
            /**
             * Captura estado antes do processamento para detectar fechamento.
             * Se teclado fechar (OK ou B pressionado), ativa bypass de eventos.
             */
            bool processou = i.getTecladoVirtual()->processarControle(e, i.getBotaoPesquisa());
            
            /**
             * Detecta se teclado foi fechado neste frame.
             * Se sim, configura flag para ignorar próximo BUTTONUP do botão A.
             */
            if (!i.getTecladoVirtual()->estaVisivel()) {
                if (e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
                    this->ignorarProximoUpA = true;
                } else {
                    this->ignorarProximoUpA = false;
                }
                
                /**
                 * Garante que foco retorne à barra de pesquisa após fechar teclado.
                 * Permite navegação imediata nos resultados com D-Pad.
                 */
                if (botaoFocadoIndex != -1 && botoesNavegaveis[botaoFocadoIndex] == i.getBotaoPesquisa()) {
                     botoesNavegaveis[botaoFocadoIndex]->setFocado(true);
                }
            }

            /// Se não processou evento, fecha o teclado
            if (!processou) {
                i.getTecladoVirtual()->fechar();
            }
            return; ///< Bloqueia propagação do evento
        }
    }
    
    // PRIORIDADE 2: BARRA DE PESQUISA (INPUT DE TEXTO)
    
    /**
     * Processa eventos da barra de pesquisa e lista de resultados.
     * 
     * Ações possíveis:
     * - Solicita teclado virtual (clique com controle)
     * - Processa texto digitado (teclado físico)
     * - Detecta clique em jogo da lista de resultados
     */
    if (i.getBotaoPesquisa() && i.getBotaoPesquisa()->tratarEvento(e, 0, 0)) {
        /**
         * Se barra de pesquisa solicitar teclado (clique com controle),
         * abre o teclado virtual automaticamente.
         */
        if (i.getBotaoPesquisa()->getSolicitaTeclado()) {
            if(i.getTecladoVirtual()) i.getTecladoVirtual()->abrir();
            i.getBotaoPesquisa()->resetarSolicitacaoTeclado();
        }
        
        /**
         * Verifica se um jogo foi clicado na lista de resultados.
         * Se sim, abre janela de detalhes desse jogo.
         */
        if (i.getBotaoPesquisa()->getJogoClicado()) {
             Jogo* j = i.getBotaoPesquisa()->getJogoClicado();
             
             /// Fecha janela anterior se existir
             if (s.janelaJogoAtual) delete s.janelaJogoAtual;
             
             /**
              * Monta descrição formatada com gênero e sinopse.
              * Utiliza stringstream para concatenação eficiente.
              */
             stringstream ss; 
             ss << "\nGênero: " << j->getCodigo() << "\nSinopse: " << j->getDescricaoLonga();
             
             /// Cria nova janela de detalhes com dados do jogo
             s.janelaJogoAtual = new JanelaJogo(r, j->getJogoExecutavel(), j->getNome(), 
                                                j->getCapaJanelaJogo(), ss.str(), j->getCapturas());
             i.getBotaoPesquisa()->resetarJogoClicado();
        }
        return; ///< Evento foi consumido pela pesquisa
    }

    // PRIORIDADE 3: DISTRIBUIÇÃO DE EVENTOS PADRÃO
    
    /**
     * Roteia eventos para os processadores específicos baseado no tipo.
     * Apenas executado se nenhum sistema prioritário consumiu o evento.
     */
    switch(e.type) {
        case SDL_KEYDOWN: 
            processarTeclado(e, s); 
            break;
            
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL: 
            processarMouse(e, s, i, r); 
            break;
            
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
        case SDL_CONTROLLERAXISMOTION:
            /**
             * Verifica novamente se teclado virtual não está aberto.
             * Dupla verificação para segurança (pode ter sido aberto durante processamento).
             */
            if (!i.getTecladoVirtual() || !i.getTecladoVirtual()->estaVisivel()) 
                processarGamepad(e, s, i, r);
            break;
    }
}

/**
 * @brief Processa atalhos de teclado físico para navegação e controle do sistema.
 * 
 * Implementa controles básicos de debug e navegação rápida:
 * - **Setas direcionais**: Scroll manual da tela (20px por tecla)
 * - **ESC**: Fecha configurações (se aberta) ou encerra aplicação
 * 
 * Após modificar scroll, aplica clamping para evitar que usuário
 * navegue para fora dos limites da página virtual.
 * 
 * @param evento Evento SDL_KEYDOWN capturado.
 * @param estado Estado global contendo valores de scroll e flags.
 * 
 * @note Limites de scroll: X[0, 0], Y[0, 1280] (página de 2560px de altura).
 */
void GerenciarInputs::processarTeclado(SDL_Event& evento, GerenciarScroll& estado) {
    switch (evento.key.keysym.sym) {
        case SDLK_LEFT:  estado.scrollX -= 20; break;
        case SDLK_RIGHT: estado.scrollX += 20; break;
        case SDLK_UP:    estado.scrollY -= 20; break;
        case SDLK_DOWN:  estado.scrollY += 20; break;
        case SDLK_ESCAPE: 
            /// ESC fecha configurações se abertas, senão encerra programa
            if(estado.configAberta) estado.configAberta = false; 
            else estado.rodando = false; 
            break;
    }
    
    /**
     * Clamping para evitar que o scroll saia dos limites da página.
     * Dimensões da página virtual: 1920x2560 (visível: 1920x1280).
     */
    estado.scrollX = max(0, min(estado.scrollX, 1920 - 1920)); ///< X sempre 0 (sem scroll horizontal)
    estado.scrollY = max(0, min(estado.scrollY, 2560 - 1280)); ///< Y de 0 a 1280
}

/**
 * @brief Processa todas as interações via Mouse (movimento, cliques, scroll).
 * 
 * **Lógica de Interação Hierárquica:**
 * 
 * 1. **Scroll de Roda do Mouse:**
 *    - Vertical: Navegação na página (wheel.y)
 *    - Horizontal: Movimentação lateral (wheel.x)
 *    - Valor multiplicado por 20 para velocidade confortável
 * 
 * 2. **Componentes da Barra Superior:**
 *    - Configurações, Tema, Pesquisa
 *    - Processados primeiro por estarem sempre visíveis
 * 
 * 3. **Carrossel de Categorias:**
 *    - Setas laterais para navegação
 *    - Atualiza índice com wraparound (volta ao início após última)
 * 
 * 4. **Grid de Jogos:**
 *    - Itera sobre todos os botões navegáveis
 *    - Detecta hover e cliques
 *    - Executa ações específicas por tipo de botão
 * 
 * 5. **Miniaturas de Destaque:**
 *    - Hover muda background imediatamente
 *    - Feedback visual instantâneo para exploração
 * 
 * @param evento Evento SDL de mouse capturado.
 * @param estado Estado global de scroll e navegação.
 * @param interface Gerenciador de interface gráfica.
 * @param renderer Renderizador SDL para elementos visuais.
 * 
 * @note Coordenadas do mouse são compensadas pelo scroll atual (mx, my).
 * @note Se configurações estão abertas, mouse é ignorado (early return).
 */
void GerenciarInputs::processarMouse(SDL_Event& evento, GerenciarScroll& estado, 
                                     GerenciarInterface& interface, SDL_Renderer* renderer) {
    
    /// Bloqueia input de mouse quando janela de configurações está aberta
    if (estado.configAberta) return;
    
    /**
     * Processa scroll da roda do mouse.
     * wheel.y: Scroll vertical (positivo = cima, negativo = baixo)
     * wheel.x: Scroll horizontal (raro, trackpads/magic mouse)
     */
    if (evento.type == SDL_MOUSEWHEEL) {
        estado.scrollY -= evento.wheel.y * 20; ///< Multiplica por 20 para velocidade adequada
        estado.scrollX -= evento.wheel.x * 20;
        /// Aplica limites após modificação
        estado.scrollY = max(0, min(estado.scrollY, 2560 - 1280));
    }

    /**
     * Calcula posição real do mouse no espaço virtual.
     * Compensa o scroll atual para detecção correta de colisão.
     * 
     * Exemplo: Se scroll = 100 e mouse = 50, posição real = 150
     */
    int mx = evento.motion.x + estado.scrollX; 
    int my = evento.motion.y + estado.scrollY;
    
    /// Detecta clique completo (button press + release)
    bool clicou = (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT);

    // PROCESSAMENTO DA BARRA SUPERIOR (UI FIXA)
    
    /**
     * Processa eventos de hover e clique para botões da barra superior.
     * Estes botões não são afetados por scroll (posição fixa na tela).
     */
    interface.getBotaoConfig()->tratarEvento(evento, 0, 0);
    interface.getBotaoMudarTema()->tratarEvento(evento, 0, 0);
    interface.getBotaoPesquisa()->tratarEvento(evento, 0, 0);
    
    /// Executa ações dos botões se foram clicados
    if (clicou) {
        if (interface.getBotaoConfig()->contemPonto(evento.button.x, evento.button.y)) { 
            abrirConfiguracoes(estado);  
        }
        if (interface.getBotaoMudarTema()->contemPonto(evento.button.x, evento.button.y)) {
            GerenciadorTemas::getInstance().alternarTema();
        }
    }

    // SETAS DE NAVEGAÇÃO DE CATEGORIAS
    
    /**
     * Processa cliques nas setas laterais do carrossel de categorias.
     * Offset negativo (-scrollX, -scrollY) compensa scroll para detecção correta.
     */
    Botao* se = interface.getSetaEsquerda(); 
    Botao* sd = interface.getSetaDireita();
    if(se) se->tratarEvento(evento, -estado.scrollX, -estado.scrollY);
    if(sd) sd->tratarEvento(evento, -estado.scrollX, -estado.scrollY);
    
    if(clicou) {
        int n = interface.getNumCategorias();
        
        /// Seta esquerda: volta categoria com wraparound
        if(se && se->contemPonto(mx, my)) { 
            estado.categoriaIndex = (estado.categoriaIndex - 1 + n) % n; 
            interface.atualizarPosicoes(estado); 
        }
        /// Seta direita: avança categoria com wraparound
        else if(sd && sd->contemPonto(mx, my)) { 
            estado.categoriaIndex = (estado.categoriaIndex + 1) % n; 
            interface.atualizarPosicoes(estado); 
        }
    }

    // GRID DE JOGOS E BOTÕES NAVEGÁVEIS
    
    /**
     * Itera sobre todos os botões do grid (jogos, categorias, etc).
     * Processa hover e cliques para cada elemento navegável.
     */
    for(Botao* b : botoesNavegaveis) {
        /**
         * Pula botões da barra superior já processados anteriormente.
         * Evita processamento duplicado e conflitos de eventos.
         */
        if(b == interface.getBotaoConfig() || b == interface.getBotaoMudarTema() || 
           b == interface.getBotaoPesquisa()) continue;
        
        /// Processa hover visual (muda cor, brilho, etc)
        b->tratarEvento(evento, -estado.scrollX, -estado.scrollY);

        /// Se clicado, executa ação específica do botão
        if(clicou && b->contemPonto(mx, my)) { 
            executarAcaoBotao(b, estado, interface, renderer); 
        }
    }
    
    // MINIATURAS DE DESTAQUE (HERO BANNERS)
    
    /**
     * Detecta hover sobre miniaturas de destaque na parte superior.
     * Troca background imediatamente para feedback visual instantâneo.
     * 
     * Permite que usuário explore visualmente os jogos em destaque
     * sem necessidade de cliques, apenas movimentando o mouse.
     */
    const auto& d = interface.getDestaques();
    for(size_t i = 0; i < d.size(); i++) {
        Botao& b = const_cast<Botao&>(d[i]);
        
        /// Verifica se mouse está sobre esta miniatura
        if(b.contemPonto(mx, my)) { 
            /// Atualiza background se há imagem válida para este índice
            if(i < estado.imagensDestaques.size()) {
                estado.currentDestaqueIndex = i; 
                estado.backgroundImagePath = estado.imagensDestaques[i];
            }
            break; ///< Para após encontrar primeira miniatura sob o mouse
        }
    }
}

/**
 * @brief Implementa a lógica de navegação inteligente para Controles (10-foot UI).
 * 
 * Este sistema permite navegação por D-Pad ou Analógico em uma interface complexa
 * dividida em múltiplas zonas lógicas. A navegação é otimizada para experiência
 * de TV/Console (10-foot interface), onde o usuário está distante da tela.
 * 
 * **ARQUITETURA DE ZONAS:**
 * 
 * O vetor `botoesNavegaveis` é organizado em grupos sequenciais para navegação lógica:
 * 
 * ```
 * [Índices]              [Grupo]      [Descrição]
 * ─────────────────────────────────────────────────────────────────────
 * [0 .. D-1]             Grupo 0      Destaques (Mini banners superiores)
 * [D]                    Grupo 1      Config (Botão de configurações)
 * [D+1]                  Grupo 1      Pesquisa (Barra de busca)
 * [D+2]                  Grupo 1      Mudar Tema (Alternador de tema)
 * [D+3 .. D+3+C-1]       Grupo 2      Categorias (Navegadas via LB/RB)
 * [D+3+C]                Grupo 3      Aleatório (Botão de jogo aleatório)
 * [D+3+C+1 .. Fim]       Grupo 4      Jogos (Grid principal 5 colunas)
 * ```
 * 
 * **SISTEMA DE NAVEGAÇÃO POR ZONAS:**
 * 
 * - **Zona 0 (Destaques)**: Navegação horizontal livre, saltos verticais para Config/Grade
 * - **Zona 1 (Barra Superior)**: Navegação horizontal limitada, ciclo com Destaques
 * - **Zona 2 (Categorias)**: PULADA pela navegação direcional (usa LB/RB)
 * - **Zona 3 (Grade)**: Navegação em grid 5 colunas com scroll automático
 * 
 * **REGRAS DE TRANSIÇÃO:**
 * 
 * Vertical (Cima/Baixo):
 * - Destaques ↕ Grade: Salto direto ignorando barra e categorias
 * - Barra → Qualquer direção: Retorna aos Destaques
 * - Grade: Movimentação por linhas (5 itens por linha)
 * 
 * Horizontal (Esquerda/Direita):
 * - Dentro de cada zona: Movimentação linear
 * - Limites das zonas: Ciclo para outras zonas relevantes
 * - Categorias são ignoradas (acessadas apenas por LB/RB)
 * 
 * @param dx Direção Horizontal: -1 (Esquerda), 0 (Nenhuma), 1 (Direita)
 * @param dy Direção Vertical: -1 (Cima), 0 (Nenhuma), 1 (Baixo)
 * @param estado Estado global de scroll e navegação.
 * @param interface Gerenciador de interface para acesso aos botões.
 * 
 * @note A função implementa throttling de input via podeProcessarInput().
 * @note Scroll é ajustado automaticamente para manter botão focado visível.
 * @note Sons de navegação são reproduzidos em transições bem-sucedidas.
 */
void GerenciarInputs::navegarComControle(int dx, int dy, GerenciarScroll& estado, GerenciarInterface& interface) {
    /**
     * Throttling de input para evitar navegação excessivamente rápida.
     * Garante intervalo mínimo entre comandos (INTERVALO_INPUT_CONTROLE).
     */
    if (!estado.podeProcessarInput(INTERVALO_INPUT_CONTROLE)) return;
    
    /// Validação básica: precisa haver botões para navegar
    if (botoesNavegaveis.empty()) return;

    /**
     * Desativa modo de navegação por gatilhos ao usar direcional.
     * Restaura foco visual do botão atual quando retorna da barra de categorias.
     */
    if (modoNavegacaoGatilho) {
        modoNavegacaoGatilho = false;
        if (botaoFocadoIndex >= 0 && botaoFocadoIndex < (int)botoesNavegaveis.size())
            botoesNavegaveis[botaoFocadoIndex]->setFocado(true);
    }

    /**
     * Inicialização de foco se nenhum botão estiver focado.
     * Foca automaticamente o primeiro elemento (geralmente primeiro destaque).
     */
    if (botaoFocadoIndex == -1) { 
        botaoFocadoIndex = 0; 
        botoesNavegaveis[0]->setFocado(true); 
        return; 
    }

    /// Remove foco visual do botão atual antes de calcular próximo
    botoesNavegaveis[botaoFocadoIndex]->setFocado(false);

    // DETECÇÃO DINÂMICA DE ÍNDICES DOS GRUPOS
    
    /**
     * Busca índices dos elementos-chave no vetor de navegáveis.
     * Necessário porque ordem pode variar com filtragem/busca.
     */
    int numDestaques = interface.getNumDestaques();
    
    int idxConfig = -1;     ///< Índice do botão de configurações
    int idxAleatorio = -1;  ///< Índice do botão aleatório (início da grade)

    /// Obtém referências aos botões especiais
    Botao* btnConfig = interface.getBotaoConfig();
    Botao* btnAleatorio = interface.getBotaoAleatorio();
    Botao* btnTema = interface.getBotaoMudarTema();

    /**
     * Percorre vetor procurando os botões especiais.
     * Usa comparação de ponteiros para identificação.
     */
    for(size_t i = 0; i < botoesNavegaveis.size(); i++) {
        if(botoesNavegaveis[i] == btnConfig) idxConfig = i;
        if(botoesNavegaveis[i] == btnAleatorio) idxAleatorio = i;
    }
    
    /**
     * Fallback de segurança caso botões não sejam encontrados.
     * Previne crashes em estados inconsistentes da interface.
     */
    if (idxAleatorio == -1) idxAleatorio = botoesNavegaveis.size() - 1;
    if (idxConfig == -1) idxConfig = 0;

    int idxInicioGrade = idxAleatorio; ///< Grade começa no botão aleatório
    int idxUltimoItem = botoesNavegaveis.size() - 1; ///< Último jogo do catálogo
    
    int colunas = 5; ///< Número de colunas do grid de jogos

    int current = botaoFocadoIndex; ///< Índice atual
    int next = current;              ///< Próximo índice (será calculado)

    // IDENTIFICAÇÃO DA ZONA ATUAL
    
    /**
     * Determina em qual zona lógica o foco atual se encontra:
     * - Zone 0: Destaques (mini banners horizontais)
     * - Zone 1: Barra Superior (Config, Pesquisa, Tema)
     * - Zone 2: Categorias (navegadas por LB/RB, não por direcional)
     * - Zone 3: Grade (Aleatório + Grid de jogos)
     */
    int zone = -1;
    
    if (current < idxConfig) {
        zone = 0; ///< Antes do Config = Destaques
    }
    else if (botoesNavegaveis[current] == btnConfig || 
             botoesNavegaveis[current] == interface.getBotaoPesquisa() || 
             botoesNavegaveis[current] == btnTema) {
        zone = 1; ///< É um dos botões da barra superior
    }
    else if (current < idxAleatorio) {
        zone = 2; ///< Entre barra superior e aleatório = Categorias
    }
    else {
        zone = 3; ///< A partir do aleatório = Grade de jogos
    }

    // LÓGICA DE NAVEGAÇÃO VERTICAL (CIMA ↑ / BAIXO ↓)
    
    if (dy != 0) {
        if (dy < 0) { // ═══ MOVIMENTO PARA CIMA ═══
            
            if (zone == 0) {
                /**
                 * Destaques → Config
                 * Salto direto para barra superior.
                 */
                next = idxConfig; 
            }
            else if (zone == 3) { // Grade de Jogos
                /**
                 * Calcula posição no grid (linha e coluna).
                 * Índice relativo = posição atual - início da grade.
                 */
                int indiceRelativo = current - idxInicioGrade;
                int linha = indiceRelativo / colunas;

                if (linha == 0) {
                    /**
                     * PRIMEIRA LINHA da grade (Aleatório ou Jogos 0-4):
                     * Sobe direto para primeiro Destaque, ignorando barra e categorias.
                     * Implementa atalho para navegação rápida.
                     */
                    next = 0;
                } else {
                    /**
                     * LINHAS INTERMEDIÁRIAS:
                     * Sobe uma linha no grid (subtrai número de colunas).
                     */
                    next = current - colunas;
                }
            }
            else if (zone == 1 || zone == 2) {
                /**
                 * Barra Superior ou Categorias → Destaques
                 * Retorna para os mini banners.
                 */
                next = 0; 
            }
        } 
        else { // ═══ MOVIMENTO PARA BAIXO ═══
            
            if (zone == 0) {
                /**
                 * Destaques → Aleatório (Início da Grade)
                 * Salto direto ignorando barra superior e categorias.
                 */
                next = idxInicioGrade; 
            }
            else if (zone == 1 || zone == 2) { 
                /**
                 * Barra Superior ou Categorias → Destaques
                 * Movimento para baixo retorna aos mini banners.
                 */
                next = 0; 
            }
            else if (zone == 3) { // Grade
                /**
                 * Desce uma linha no grid se não estiver na última linha.
                 * Verifica se índice resultante não ultrapassa último item.
                 */
                if (current + colunas <= idxUltimoItem) {
                    next = current + colunas;
                }
                // Se estiver na última linha, permanece onde está (next = current)
            }
        }
    } 
    
    // LÓGICA DE NAVEGAÇÃO HORIZONTAL (ESQUERDA ← / DIREITA →)
    
    else if (dx != 0) {
        if (dx < 0) { // ═══ MOVIMENTO PARA ESQUERDA ═══
            
            if (zone == 0) { // Destaques
                if (current == 0) {
                    /**
                     * PRIMEIRO DESTAQUE → Config
                     * Ciclo: esquerda no limite vai para barra superior.
                     */
                    next = idxConfig; 
                } else {
                    /**
                     * DESTAQUES INTERMEDIÁRIOS:
                     * Navegação horizontal normal dentro dos mini banners.
                     */
                    next = current - 1; 
                }
            }
            else if (zone == 1) { // Barra Superior
                if (botoesNavegaveis[current] == btnConfig) {
                    /**
                     * Config → Primeiro Destaque
                     * Ciclo: esquerda no Config retorna aos banners.
                     */
                    next = 0; 
                } else {
                    /**
                     * Pesquisa/Tema:
                     * Move para o botão anterior na barra (Config ou Pesquisa).
                     */
                    next = current - 1; 
                }
            }
            else if (zone == 3) { // Grade
                if (current == idxInicioGrade) {
                    /**
                     * ALEATÓRIO (início da grade) → Primeiro Destaque
                     * Ciclo rápido para retornar aos banners.
                     */
                    next = 0; 
                } else {
                    /**
                     * JOGOS NO GRID:
                     * Move para jogo à esquerda (coluna anterior).
                     */
                    next = current - 1;
                }
            }
            // Zone 2 (Categorias) não tem lógica horizontal (navegadas por LB/RB)
        } 
        else { // ═══ MOVIMENTO PARA DIREITA ═══
            
            if (zone == 0) { // Destaques
                if (current == numDestaques - 1) {
                    /**
                     * ÚLTIMO DESTAQUE → Aleatório (Início da Grade)
                     * Ciclo: direita no último banner vai para a grade.
                     * Implementa atalho para navegação eficiente.
                     */
                    next = idxInicioGrade; 
                } else {
                    /**
                     * DESTAQUES INTERMEDIÁRIOS:
                     * Navegação horizontal normal dentro dos mini banners.
                     */
                    next = current + 1; 
                }
            }
            else if (zone == 1) { // Barra Superior
                if (botoesNavegaveis[current] == btnTema) {
                    /**
                     * Tema (último da barra) → Primeiro Destaque
                     * Ciclo: direita no último botão retorna aos banners.
                     */
                    next = 0; 
                } else {
                    /**
                     * Config/Pesquisa:
                     * Move para próximo botão na barra (Pesquisa ou Tema).
                     */
                    next = current + 1; 
                }
            }
            else if (zone == 3) { // Grade
                /**
                 * JOGOS NO GRID:
                 * Move para jogo à direita se não for o último item.
                 */
                if (current < idxUltimoItem) next = current + 1; 
                // Se for o último jogo, permanece onde está
            }
        }
    }

    // FINALIZAÇÃO DA NAVEGAÇÃO
    
    /**
     * Reproduz som de feedback apenas se houve mudança de foco.
     * Evita sons repetitivos quando usuário tenta navegar além dos limites.
     */
    if (next != current) {
        gerAudio.tocarSom("navegacao.wav");
    }

    /**
     * Aplica novo foco e atualiza background se necessário.
     */
    botaoFocadoIndex = next;
    Botao* btn = botoesNavegaveis[botaoFocadoIndex];
    btn->setFocado(true);

    /**
     * Sincroniza background com destaque focado.
     * Se foco está em um dos mini banners, atualiza imagem de fundo.
     */
    if (botaoFocadoIndex < numDestaques) {
        estado.currentDestaqueIndex = botaoFocadoIndex;
        if (botaoFocadoIndex < (int)estado.imagensDestaques.size())
            estado.backgroundImagePath = estado.imagensDestaques[botaoFocadoIndex];
    }

    // SCROLL AUTOMÁTICO PARA MANTER BOTÃO FOCADO VISÍVEL
    
    SDL_Rect rectBtn = btn->getArea(); ///< Área do botão focado

    /**
     * Ajuste de scroll para região superior da tela.
     * Se botão estiver muito próximo do topo (< 200px), ajusta scroll para cima.
     */
    if (rectBtn.y < estado.scrollY + ConfigLayout::F(200)) {
        estado.scrollY = rectBtn.y - ConfigLayout::F(200);
    } 
    /**
     * Ajuste de scroll para região inferior da tela.
     * Se botão estiver muito próximo do fundo (> altura - 200px), ajusta scroll para baixo.
     */
    else if (rectBtn.y + rectBtn.h > estado.scrollY + ConfigLayout::F(1280 - 200)) {
        estado.scrollY = (rectBtn.y + rectBtn.h) - ConfigLayout::F(1280 - 200);
    }

    /**
     * TRAVA CRÍTICA: Impede scroll negativo.
     * ScrollY negativo causaria "efeito esticado" no topo da página,
     * mostrando área vazia acima do conteúdo.
     */
    if (estado.scrollY < 0) estado.scrollY = 0;

    /**
     * Trava o limite inferior baseado no tamanho total do conteúdo.
     * Altura total: 2560px, Altura visível: 1280px → Scroll máximo: 1280px
     */
    int limiteMaxScroll = ConfigLayout::F(2560 - 1280);
    if (estado.scrollY > limiteMaxScroll) estado.scrollY = limiteMaxScroll;
}

/**
 * @brief Processa interações vindas especificamente de Controles (Gamepads).
 * 
 * @details **Lógica de Mapeamento de Botões:**
 * - **Barra de Pesquisa:** Se o foco estiver nela, o D-Pad navega pelos resultados suspensos.
 * - **Botão A:** Confirma a seleção, abre o teclado virtual ou executa o jogo.
 * - **Botão B:** Funciona como "Voltar" ou "Sair".
 * - **Botão X:** Atalho rápido para focar na Pesquisa e abrir o Teclado Virtual.
 * - **Botão Y:** Atalho para selecionar um jogo aleatório.
 * - **LB/RB (Shoulders):** Navegam exclusivamente pelas Categorias (Filtros).
 * - **LT/RT (Triggers):** Navegam exclusivamente pelos Banners de Destaque (Topo).
 * 
 * @param evento Referência ao evento SDL de controle.
 * @param estado Estado global de navegação e janelas.
 * @param interface Gerenciador de componentes visuais.
 * @param renderer Renderizador (necessário para instanciar novas janelas).
 */
void GerenciarInputs::processarGamepad(SDL_Event& evento, GerenciarScroll& estado, GerenciarInterface& interface, SDL_Renderer* renderer) {
    
    // 1. INTERAÇÃO ESPECIAL COM A BARRA DE PESQUISA
    
    if (evento.type == SDL_CONTROLLERBUTTONDOWN || evento.type == SDL_CONTROLLERAXISMOTION) {
        
        // BOTÃO B: Voltar / Cancelar Pesquisa
        /**
         * Prioriza o fechamento do teclado virtual e cancelamento da pesquisa
         * antes de processar outras ações do botão B.
         * 
         * Verifica se o teclado está aberto OU se há resultados de pesquisa visíveis.
         * Se algum deles estiver ativo, fecha/cancela e retorna sem processar mais nada.
         */
        if (evento.type == SDL_CONTROLLERBUTTONDOWN && evento.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
            
            // Verifica usando os métodos públicos GETTERS para encapsulamento
            bool tecladoAberto = (interface.getTecladoVirtual() && interface.getTecladoVirtual()->estaVisivel());
            bool pesquisaAberta = (interface.getBotaoPesquisa() && interface.getBotaoPesquisa()->temResultadosVisiveis());

            if (tecladoAberto || pesquisaAberta) {
                if (interface.getTecladoVirtual()) interface.getTecladoVirtual()->fechar();
                if (interface.getBotaoPesquisa()) interface.getBotaoPesquisa()->cancelarBusca();
                return; 
            }
        }

        // Navegação nos Resultados da Pesquisa
        /**
         * Se o botão de pesquisa está focado, permite navegação pelos resultados
         * usando D-Pad ou analógico esquerdo (eixo Y).
         * 
         * - DOWN: Avança para o próximo resultado
         * - UP: Volta para o resultado anterior (apenas se já houver um resultado focado)
         */
        if (botaoFocadoIndex != -1 && botoesNavegaveis[botaoFocadoIndex] == interface.getBotaoPesquisa()) {
            BotaoPesquisa* btnPesquisa = interface.getBotaoPesquisa();
            
            // Detecta movimento para baixo (D-Pad ou Analógico)
            // Threshold de 16000 para evitar movimentos acidentais (dead zone)
            bool down = (evento.type == SDL_CONTROLLERBUTTONDOWN && evento.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
                        (evento.type == SDL_CONTROLLERAXISMOTION && evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY && evento.caxis.value > 16000);
            
            // Detecta movimento para cima
            bool up = (evento.type == SDL_CONTROLLERBUTTONDOWN && evento.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) ||
                      (evento.type == SDL_CONTROLLERAXISMOTION && evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY && evento.caxis.value < -16000);
            
            // Navega para baixo e retorna se a navegação foi bem-sucedida
            if (down) { if (btnPesquisa->navegarResultados(1)) return; }
            
            // Só navega para cima se já houver um resultado focado
            if (up) { if (btnPesquisa->temResultadoFocado()) { btnPesquisa->navegarResultados(-1); return; } }
            

            // Botão A: Confirmar Seleção do Resultado da Pesquisa

            /**
             * Quando o botão A é pressionado com um resultado focado,
             * abre a janela de detalhes do jogo selecionado.
             * 
             * Cria uma nova JanelaJogo com as informações do jogo e a atribui
             * ao estado global para ser renderizada no loop principal.
             */
            if (evento.type == SDL_CONTROLLERBUTTONDOWN && evento.cbutton.button == SDL_CONTROLLER_BUTTON_A && btnPesquisa->temResultadoFocado()) {
                Jogo* jogo = btnPesquisa->getJogoResultadoFocado();
                if (jogo) {
                    // Libera janela anterior se existir para evitar memory leak
                    if (estado.janelaJogoAtual) delete estado.janelaJogoAtual;
        
                    // Monta a descrição completa do jogo
                    stringstream ss; 
                    ss << "Título: " << jogo->getNome() << "\nGênero: " << jogo->getCodigo() << "\nSinopse: " << jogo->getDescricaoLonga();
                    
                    // Cria a nova janela de detalhes do jogo
                    // O ponteiro é atribuído ao estado para que o loop principal desenhe e processe
                    estado.janelaJogoAtual = new JanelaJogo(renderer, jogo->getJogoExecutavel(), jogo->getNome(), 
                                                            jogo->getCapaJanelaJogo(), ss.str(), jogo->getCapturas());

                }
                return;
            }

        }
    }

    // 2. PROCESSAMENTO DE BOTÕES DE AÇÃO
    
    if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (evento.cbutton.button) {
            

            // BOTÃO A: Confirmar / Selecionar

            /**
             * Comportamento principal do botão A:
             * - Se estava em modo de navegação por gatilho, desativa esse modo
             * - Ativa visualmente o botão focado
             * - Implementa sistema de clique duplo para executar jogos
             * - Para botões de sistema (pesquisa), executa imediatamente
             * 
             * Sistema de Clique Duplo:
             * - Primeiro pressionar: Inicia timer (tratado no BUTTONUP)
             * - Segundo pressionar rápido: Executa direto sem timer
             */
            case SDL_CONTROLLER_BUTTON_A:
                // Se estava em modo gatilho, apenas desativa e continua o processamento normal
                if (modoNavegacaoGatilho) {
                    modoNavegacaoGatilho = false;
                }

                if (botaoFocadoIndex != -1) {
                    Botao* focado = botoesNavegaveis[botaoFocadoIndex];
                    focado->ativarPorControle(); // Feedback visual
                    
                    /**
                     * @section Lógica_Clique_Duplo
                     * Se o usuário pressionar 'A' e já houver um clique pendente,
                     * executa DIRETO (executarDireto = true).
                     */
                    if (cliquePendente && botaoPendente == focado) {
                        executarAcaoBotao(focado, estado, interface, renderer, true);
                        cliquePendente = false; 
                        botaoPendente = nullptr; 
                        ignorarProximoUpA = true; // Evita processar o BUTTONUP correspondente
                    } else {
                        ignorarProximoUpA = false;
                        // Ação normal (clique simples) para botões de sistema (Pesquisa, etc)
                        // Para Jogos, a ação real acontece no BUTTONUP (via timer), não aqui.
                        if(focado == interface.getBotaoPesquisa()) {
                            executarAcaoBotao(focado, estado, interface, renderer);
                        }
                    }
                }
                break;


            // BOTÃO B: Voltar / Cancelar / Sair

            /**
             * Hierarquia de prioridades do botão B:
             * 1. Se há uma janela de jogo aberta -> Fecha a janela
             * 2. Se o menu de configurações está aberto -> Fecha as configurações
             * 3. Caso contrário -> Sai da aplicação
             */
            case SDL_CONTROLLER_BUTTON_B: 
                 if(estado.janelaJogoAtual) { 
                     delete estado.janelaJogoAtual; 
                     estado.janelaJogoAtual=nullptr; 
                 }
                 else if(estado.configAberta) {
                     estado.configAberta=false;
                 }
                 else {
                     estado.rodando=false; // Encerra o loop principal
                 }
                 break;


            // BOTÃO X: Atalho para Pesquisa

            /**
             * Atalho rápido que:
             * - Remove o foco do botão atual
             * - Foca diretamente no campo de pesquisa
             * - Abre automaticamente o teclado virtual
             * 
             * Permite acesso rápido à pesquisa de qualquer lugar da interface.
             */
            case SDL_CONTROLLER_BUTTON_X: {
                BotaoPesquisa* bp = interface.getBotaoPesquisa();
                if(bp) {
                    // Remove foco do botão atual
                    if(botaoFocadoIndex!=-1) botoesNavegaveis[botaoFocadoIndex]->setFocado(false);
                    
                    // Localiza o índice do botão de pesquisa no array de navegáveis
                    for(size_t i=0; i<botoesNavegaveis.size(); i++) {
                        if(botoesNavegaveis[i]==bp) { 
                            botaoFocadoIndex=i; 
                            break; 
                        }
                    }
                    
                    bp->setFocado(true);
                    if(interface.getTecladoVirtual()) interface.getTecladoVirtual()->abrir();
                }
                break;
            }


            // BOTÃO START: Abrir Menu de Configurações

            case SDL_CONTROLLER_BUTTON_START: 
                abrirConfiguracoes(estado); 
                break;


            // BOTÃO Y: Jogo Aleatório

            /**
             * Atalho para selecionar e abrir um jogo aleatório.
             * Delega a execução para o botão aleatório da interface.
             */
            case SDL_CONTROLLER_BUTTON_Y: 
                if(interface.getBotaoAleatorio()) {
                    executarAcaoBotao(interface.getBotaoAleatorio(), estado, interface, renderer);
                }
                break;


            // D-PAD: Navegação Direcional

            /**
             * Navegação básica pelos elementos da interface usando o D-Pad.
             * Cada direção chama navegarComControle com os deslocamentos apropriados:
             * - UP: (0, -1) - Move para cima
             * - DOWN: (0, 1) - Move para baixo
             * - LEFT: (-1, 0) - Move para esquerda
             * - RIGHT: (1, 0) - Move para direita
             */
            case SDL_CONTROLLER_BUTTON_DPAD_UP:    navegarComControle(0, -1, estado, interface); break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  navegarComControle(0, 1, estado, interface); break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  navegarComControle(-1, 0, estado, interface); break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: navegarComControle(1, 0, estado, interface); break;


            // LB (Left Shoulder): Navegar Categorias para Esquerda

            /**
             * Navegação rápida pelas categorias usando o shoulder esquerdo.
             * 
             * Comportamento:
             * - Ativa o modo de navegação por gatilho (desativa foco normal)
             * - Move para a categoria anterior (circular)
             * - Ajusta automaticamente a visualização do carrossel
             * - Aplica scroll automático para manter a categoria visível
             * - Atualiza a grade de jogos com o novo filtro
             * - Toca som de navegação como feedback
             * 
             * Sistema de debounce impede ativações múltiplas muito rápidas.
             */
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: {
                modoNavegacaoGatilho = true;
                if(botaoFocadoIndex != -1) botoesNavegaveis[botaoFocadoIndex]->setFocado(false);
                
                // Debounce: Só processa se passou tempo suficiente desde a última ativação
                if(SDL_GetTicks() - ultimoTempoLB > DEBOUNCE_GATILHOS) {
                    int n = interface.getNumCategorias();
                    
                    // Inicializa na última categoria se ainda não há foco
                    if(estado.categoriaFocadaPorGatilho == -1) {
                        estado.categoriaFocadaPorGatilho = n-1;
                    }
                    else {
                        // Move para categoria anterior (circular)
                        estado.categoriaFocadaPorGatilho = (estado.categoriaFocadaPorGatilho - 1 + n) % n;
                    }
                    
                    // Ajusta a visualização do carrossel para mostrar a categoria selecionada
                    int rel = estado.categoriaFocadaPorGatilho - estado.categoriaIndex;
                    if(rel < 0) rel += n; // Ajuste para navegação circular
                    if(rel >= 5) estado.categoriaIndex = estado.categoriaFocadaPorGatilho;
                    
                    Botao* catSelecionada = interface.getBotaoCategoria(estado.categoriaFocadaPorGatilho);
                    if(catSelecionada) {
                        gerAudio.tocarSom("navegacao.wav");
                        
                        // Verifica se é a categoria "Todas" (índice 0)
                        if (estado.categoriaFocadaPorGatilho == 0) {
                            estado.categoriaAtual = "Todas"; 
                            estado.categoriaAtivaIndex = -1;
                        } else {
                            // Array com nomes das categorias (deve corresponder à ordem real)
                            vector<string> nomes = {"Todas", "Aventura", "Casual", "Acao", "RPG", "Estrategia", "Esportes"};
                            if (estado.categoriaFocadaPorGatilho < (int)nomes.size()) {
                                estado.categoriaAtual = nomes[estado.categoriaFocadaPorGatilho];
                            }
                            estado.categoriaAtivaIndex = estado.categoriaFocadaPorGatilho;
                        }
                        
                        // Recarrega a grade de jogos com o novo filtro de categoria
                        interface.atualizarJogosPorCategoria(estado.categoriaAtual, renderer);

                        // --- SCROLL AUTOMÁTICO ---
                        /**
                         * Calcula e aplica scroll automático para garantir que
                         * a categoria selecionada fique visível na tela.
                         * 
                         * - Pega a posição Y absoluta do botão
                         * - Subtrai 250px para deixar espaço para o título "Categorias"
                         * - Limita a 0 para não ter scroll negativo
                         */
                        int yAbsolutoBtn = catSelecionada->getArea().y;
                        int novoScroll = yAbsolutoBtn - 250;
                        if (novoScroll < 0) novoScroll = 0;
                        estado.scrollY = novoScroll;
                    }
                    
                    ultimoTempoLB = SDL_GetTicks();
                }
                break;
            }


            // RB (Right Shoulder): Navegar Categorias para Direita

            /**
             * Navegação rápida pelas categorias usando o shoulder direito.
             * 
             * Funcionalidade idêntica ao LB, mas navega para a próxima categoria
             * ao invés da anterior. Mantém toda a mesma lógica de:
             * - Modo de navegação por gatilho
             * - Ajuste de visualização do carrossel
             * - Scroll automático
             * - Atualização da grade de jogos
             * - Feedback sonoro
             */
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: {
                modoNavegacaoGatilho = true;
                if(botaoFocadoIndex != -1) botoesNavegaveis[botaoFocadoIndex]->setFocado(false);
                
                if(SDL_GetTicks() - ultimoTempoRB > DEBOUNCE_GATILHOS) {
                    int n = interface.getNumCategorias();
                    
                    // Inicializa na primeira categoria se ainda não há foco
                    if(estado.categoriaFocadaPorGatilho == -1) {
                        estado.categoriaFocadaPorGatilho = 0;
                    }
                    else {
                        // Move para próxima categoria (circular)
                        estado.categoriaFocadaPorGatilho = (estado.categoriaFocadaPorGatilho + 1) % n;
                    }
                    
                    // Ajusta visualização do carrossel
                    int rel = estado.categoriaFocadaPorGatilho - estado.categoriaIndex;
                    if(rel < 0) rel += n;
                    if(rel >= 5) estado.categoriaIndex = (estado.categoriaIndex + 1) % n;
                    
                    Botao* catSelecionada = interface.getBotaoCategoria(estado.categoriaFocadaPorGatilho);
                    if(catSelecionada) {
                        gerAudio.tocarSom("navegacao.wav");
                        
                        if (estado.categoriaFocadaPorGatilho == 0) {
                            estado.categoriaAtual = "Todas"; 
                            estado.categoriaAtivaIndex = -1;
                        } else {
                            vector<string> nomes = {"Todas", "Aventura", "Casual", "Acao", "RPG", "Estrategia", "Esportes"};
                            if (estado.categoriaFocadaPorGatilho < (int)nomes.size()) {
                                estado.categoriaAtual = nomes[estado.categoriaFocadaPorGatilho];
                            }
                            estado.categoriaAtivaIndex = estado.categoriaFocadaPorGatilho;
                        }
                        
                        interface.atualizarJogosPorCategoria(estado.categoriaAtual, renderer);

                        // --- SCROLL AUTOMÁTICO ---
                        /**
                         * Centraliza visualmente o botão de categoria selecionado,
                         * deixando espaço para o título "Categorias" aparecer acima.
                         */
                        int yAbsolutoBtn = catSelecionada->getArea().y;
                        int novoScroll = yAbsolutoBtn - 250;
                        if (novoScroll < 0) novoScroll = 0;
                        estado.scrollY = novoScroll;
                    }
                    
                    ultimoTempoRB = SDL_GetTicks();
                }
                break;
            }

        }
    } 
    
    // PROCESSAMENTO DE BUTTON UP (Soltar Botão)
    /**
     * Processa quando o botão A é solto.
     * 
     * Sistema de Timer para Clique Duplo:
     * - Ao soltar o botão A, desativa o feedback visual
     * - Se não deve ignorar este UP (não foi clique duplo):
     *   - Se não há clique pendente, inicia o processo de detecção de clique duplo
     *   - Marca o botão e o tempo para comparação futura
     */
    else if (evento.type == SDL_CONTROLLERBUTTONUP) {
        if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
            if (!modoNavegacaoGatilho && botaoFocadoIndex != -1) {
                botoesNavegaveis[botaoFocadoIndex]->desativarPorControle();
                
                // Se deve ignorar este UP (foi clique duplo), apenas reseta a flag
                if(ignorarProximoUpA) {
                    ignorarProximoUpA = false; 
                }
                // Senão, inicia o sistema de timer para clique duplo
                else if(!cliquePendente) {
                    cliquePendente = true;
                    botaoPendente = botoesNavegaveis[botaoFocadoIndex];
                    tempoCliquePendente = SDL_GetTicks();
                }
            }
        }
    }

    // 3. PROCESSAMENTO DE EIXOS ANALÓGICOS
    
    if (evento.type == SDL_CONTROLLERAXISMOTION) {
        
        // Analógico Esquerdo: Navegação
        /**
         * Permite navegação pelos botões usando o analógico esquerdo.
         * 
         * - Eixo X (horizontal): Esquerda/Direita
         * - Eixo Y (vertical): Cima/Baixo
         * 
         * Dead zone de 16000 (aproximadamente 50% do range) evita
         * movimentos acidentais por drift ou pequenas inclinações.
         */
        if (evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX || evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
            if (abs(evento.caxis.value) > 16000) {
                int dx = 0, dy = 0;
                
                if(evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                    dx = (evento.caxis.value > 0) ? 1 : -1; // Direita = 1, Esquerda = -1
                }
                else {
                    dy = (evento.caxis.value > 0) ? 1 : -1; // Baixo = 1, Cima = -1
                }
                
                navegarComControle(dx, dy, estado, interface);
            }
        }
        
        // LT (Left Trigger): Trocar Banner de Destaque (Anterior)
        /**
         * Navegação pelos banners de destaque usando o gatilho esquerdo.
         * 
         * Comportamento:
         * - Move para o banner anterior (circular)
         * - Desativa modo de navegação por gatilho
         * - Sincroniza o foco visual com o banner selecionado
         * - Atualiza a imagem de fundo do destaque
         * 
         * Debounce de 500ms evita mudanças muito rápidas.
         * Threshold de 16000 garante que o gatilho está pressionado significativamente.
         */
        else if (evento.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT && evento.caxis.value > 16000) {
            if (SDL_GetTicks() - ultimoTempoLT > 500) {
                if (!estado.imagensDestaques.empty()) {
                    // Move para banner anterior (circular)
                    estado.currentDestaqueIndex = (estado.currentDestaqueIndex - 1 + estado.imagensDestaques.size()) % estado.imagensDestaques.size();
                    
                    // Desativa modo gatilho e atualiza foco
                    modoNavegacaoGatilho = false;
                    if (botaoFocadoIndex != -1) botoesNavegaveis[botaoFocadoIndex]->setFocado(false);
                    
                    // Sincroniza foco com o índice do banner
                    botaoFocadoIndex = estado.currentDestaqueIndex;
                    if (botaoFocadoIndex < (int)botoesNavegaveis.size()) {
                        botoesNavegaveis[botaoFocadoIndex]->setFocado(true);
                    }

                    // Atualiza a imagem de fundo
                    estado.backgroundImagePath = estado.imagensDestaques[estado.currentDestaqueIndex];
                }
                ultimoTempoLT = SDL_GetTicks();
            }
        }
        
        // RT (Right Trigger): Trocar Banner de Destaque (Próximo)
        /**
         * Navegação pelos banners de destaque usando o gatilho direito.
         * 
         * Funcionalidade idêntica ao LT, mas avança para o próximo banner
         * ao invés de voltar. Mantém toda a mesma lógica de:
         * - Navegação circular
         * - Sincronização de foco
         * - Atualização de imagem de fundo
         * - Sistema de debounce
         */
        else if (evento.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT && evento.caxis.value > 16000) {
            if (SDL_GetTicks() - ultimoTempoRT > 500) {
                if (!estado.imagensDestaques.empty()) {
                    // Move para próximo banner (circular)
                    estado.currentDestaqueIndex = (estado.currentDestaqueIndex + 1) % estado.imagensDestaques.size();
                    
                    modoNavegacaoGatilho = false;
                    if (botaoFocadoIndex != -1) botoesNavegaveis[botaoFocadoIndex]->setFocado(false);
                    
                    botaoFocadoIndex = estado.currentDestaqueIndex;
                    if (botaoFocadoIndex < (int)botoesNavegaveis.size()) {
                        botoesNavegaveis[botaoFocadoIndex]->setFocado(true);
                    }

                    estado.backgroundImagePath = estado.imagensDestaques[estado.currentDestaqueIndex];
                }
                ultimoTempoRT = SDL_GetTicks();
            }
        }
    }
}

/**
 * @brief Roteador de Ações dos Botões.
 * 
 * @details **Lógica de Execução de Ações:**
 * Este é o método que efetivamente "faz as coisas acontecerem" quando um botão é clicado.
 * 
 * **Fluxo de Processamento:**
 * 1. **Sistema:** Abre configurações, alterna temas ou abre o teclado de busca.
 * 2. **Aleatório:** Sorteia um índice no `GerenciadorJogos` e abre a janela de detalhes do sorteado.
 * 3. **Filtros de Categoria:** Identifica qual botão de categoria foi pressionado e solicita ao 
 *    `GerenciarInterface` que reconstrua a grade de jogos baseada no novo filtro.
 * 4. **Jogos (Ação Principal):**
 *    - Busca o objeto `Jogo` correspondente ao nome vinculado ao botão.
 *    - **Se `executarDireto` for FALSE:** Cria uma nova instância de `JanelaJogo` (Popup) 
 *      com descrição, capturas e botões interativos.
 *    - **Se `executarDireto` for TRUE (Clique Duplo):** Instancia uma `JanelaJogo` temporária 
 *      apenas para invocar o método `executarJogo()`, que dispara o processo no SO.
 * 
 * @param botao Ponteiro para o componente que disparou a ação.
 * @param estado Estado global para modificação de janelas e filtros.
 * @param interface Necessário para reconstruir elementos visuais.
 * @param renderer Renderizador para criação de texturas e janelas.
 * @param executarDireto Se TRUE, inicia o jogo no SO imediatamente (clique duplo).
 */
void GerenciarInputs::executarAcaoBotao(Botao* botao, GerenciarScroll& estado, GerenciarInterface& interface, SDL_Renderer* renderer, bool executarDireto) {

    if (!botao) return;

    // AÇÕES DE INTERFACE DO SISTEMA
    // Botão de Configuração
    /**
     * Abre o menu de configurações da aplicação.
     * Toca um som de feedback e chama a função dedicada para ativar
     * o estado de configurações abertas.
     */
    if (botao == interface.getBotaoConfig()) {
        gerAudio.tocarSom("abrir_config.wav");
        abrirConfiguracoes(estado);
        return;
    }    
    // Botão de Alternar Tema
    /**
     * Alterna entre os temas visuais disponíveis (claro/escuro).
     * Utiliza o padrão Singleton do GerenciadorTemas para manter
     * consistência global da aparência.
     */
    if (botao == interface.getBotaoMudarTema()) {
        gerAudio.tocarSom("mudar_tema.wav"); 
        GerenciadorTemas::getInstance().alternarTema(); 
        return;
    }    

    // Botão de Pesquisa
    /**
     * Ativa o campo de pesquisa e abre o teclado virtual automaticamente.
     * Permite busca rápida de jogos na biblioteca.
     */
    if (botao == interface.getBotaoPesquisa()) {
        gerAudio.tocarSom("abrir_pesquisa.wav"); 
        if(interface.getTecladoVirtual()) interface.getTecladoVirtual()->abrir();
        return;
    }
    
    // LÓGICA DO BOTÃO ALEATÓRIO
    /**
     * Seleciona e abre um jogo aleatório da biblioteca.
     * 
     * Comportamento:
     * - Se o filtro atual é "Todas" ou vazio: Sorteia entre todos os jogos
     * - Se há categoria ativa: Sorteia apenas entre jogos daquela categoria
     * 
     * Após sortear, cria uma JanelaJogo com os detalhes do jogo selecionado,
     * incluindo capa, sinopse e capturas de tela.
     */
    if (botao == interface.getBotaoAleatorio()) {
        vector<Jogo*> jogosCandidatos;
        gerAudio.tocarSom("aleatorio.wav");

        // 1. Definição dos candidatos
        if (estado.categoriaAtual == "Todas" || estado.categoriaAtual.empty()) {
            vector<Jogo> todos = gerenciadorJogos.listarJogos();
            for(size_t i = 0; i < todos.size(); i++) {
                jogosCandidatos.push_back(gerenciadorJogos.obterJogoPorIndice(i));
            }
        } else {
            jogosCandidatos = gerenciadorJogos.buscarPorCategoria(estado.categoriaAtual);
        }

        // 2. Lógica de sorteio unificada
        if (!jogosCandidatos.empty()) {
            int idx = rand() % jogosCandidatos.size();
            Jogo* j = jogosCandidatos[idx];
            
            if(j) {
                if (estado.janelaJogoAtual) delete estado.janelaJogoAtual;
                
                stringstream ss;
                ss << "Gênero: " << j->getCodigo() << "\n" << "Sinopse: " << j->getDescricaoLonga() << "\n";
                
                estado.janelaJogoAtual = new JanelaJogo(renderer, j->getJogoExecutavel(), 
                                                    j->getNome(), j->getCapaJanelaJogo(), 
                                                    ss.str(), j->getCapturas());
            }
        }
        return; // Finaliza o processamento do botão aleatório
    }
    

    // IDENTIFICAÇÃO DE CLIQUES EM CATEGORIAS (FILTROS)
    /**
     * Processa cliques nos botões de categoria para filtrar a grade de jogos.
     * 
     * Sistema de Toggle:
     * - Clicar na categoria "Todas" (índice 0): Sempre reseta para "Todas"
     * - Clicar em uma categoria já ativa: Desativa o filtro (volta para "Todas")
     * - Clicar em categoria diferente: Ativa o novo filtro
     * 
     * Após alterar o filtro, solicita à interface que reconstrua a grade
     * com apenas os jogos da categoria selecionada.
     */
    const auto& cats = interface.getCategorias();
    for (size_t i = 0; i < cats.size(); ++i) {
        if (botao == &cats[i]) {
            gerAudio.tocarSom("navegacao.wav");
        
            // Categoria "Todas" (índice 0
            if (i == 0) {
                // Sempre reseta para exibir todos os jogos
                estado.categoriaAtual = "Todas";
                estado.categoriaAtivaIndex = -1; // -1 indica nenhum filtro ativo
            
            // Categoria já estava ativa (Toggle OFF
            } else if (estado.categoriaAtivaIndex == (int)i) {
                // Desativa o filtro voltando para "Todas"
                estado.categoriaAtual = "Todas";
                estado.categoriaAtivaIndex = -1;
            
            // Ativar nova categori
            } else {
                // Array com nomes das categorias (deve corresponder à ordem dos botões)
                vector<string> nomes = {"Todas", "Aventura", "Casual", "Acao", "RPG", "Estrategia", "Esportes"};
                if (i < nomes.size()) {
                    estado.categoriaAtual = nomes[i];
                    estado.categoriaAtivaIndex = i;
                }
            }
            
            // Atualiza a grade de jogos com o novo filtro
            interface.atualizarJogosPorCategoria(estado.categoriaAtual, renderer);
            return;
        }
    }

    // AÇÃO SOBRE UM JOGO DA BIBLIOTECA
    /**
     * Processa cliques em cards de jogos na grade principal.
     * 
     * Identifica o jogo através do nome armazenado no botão e então:
     * - Se executarDireto = TRUE (clique duplo): Executa o jogo imediatamente
     * - Se executarDireto = FALSE (clique simples): Abre a janela de detalhes
     * 
     * A janela de detalhes exibe informações completas do jogo antes de executá-lo.
     */
    if (!botao->getNomeJogo().empty()) {
        vector<Jogo> todos = gerenciadorJogos.listarJogos();
        
        // Busca o jogo correspondente ao nome do botão
        for (auto& jogo : todos) {
            if (jogo.getNome() == botao->getNomeJogo()) {
                
                // Modo: Execução Direta (Clique Duplo)
                if (executarDireto) {
                    /** 
                     * @section EXECUTAR_JOGO
                     * Simula o comportamento de "Play" imediato. Instancia a lógica
                     * e chama o system() definido na JanelaJogo.
                     * 
                     * Cria uma janela temporária apenas para acessar o método executarJogo(),
                     * que dispara o processo do jogo no sistema operacional.
                     */
                    JanelaJogo tempJanela(renderer, jogo.getJogoExecutavel(), jogo.getNome(), jogo.getCapaJanelaJogo(), "", jogo.getCapturas());
                    tempJanela.executarJogo();
                } 
                // Modo: Abrir Página de Detalhes (Clique Simples)
                else {
                    /**
                     * @section ABRIR_PAGINA_DETALHES
                     * Abre a interface interna com sinopse e screenshots antes de jogar.
                     * 
                     * Permite que o usuário visualize:
                     * - Capa do jogo em alta resolução
                     * - Gênero e sinopse completa
                     * - Carrossel de capturas de tela
                     * - Botão "Jogar" para executar quando desejado
                     */
                    // Libera janela anterior se existir
                    if (estado.janelaJogoAtual) delete estado.janelaJogoAtual;
                    
                    // Monta a descrição formatada com gênero e sinopse
                    stringstream ss;
                    ss << "Gênero: " << jogo.getCodigo() << "\n" << "Sinopse: " << jogo.getDescricaoLonga() << "\n";
                    
                    // Cria a janela de detalhes do jogo
                    estado.janelaJogoAtual = new JanelaJogo(renderer, jogo.getJogoExecutavel(), jogo.getNome(), jogo.getCapaJanelaJogo(), ss.str(), jogo.getCapturas());
                }
                return;
            }
        }
    }
}


/**
 * @brief Ativa o estado de abertura da janela de configurações.
 * 
 * Função auxiliar que altera o estado global para exibir o menu de configurações.
 * Se as configurações já estiverem abertas, não faz nada (evita redundância).
 * 
 * @param estado Referência ao estado global da aplicação
 */
void GerenciarInputs::abrirConfiguracoes(GerenciarScroll& estado) {
    if (!estado.configAberta) estado.configAberta = true;
}