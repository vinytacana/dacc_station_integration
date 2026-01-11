/**
 * @file JanelaConfiguracao.cpp
 * @brief Implementação da janela de configurações com layout responsivo.
 *
 * Este arquivo contém a lógica de funcionamento de uma janela secundária do SDL2 dedicada
 * às configurações do sistema. Implementa a criação de uma janela independente, gestão 
 * de renderizador próprio, tratamento de eventos de navegação por controle (gamepad) 
 * e organização de submenus funcionais.
 */

#include "JanelaConfiguracao.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include <iostream>
#include <SDL2/SDL.h> 

using namespace MeuProjeto;

/**
 * @brief Construtor da classe JanelaConfiguracao.
 *
 * Inicializa a instância da classe. O estado inicial da janela é definido como fechado.
 */
JanelaConfiguracao::JanelaConfiguracao() {
}

/**
 * @brief Destrutor da classe JanelaConfiguracao.
 *
 * Garante o encerramento seguro da janela e a liberação de recursos ao destruir a instância.
 */
JanelaConfiguracao::~JanelaConfiguracao() {
    fechar();
}

/**
 * @brief Configura e instancia os componentes interativos da janela.
 *
 * Este método define as propriedades visuais e posicionais dos botões do menu principal,
 * botões de fechamento e de retorno. Utiliza a classe ConfigLayout para converter 
 * coordenadas estáticas em dimensões escaladas para a resolução do sistema e aplica 
 * as cores provenientes do GerenciadorTemas. Também gerencia a definição inicial de 
 * foco caso um controlador de jogo esteja conectado.
 */
void JanelaConfiguracao::inicializarBotoes() {
    botoesMenu.clear();
    btnVoltar.reset();
    btnFechar.reset();

    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color btnNormal = tema.getCorBotaoNormal();
    SDL_Color btnHover = tema.getCorBotaoHover();
    SDL_Color btnPress = tema.getCorBotaoPressionado();

    // Instanciação dos botões principais com coordenadas responsivas via ConfigLayout
    auto btn1 = std::make_unique<Botao>(
        ConfigLayout::X(217), ConfigLayout::Y(274), 
        ConfigLayout::X(1101), ConfigLayout::Y(122), 
        "Alterar Controle"
    );
    btn1->setCor(btnNormal, btnHover, btnPress); 
    botoesMenu.push_back(std::move(btn1));
    
    auto btn2 = std::make_unique<Botao>(
        ConfigLayout::X(217), ConfigLayout::Y(550), 
        ConfigLayout::X(1101), ConfigLayout::Y(122), 
        "Suporte/Ajuda"
    );
    btn2->setCor(btnNormal, btnHover, btnPress);
    botoesMenu.push_back(std::move(btn2));
    
    auto btn3 = std::make_unique<Botao>(
        ConfigLayout::X(217), ConfigLayout::Y(826), 
        ConfigLayout::X(1101), ConfigLayout::Y(122), 
        "Configurações Avançadas"
    );
    btn3->setCor(btnNormal, btnHover, btnPress);
    botoesMenu.push_back(std::move(btn3));

    // Configuração do componente de fechamento (X) com esquema de cores de alerta
    btnFechar = std::make_unique<Botao>(
        ConfigLayout::X(1465), ConfigLayout::Y(10), 
        ConfigLayout::X(50), ConfigLayout::Y(50), 
        "X"
    );
    btnFechar->setCor({200, 20, 20, 255}, {250, 50, 50, 255}, {180, 0, 0, 255});

    // Configuração do componente de navegação de retorno
    btnVoltar = std::make_unique<Botao>(
        ConfigLayout::X(50), ConfigLayout::Y(10), 
        ConfigLayout::X(150), ConfigLayout::Y(50), 
        "< Voltar"
    );
    btnVoltar->setCor(btnNormal, btnHover, btnPress);

    // Lógica de ativação de foco baseada na presença de periféricos de entrada (Joystick)
    if (SDL_NumJoysticks() > 0) {
        indiceFocado = 0; 
    } else {
        indiceFocado = -1; 
    }
}

/**
 * @brief Realiza a criação da janela SDL2 e seu renderizador acelerado.
 *
 * Define as dimensões da janela de acordo com o design original escalado, remove 
 * bordas decorativas do sistema operacional (SDL_WINDOW_BORDERLESS) e garante 
 * que a janela receba o foco de exibição (SDL_RaiseWindow).
 */
void JanelaConfiguracao::abrir() {
    if (aberta) return;

    janela = SDL_CreateWindow("Configurações", 
                             SDL_WINDOWPOS_CENTERED, 
                             SDL_WINDOWPOS_CENTERED, 
                             ConfigLayout::X(1525), ConfigLayout::Y(1116), 
                             SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    
    if (janela) {
        renderer = SDL_CreateRenderer(janela, -1, SDL_RENDERER_ACCELERATED);
        if (renderer) {
            aberta = true;
            inicializarBotoes(); 
            SDL_RaiseWindow(janela); 
        }
    }
}

/**
 * @brief Encerra a janela de configurações e libera os recursos SDL.
 *
 * Destrói as instâncias de SDL_Renderer e SDL_Window, limpa os vetores de botões 
 * e redefine os sinalizadores de estado interno e submenus.
 */
void JanelaConfiguracao::fechar() {
    if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
    if (janela) { SDL_DestroyWindow(janela); janela = nullptr; }

    botoesMenu.clear(); 
    btnVoltar.reset(); 
    btnFechar.reset();
    
    aberta = false;
    submenuAtivo = SubmenuConfig::NENHUM;
}

/**
 * @brief Recupera o identificador único da janela SDL.
 * @return Uint32 contendo o ID da janela ou 0 se a janela não estiver instanciada.
 */
Uint32 JanelaConfiguracao::getIDJanela() const { return janela ? SDL_GetWindowID(janela) : 0; }

/**
 * @brief Altera o estado da janela para exibir um submenu específico.
 * @param indice Índice do botão pressionado que determina o submenu alvo.
 */
void JanelaConfiguracao::executarAcaoMenu(int indice) {
    std::cout << "[CONFIG] Botão pressionado: " << indice << std::endl; 

    switch (indice) {
        case 0: submenuAtivo = SubmenuConfig::ALTERAR_CONTROLE; break;
        case 1: submenuAtivo = SubmenuConfig::SUPORTE_AJUDA; break;
        case 2: submenuAtivo = SubmenuConfig::CONFIG_AVANCADAS; break;
        default: break;
    }
}

/**
 * @brief Processa a fila de eventos do SDL para a janela de configurações.
 *
 * Realiza o tratamento de eventos de janela, cliques de mouse nos botões, 
 * movimentação de eixos analógicos e botões de controladores de jogo. Implementa 
 * lógica de navegação vertical por foco, respeitando zonas mortas (deadzone) e 
 * intervalos mínimos de processamento para inputs analógicos.
 * 
 * @param evento Referência para o evento capturado pelo loop principal.
 * @return true se o evento foi processado e consumido, false caso contrário.
 */
bool JanelaConfiguracao::processarEvento(SDL_Event& evento) {
    if (!aberta) return false;

    // Tratamento de eventos nativos da janela SDL
    if (evento.type == SDL_WINDOWEVENT && evento.window.windowID == getIDJanela()) {
        if (evento.window.event == SDL_WINDOWEVENT_CLOSE) {
            fechar();
            return true;
        }
    }

    // Lógica de interação com submenus e botões de navegação
    if (submenuAtivo != SubmenuConfig::NENHUM) {
        if (btnVoltar->tratarEvento(evento, 0, 0) && evento.type == SDL_MOUSEBUTTONUP) {
            submenuAtivo = SubmenuConfig::NENHUM;
            return true;
        }
    } else {
        for (size_t i = 0; i < botoesMenu.size(); i++) {
            if (botoesMenu[i]->tratarEvento(evento, 0, 0) && evento.type == SDL_MOUSEBUTTONUP) {
                executarAcaoMenu(i); 
                return true;
            }
        }
    }

    if (btnFechar->tratarEvento(evento, 0, 0) && evento.type == SDL_MOUSEBUTTONUP) {
        fechar();
        return true;
    }
    
    // Tratamento de eixos analógicos para navegação de foco no menu
    if (evento.type == SDL_CONTROLLERAXISMOTION) {
        Uint32 tempoAtual = SDL_GetTicks();
        if (tempoAtual - ultimoInputAnalogico > INTERVALO_ANALOGICO) {
            if (submenuAtivo == SubmenuConfig::NENHUM) {
                if (evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                    if (indiceFocado == -1) indiceFocado = 0; 
                    if (evento.caxis.value > DEADZONE) { 
                        if (indiceFocado >= (int)botoesMenu.size() - 1) indiceFocado = 0;
                        else indiceFocado++;
                        ultimoInputAnalogico = tempoAtual;
                        return true;
                    } 
                    else if (evento.caxis.value < -DEADZONE) { 
                        if (indiceFocado <= 0) indiceFocado = botoesMenu.size() - 1;
                        else indiceFocado--;
                        ultimoInputAnalogico = tempoAtual;
                        return true;
                    }
                }
            }
        }
    }

    // Tratamento de botões físicos do controlador (D-Pad, A, B)
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
            if (submenuAtivo == SubmenuConfig::NENHUM && indiceFocado != -1) {
                executarAcaoMenu(indiceFocado);
            } else if (submenuAtivo != SubmenuConfig::NENHUM) {
                submenuAtivo = SubmenuConfig::NENHUM; 
            }
            return true;
        }
        else if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
            if (submenuAtivo != SubmenuConfig::NENHUM) {
                submenuAtivo = SubmenuConfig::NENHUM; 
            } else {
                fechar(); 
            }
            return true;
        }
    }

    return false;
}

/**
 * @brief Executa o ciclo de renderização da janela de configurações.
 *
 * Limpa o backbuffer com a cor de fundo do tema, desenha os elementos textuais 
 * e botões correspondentes ao estado atual (menu principal ou submenu). 
 * Aplica lógica de visibilidade para o destaque de foco em periféricos e 
 * finaliza apresentando o frame no renderizador.
 */
void JanelaConfiguracao::desenhar() {
    if (!aberta || !renderer) return;

    auto& tema = GerenciadorTemas::getInstance();

    SDL_Color corFundo = tema.getCorFundo(); 
    SDL_SetRenderDrawColor(renderer, corFundo.r, corFundo.g, corFundo.b, corFundo.a);
    SDL_RenderClear(renderer);

    if (submenuAtivo != SubmenuConfig::NENHUM) {
        // Cálculo e renderização do painel centralizado para submenus
        int rectW = ConfigLayout::X(200);
        int rectH = ConfigLayout::Y(200);
        int rectX = (ConfigLayout::X(1525) - rectW) / 2;
        int rectY = (ConfigLayout::Y(1116) - rectH) / 2;

        SDL_Rect quadrado = { rectX, rectY, rectW, rectH };
        
        SDL_Color corPainel = tema.getCorRetangulos();
        SDL_SetRenderDrawColor(renderer, corPainel.r, corPainel.g, corPainel.b, corPainel.a);
        SDL_RenderFillRect(renderer, &quadrado);
        
        if(btnVoltar) btnVoltar->desenhar(renderer);
    } else {
        // Renderização da interface do menu de configurações principal
        desenharTexto(renderer, "Configurações", 
                      ConfigLayout::X(535), ConfigLayout::Y(90), 
                      tema.getCorTextoNegrito(), 
                      ConfigLayout::F(60)); 
        
        for (int i = 0; i < (int)botoesMenu.size(); i++) {
            // Sincronização do estado de foco visual com o índice de navegação do controlador
            if (i == indiceFocado && SDL_NumJoysticks() > 0) { 
                botoesMenu[i]->setFocado(true);
            } else {
                botoesMenu[i]->setFocado(false);
            }
            botoesMenu[i]->desenhar(renderer);
        }
    }

    if(btnFechar) btnFechar->desenhar(renderer);
    
    SDL_RenderPresent(renderer);
}