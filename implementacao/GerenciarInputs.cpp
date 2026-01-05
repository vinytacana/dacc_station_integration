/**
 * @file GerenciarInputs.cpp
 * @brief Implementação do núcleo de processamento de entradas (Mouse, Teclado e Gamepad).
 * 
 * Este arquivo contém a lógica que permite navegar pela biblioteca "Steam-like".
 * Ele gerencia o foco visual, a transição entre dispositivos de entrada, o sistema
 * de clique duplo em controles e o roteamento de ações para cada botão.
 */

#include "GerenciarInputs.hpp"
#include "GerenciadorJogos.hpp"
#include "GerenciadorImagens.hpp"
#include "JanelaJogo.hpp"
#include "Utils.hpp"
#include "TecladoVirtual.hpp" 
#include "GerenciadorTemas.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>

using namespace MeuProjeto;
using namespace std;

/** @brief Referência externa ao banco de dados de jogos carregado no main. */
extern GerenciadorJogos gerenciadorJogos;
/** @brief Referência externa ao cache de imagens. */
extern GerenciadorImagens gerImg;

/** 
 * @brief Flag estática para controle de estado. 
 * Indica se o usuário está navegando pela barra de categorias usando os Gatilhos/Shoulders.
 */
static bool modoNavegacaoGatilho = false;

GerenciarInputs::GerenciarInputs() {}
GerenciarInputs::~GerenciarInputs() { fecharControle(); }

/**
 * @brief Inicializa o primeiro Gamepad compatível encontrado pelo sistema.
 */
void GerenciarInputs::inicializarControle() {
    int n = SDL_NumJoysticks();
    for(int i=0; i<n; i++) {
        if(SDL_IsGameController(i)) {
            gameController = SDL_GameControllerOpen(i);
            break;
        }
    }
}

/**
 * @brief Encerra a conexão com o controle e limpa o ponteiro.
 */
void GerenciarInputs::fecharControle() {
    if(gameController) { SDL_GameControllerClose(gameController); gameController=nullptr; }
}

/**
 * @brief Método principal de atualização chamado a cada frame do loop principal.
 * 
 * @details **Fluxo de Processamento:**
 * 1. Atualiza as posições dos botões baseadas no scroll atual.
 * 2. Reconstrói o vetor de `botoesNavegaveis` para garantir que o foco considere elementos novos.
 * 3. **Lógica de Clique Pendente:** Gerencia o tempo para diferenciar um clique simples (abrir detalhes) 
 *    de um clique duplo (executar jogo diretamente) no controle.
 * 4. **Prioridade de Input:**
 *    - Se o Teclado Virtual estiver aberto, ele consome todos os eventos primeiro.
 *    - Se a Barra de Pesquisa estiver ativa, ela processa eventos de texto.
 *    - Caso contrário, os eventos são distribuídos para Mouse, Teclado ou Gamepad.
 */
void GerenciarInputs::atualizar(SDL_Event& e, GerenciarScroll& s, GerenciarInterface& i, SDL_Renderer* r) {
    i.atualizarPosicoes(s);
    botoesNavegaveis = i.obterBotoesNavegaveis();

    // Gerenciador de Tempo para Clique Simples vs Duplo (Controle)
    if (cliquePendente) {
        if (SDL_GetTicks() - tempoCliquePendente >= INTERVALO_DUPLO_CLIQUE) {
            // Se o tempo expirou e não houve segundo clique, executa a ação normal (abrir popup)
            executarAcaoBotao(botaoPendente, s, i, r, false);
            cliquePendente = false; botaoPendente = nullptr;
        }
    }

    // PRIORIDADE 1: Teclado Virtual (Overlay)
    if (i.getTecladoVirtual() && i.getTecladoVirtual()->estaVisivel()) {
        if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION) {
            if (i.getTecladoVirtual()->processarMouse(e, i.getBotaoPesquisa())) return;
        }
        if (e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERAXISMOTION) {
            if (!i.getTecladoVirtual()->processarControle(e, i.getBotaoPesquisa())) {
                i.getTecladoVirtual()->fechar();
            }
            return;
        }
    }
    
    // PRIORIDADE 2: Barra de Pesquisa (Input de Texto)
    if (i.getBotaoPesquisa() && i.getBotaoPesquisa()->tratarEvento(e, 0, 0)) {
        // Se a barra de pesquisa pedir teclado (clique via controle), abre o virtual
        if (i.getBotaoPesquisa()->getSolicitaTeclado()) {
            if(i.getTecladoVirtual()) i.getTecladoVirtual()->abrir();
            i.getBotaoPesquisa()->resetarSolicitacaoTeclado();
        }
        // Se um jogo foi clicado na lista de resultados da pesquisa
        if (i.getBotaoPesquisa()->getJogoClicado()) {
             Jogo* j = i.getBotaoPesquisa()->getJogoClicado();
             if (s.janelaJogoAtual) delete s.janelaJogoAtual;
             stringstream ss; ss  << "\nGênero: " << j->getCodigo() << "\nSinopse: " << j->getDescricao();
             s.janelaJogoAtual = new JanelaJogo(r, j->getNome(), j->getCapaJanelaJogo(), ss.str(), j->getCapturas());
             i.getBotaoPesquisa()->resetarJogoClicado();
        }
        return;
    }

    // PRIORIDADE 3: Distribuição de Eventos Padrão
    switch(e.type) {
        case SDL_KEYDOWN: processarTeclado(e, s); break;
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL: processarMouse(e, s, i, r); break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
        case SDL_CONTROLLERAXISMOTION:
            if (!i.getTecladoVirtual() || !i.getTecladoVirtual()->estaVisivel()) 
                processarGamepad(e, s, i, r);
            break;
    }
}

/**
 * @brief Processa atalhos de teclado físico.
 * Utilizado principalmente para scroll rápido e saída do sistema.
 */
void GerenciarInputs::processarTeclado(SDL_Event& evento, GerenciarScroll& estado) {
    switch (evento.key.keysym.sym) {
        case SDLK_LEFT:  estado.scrollX -= 20; break;
        case SDLK_RIGHT: estado.scrollX += 20; break;
        case SDLK_UP:    estado.scrollY -= 20; break;
        case SDLK_DOWN:  estado.scrollY += 20; break;
        case SDLK_ESCAPE: 
            if(estado.configAberta) estado.configAberta=false; 
            else estado.rodando=false; 
            break;
    }
    // Clamping para evitar que o scroll saia dos limites da página
    estado.scrollX = max(0, min(estado.scrollX, 1920 - 1920));
    estado.scrollY = max(0, min(estado.scrollY, 2560 - 1280));
}

/**
 * @brief Processa todas as interações via Mouse.
 * 
 * @details **Lógica de Interação:**
 * 1. **Scroll:** A roda do mouse altera os valores de scroll global.
 * 2. **Componentes Fixos:** Trata eventos para Configurações, Tema e Pesquisa.
 * 3. **Carrossel de Categorias:** Verifica cliques nas setas laterais para girar a lista.
 * 4. **Grid de Jogos:** Itera sobre os botões navegáveis (jogos) para detectar cliques.
 * 5. **Destaques:** Atualiza o banner principal se o mouse passar sobre as miniaturas de destaque.
 */
void GerenciarInputs::processarMouse(SDL_Event& evento, GerenciarScroll& estado, GerenciarInterface& interface, SDL_Renderer* renderer) {
    if (evento.type == SDL_MOUSEWHEEL) {
        estado.scrollY -= evento.wheel.y * 20;
        estado.scrollX -= evento.wheel.x * 20;
        estado.scrollY = max(0, min(estado.scrollY, 2560 - 1280));
    }

    // Calcula a posição do mouse compensando o scroll da tela
    int mx = evento.motion.x + estado.scrollX; 
    int my = evento.motion.y + estado.scrollY;
    
    bool clicou = (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT);
    bool clicouEmAlgo = false;

    // Processa a barra superior
    interface.getBotaoConfig()->tratarEvento(evento, 0, 0);
    interface.getBotaoMudarTema()->tratarEvento(evento, 0, 0);
    interface.getBotaoPesquisa()->tratarEvento(evento, 0, 0);
    
    if (clicou) {
        if (interface.getBotaoConfig()->contemPonto(evento.button.x, evento.button.y)) { 
            abrirConfiguracoes(estado, renderer); clicouEmAlgo=true; 
        }
        if (interface.getBotaoMudarTema()->contemPonto(evento.button.x, evento.button.y)) {
            GerenciadorTemas::getInstance().alternarTema();
            clicouEmAlgo=true;
        }
    }

    // Setas de Categorias (Cálculo de scroll horizontal relativo)
    Botao* se = interface.getSetaEsquerda(); Botao* sd = interface.getSetaDireita();
    if(se) se->tratarEvento(evento, -estado.scrollX, -estado.scrollY);
    if(sd) sd->tratarEvento(evento, -estado.scrollX, -estado.scrollY);
    
    if(clicou) {
        int n = interface.getNumCategorias();
        if(se && se->contemPonto(mx, my)) { 
            estado.categoriaIndex = (estado.categoriaIndex - 1 + n)%n; 
            interface.atualizarPosicoes(estado); 
            clicouEmAlgo=true; 
        }
        else if(sd && sd->contemPonto(mx, my)) { 
            estado.categoriaIndex = (estado.categoriaIndex + 1)%n; 
            interface.atualizarPosicoes(estado); 
            clicouEmAlgo=true; 
        }
    }

    // Itera por todos os botões (Jogos, Categorias, etc)
    for(Botao* b : botoesNavegaveis) {
        // Pula botões da barra superior já processados
        if(b==interface.getBotaoConfig() || b==interface.getBotaoMudarTema() || b==interface.getBotaoPesquisa()) continue;
        
        b->tratarEvento(evento, -estado.scrollX, -estado.scrollY);
        if(clicou && b->contemPonto(mx, my)) { 
            executarAcaoBotao(b, estado, interface, renderer); 
            clicouEmAlgo=true; 
        }
    }
    
    // Troca manual dos Banners de Destaque por Hover
    const auto& d = interface.getDestaques();
    for(size_t i=0; i<d.size(); i++) {
        Botao& b = const_cast<Botao&>(d[i]);
        if(b.contemPonto(mx, my)) { 
            if(i<estado.imagensDestaques.size()) estado.currentDestaqueIndex=i; 
            break; 
        }
    }
}

/**
 * @brief Implementa a lógica de navegação inteligente para Controles (10-foot UI).
 * 
 * @details **Sistema de Índices e Grupos:**
 * O vetor `botoesNavegaveis` é organizado em uma ordem lógica para facilitar o salto entre seções.
 * A navegação calcula a "Zona" atual para decidir para onde o cursor deve ir em comandos de 4 direções.
 * 
   // Definição dos Índices dos Grupos
   // A ordem no vetor 'botoesNavegaveis' é:
   // [0 .. D-1]       -> Destaques (Grupo 0): Banners superiores.
   // [D]              -> Config (Grupo 1 - Início): Barra de ferramentas.
   // [D+1]            -> Pesquisa
   // [D+2]            -> Mudar Tema (Grupo 1 - Fim)
   // [D+3 .. D+3+C-1] -> Categorias (Grupo 2 - PULAR): Navegadas via Gatilhos (LB/RB).
   // [D+3+C]          -> Aleatório (Grupo 3): Botão de utilidade.
   // [D+3+C+1 .. Fim] -> Jogos (Grupo 4): A grade principal (grid 4 colunas).
 *
 * @param dx Direção Horizontal (-1, 0, 1)
 * @param dy Direção Vertical (-1, 0, 1)
 */
void GerenciarInputs::navegarComControle(int dx, int dy, GerenciarScroll& estado, GerenciarInterface& interface) {
    if (!estado.podeProcessarInput(INTERVALO_INPUT_CONTROLE)) return;
    if (botoesNavegaveis.empty()) return;

    // Sai do modo de navegação de gatilho se o usuário mover o direcional
    if (modoNavegacaoGatilho) {
        modoNavegacaoGatilho = false;
        if (botaoFocadoIndex >= 0 && botaoFocadoIndex < (int)botoesNavegaveis.size())
            botoesNavegaveis[botaoFocadoIndex]->setFocado(true);
    }

    if (botaoFocadoIndex == -1) { 
        botaoFocadoIndex = 0; 
        botoesNavegaveis[0]->setFocado(true); 
        return; 
    }

    // Remove o foco do botão atual para aplicar ao próximo
    botoesNavegaveis[botaoFocadoIndex]->setFocado(false);

    // Mapeamento de Zonas
    int numDestaques = interface.getNumDestaques();
    int numBarra = 3; 
    int numCategorias = interface.getNumCategorias();
    
    int idxConfig = numDestaques; 
    int idxMudarTema = idxConfig + 2;
    int idxPrimeiraCategoria = idxConfig + numBarra;
    int idxUltimaCategoria = idxPrimeiraCategoria + numCategorias - 1;
    int idxAleatorio = idxUltimaCategoria + 1;
    int idxPrimeiroJogo = idxAleatorio + 1;
    int idxUltimoJogo = botoesNavegaveis.size() - 1;

    int current = botaoFocadoIndex;
    int zone = -1; // Identifica em qual seção visual o foco está
    if (current < idxConfig) zone = 0;
    else if (current < idxPrimeiraCategoria) zone = 1;
    else if (current <= idxUltimaCategoria) zone = 2; 
    else if (current == idxAleatorio) zone = 3;
    else zone = 4; 

    int next = current;
    int colunasJogos = 4; 

    // Lógica de Movimento Vertical (Cima/Baixo)
    if (dy != 0) {
        if (dy < 0) { // COMANDO CIMA
            if (zone == 0) next = idxConfig; // Destaque sobe para Barra
            else if (zone == 3) next = 0;   // Aleatório sobe para Destaques
            else if (zone == 4) {           // Navegação na grade (sobe uma linha)
                if (current - colunasJogos < idxPrimeiroJogo) next = idxAleatorio;
                else next = current - colunasJogos;
            }
        } 
        else { // COMANDO BAIXO
            if (zone == 0) next = idxAleatorio; // Destaque desce para Aleatório
            else if (zone == 1) next = 0;       // Barra desce para Destaques
            else if (zone == 3) {               // Aleatório desce para Grid
                if (idxPrimeiroJogo <= idxUltimoJogo) next = idxPrimeiroJogo;
            }
            else if (zone == 4) {               // Navegação na grade (desce uma linha)
                if (current + colunasJogos <= idxUltimoJogo) next = current + colunasJogos;
            }
        }
    } 
    // Lógica de Movimento Horizontal (Esquerda/Direita)
    else if (dx != 0) {
        if (dx < 0) { // COMANDO ESQUERDA
            if (zone == 0) { if (current == 0) next = idxConfig; else next = current - 1; }
            else if (zone == 1) { if (current == idxConfig) next = 0; else next = current - 1; }
            else if (zone == 4) { if (current == idxPrimeiroJogo) next = idxAleatorio; else next = current - 1; }
        } 
        else { // COMANDO DIREITA
            if (zone == 0) { if (current == numDestaques - 1) next = idxAleatorio; else next = current + 1; }
            else if (zone == 1) { if (current == idxMudarTema) next = 0; else next = current + 1; }
            else if (zone == 4) { if (current == idxUltimoJogo) next = current; else next = current + 1; }
        }
    }

    // Aplica o novo foco e garante que o scroll da tela acompanhe o cursor
    botaoFocadoIndex = next;
    Botao* btn = botoesNavegaveis[botaoFocadoIndex];
    btn->setFocado(true);

    // Lógica de Scroll Automático (Câmera segue o Foco)
    SDL_Rect r = btn->getArea();
    if (r.y < estado.scrollY + 200) estado.scrollY = r.y - 200;
    else if (r.y+r.h > estado.scrollY + 1280 - 200) estado.scrollY = (r.y+r.h) - 1280 + 200;
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
    
    // 1. Interação Especial com a Barra de Pesquisa
    if (evento.type == SDL_CONTROLLERBUTTONDOWN || evento.type == SDL_CONTROLLERAXISMOTION) {
        if (botaoFocadoIndex != -1 && botoesNavegaveis[botaoFocadoIndex] == interface.getBotaoPesquisa()) {
            BotaoPesquisa* btnPesquisa = interface.getBotaoPesquisa();
            
            // Detecta movimento para baixo (D-Pad ou Analógico)
            bool down = (evento.type == SDL_CONTROLLERBUTTONDOWN && evento.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
                        (evento.type == SDL_CONTROLLERAXISMOTION && evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY && evento.caxis.value > 16000);
            
            // Detecta movimento para cima
            bool up = (evento.type == SDL_CONTROLLERBUTTONDOWN && evento.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) ||
                      (evento.type == SDL_CONTROLLERAXISMOTION && evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY && evento.caxis.value < -16000);
            
            if (down) { if (btnPesquisa->navegarResultados(1)) return; }
            if (up) { if (btnPesquisa->temResultadoFocado()) { btnPesquisa->navegarResultados(-1); return; } }
            
            // Botão A no resultado da pesquisa: Abre a página do jogo selecionado
            if (evento.type == SDL_CONTROLLERBUTTONDOWN && evento.cbutton.button == SDL_CONTROLLER_BUTTON_A && btnPesquisa->temResultadoFocado()) {
                Jogo* jogo = btnPesquisa->getJogoResultadoFocado();
                if (jogo) {
                     if (estado.janelaJogoAtual) delete estado.janelaJogoAtual;
                     stringstream ss; ss << "Título:" << jogo->getNome() << "\nGênero: " << jogo->getCodigo() << "\nSinopse: " << jogo->getDescricao();
                     estado.janelaJogoAtual = new JanelaJogo(renderer, jogo->getNome(), jogo->getCapaJanelaJogo(), ss.str(), jogo->getCapturas());
                }
                return;
            }
        }
    }

    // 2. Processamento de Botões de Ação
    if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (evento.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_A:
                if (modoNavegacaoGatilho) {
                    // Se estiver navegando categorias com LB/RB, o 'A' confirma o filtro
                    if (estado.categoriaFocadaPorGatilho != -1) {
                         Botao* cat = interface.getBotaoCategoria(estado.categoriaFocadaPorGatilho);
                         if(cat) executarAcaoBotao(cat, estado, interface, renderer);
                    }
                } else if (botaoFocadoIndex != -1) {
                    Botao* focado = botoesNavegaveis[botaoFocadoIndex];
                    focado->ativarPorControle();
                    
                    /**
                     * @section Lógica_Clique_Duplo
                     * Se o usuário pressionar 'A' e já houver um clique pendente no mesmo botão,
                     * o sistema interpreta como um "Double Click" e executa o jogo DIRETAMENTE
                     * sem abrir a janela de detalhes (executarDireto = true).
                     */
                    if (cliquePendente && botaoPendente == focado) {
                        executarAcaoBotao(focado, estado, interface, renderer, true);
                        cliquePendente = false; botaoPendente = nullptr; ignorarProximoUpA = true;
                    } else {
                        ignorarProximoUpA = false;
                        if(focado == interface.getBotaoPesquisa()) executarAcaoBotao(focado, estado, interface, renderer);
                    }
                }
                break;

            case SDL_CONTROLLER_BUTTON_B: 
                 // Prioridade de fechamento: Janela de Jogo -> Configurações -> Sair do App
                 if(estado.janelaJogoAtual) { delete estado.janelaJogoAtual; estado.janelaJogoAtual=nullptr; }
                 else if(estado.configAberta) estado.configAberta=false;
                 else estado.rodando=false;
                 break;

            case SDL_CONTROLLER_BUTTON_X: {
                // Atalho: Salta para a pesquisa e abre o teclado
                BotaoPesquisa* bp = interface.getBotaoPesquisa();
                if(bp) {
                    if(botaoFocadoIndex!=-1) botoesNavegaveis[botaoFocadoIndex]->setFocado(false);
                    for(size_t i=0; i<botoesNavegaveis.size(); i++) if(botoesNavegaveis[i]==bp) { botaoFocadoIndex=i; break; }
                    bp->setFocado(true);
                    if(interface.getTecladoVirtual()) interface.getTecladoVirtual()->abrir();
                }
                break;
            }

            case SDL_CONTROLLER_BUTTON_START: abrirConfiguracoes(estado, renderer); break;

            case SDL_CONTROLLER_BUTTON_Y: 
                if(interface.getBotaoAleatorio()) executarAcaoBotao(interface.getBotaoAleatorio(), estado, interface, renderer);
                break;

            // Navegação Direcional (D-Pad)
            case SDL_CONTROLLER_BUTTON_DPAD_UP:    navegarComControle(0, -1, estado, interface); break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  navegarComControle(0, 1, estado, interface); break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  navegarComControle(-1, 0, estado, interface); break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: navegarComControle(1, 0, estado, interface); break;

            // Navegação Rápida de Categorias (LB/RB)
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: {
                modoNavegacaoGatilho = true;
                if(botaoFocadoIndex != -1) botoesNavegaveis[botaoFocadoIndex]->setFocado(false);
                if(SDL_GetTicks() - ultimoTempoLB > DEBOUNCE_GATILHOS) {
                    int n = interface.getNumCategorias();
                    if(estado.categoriaFocadaPorGatilho == -1) estado.categoriaFocadaPorGatilho = n-1;
                    else estado.categoriaFocadaPorGatilho = (estado.categoriaFocadaPorGatilho - 1 + n) % n;
                    // Ajusta a visualização para a categoria focada aparecer na tela
                    int rel = estado.categoriaFocadaPorGatilho - estado.categoriaIndex;
                    if(rel < 0) rel += n;
                    if(rel >= 4) estado.categoriaIndex = estado.categoriaFocadaPorGatilho;
                    ultimoTempoLB = SDL_GetTicks();
                }
                break;
            }
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: {
                modoNavegacaoGatilho = true;
                if(botaoFocadoIndex != -1) botoesNavegaveis[botaoFocadoIndex]->setFocado(false);
                if(SDL_GetTicks() - ultimoTempoRB > DEBOUNCE_GATILHOS) {
                    int n = interface.getNumCategorias();
                    if(estado.categoriaFocadaPorGatilho == -1) estado.categoriaFocadaPorGatilho = 0;
                    else estado.categoriaFocadaPorGatilho = (estado.categoriaFocadaPorGatilho + 1) % n;
                    int rel = estado.categoriaFocadaPorGatilho - estado.categoriaIndex;
                    if(rel < 0) rel += n;
                    if(rel >= 4) estado.categoriaIndex = (estado.categoriaIndex + 1) % n;
                    ultimoTempoRB = SDL_GetTicks();
                }
                break;
            }
        }
    }
    else if (evento.type == SDL_CONTROLLERBUTTONUP) {
        if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
            if (!modoNavegacaoGatilho && botaoFocadoIndex != -1) {
                botoesNavegaveis[botaoFocadoIndex]->desativarPorControle();
                // Inicia o timer para detecção de clique duplo
                if(!ignorarProximoUpA && !cliquePendente) {
                    cliquePendente = true;
                    botaoPendente = botoesNavegaveis[botaoFocadoIndex];
                    tempoCliquePendente = SDL_GetTicks();
                }
            }
        }
    }

    // 3. Processamento de Eixos Analógicos
    if (evento.type == SDL_CONTROLLERAXISMOTION) {
        // Analógico Esquerdo para Navegação
        if (evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX || evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
            if (abs(evento.caxis.value) > 16000) {
                int dx = 0, dy = 0;
                if(evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) dx = (evento.caxis.value > 0) ? 1 : -1;
                else dy = (evento.caxis.value > 0) ? 1 : -1;
                navegarComControle(dx, dy, estado, interface);
            }
        }
        // Gatilhos (LT/RT) para trocar Banners de Destaque
        else if (evento.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT && evento.caxis.value > 16000) {
            if (SDL_GetTicks() - ultimoTempoLT > 500) {
                if (!estado.imagensDestaques.empty()) {
                    estado.currentDestaqueIndex = (estado.currentDestaqueIndex - 1 + estado.imagensDestaques.size()) % estado.imagensDestaques.size();
                    modoNavegacaoGatilho = false;
                    if (botaoFocadoIndex != -1) botoesNavegaveis[botaoFocadoIndex]->setFocado(false);
                    botaoFocadoIndex = estado.currentDestaqueIndex; // Sincroniza o foco com o banner
                    if (botaoFocadoIndex < (int)botoesNavegaveis.size()) botoesNavegaveis[botaoFocadoIndex]->setFocado(true);
                }
                ultimoTempoLT = SDL_GetTicks();
            }
        }
        else if (evento.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT && evento.caxis.value > 16000) {
            if (SDL_GetTicks() - ultimoTempoRT > 500) {
                if (!estado.imagensDestaques.empty()) {
                    estado.currentDestaqueIndex = (estado.currentDestaqueIndex + 1) % estado.imagensDestaques.size();
                    modoNavegacaoGatilho = false;
                    if (botaoFocadoIndex != -1) botoesNavegaveis[botaoFocadoIndex]->setFocado(false);
                    botaoFocadoIndex = estado.currentDestaqueIndex;
                    if (botaoFocadoIndex < (int)botoesNavegaveis.size()) botoesNavegaveis[botaoFocadoIndex]->setFocado(true);
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
 * @param executarDireto Se TRUE, inicia o jogo no SO imediatamente.
 */
void GerenciarInputs::executarAcaoBotao(Botao* botao, GerenciarScroll& estado, GerenciarInterface& interface, SDL_Renderer* renderer, bool executarDireto) {
    if (!botao) return;

    // Ações de Interface do Sistema
    if (botao == interface.getBotaoConfig()) { abrirConfiguracoes(estado, renderer); return; }
    if (botao == interface.getBotaoMudarTema()) { GerenciadorTemas::getInstance().alternarTema(); return; }
    if (botao == interface.getBotaoPesquisa()) { if(interface.getTecladoVirtual()) interface.getTecladoVirtual()->abrir(); return; }
    
    // Lógica do Botão Aleatório
    if (botao == interface.getBotaoAleatorio()) {
        if (!gerenciadorJogos.listarJogos().empty()) {
            vector<Jogo> todos = gerenciadorJogos.listarJogos();
            int idx = rand() % todos.size();
            Jogo* j = gerenciadorJogos.obterJogoPorIndice(idx);
            if(j) {
                if (estado.janelaJogoAtual) delete estado.janelaJogoAtual;
                stringstream ss; ss << "Gênero: " << j->getCodigo() << "\n" << "Sinopse: " << j->getDescricao() << "\n";
                estado.janelaJogoAtual = new JanelaJogo(renderer, j->getNome(), j->getCapaJanelaJogo(), ss.str(), j->getCapturas());
            }
        }
        return;
    }

    // Identificação de Cliques em Categorias (Filtros)
    const auto& cats = interface.getCategorias();
    for (size_t i = 0; i < cats.size(); ++i) {
        if (botao == &cats[i]) {
            if (estado.categoriaAtivaIndex == (int)i) {
                estado.categoriaAtual = "Todas"; // Se clicar na mesma, reseta o filtro
                estado.categoriaAtivaIndex = -1;
            } else {
                vector<string> nomes = {"Aventura", "Casual", "Acao", "RPG", "Estrategia", "Esportes"};
                if (i < nomes.size()) estado.categoriaAtual = nomes[i];
                estado.categoriaAtivaIndex = i;
            }
            interface.atualizarJogosPorCategoria(estado.categoriaAtual, renderer);
            return;
        }
    }

    // Ação sobre um Jogo da Biblioteca
    if (!botao->getNomeJogo().empty()) {
        vector<Jogo> todos = gerenciadorJogos.listarJogos();
        for (auto& jogo : todos) {
            if (jogo.getNome() == botao->getNomeJogo()) {
                
                if (executarDireto) {
                    /** 
                     * @section EXECUTAR_JOGO
                     * Simula o comportamento de "Play" imediato. Instancia a lógica
                     * e chama o system() definido na JanelaJogo.
                     */
                    JanelaJogo tempJanela(renderer, jogo.getNome(), jogo.getCapaJanelaJogo(), "", jogo.getCapturas());
                    tempJanela.executarJogo();
                } else {
                    /**
                     * @section ABRIR_PAGINA_DETALHES
                     * Abre a interface interna com sinopse e screenshots antes de jogar.
                     */
                    if (estado.janelaJogoAtual) delete estado.janelaJogoAtual;
                    stringstream ss;
                    ss << "Gênero: " << jogo.getCodigo() << "\n" << "Sinopse: " << jogo.getDescricao() << "\n";
                    estado.janelaJogoAtual = new JanelaJogo(renderer, jogo.getNome(), jogo.getCapaJanelaJogo(), ss.str(), jogo.getCapturas());
                }
                return;
            }
        }
    }
}

/**
 * @brief Ativa o estado de abertura da janela de configurações.
 */
void GerenciarInputs::abrirConfiguracoes(GerenciarScroll& estado, SDL_Renderer* renderer) {
    if (!estado.configAberta) estado.configAberta = true;
}