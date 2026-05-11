/**
 * @file JanelaConfiguracao.cpp
 * @brief Implementação da classe JanelaConfiguracao para gerenciamento das configurações do sistema.
 * 
 * @details Esta classe implementa a janela principal de configurações do sistema,
 * fornecendo acesso a rede, áudio/vídeo, Bluetooth e informações do sistema.
 */

#include "JanelaConfiguracao.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include "GerenciadorAudio.hpp"
#include "GerenciadorImagens.hpp"
#include "SystemStatus.hpp"
#include <iostream>
#include <SDL2/SDL.h>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace MeuProjeto;

extern GerenciadorAudio gerAudio;       ///< Instância global do gerenciador de áudio
extern GerenciadorImagens gerImg;       ///< Instância global do gerenciador de imagens

// CONSTRUTOR E DESTRUTOR

/**
 * @brief Construtor da classe JanelaConfiguracao.
 * 
 * @details Inicializa a janela de configurações e cria uma instância da janela de rede.
 */
JanelaConfiguracao::JanelaConfiguracao() {}

/**
 * @brief Destrutor da classe JanelaConfiguracao.
 * 
 * @details Libera todos os recursos alocados, incluindo texturas, janelas e renderizadores.
 */
JanelaConfiguracao::~JanelaConfiguracao() {
    liberarIconeBateria();
    fechar();
}

// INICIALIZAÇÃO

/**
 * @brief Inicializa todos os botões da interface.
 * 
 * @details Cria e configura os botões do menu principal, botão de voltar e botão de fechar.
 * As cores são obtidas do gerenciador de temas e as dimensões do ConfigLayout.
 */
void JanelaConfiguracao::inicializarBotoes() {
    botoesMenu.clear();
    btnVoltar.reset();
    btnFechar.reset();

    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color btnNormal = tema.getCorBotaoNormal();
    SDL_Color btnHover = tema.getCorBotaoHover();
    SDL_Color btnPress = tema.getCorBotaoPressionado();

    auto btn1 = std::make_unique<Botao>(
        ConfigLayout::X(217), ConfigLayout::Y(200), 
        ConfigLayout::X(1101), ConfigLayout::Y(122), 
        "REDE"
    );
    btn1->setCor(btnNormal, btnHover, btnPress); 
    btn1->setRetanguloBordasArredondadas(20);
    botoesMenu.push_back(std::move(btn1));
    
    auto btn2 = std::make_unique<Botao>(
        ConfigLayout::X(217), ConfigLayout::Y(370), 
        ConfigLayout::X(1101), ConfigLayout::Y(122), 
        "ÁUDIO & VÍDEO"
    );
    btn2->setCor(btnNormal, btnHover, btnPress);
    btn2->setRetanguloBordasArredondadas(20);
    botoesMenu.push_back(std::move(btn2));
    
    auto btn3 = std::make_unique<Botao>(
        ConfigLayout::X(217), ConfigLayout::Y(540), 
        ConfigLayout::X(1101), ConfigLayout::Y(122), 
        "BLUETOOTH"
    );
    btn3->setCor(btnNormal, btnHover, btnPress);
    btn3->setRetanguloBordasArredondadas(20);
    botoesMenu.push_back(std::move(btn3));

    auto btn4 = std::make_unique<Botao>(
        ConfigLayout::X(217), ConfigLayout::Y(710), 
        ConfigLayout::X(1101), ConfigLayout::Y(122), 
        "INFORMAÇÕES DO SISTEMA"
    );
    btn4->setCor(btnNormal, btnHover, btnPress);
    btn4->setRetanguloBordasArredondadas(20);
    botoesMenu.push_back(std::move(btn4));

    btnFechar = std::make_unique<Botao>(
        ConfigLayout::X(1465), ConfigLayout::Y(10), 
        ConfigLayout::X(50), ConfigLayout::Y(50), 
        "X"
    );
    btnFechar->setCor({200, 20, 20, 255}, {250, 50, 50, 255}, {180, 0, 0, 255});
    btnFechar->setIsRound(true);

    btnVoltar = std::make_unique<Botao>(
        ConfigLayout::X(50), ConfigLayout::Y(10), 
        ConfigLayout::X(150), ConfigLayout::Y(50), 
        "< Voltar"
    );
    btnVoltar->setCor(btnNormal, btnHover, btnPress);
    btnVoltar->setRetanguloBordasArredondadas(20);

    if (SDL_NumJoysticks() > 0) {
        indiceFocado = 0; 
        ajustarFocoMenuParaDisponivel();
    } else {
        indiceFocado = -1; 
    }
}

void JanelaConfiguracao::atualizarCapacidadesSistema() {
    capacidadesSistema = ::obter_capacidades_sistema();
}

SubmenuConfig JanelaConfiguracao::submenuPorIndice(int indice) const {
    switch (indice) {
        case 0: return SubmenuConfig::REDE;
        case 1: return SubmenuConfig::AUDIO_VIDEO;
        case 2: return SubmenuConfig::BLUETOOTH;
        case 3: return SubmenuConfig::SISTEMA;
        default: return SubmenuConfig::NENHUM;
    }
}

bool JanelaConfiguracao::submenuDisponivel(SubmenuConfig submenu) const {
    switch (submenu) {
        case SubmenuConfig::REDE:
            return capacidadesSistema.network;
        case SubmenuConfig::AUDIO_VIDEO:
            return capacidadesSistema.audio_list ||
                   capacidadesSistema.volume_control ||
                   capacidadesSistema.display_info ||
                   capacidadesSistema.display_resolution ||
                   capacidadesSistema.display_scale ||
                   capacidadesSistema.brightness;
        case SubmenuConfig::BLUETOOTH:
            return capacidadesSistema.bluetooth;
        case SubmenuConfig::SISTEMA:
            return true;
        case SubmenuConfig::NENHUM:
        default:
            return false;
    }
}

std::string JanelaConfiguracao::mensagemSubmenuIndisponivel(SubmenuConfig submenu) const {
    switch (submenu) {
        case SubmenuConfig::REDE:
            return "Rede indisponivel: NetworkManager/nmcli nao detectado.";
        case SubmenuConfig::AUDIO_VIDEO:
            return "Audio e video indisponiveis neste ambiente.";
        case SubmenuConfig::BLUETOOTH:
            return "Bluetooth indisponivel: bluetoothctl nao detectado.";
        case SubmenuConfig::SISTEMA:
        case SubmenuConfig::NENHUM:
        default:
            return "";
    }
}

void JanelaConfiguracao::ajustarFocoMenuParaDisponivel(int direcao) {
    if (botoesMenu.empty()) {
        indiceFocado = -1;
        return;
    }

    if (indiceFocado < 0 || indiceFocado >= static_cast<int>(botoesMenu.size())) {
        indiceFocado = 0;
    }

    direcao = direcao < 0 ? -1 : 1;
    const int total = static_cast<int>(botoesMenu.size());
    for (int tentativas = 0; tentativas < total; ++tentativas) {
        if (submenuDisponivel(submenuPorIndice(indiceFocado))) {
            return;
        }
        indiceFocado = (indiceFocado + direcao + total) % total;
    }

    indiceFocado = -1;
}

/**
 * @brief Abre a janela de configurações.
 * 
 * @details Cria a janela e o renderizador SDL, inicializa os componentes da interface
 * e força a primeira renderização. Inclui configuração de qualidade de renderização.
 */
void JanelaConfiguracao::abrir() {
    if (aberta) return;

    atualizarCapacidadesSistema();

    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2")) {
        std::cerr << "[AVISO] Qualidade '2' não suportada, tentando '1'..." << std::endl;
        if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
            std::cerr << "[AVISO] Usando qualidade padrão (nearest neighbor)." << std::endl;
        }
    }

    janela = SDL_CreateWindow("Configurações", 
                             SDL_WINDOWPOS_CENTERED, 
                             SDL_WINDOWPOS_CENTERED, 
                             ConfigLayout::X(1525), ConfigLayout::Y(1116), 
                             SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP);
    
    if (janela) {
        renderer = SDL_CreateRenderer(janela, -1, SDL_RENDERER_ACCELERATED);
        if (renderer) {
            aberta = true;
            
            limparCacheTexto();
            
            inicializarBotoes();
            carregarIconeBateria();
            SDL_RaiseWindow(janela); 
            
            // Força renderização inicial
            desenhar();
            SDL_RenderPresent(renderer);
        }
    }
}

/**
 * @brief Fecha a janela de configurações.
 * 
 * @details Libera todos os recursos SDL e reseta o estado da janela.
 * Inclui limpeza do cache de texto para evitar problemas de memória.
 */
void JanelaConfiguracao::fechar() {
    if (renderer) {
        limparCacheTexto();
    }

    janelaRede.reset();
    janelaAudioVideo.reset();
    janelaBluetooth.reset();
    janelaInfosSistema.reset();
    liberarIconeBateria();
    
    if (renderer) {
        gerImg.liberarRenderer(renderer);
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (janela) { SDL_DestroyWindow(janela); janela = nullptr; }

    botoesMenu.clear(); 
    btnVoltar.reset(); 
    btnFechar.reset();
    
    aberta = false;
    submenuAtivo = SubmenuConfig::NENHUM;
}

/**
 * @brief Notifica mudança de tema e recria todo o estado visual.
 * 
 * Texturas de texto pertencem ao renderer que as criou; ao alternar o tema,
 * a janela de configuração precisa recriar esses recursos no renderer ativo.
 */
void JanelaConfiguracao::notificarMudancaTemaEmTodasJanelas() {
    if (!renderer) {
        std::cout << "[ERRO] Renderer é NULL! Abortando..." << std::endl;
        return;
    }
    
    limparCacheTexto();
    SDL_Delay(50);
    
    liberarIconeBateria();
    carregarIconeBateria();
    
    inicializarBotoes();
    
    limparCacheTexto();

    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color corFundo = tema.getCorFundo();
    SDL_SetRenderDrawColor(renderer, corFundo.r, corFundo.g, corFundo.b, 255);
    SDL_RenderClear(renderer);
    
    desenhar();
    SDL_RenderPresent(renderer);
    
    SDL_Delay(100);
    
    desenhar();
    SDL_RenderPresent(renderer);
}

/**
 * @brief Obtém o ID da janela SDL.
 * 
 * @return ID da janela SDL, ou 0 se a janela não existir.
 */
Uint32 JanelaConfiguracao::getIDJanela() const { 
    return janela ? SDL_GetWindowID(janela) : 0; 
}

/**
 * @brief Executa a ação correspondente a um item do menu.
 * 
 * @details Ativa o submenu correspondente ao índice fornecido e limpa o cache de texto.
 * Também reseta os submenus anteriores antes de mudar para um novo submenu.
 * 
 * @param indice Índice do item do menu a ser ativado (0-3).
 */
void JanelaConfiguracao::executarAcaoMenu(int indice) {
    std::cout << "[CONFIG] Botão pressionado: " << indice << std::endl; 

    SubmenuConfig submenuSelecionado = submenuPorIndice(indice);
    if (!submenuDisponivel(submenuSelecionado)) {
        mensagemMenu = mensagemSubmenuIndisponivel(submenuSelecionado);
        mensagemMenuErro = true;
        return;
    }

    if (submenuAtivo == SubmenuConfig::REDE && janelaRede) {
        janelaRede->resetar();
    }

    if (submenuAtivo == SubmenuConfig::AUDIO_VIDEO && janelaAudioVideo) {
        janelaAudioVideo->resetar();
    }

    if (submenuAtivo == SubmenuConfig::BLUETOOTH && janelaBluetooth) {
        janelaBluetooth->resetar();
    }

    submenuAtivo = submenuSelecionado;
    mensagemMenu.clear();
    mensagemMenuErro = false;
    
    // Limpa cache ao mudar de submenu
    limparCacheTexto();
}

// PROCESSAMENTO DE EVENTOS

/**
 * @brief Processa eventos SDL (mouse, controle, etc.).
 * 
 * @details Gerencia toda a interação do usuário com a interface, incluindo:
 * - Eventos de janela (fechar)
 * - Cliques do mouse em botões
 * - Navegação com controle (DPad, analógicos)
 * - Ações de confirmar (Botão A) e cancelar (Botão B)
 * - Redirecionamento de eventos para submenus ativos
 * 
 * @param evento Referência ao evento SDL a ser processado.
 * @return true se o evento foi processado, false caso contrário.
 */
bool JanelaConfiguracao::processarEvento(SDL_Event& evento) {
    if (!aberta) return false;

    if (evento.type == SDL_WINDOWEVENT && evento.window.windowID == getIDJanela()) {
        if (evento.window.event == SDL_WINDOWEVENT_CLOSE) {
            gerAudio.tocarSom("fechar.wav");
            fechar();
            return true;
        }
    }

    if (submenuAtivo != SubmenuConfig::NENHUM) {
        if (btnVoltar->tratarEvento(evento, 0, 0) && evento.type == SDL_MOUSEBUTTONUP) {
            gerAudio.tocarSom("navegacao.wav");
            submenuAtivo = SubmenuConfig::NENHUM;
            limparCacheTexto(); // Limpa ao voltar
            return true;
        }
    } else {
        for (size_t i = 0; i < botoesMenu.size(); i++) {
            if (botoesMenu[i]->tratarEvento(evento, 0, 0) && evento.type == SDL_MOUSEBUTTONUP) {
                gerAudio.tocarSom("navegacao.wav");
                executarAcaoMenu(i); 
                return true;
            }
        }
    }

    if (submenuAtivo == SubmenuConfig::REDE && janelaRede) {
        if (janelaRede->processarEvento(evento)) {
            return true;
        }
    }

    if (submenuAtivo == SubmenuConfig::AUDIO_VIDEO && janelaAudioVideo) {
        if (janelaAudioVideo->processarEvento(evento)) {
            return true;
        }
    }

    if (submenuAtivo == SubmenuConfig::BLUETOOTH && janelaBluetooth) {
        if (janelaBluetooth->processarEvento(evento)) {
            return true;
        }
    }

    if (submenuAtivo == SubmenuConfig::SISTEMA && janelaInfosSistema) {
        if (janelaInfosSistema->processarEvento(evento)) {
            return true;
        }
    }

    if (btnFechar->tratarEvento(evento, 0, 0) && evento.type == SDL_MOUSEBUTTONUP) {
        gerAudio.tocarSom("fechar.wav");
        fechar();
        return true;
    }
    
    if (evento.type == SDL_CONTROLLERAXISMOTION) {
        Uint32 tempoAtual = SDL_GetTicks();
        if (tempoAtual - ultimoInputAnalogico > INTERVALO_ANALOGICO) {
            if (submenuAtivo == SubmenuConfig::NENHUM) {
                if (indiceFocado == -1) indiceFocado = 0;
                
                if (evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                    if (evento.caxis.value > DEADZONE) {
                        if (indiceFocado >= (int)botoesMenu.size() - 1) indiceFocado = 0;
                        else indiceFocado++;
                        ajustarFocoMenuParaDisponivel(1);
                        gerAudio.tocarSom("navegacao.wav");
                        ultimoInputAnalogico = tempoAtual;
                        return true;
                    } 
                    else if (evento.caxis.value < -DEADZONE) {
                        if (indiceFocado <= 0) indiceFocado = botoesMenu.size() - 1;
                        else indiceFocado--;
                        ajustarFocoMenuParaDisponivel(-1);
                        gerAudio.tocarSom("navegacao.wav");
                        ultimoInputAnalogico = tempoAtual;
                        return true;
                    }
                }
            }
        }
    }

    if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
        if (indiceFocado == -1) indiceFocado = 0;

        if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
            if (submenuAtivo == SubmenuConfig::NENHUM) {
                if (indiceFocado <= 0) indiceFocado = botoesMenu.size() - 1;
                else indiceFocado--;
                ajustarFocoMenuParaDisponivel(-1);
                gerAudio.tocarSom("navegacao.wav");
            }
            return true;
        }
        else if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
            if (submenuAtivo == SubmenuConfig::NENHUM) {
                if (indiceFocado >= (int)botoesMenu.size() - 1) indiceFocado = 0;
                else indiceFocado++;
                ajustarFocoMenuParaDisponivel(1);
                gerAudio.tocarSom("navegacao.wav");
            }
            return true;
        }
        else if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
            gerAudio.tocarSom("navegacao.wav");
            if (submenuAtivo == SubmenuConfig::NENHUM && indiceFocado != -1) {
                executarAcaoMenu(indiceFocado);
            } else if (submenuAtivo != SubmenuConfig::NENHUM) {
                submenuAtivo = SubmenuConfig::NENHUM; 
                limparCacheTexto();
            }
            return true;
        }
        else if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
            if (submenuAtivo != SubmenuConfig::NENHUM) {
                submenuAtivo = SubmenuConfig::NENHUM; 
                limparCacheTexto();
            } else {
                gerAudio.tocarSom("fechar.wav");
                fechar(); 
            }
            return true;
        }
    }

    return false;
}

/**
 * @brief Obtém a hora atual formatada.
 * 
 * @return String com a hora atual no formato "HH:MM".
 */
std::string JanelaConfiguracao::obterHoraAtual() {
    std::time_t t = std::time(nullptr);
    std::tm* tm_info = std::localtime(&t);
    
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << tm_info->tm_hour << ":" 
       << std::setfill('0') << std::setw(2) << tm_info->tm_min;
    
    return ss.str();
}

/**
 * @brief Obtém o nível atual da bateria.
 * 
 * @details Usa as funções SDL para obter informações de energia.
 * 
 * @return Percentual da bateria (0-100), ou BATERIA_INDISPONIVEL se não houver bateria.
 */
int JanelaConfiguracao::obterNivelBateria() {
    int percentual = ::obter_bateria();
    if (percentual == BATERIA_INDISPONIVEL) {
        SDL_GetPowerInfo(nullptr, &percentual);
    }
    
    if (percentual < 0 || percentual > 100) {
        return BATERIA_INDISPONIVEL;
    }
    
    return percentual;
}

/**
 * @brief Obtém o título do submenu ativo.
 * 
 * @return String com o título do submenu ativo, ou string vazia se nenhum submenu estiver ativo.
 */
std::string JanelaConfiguracao::obterTituloSubmenu() {
    switch (submenuAtivo) {
        case SubmenuConfig::REDE:
            return "CONFIGURAÇÕES DE REDE";
        case SubmenuConfig::AUDIO_VIDEO:
            return "ÁUDIO & VÍDEO";
        case SubmenuConfig::BLUETOOTH:
            return "CONFIGURAÇÃO BLUETOOTH";
        case SubmenuConfig::SISTEMA:
            return "INFORMAÇÕES DO SISTEMA";
        default:
            return "";
    }
}

/**
 * @brief Desenha a barra de status no topo da janela.
 * 
 * @details Renderiza a hora atual e o nível da bateria com ícone.
 * A aparência varia conforme o tema atual (claro/escuro).
 */
void JanelaConfiguracao::desenharBarraStatus() {
    auto& tema = GerenciadorTemas::getInstance();
    
    std::string hora = obterHoraAtual();
    int bateria = obterNivelBateria();
    
    std::string textoBateria;
    if (bateria == BATERIA_INDISPONIVEL) {
        textoBateria = "AC/Desktop";
    } else {
        std::stringstream ssBateria;
        ssBateria << bateria << "%";
        textoBateria = ssBateria.str();
    }
    
    int posX = ConfigLayout::X(762);
    int posY = ConfigLayout::Y(25);
    
    // Desenha hora com cor do tema
    desenharTexto(renderer, hora, posX - ConfigLayout::X(100), posY, 
                  tema.getCorTextoNegrito(), ConfigLayout::F(24));
    
    if (bateria == BATERIA_INDISPONIVEL) {
        desenharTexto(renderer, textoBateria, posX + ConfigLayout::X(20), posY,
                      tema.getCorTextoNegrito(), ConfigLayout::F(24));
    } else if (texturaBateria) {
        int larguraIcone = ConfigLayout::X(100);
        int alturaIcone = ConfigLayout::Y(50);
        
        SDL_Rect destIcone = {
            posX + ConfigLayout::X(0),
            posY - ConfigLayout::Y(5),
            larguraIcone,
            alturaIcone
        };
        
        SDL_RenderCopy(renderer, texturaBateria, nullptr, &destIcone);
        
        desenharTexto(renderer, textoBateria, 
                      posX + ConfigLayout::X(130), posY, 
                      tema.getCorTextoNegrito(), ConfigLayout::F(24));
    } else {
        desenharTexto(renderer, textoBateria, posX + ConfigLayout::X(50), posY, 
                      tema.getCorTextoNegrito(), ConfigLayout::F(24));
    }
}

/**
 * @brief Carrega o ícone da bateria conforme o tema atual.
 * 
 * @details Carrega a textura do ícone da bateria (claro ou escuro) baseado no tema.
 * Usa o GerenciadorImagens para carregar a textura.
 */
void JanelaConfiguracao::carregarIconeBateria() {
    if (!renderer) return;
    
    auto& tema = GerenciadorTemas::getInstance();
    std::string caminhoIcone;
    
    if (tema.getTemaAtual() == TipoTema::CLARO) {
        caminhoIcone = "assets/images/light/bateriaClaro.png";
    } else {
        caminhoIcone = "assets/images/dark/bateriaEscuro.png";
    }
    
    texturaBateria = gerImg.carregar(renderer, caminhoIcone);
    
    if (!texturaBateria) {
        SDL_Log("[AVISO] Ícone de bateria não encontrado: %s", caminhoIcone.c_str());
    }
}

/**
 * @brief Libera o ícone da bateria da memória.
 */
void JanelaConfiguracao::liberarIconeBateria() {
    texturaBateria = nullptr;
}

/**
 * @brief Desenha o menu principal de configurações.
 * 
 * @details Renderiza o título "Configurações" e todos os botões do menu principal.
 * Diferencia visualmente o botão focado quando um controle está conectado.
 */
void JanelaConfiguracao::desenharMenuPrincipal() {
    auto& tema = GerenciadorTemas::getInstance();
    
    desenharTexto(renderer, "Configurações", 
                  ConfigLayout::X(535), ConfigLayout::Y(90), 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(60)); 
    
    for (int i = 0; i < (int)botoesMenu.size(); i++) {
        const bool disponivel = submenuDisponivel(submenuPorIndice(i));
        if (!disponivel) {
            botoesMenu[i]->setCor({80, 80, 80, 150}, {80, 80, 80, 150}, {80, 80, 80, 150});
            botoesMenu[i]->setCorTexto({180, 180, 180, 255});
        }

        if (i == indiceFocado && SDL_NumJoysticks() > 0) { 
            botoesMenu[i]->setFocado(true);
        } else {
            botoesMenu[i]->setFocado(false);
        }
        botoesMenu[i]->desenhar(renderer);

        if (!disponivel) {
            desenharTexto(renderer, "Indisponivel",
                          ConfigLayout::X(1125), botoesMenu[i]->area.y + ConfigLayout::Y(45),
                          SDL_Color{190, 190, 190, 255}, ConfigLayout::F(22));
        }
    }

    if (!mensagemMenu.empty()) {
        SDL_Color cor = mensagemMenuErro ? SDL_Color{255, 120, 120, 255}
                                         : SDL_Color{120, 255, 120, 255};
        desenharTexto(renderer, mensagemMenu,
                      ConfigLayout::X(217), ConfigLayout::Y(865),
                      cor, ConfigLayout::F(22));
    }
}

/**
 * @brief Desenha o submenu de configurações de rede.
 * 
 * @details Renderiza o fundo do submenu, barra de status e a janela de rede.
 * Inclui o botão de voltar para retornar ao menu principal.
 */
void JanelaConfiguracao::desenharSubmenuRede() {
    auto& tema = GerenciadorTemas::getInstance();
    
    SDL_Rect fundoSubmenu = {
        0, ConfigLayout::Y(80),
        ConfigLayout::X(1525), ConfigLayout::Y(1036)
    };
    SDL_Color corFundoSubmenu = tema.getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, corFundoSubmenu.r, corFundoSubmenu.g, 
                          corFundoSubmenu.b, corFundoSubmenu.a);
    SDL_RenderFillRect(renderer, &fundoSubmenu);
    
    desenharBarraStatus();
    
    if (!janelaRede) {
        janelaRede = std::make_unique<JanelaRede>(capacidadesSistema);
    }

    janelaRede->desenhar(renderer);
    
    if(btnVoltar) btnVoltar->desenhar(renderer);
}

/**
 * @brief Desenha o submenu de configurações de áudio e vídeo.
 * 
 * @details Renderiza o fundo do submenu, barra de status e a janela de áudio/vídeo.
 * Inclui o botão de voltar para retornar ao menu principal.
 */
void JanelaConfiguracao::desenharSubmenuAudioVideo() {
    auto& tema = GerenciadorTemas::getInstance();
    
    SDL_Rect fundoSubmenu = {
        0, ConfigLayout::Y(80),
        ConfigLayout::X(1525), ConfigLayout::Y(1036)
    };
    SDL_Color corFundoSubmenu = tema.getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, corFundoSubmenu.r, corFundoSubmenu.g, 
                          corFundoSubmenu.b, corFundoSubmenu.a);
    SDL_RenderFillRect(renderer, &fundoSubmenu);
    
    desenharBarraStatus();
    
    if (!janelaAudioVideo) {
        janelaAudioVideo = std::make_unique<JanelaAudioEVideo>(capacidadesSistema);
    }
    
    janelaAudioVideo->desenhar(renderer);
    
    if(btnVoltar) btnVoltar->desenhar(renderer);
}

/**
 * @brief Desenha o submenu de configurações Bluetooth.
 * 
 * @details Renderiza o fundo do submenu, barra de status e a janela Bluetooth.
 * Inclui o botão de voltar para retornar ao menu principal.
 */
void JanelaConfiguracao::desenharSubmenuBluetooth() {
    auto& tema = GerenciadorTemas::getInstance();
    
    SDL_Rect fundoSubmenu = {
        0, ConfigLayout::Y(80),
        ConfigLayout::X(1525), ConfigLayout::Y(1036)
    };
    SDL_Color corFundoSubmenu = tema.getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, corFundoSubmenu.r, corFundoSubmenu.g, 
                          corFundoSubmenu.b, corFundoSubmenu.a);
    SDL_RenderFillRect(renderer, &fundoSubmenu);
    
    desenharBarraStatus();
    
    if (!janelaBluetooth) {
        janelaBluetooth = std::make_unique<JanelaBluetooth>(capacidadesSistema);
    }
    
    janelaBluetooth->desenhar(renderer);
    
    if(btnVoltar) btnVoltar->desenhar(renderer);
}

/**
 * @brief Desenha o submenu de informações do sistema.
 * 
 * @details Renderiza o fundo do submenu, barra de status e a janela de informações do sistema.
 * Inclui o botão de voltar para retornar ao menu principal.
 */
void JanelaConfiguracao::desenharSubmenuSistemaInfos() {
    auto& tema = GerenciadorTemas::getInstance();
    
    SDL_Rect fundoSubmenu = {
        0, ConfigLayout::Y(80),
        ConfigLayout::X(1525), ConfigLayout::Y(1036)
    };
    SDL_Color corFundoSubmenu = tema.getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, corFundoSubmenu.r, corFundoSubmenu.g, 
                          corFundoSubmenu.b, corFundoSubmenu.a);
    SDL_RenderFillRect(renderer, &fundoSubmenu);
    
    desenharBarraStatus();
    
    if (!janelaInfosSistema) {
        janelaInfosSistema = std::make_unique<JanelaInfosSistema>();
        janelaInfosSistema->carregarTexturas(renderer);
    }
    
    janelaInfosSistema->desenhar(renderer);
    
    if(btnVoltar) btnVoltar->desenhar(renderer);
}

/**
 * @brief Desenha toda a interface da janela de configurações.
 * 
 * @details Renderiza todos os componentes da interface com base no estado atual
 * (menu principal ou submenu ativo). Inclui limpeza do renderer e apresentação final.
 */
void JanelaConfiguracao::desenhar() {
    if (!aberta || !renderer) return;

    auto& tema = GerenciadorTemas::getInstance();

    SDL_Color corFundo = tema.getCorFundo(); 
    SDL_SetRenderDrawColor(renderer, corFundo.r, corFundo.g, corFundo.b, corFundo.a);
    SDL_RenderClear(renderer);

    if(btnFechar) btnFechar->desenhar(renderer);

    if (submenuAtivo != SubmenuConfig::NENHUM) {
        switch (submenuAtivo) {
            case SubmenuConfig::REDE:
                desenharSubmenuRede();
                break;
            case SubmenuConfig::AUDIO_VIDEO:
                desenharSubmenuAudioVideo();
                break;
            case SubmenuConfig::BLUETOOTH:
                desenharSubmenuBluetooth();
                break;
            case SubmenuConfig::SISTEMA:
                desenharSubmenuSistemaInfos();
                break;
            default:
                break;
        }
    } else {
        desenharMenuPrincipal();
    }
    
    SDL_RenderPresent(renderer);
}
