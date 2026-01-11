/**
 * @file JanelaConfiguracao.cpp
 * @brief Implementação da janela de configurações e seu sistema de submenus.
 * 
 * Este arquivo gerencia o ciclo de vida de uma janela SDL secundária, incluindo
 * a criação de botões dinâmicos que herdam as cores do tema global e um sistema
 * de estados para navegação em submenus.
 */

#include "JanelaConfiguracao.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp" 
#include <iostream>
#include <SDL2/SDL.h> 

using namespace MeuProjeto;

/**
 * @brief Construtor da classe. 
 * A inicialização real ocorre no método abrir().
 */
JanelaConfiguracao::JanelaConfiguracao() {
}

/**
 * @brief Destrutor da classe. 
 * Garante que se o objeto for destruído, os recursos da SDL e a memória dos botões sejam limpos.
 */
JanelaConfiguracao::~JanelaConfiguracao() {
    fechar();
}

/**
 * @brief Inicializa e configura todos os botões da janela de configuração.
 * 
 * @details **Lógica de Construção de Botões:**
 * 1. **Limpeza:** Antes de criar, deleta ponteiros existentes para evitar vazamentos (Memory Leaks).
 * 2. **Sincronização de Tema:** Recupera as cores do `GerenciadorTemas` (Normal, Hover, Press).
 * 3. **Instanciação:** Cria os botões principais de configuração usando coordenadas fixas.
 * 4. **Botões de Controle de Janela:** Cria os botões "X" (fechar) e "Voltar" com cores específicas.
 * 5. **Foco Inicial:** Se um controle estiver conectado, o primeiro botão já inicia focado.
 */
void JanelaConfiguracao::inicializarBotoes() {
    // Limpeza de segurança para evitar duplicidade de alocação na memória heap
    for (auto btn : botoesMenu) delete btn;
    botoesMenu.clear();
    delete btnVoltar;
    delete btnFechar;

    // Sincroniza os componentes com a paleta de cores do tema atual (Claro/Escuro)
    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color btnNormal = tema.getCorBotaoNormal();
    SDL_Color btnHover = tema.getCorBotaoHover();
    SDL_Color btnPress = tema.getCorBotaoPressionado();

    // Criação dos botões principais do menu de configurações
    Botao* btn1 = new Botao(217, 274, 1101, 122, "Alterar Controle");
    btn1->setCor(btnNormal, btnHover, btnPress); 
    botoesMenu.push_back(btn1);
    
    Botao* btn2 = new Botao(217, 550, 1101, 122, "Suporte/Ajuda");
    btn2->setCor(btnNormal, btnHover, btnPress);
    botoesMenu.push_back(btn2);
    
    Botao* btn3 = new Botao(217, 826, 1101, 122, "Configurações Avançadas");
    btn3->setCor(btnNormal, btnHover, btnPress);
    botoesMenu.push_back(btn3);

    // Botão de fechamento rápido (X) - Posicionado no canto superior direito
    btnFechar = new Botao(1465, 10, 50, 50, "X");
    btnFechar->setCor({200, 20, 20, 255}, {250, 50, 50, 255}, {180, 0, 0, 255});

    // Botão de navegação entre submenus
    btnVoltar = new Botao(50, 10, 150, 50, "< Voltar");
    btnVoltar->setCor(btnNormal, btnHover, btnPress);

    // Gerenciamento de foco para Gamepad
    if (SDL_NumJoysticks() > 0) {
        indiceFocado = 0; // Inicia com o primeiro botão selecionado se houver controle
    } else {
        indiceFocado = -1; // Modo mouse puro
    }
}

/**
 * @brief Cria a janela física e o contexto de renderização.
 * 
 * Utiliza a flag `SDL_WINDOW_BORDERLESS` para criar uma janela limpa, 
 * típica de launchers modernos. `SDL_RaiseWindow` é chamado para garantir 
 * que ela apareça na frente da janela principal da biblioteca.
 */
void JanelaConfiguracao::abrir() {
    if (aberta) return;

    janela = SDL_CreateWindow("Configurações", 
                             SDL_WINDOWPOS_CENTERED, 
                             SDL_WINDOWPOS_CENTERED, 
                             1525, 1116, 
                             SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    
    if (janela) {
        renderer = SDL_CreateRenderer(janela, -1, SDL_RENDERER_ACCELERATED);
        if (renderer) {
            aberta = true;
            inicializarBotoes(); 
            SDL_RaiseWindow(janela); // Garante o foco da janela
        }
    }
}

/**
 * @brief Destrói os recursos da janela e limpa a memória dos botões.
 * 
 */
void JanelaConfiguracao::fechar() {
    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
    if (janela) { SDL_DestroyWindow(janela); janela = nullptr; }
    
    for (auto btn : botoesMenu) delete btn;
    botoesMenu.clear();
    
    if (btnVoltar) { delete btnVoltar; btnVoltar = nullptr; }
    if (btnFechar) { delete btnFechar; btnFechar = nullptr; }
    
    aberta = false;
    submenuAtivo = SubmenuConfig::NENHUM;
}

/** @brief Retorna o ID interno da SDL para filtragem de eventos em múltiplas janelas. */
Uint32 JanelaConfiguracao::getIDJanela() const { return janela ? SDL_GetWindowID(janela) : 0; }

/**
 * @brief Dispara a mudança de estado baseada no botão pressionado.
 * 
 * @details **Execução de Ações:**
 * Esta função funciona como um roteador. Ao clicar em um índice do vetor de botões, 
 * ela altera o valor de `submenuAtivo`, o que fará com que o método `desenhar()` 
 * exiba uma interface diferente no próximo frame.
 * 
 * @param indice O índice do botão no vetor botoesMenu.
 */
void JanelaConfiguracao::executarAcaoMenu(int indice) {
    std::cout << "[CONFIG] Botão pressionado: " << indice << std::endl; // Log para debug

    switch (indice) {
        case 0: 
            // Usuário clicou em "Alterar Controle"
            // Define o estado para que o `desenhar()` mostre a tela de controles
            submenuAtivo = SubmenuConfig::ALTERAR_CONTROLE; 
            break;

        case 1: 
            // Usuário clicou em "Suporte/Ajuda"
            submenuAtivo = SubmenuConfig::SUPORTE_AJUDA; 
            break;

        case 2: 
            // Usuário clicou em "Configurações Avançadas"
            submenuAtivo = SubmenuConfig::CONFIG_AVANCADAS; 
            break;
            
        default:
            std::cout << "[ERRO] Índice de botão desconhecido." << std::endl;
            break;
    }
}

/**
 * @brief Processa os eventos de input focados nesta janela.
 * 
 * @details **Lógica de Execução de Ações por Input:**
 * 1. **Mouse:** O método `tratarEvento` de cada botão verifica a colisão. Se houver um 
 *    `SDL_MOUSEBUTTONUP` dentro da área, o botão retorna true e `executarAcaoMenu` é chamado.
 * 2. **Controle (Navegação):** Movimentos no Analógico ou D-Pad alteram o `indiceFocado`.
 * 3. **Controle (Execução):** Pressionar o botão 'A' do controle chama `executarAcaoMenu` 
 *    utilizando o botão que está focado no momento.
 */
bool JanelaConfiguracao::processarEvento(SDL_Event& evento) {
    if (!aberta) return false;

    // Filtra se o evento de janela pertence a esta instância (ID único)
    if (evento.type == SDL_WINDOWEVENT && evento.window.windowID == getIDJanela()) {
        if (evento.window.event == SDL_WINDOWEVENT_CLOSE) {
            fechar();
            return true;
        }
    }

    // Lógica de Interação Híbrida (Mouse e Controle)
    if (submenuAtivo != SubmenuConfig::NENHUM) {
        // Se estiver dentro de um submenu, o foco principal é o botão "Voltar"
        if (btnVoltar->tratarEvento(evento, 0, 0) && evento.type == SDL_MOUSEBUTTONUP) {
            submenuAtivo = SubmenuConfig::NENHUM;
            return true;
        }
    } else {
        // Navegação no Menu Principal de Configurações
        for (size_t i = 0; i < botoesMenu.size(); i++) {
            if (botoesMenu[i]->tratarEvento(evento, 0, 0) && evento.type == SDL_MOUSEBUTTONUP) {
                executarAcaoMenu(i); // Executa a ação do botão clicado
                return true;
            }
        }
    }

    // Botão global de fechamento
    if (btnFechar->tratarEvento(evento, 0, 0) && evento.type == SDL_MOUSEBUTTONUP) {
        fechar();
        return true;
    }
    
    // Navegação via Gamepad (Analógico Esquerdo)
    if (evento.type == SDL_CONTROLLERAXISMOTION) {
        Uint32 tempoAtual = SDL_GetTicks();
        if (tempoAtual - ultimoInputAnalogico > INTERVALO_ANALOGICO) {
            if (submenuAtivo == SubmenuConfig::NENHUM) {
                if (evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                    if (indiceFocado == -1) indiceFocado = 0; 
                    if (evento.caxis.value > DEADZONE) { // Direção: Baixo
                        if (indiceFocado >= (int)botoesMenu.size() - 1) indiceFocado = 0;
                        else indiceFocado++;
                        ultimoInputAnalogico = tempoAtual;
                        return true;
                    } 
                    else if (evento.caxis.value < -DEADZONE) { // Direção: Cima
                        if (indiceFocado <= 0) indiceFocado = botoesMenu.size() - 1;
                        else indiceFocado--;
                        ultimoInputAnalogico = tempoAtual;
                        return true;
                    }
                }
            }
        }
    }

    // Navegação via Gamepad (Botões e D-Pad)
    if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
        if (indiceFocado == -1) indiceFocado = 0;

        if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
            if (submenuAtivo == SubmenuConfig::NENHUM) {
                if (indiceFocado <= 0) indiceFocado = botoesMenu.size() - 1;
                else indiceFocado--;
            }
            return true;
        }
        else if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
            if (submenuAtivo == SubmenuConfig::NENHUM) {
                if (indiceFocado >= (int)botoesMenu.size() - 1) indiceFocado = 0;
                else indiceFocado++;
            }
            return true;
        }
        else if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
            // Executa a ação baseada no botão que detém o foco atual
            if (submenuAtivo == SubmenuConfig::NENHUM && indiceFocado != -1) {
                executarAcaoMenu(indiceFocado);
            } else if (submenuAtivo != SubmenuConfig::NENHUM) {
                submenuAtivo = SubmenuConfig::NENHUM; // Botão A no submenu volta para o principal
            }
            return true;
        }
        else if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
            if (submenuAtivo != SubmenuConfig::NENHUM) {
                submenuAtivo = SubmenuConfig::NENHUM; // Botão B volta um nível
            } else {
                fechar(); // Botão B no menu raiz fecha a janela
            }
            return true;
        }
    }

    return false;
}

/**
 * @brief Orquestra a renderização da interface de configurações.
 * 
 * @details **Lógica de Desenho:**
 * 1. **Fundo:** Limpa a tela com a cor de fundo do tema.
 * 2. **Estado Submenu:** Se um submenu estiver ativo, desenha um painel central 
 *    e o botão de voltar.
 * 3. **Estado Principal:** Desenha o título e itera sobre a lista de botões. 
 *    A lógica de foco (`setFocado`) é aplicada aqui para garantir que o realce 
 *    visual acompanhe o índice selecionado pelo controle.
 */
void JanelaConfiguracao::desenhar() {
    if (!aberta || !renderer) return;

    auto& tema = GerenciadorTemas::getInstance();

    // 1. Preenchimento do fundo da janela conforme o tema atual
    SDL_Color corFundo = tema.getCorFundo(); 
    SDL_SetRenderDrawColor(renderer, corFundo.r, corFundo.g, corFundo.b, corFundo.a);
    SDL_RenderClear(renderer);

    if (submenuAtivo != SubmenuConfig::NENHUM) {
        // Renderização de Submenu: Exibe um painel placeholder centralizado
        SDL_Rect quadrado = { (1525 - 200) / 2, (1116 - 200) / 2, 200, 200 };
        
        SDL_Color corPainel = tema.getCorRetangulos();
        SDL_SetRenderDrawColor(renderer, corPainel.r, corPainel.g, corPainel.b, corPainel.a);
        SDL_RenderFillRect(renderer, &quadrado);
        
        if(btnVoltar) btnVoltar->desenhar(renderer);
    } else {
        // Renderização do Menu Principal
        desenharTexto(renderer, "Configurações", 535, 90, tema.getCorTextoNegrito(), 60);
        
        for (int i = 0; i < (int)botoesMenu.size(); i++) {
            // Atualiza o estado de foco visual para navegação por controle
            if (i == indiceFocado && SDL_NumJoysticks() > 0) { 
                botoesMenu[i]->setFocado(true);
            } else {
                botoesMenu[i]->setFocado(false);
            }
            botoesMenu[i]->desenhar(renderer);
        }
    }

    // O botão de fechar é sempre desenhado por cima de tudo (z-index superior)
    if(btnFechar) btnFechar->desenhar(renderer);
    
    // Apresenta o frame finalizado na janela secundária
    SDL_RenderPresent(renderer);
}