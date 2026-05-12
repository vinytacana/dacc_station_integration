/**
 * @file JanelaAudioEVideo.cpp
 * @brief Implementação da interface de configurações de áudio e vídeo.
 *
 * Este arquivo contém toda a lógica visual e de interação para o submenu
 * de configurações de áudio e vídeo, incluindo controles de volume, dispositivos,
 * resolução e escala da janela.
 */

#include "JanelaAudioEVideo.hpp"
#include "config-dacc/functions.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include "GerenciadorAudio.hpp"
#include "GerenciadorImagens.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <SDL2/SDL.h>

using namespace MeuProjeto;

extern GerenciadorAudio gerAudio;
extern GerenciadorImagens gerImg;

namespace {

constexpr int LAYOUT_LARGURA_TELA = 1525;
constexpr int LAYOUT_CONTROLE_X = 250;
constexpr int LAYOUT_BARRA_X = 330;
constexpr int LAYOUT_BARRA_LARGURA = 910;
constexpr int LAYOUT_BOTAO_LARGURA = 60;
constexpr int LAYOUT_BOTAO_ALTURA = 50;
constexpr int LAYOUT_BOTAO_GAP = 20;
constexpr int LAYOUT_VALOR_X = 1350;
constexpr int LAYOUT_LABEL_Y_INICIAL = 250;
constexpr int LAYOUT_GRUPO_GAP = 135;
constexpr int LAYOUT_LABEL_CONTROLE_GAP = 45;
constexpr int LAYOUT_APLICAR_X = 1200;
constexpr int LAYOUT_APLICAR_Y = 970;
constexpr int LAYOUT_APLICAR_LARGURA = 200;
constexpr int LAYOUT_RODAPE_Y = 1050;
constexpr int LAYOUT_RODAPE_ALTURA = 30;

int labelY(int grupo) {
    return LAYOUT_LABEL_Y_INICIAL + (grupo * LAYOUT_GRUPO_GAP);
}

int controleY(int grupo) {
    return labelY(grupo) + LAYOUT_LABEL_CONTROLE_GAP;
}

int botaoIncrementoX() {
    return LAYOUT_BARRA_X + LAYOUT_BARRA_LARGURA + LAYOUT_BOTAO_GAP;
}

void configurarBotaoPorSuporte(std::unique_ptr<Botao>& botao, bool suportado) {
    if (!botao || suportado) {
        return;
    }

    botao->setCor({80, 80, 80, 150}, {80, 80, 80, 150}, {80, 80, 80, 150});
    botao->setCorTexto({180, 180, 180, 255});
    botao->setFocado(false);
}

SDL_Color corTextoSecundario(bool suportado, const SDL_Color& corPadrao) {
    return suportado ? corPadrao : SDL_Color{170, 170, 170, 255};
}

int calcularSegmentoSlider(int mouseX, const SDL_Rect& area, int totalSegmentos) {
    if (totalSegmentos <= 0 || area.w <= 0) {
        return 0;
    }

    int posicao = std::clamp(mouseX - area.x, 0, area.w - 1);
    int segmento = (posicao * totalSegmentos) / area.w + 1;
    return std::clamp(segmento, 0, totalSegmentos);
}

} // namespace

/**
 * @brief Retorna a resolução formatada como string.
 */
std::string Resolucao::toString() const {
    std::stringstream ss;
    ss << largura << "x" << altura;
    return ss.str();
}

/**
 * @brief Construtor da classe JanelaAudioEVideo.
 * Inicializa o estado e os componentes da tela.
 */
JanelaAudioEVideo::JanelaAudioEVideo()
    : JanelaAudioEVideo(::obter_capacidades_sistema()) {}

JanelaAudioEVideo::JanelaAudioEVideo(const station_capabilities& capacidades)
    : capacidadesSistema(capacidades) {
    inicializarDispositivos();
    inicializarResolucoes();
    inicializarBotoes();

    if (capacidadesSistema.volume_control) {
        int volume = volumeGeral;
        system_result volumeResult = ::obter_volume_atual_result(volume);
        if (volumeResult.ok) {
            volumeGeral = volume;
        } else {
            definirMensagemStatus(
                volumeResult.mensagem.empty() ? "Volume indisponivel." : volumeResult.mensagem,
                true
            );
        }
    } else {
        volumeGeral = 0;
    }

    if (capacidadesSistema.brightness) {
        int brilho = brilhoGeral;
        system_result brilhoResult = ::obter_brilho_result(brilho);
        if (brilhoResult.ok) {
            brilhoGeral = std::clamp(brilho, 0, MAX_BRILHO);
        } else {
            definirMensagemStatus(
                brilhoResult.mensagem.empty() ? "Brilho indisponivel." : brilhoResult.mensagem,
                true
            );
        }
    } else {
        brilhoGeral = 0;
    }
    
    // Inicializa áreas de interação das barras
    areaBarraVolume = {0, 0, 0, 0};
    areaBarraEscala = {0, 0, 0, 0};
    areaBarraBrilho = {0, 0, 0, 0};
}

/**
 * @brief Destrutor da classe JanelaAudioEVideo.
 */
JanelaAudioEVideo::~JanelaAudioEVideo() {
    btnVolumeDecremento.reset();
    btnVolumeIncremento.reset();
    btnDispositivoAnterior.reset();
    btnDispositivoProximo.reset();
    btnResolucaoAnterior.reset();
    btnResolucaoProxima.reset();
    btnEscalaDecremento.reset();
    btnEscalaIncremento.reset();
    btnBrilhoDecremento.reset();
    btnBrilhoIncremento.reset();
    
    // Libera a textura explicativa (o gerenciador já cuida da limpeza)
    texturaExplicacao = nullptr;
}

/**
 * @brief Inicializa a lista de dispositivos de áudio com dados de exemplo.
 */
void JanelaAudioEVideo::inicializarDispositivos() {
    dispositivos.clear();

    if (!capacidadesSistema.audio_list) {
        dispositivos.push_back(DispositivoAudio("Audio indisponivel", -1));
        indiceDispositivoAtual = 0;
        return;
    }
    
    std::vector<device_audio> listaDoSistema;
    system_result resultado = ::listar_dispositivos_audio_result(listaDoSistema);
    
    // Fallback se não encontrar nada
    if (!resultado.ok || listaDoSistema.empty()) {
        dispositivos.push_back(DispositivoAudio(
            resultado.mensagem.empty() ? "Nenhum dispositivo encontrado" : resultado.mensagem,
            -1
        ));
        indiceDispositivoAtual = 0;
        return;
    }

    indiceDispositivoAtual = 0; // Padrão
    
    for (size_t i = 0; i < listaDoSistema.size(); i++) {
        const auto& dev = listaDoSistema[i];
        
        // Cria o objeto da UI com (Nome, ID)
        dispositivos.push_back(DispositivoAudio(dev.descricao, dev.id));
        
        // 3. Se este for o dispositivo padrão (marcado com * no wpctl), seleciona ele na UI
        if (dev.padrao) {
            indiceDispositivoAtual = i;
        }
    }
    
}

/**
 * @brief Inicializa a lista de resoluções disponíveis.
 */
void JanelaAudioEVideo::inicializarResolucoes() {
    resolucoes.clear();

    if (!capacidadesSistema.display_info) {
        resolucoes.push_back(Resolucao(1920, 1080));
        indiceResolucaoAtual = 0;
        return;
    }
    
    // 1. Busca info do sistema
    std::vector<DisplayOutput> displays;
    system_result resultado = ::listar_displays_result(displays);
    
    if (!resultado.ok || displays.empty()) {
        // Fallback se não detectar nada (ex: rodando em VM sem xrandr)
        resolucoes.push_back(Resolucao(1920, 1080));
        resolucoes.push_back(Resolucao(1280, 720));
        indiceResolucaoAtual = 0;
        return;
    }

    // 2. Pega o primeiro monitor conectado
    const auto& displayPrincipal = displays[0];
    
    // 3. Preenche o vetor da UI
    for (const auto& mode : displayPrincipal.modes) {
        // Filtro: Evitar resoluções muito baixas que quebrem a UI
        if (mode.width >= 800) {
            resolucoes.push_back(Resolucao(mode.width, mode.height));
            
            // Tenta manter a seleção na resolução que o sistema diz ser a "current"
            if (mode.is_current) {
                indiceResolucaoAtual = resolucoes.size() - 1;
            }
        }
    }
    
    // Segurança caso o loop não tenha achado nada
    if (resolucoes.empty()) {
        resolucoes.push_back(Resolucao(1920, 1080));
        indiceResolucaoAtual = 0;
    }
    
    // Se o indice ficou invalido (-1), reseta
    if (indiceResolucaoAtual < 0) indiceResolucaoAtual = 0;
}

/**
 * @brief Inicializa os botões de navegação.
 */
void JanelaAudioEVideo::inicializarBotoes() {
    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color btnNormal = tema.getCorBotaoNormal();
    SDL_Color btnHover = tema.getCorBotaoHover();
    SDL_Color btnPress = tema.getCorBotaoPressionado();

    // Botões de Volume
    btnVolumeDecremento = std::make_unique<Botao>(
        ConfigLayout::X(LAYOUT_CONTROLE_X), ConfigLayout::Y(controleY(0)),
        ConfigLayout::X(LAYOUT_BOTAO_LARGURA), ConfigLayout::Y(LAYOUT_BOTAO_ALTURA),
        "-"
    );
    btnVolumeDecremento->setCor(btnNormal, btnHover, btnPress);
    btnVolumeDecremento->setRetanguloBordasArredondadas(15);

    btnVolumeIncremento = std::make_unique<Botao>(
        ConfigLayout::X(botaoIncrementoX()), ConfigLayout::Y(controleY(0)),
        ConfigLayout::X(LAYOUT_BOTAO_LARGURA), ConfigLayout::Y(LAYOUT_BOTAO_ALTURA),
        "+"
    );
    btnVolumeIncremento->setCor(btnNormal, btnHover, btnPress);
    btnVolumeIncremento->setRetanguloBordasArredondadas(15);

    // Botões de Dispositivo
    btnDispositivoAnterior = std::make_unique<Botao>(
        ConfigLayout::X(LAYOUT_CONTROLE_X), ConfigLayout::Y(controleY(1)),
        ConfigLayout::X(LAYOUT_BOTAO_LARGURA), ConfigLayout::Y(LAYOUT_BOTAO_ALTURA),
        "<"
    );
    btnDispositivoAnterior->setCor(btnNormal, btnHover, btnPress);
    btnDispositivoAnterior->setRetanguloBordasArredondadas(15);

    btnDispositivoProximo = std::make_unique<Botao>(
        ConfigLayout::X(botaoIncrementoX()), ConfigLayout::Y(controleY(1)),
        ConfigLayout::X(LAYOUT_BOTAO_LARGURA), ConfigLayout::Y(LAYOUT_BOTAO_ALTURA),
        ">"
    );
    btnDispositivoProximo->setCor(btnNormal, btnHover, btnPress);
    btnDispositivoProximo->setRetanguloBordasArredondadas(15);

    // Botões de Resolução
    btnResolucaoAnterior = std::make_unique<Botao>(
        ConfigLayout::X(LAYOUT_CONTROLE_X), ConfigLayout::Y(controleY(2)),
        ConfigLayout::X(LAYOUT_BOTAO_LARGURA), ConfigLayout::Y(LAYOUT_BOTAO_ALTURA),
        "<"
    );
    btnResolucaoAnterior->setCor(btnNormal, btnHover, btnPress);
    btnResolucaoAnterior->setRetanguloBordasArredondadas(15);

    btnResolucaoProxima = std::make_unique<Botao>(
        ConfigLayout::X(botaoIncrementoX()), ConfigLayout::Y(controleY(2)),
        ConfigLayout::X(LAYOUT_BOTAO_LARGURA), ConfigLayout::Y(LAYOUT_BOTAO_ALTURA),
        ">"
    );
    btnResolucaoProxima->setCor(btnNormal, btnHover, btnPress);
    btnResolucaoProxima->setRetanguloBordasArredondadas(15);

    // Botões de Escala
    btnEscalaDecremento = std::make_unique<Botao>(
        ConfigLayout::X(LAYOUT_CONTROLE_X), ConfigLayout::Y(controleY(3)),
        ConfigLayout::X(LAYOUT_BOTAO_LARGURA), ConfigLayout::Y(LAYOUT_BOTAO_ALTURA),
        "-"
    );
    btnEscalaDecremento->setCor(btnNormal, btnHover, btnPress);
    btnEscalaDecremento->setRetanguloBordasArredondadas(15);

    btnEscalaIncremento = std::make_unique<Botao>(
        ConfigLayout::X(botaoIncrementoX()), ConfigLayout::Y(controleY(3)),
        ConfigLayout::X(LAYOUT_BOTAO_LARGURA), ConfigLayout::Y(LAYOUT_BOTAO_ALTURA),
        "+"
    );
    btnEscalaIncremento->setCor(btnNormal, btnHover, btnPress);
    btnEscalaIncremento->setRetanguloBordasArredondadas(15);

    // Botões de Brilho
    btnBrilhoDecremento = std::make_unique<Botao>(
        ConfigLayout::X(LAYOUT_CONTROLE_X), ConfigLayout::Y(controleY(4)),
        ConfigLayout::X(LAYOUT_BOTAO_LARGURA), ConfigLayout::Y(LAYOUT_BOTAO_ALTURA),
        "-"
    );
    btnBrilhoDecremento->setCor(btnNormal, btnHover, btnPress);
    btnBrilhoDecremento->setRetanguloBordasArredondadas(15);

    btnBrilhoIncremento = std::make_unique<Botao>(
        ConfigLayout::X(botaoIncrementoX()), ConfigLayout::Y(controleY(4)),
        ConfigLayout::X(LAYOUT_BOTAO_LARGURA), ConfigLayout::Y(LAYOUT_BOTAO_ALTURA),
        "+"
    );
    btnBrilhoIncremento->setCor(btnNormal, btnHover, btnPress);
    btnBrilhoIncremento->setRetanguloBordasArredondadas(15);

    // Inicializa foco se houver controle conectado
    if (SDL_NumJoysticks() > 0) {
        indiceFocado = 0;
    } else {
        indiceFocado = -1;
    }

    // --- NOVO: Botão Aplicar ---
    // Posicionado lá embaixo, centralizado ou à direita
    btnAplicar = std::make_unique<Botao>(
        ConfigLayout::X(LAYOUT_APLICAR_X), ConfigLayout::Y(LAYOUT_APLICAR_Y),
        ConfigLayout::X(LAYOUT_APLICAR_LARGURA), ConfigLayout::Y(LAYOUT_BOTAO_ALTURA),
        "Aplicar"
    );
    btnAplicar->setCor(tema.getCorBotaoNormal(), tema.getCorBotaoHover(), tema.getCorBotaoPressionado());
    btnAplicar->setRetanguloBordasArredondadas(15);

    configurarBotaoPorSuporte(btnVolumeDecremento, capacidadesSistema.volume_control);
    configurarBotaoPorSuporte(btnVolumeIncremento, capacidadesSistema.volume_control);
    configurarBotaoPorSuporte(btnDispositivoAnterior, capacidadesSistema.audio_select);
    configurarBotaoPorSuporte(btnDispositivoProximo, capacidadesSistema.audio_select);
    configurarBotaoPorSuporte(btnResolucaoAnterior, capacidadesSistema.display_resolution);
    configurarBotaoPorSuporte(btnResolucaoProxima, capacidadesSistema.display_resolution);
    configurarBotaoPorSuporte(btnEscalaDecremento, capacidadesSistema.display_scale);
    configurarBotaoPorSuporte(btnEscalaIncremento, capacidadesSistema.display_scale);
    configurarBotaoPorSuporte(btnBrilhoDecremento, capacidadesSistema.brightness);
    configurarBotaoPorSuporte(btnBrilhoIncremento, capacidadesSistema.brightness);
    // ---------------------------
}

/**
 * @brief Renderiza toda a interface de configurações de áudio e vídeo.
 */
void JanelaAudioEVideo::desenhar(SDL_Renderer* renderer) {
    if (!renderer) return;

    desenharCabecalho(renderer);
    desenharInfoSistema(renderer);
    desenharStatusOperacional(renderer);
    desenharControleVolume(renderer);
    desenharSeletorDispositivo(renderer);
    desenharSeletorResolucao(renderer);
    desenharControleEscala(renderer);
    desenharControleBrilho(renderer);
    desenharImagemExplicativa(renderer);
    

    if (btnAplicar) {
        // Verifica se o foco está nele (índice 10)
        if (indiceFocado == 10 && SDL_NumJoysticks() > 0) {
            btnAplicar->setFocado(true);
        } else {
            btnAplicar->setFocado(false);
        }
        
        btnAplicar->desenhar(renderer);
    }


}

/**
 * @brief Renderiza o cabeçalho da seção.
 */
void JanelaAudioEVideo::desenharCabecalho(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Título da seção (centralizado)
    std::string titulo = "Configurações de Áudio e Vídeo";
    int tamanhoFonte = ConfigLayout::F(48);
    
    // Calcula largura do texto para centralizar
    // Estima aproximadamente 0.6 * tamanhoFonte por caractere
    int larguraEstimada = static_cast<int>(titulo.length() * tamanhoFonte * 0.4);
    int larguraTela = ConfigLayout::X(LAYOUT_LARGURA_TELA);
    int posX = (larguraTela - larguraEstimada) / 2;
    
    desenharTexto(renderer, titulo, 
                  posX, ConfigLayout::Y(150), 
                  tema.getCorTextoNegrito(), 
                  tamanhoFonte);
}

/**
 * @brief Renderiza o controle de volume com barra progressiva.
 */
void JanelaAudioEVideo::desenharControleVolume(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Label
    desenharTexto(renderer, "Volume Geral:",
                  ConfigLayout::X(LAYOUT_CONTROLE_X), ConfigLayout::Y(labelY(0)),
                  tema.getCorTextoNegrito(),
                  ConfigLayout::F(32));
    
    // Aplica foco aos botões se necessário (índices 0 e 1)
    if (capacidadesSistema.volume_control && indiceFocado == 0 && SDL_NumJoysticks() > 0) {
        btnVolumeDecremento->setFocado(true);
    } else {
        btnVolumeDecremento->setFocado(false);
    }
    
    if (capacidadesSistema.volume_control && indiceFocado == 1 && SDL_NumJoysticks() > 0) {
        btnVolumeIncremento->setFocado(true);
    } else {
        btnVolumeIncremento->setFocado(false);
    }
    
    // Desenha botões
    btnVolumeDecremento->desenhar(renderer);
    btnVolumeIncremento->desenhar(renderer);
    
    // Calcula quantas barras estão preenchidas
    int barrasPreenchidas = (volumeGeral * NUM_BARRAS_VOLUME) / MAX_VOLUME;
    
    // Define área da barra para interação com mouse
    int barraX = ConfigLayout::X(LAYOUT_BARRA_X);
    int barraY = ConfigLayout::Y(controleY(0));
    int barraLargura = ConfigLayout::X(LAYOUT_BARRA_LARGURA);
    int barraAltura = ConfigLayout::Y(LAYOUT_BOTAO_ALTURA);
    
    areaBarraVolume = {barraX, barraY, barraLargura, barraAltura};
    
    // Desenha a barra de volume
    SDL_Color corPreenchida = capacidadesSistema.volume_control ? tema.getCorDestaque() : SDL_Color{120, 120, 120, 160};
    SDL_Color corVazia = capacidadesSistema.volume_control ? tema.getCorBotaoNormal() : SDL_Color{70, 70, 70, 130};
    
    desenharBarraProgresso(renderer, barraX, barraY, barraLargura, barraAltura,
                          NUM_BARRAS_VOLUME, barrasPreenchidas, 
                          corPreenchida, corVazia);
    
    // Texto do percentual
    std::stringstream ss;
    ss << volumeGeral << "%";
    desenharTexto(renderer, ss.str(),
                  ConfigLayout::X(LAYOUT_VALOR_X), ConfigLayout::Y(controleY(0) + 10),
                  corTextoSecundario(capacidadesSistema.volume_control, tema.getCorTextoNegrito()),
                  ConfigLayout::F(28));

    if (!capacidadesSistema.volume_control) {
        desenharTexto(renderer, "Controle indisponivel",
                      ConfigLayout::X(LAYOUT_BARRA_X), ConfigLayout::Y(controleY(0) + 60),
                      SDL_Color{170, 170, 170, 255}, ConfigLayout::F(20));
    }
}

/**
 * @brief Renderiza o seletor de dispositivo de áudio.
 */
void JanelaAudioEVideo::desenharSeletorDispositivo(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Label
    desenharTexto(renderer, "Dispositivo de Saída:",
                  ConfigLayout::X(LAYOUT_CONTROLE_X), ConfigLayout::Y(labelY(1)),
                  tema.getCorTextoNegrito(),
                  ConfigLayout::F(32));
    
    // Aplica foco aos botões se necessário (índices 2 e 3)
    if (capacidadesSistema.audio_select && indiceFocado == 2 && SDL_NumJoysticks() > 0) {
        btnDispositivoAnterior->setFocado(true);
    } else {
        btnDispositivoAnterior->setFocado(false);
    }
    
    if (capacidadesSistema.audio_select && indiceFocado == 3 && SDL_NumJoysticks() > 0) {
        btnDispositivoProximo->setFocado(true);
    } else {
        btnDispositivoProximo->setFocado(false);
    }
    
    // Desenha botões
    btnDispositivoAnterior->desenhar(renderer);
    btnDispositivoProximo->desenhar(renderer);
    
    // Texto do dispositivo atual
    std::string dispositivo = dispositivos[indiceDispositivoAtual].nome;
    desenharTexto(renderer, dispositivo,
                  ConfigLayout::X(LAYOUT_BARRA_X), ConfigLayout::Y(controleY(1) + 10),
                  corTextoSecundario(capacidadesSistema.audio_select, tema.getCorTextoNormal()),
                  ConfigLayout::F(28));
}

/**
 * @brief Renderiza o seletor de resolução.
 */
void JanelaAudioEVideo::desenharSeletorResolucao(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Label
    desenharTexto(renderer, "Resolução:",
                  ConfigLayout::X(LAYOUT_CONTROLE_X), ConfigLayout::Y(labelY(2)),
                  tema.getCorTextoNegrito(),
                  ConfigLayout::F(32));
    
    // Aplica foco aos botões se necessário (índices 4 e 5)
    if (capacidadesSistema.display_resolution && indiceFocado == 4 && SDL_NumJoysticks() > 0) {
        btnResolucaoAnterior->setFocado(true);
    } else {
        btnResolucaoAnterior->setFocado(false);
    }
    
    if (capacidadesSistema.display_resolution && indiceFocado == 5 && SDL_NumJoysticks() > 0) {
        btnResolucaoProxima->setFocado(true);
    } else {
        btnResolucaoProxima->setFocado(false);
    }
    
    // Desenha botões
    btnResolucaoAnterior->desenhar(renderer);
    btnResolucaoProxima->desenhar(renderer);
    
    // Texto da resolução atual
    std::string resolucao = capacidadesSistema.display_resolution
        ? resolucoes[indiceResolucaoAtual].toString()
        : "Resolucao indisponivel";
    desenharTexto(renderer, resolucao,
                  ConfigLayout::X(LAYOUT_BARRA_X), ConfigLayout::Y(controleY(2) + 10),
                  corTextoSecundario(capacidadesSistema.display_resolution, tema.getCorTextoNormal()),
                  ConfigLayout::F(28));
}

/**
 * @brief Renderiza o controle de escala com barra progressiva.
 */
void JanelaAudioEVideo::desenharControleEscala(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Label
    desenharTexto(renderer, "Escala da Janela:",
                  ConfigLayout::X(LAYOUT_CONTROLE_X), ConfigLayout::Y(labelY(3)),
                  tema.getCorTextoNegrito(),
                  ConfigLayout::F(32));
    
    // Aplica foco aos botões se necessário (índices 6 e 7)
    if (capacidadesSistema.display_scale && indiceFocado == 6 && SDL_NumJoysticks() > 0) {
        btnEscalaDecremento->setFocado(true);
    } else {
        btnEscalaDecremento->setFocado(false);
    }
    
    if (capacidadesSistema.display_scale && indiceFocado == 7 && SDL_NumJoysticks() > 0) {
        btnEscalaIncremento->setFocado(true);
    } else {
        btnEscalaIncremento->setFocado(false);
    }
    
    // Desenha botões
    btnEscalaDecremento->desenhar(renderer);
    btnEscalaIncremento->desenhar(renderer);
    
    // Calcula quantas barras estão preenchidas
    // Escala vai de 0.5 a 2.0, então normalizamos para 0-15 barras
    float escalaRelativa = (escalaJanela - MIN_ESCALA) / (MAX_ESCALA - MIN_ESCALA);
    int barrasPreenchidas = static_cast<int>(escalaRelativa * NUM_BARRAS_ESCALA);
    
    // Define área da barra para interação com mouse
    int barraX = ConfigLayout::X(LAYOUT_BARRA_X);
    int barraY = ConfigLayout::Y(controleY(3));
    int barraLargura = ConfigLayout::X(LAYOUT_BARRA_LARGURA);
    int barraAltura = ConfigLayout::Y(LAYOUT_BOTAO_ALTURA);
    
    areaBarraEscala = {barraX, barraY, barraLargura, barraAltura};
    
    // Desenha a barra de escala
    SDL_Color corPreenchida = capacidadesSistema.display_scale ? tema.getCorDestaque() : SDL_Color{120, 120, 120, 160};
    SDL_Color corVazia = capacidadesSistema.display_scale ? tema.getCorBotaoNormal() : SDL_Color{70, 70, 70, 130};
    
    desenharBarraProgresso(renderer, barraX, barraY, barraLargura, barraAltura,
                          NUM_BARRAS_ESCALA, barrasPreenchidas, 
                          corPreenchida, corVazia);
    
    // Texto da escala
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << escalaJanela << "x";
    desenharTexto(renderer, ss.str(),
                  ConfigLayout::X(LAYOUT_VALOR_X), ConfigLayout::Y(controleY(3) + 10),
                  corTextoSecundario(capacidadesSistema.display_scale, tema.getCorTextoNegrito()),
                  ConfigLayout::F(28));

    if (!capacidadesSistema.display_scale) {
        desenharTexto(renderer, "Escala indisponivel nesta sessao",
                      ConfigLayout::X(LAYOUT_BARRA_X), ConfigLayout::Y(controleY(3) + 60),
                      SDL_Color{170, 170, 170, 255}, ConfigLayout::F(20));
    }
}

/**
 * @brief Renderiza o controle de brilho com barra progressiva.
 */
void JanelaAudioEVideo::desenharControleBrilho(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();

    desenharTexto(renderer, "Brilho:",
                  ConfigLayout::X(LAYOUT_CONTROLE_X), ConfigLayout::Y(labelY(4)),
                  tema.getCorTextoNegrito(),
                  ConfigLayout::F(28));

    if (capacidadesSistema.brightness && indiceFocado == 8 && SDL_NumJoysticks() > 0) {
        btnBrilhoDecremento->setFocado(true);
    } else {
        btnBrilhoDecremento->setFocado(false);
    }

    if (capacidadesSistema.brightness && indiceFocado == 9 && SDL_NumJoysticks() > 0) {
        btnBrilhoIncremento->setFocado(true);
    } else {
        btnBrilhoIncremento->setFocado(false);
    }

    btnBrilhoDecremento->desenhar(renderer);
    btnBrilhoIncremento->desenhar(renderer);

    int barrasPreenchidas = (brilhoGeral * NUM_BARRAS_BRILHO) / MAX_BRILHO;

    int barraX = ConfigLayout::X(LAYOUT_BARRA_X);
    int barraY = ConfigLayout::Y(controleY(4));
    int barraLargura = ConfigLayout::X(LAYOUT_BARRA_LARGURA);
    int barraAltura = ConfigLayout::Y(LAYOUT_BOTAO_ALTURA);

    areaBarraBrilho = {barraX, barraY, barraLargura, barraAltura};

    SDL_Color corPreenchida = capacidadesSistema.brightness ? tema.getCorDestaque() : SDL_Color{120, 120, 120, 160};
    SDL_Color corVazia = capacidadesSistema.brightness ? tema.getCorBotaoNormal() : SDL_Color{70, 70, 70, 130};

    desenharBarraProgresso(renderer, barraX, barraY, barraLargura, barraAltura,
                          NUM_BARRAS_BRILHO, barrasPreenchidas,
                          corPreenchida, corVazia);

    std::stringstream ss;
    ss << brilhoGeral << "%";
    desenharTexto(renderer, ss.str(),
                  ConfigLayout::X(LAYOUT_VALOR_X), ConfigLayout::Y(controleY(4) + 10),
                  corTextoSecundario(capacidadesSistema.brightness, tema.getCorTextoNegrito()),
                  ConfigLayout::F(26));

    if (!capacidadesSistema.brightness) {
        desenharTexto(renderer, "Brilho indisponivel neste ambiente",
                      ConfigLayout::X(LAYOUT_BARRA_X), ConfigLayout::Y(controleY(4) + 60),
                      SDL_Color{170, 170, 170, 255}, ConfigLayout::F(18));
    }
}

/**
 * @brief Renderiza a imagem explicativa no rodapé da tela.
 * 
 * A imagem é exibida com altura fixa de 30px ocupando toda a largura
 * da tela, posicionada na parte inferior. A textura é carregada
 * automaticamente de acordo com o tema ativo (claro/escuro).
 */
void JanelaAudioEVideo::desenharImagemExplicativa(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    static TipoTema ultimoTema = static_cast<TipoTema>(-1);

    std::string caminhoImagem;
    if (tema.getTemaAtual() == TipoTema::CLARO) {
        caminhoImagem = "assets/images/light/explicacaoBotoesJanelaAudioEVideoClaro.jpg";
    } else {
        caminhoImagem = "assets/images/dark/explicacaoBotoesJanelaAudioEVideoEscuro.jpg";
    }

    if (texturaExplicacao == nullptr || ultimoTema != tema.getTemaAtual()) {
        texturaExplicacao = gerImg.carregar(renderer, caminhoImagem);
        ultimoTema = tema.getTemaAtual();
    }
    
    if (!texturaExplicacao) {
        return;
    }
    
    // 1. Define a altura fixa da imagem e largura total da tela
    int larguraTela = ConfigLayout::X(LAYOUT_LARGURA_TELA);
    int alturaImagem = ConfigLayout::Y(LAYOUT_RODAPE_ALTURA);
    
    // 2. Define a posição Y como (Altura da Tela - Altura da Imagem)
    int posY = ConfigLayout::Y(LAYOUT_RODAPE_Y);
    
    // 3. Monta o retângulo de destino
    SDL_Rect destExplicacao = {
        0,              // X inicial no canto esquerdo
        posY,           // Y calculado para o rodapé
        larguraTela,    // Largura total
        alturaImagem    // Altura de 30px
    };
    
    // 4. Renderiza
    SDL_RenderCopy(renderer, texturaExplicacao, nullptr, &destExplicacao);
}

/**
 * @brief Desenha uma barra de progresso visual com segmentos.
 */
void JanelaAudioEVideo::desenharBarraProgresso(SDL_Renderer* renderer, int x, int y, 
                                               int larguraTotal, int altura, int numBarras, 
                                               int barrasPreenchidas, SDL_Color corPreenchida, 
                                               SDL_Color corVazia) {
    const int espacamento = 4; // Espaço entre as barrinhas
    int larguraBarra = (larguraTotal - (espacamento * (numBarras - 1))) / numBarras;
    
    for (int i = 0; i < numBarras; i++) {
        int posX = x + i * (larguraBarra + espacamento);
        
        SDL_Rect barra = {posX, y, larguraBarra, altura};
        
        // Escolhe a cor baseado no preenchimento
        SDL_Color cor = (i < barrasPreenchidas) ? corPreenchida : corVazia;
        
        // Desenha a barra com bordas arredondadas (simulado com retângulos)
        SDL_SetRenderDrawColor(renderer, cor.r, cor.g, cor.b, cor.a);
        SDL_RenderFillRect(renderer, &barra);
    }
}

/**
 * @brief Incrementa o volume.
 */
void JanelaAudioEVideo::aumentarVolume() {
    if (!capacidadesSistema.volume_control) {
        definirMensagemStatus("Controle de volume indisponivel.", true);
        return;
    }

    if (volumeGeral < MAX_VOLUME) {
        volumeGeral = std::min(MAX_VOLUME, volumeGeral + 5);
        gerAudio.tocarSom("select.wav");

        system_result resultado = ::aumentar_volume_result();
        if (!resultado.ok) {
            definirMensagemStatus(
                resultado.mensagem.empty() ? "Falha ao aumentar volume." : resultado.mensagem,
                true
            );
            return;
        }
    }
}

/**
 * @brief Decrementa o volume.
 */
void JanelaAudioEVideo::diminuirVolume() {
    if (!capacidadesSistema.volume_control) {
        definirMensagemStatus("Controle de volume indisponivel.", true);
        return;
    }

    if (volumeGeral > 0) {
        volumeGeral = std::max(0, volumeGeral - 5);
        gerAudio.tocarSom("select.wav");
        system_result resultado = ::diminuir_volume_result();
        if (!resultado.ok) {
            definirMensagemStatus(
                resultado.mensagem.empty() ? "Falha ao diminuir volume." : resultado.mensagem,
                true
            );
            return;
        }
    }
}

/**
 * @brief Define o volume diretamente.
 */
void JanelaAudioEVideo::setVolume(int novoVolume) {
    int volumeAnterior = volumeGeral;
    atualizarVolumeVisual(novoVolume);
    if (!aplicarVolumeAtual()) {
        volumeGeral = volumeAnterior;
    }
}

void JanelaAudioEVideo::atualizarVolumeVisual(int novoVolume) {
    volumeGeral = std::clamp(novoVolume, 0, MAX_VOLUME);
}

bool JanelaAudioEVideo::aplicarVolumeAtual() {
    if (!capacidadesSistema.volume_control) {
        definirMensagemStatus("Controle de volume indisponivel.", true);
        return false;
    }

    system_result resultado = ::definir_volume_result(volumeGeral);
    if (!resultado.ok) {
        definirMensagemStatus(
            resultado.mensagem.empty() ? "Falha ao definir volume." : resultado.mensagem,
            true
        );
        return false;
    }
    return true;
}

/**
 * @brief Incrementa o brilho.
 */
void JanelaAudioEVideo::aumentarBrilho() {
    if (!capacidadesSistema.brightness) {
        definirMensagemStatus("Controle de brilho indisponivel.", true);
        return;
    }

    if (brilhoGeral < MAX_BRILHO) {
        setBrilho(std::min(MAX_BRILHO, brilhoGeral + 5));
        gerAudio.tocarSom("select.wav");
    }
}

/**
 * @brief Decrementa o brilho.
 */
void JanelaAudioEVideo::diminuirBrilho() {
    if (!capacidadesSistema.brightness) {
        definirMensagemStatus("Controle de brilho indisponivel.", true);
        return;
    }

    if (brilhoGeral > 0) {
        setBrilho(std::max(0, brilhoGeral - 5));
        gerAudio.tocarSom("select.wav");
    }
}

/**
 * @brief Define o brilho diretamente.
 */
void JanelaAudioEVideo::setBrilho(int novoBrilho) {
    int brilhoAnterior = brilhoGeral;
    atualizarBrilhoVisual(novoBrilho);
    if (!aplicarBrilhoAtual()) {
        brilhoGeral = brilhoAnterior;
    }
}

void JanelaAudioEVideo::atualizarBrilhoVisual(int novoBrilho) {
    brilhoGeral = std::clamp(novoBrilho, 0, MAX_BRILHO);
}

bool JanelaAudioEVideo::aplicarBrilhoAtual() {
    if (!capacidadesSistema.brightness) {
        definirMensagemStatus("Controle de brilho indisponivel.", true);
        return false;
    }

    system_result resultado = ::definir_brilho_result(brilhoGeral);
    if (!resultado.ok) {
        definirMensagemStatus(
            resultado.mensagem.empty() ? "Falha ao definir brilho." : resultado.mensagem,
            true
        );
        return false;
    }
    return true;
}

/**
 * @brief Seleciona o dispositivo anterior.
 */
void JanelaAudioEVideo::dispositivoAnterior() {
    if (!capacidadesSistema.audio_select) {
        definirMensagemStatus("Selecao de dispositivo de audio indisponivel.", true);
        return;
    }
    if (dispositivos.empty()) return;
    if (indiceDispositivoAtual > 0) {
        indiceDispositivoAtual--;
    } else {
        indiceDispositivoAtual = dispositivos.size() - 1; 
    }
    gerAudio.tocarSom("navegacao.wav");

}

/**
 * @brief Seleciona o próximo dispositivo.
 */
void JanelaAudioEVideo::dispositivoProximo() {
    if (!capacidadesSistema.audio_select) {
        definirMensagemStatus("Selecao de dispositivo de audio indisponivel.", true);
        return;
    }
    if (dispositivos.empty()) return;
    if (indiceDispositivoAtual < (int)dispositivos.size() - 1) {
        indiceDispositivoAtual++;
    } else {
        indiceDispositivoAtual = 0; 
    }
    gerAudio.tocarSom("navegacao.wav");
}

/**
 * @brief Seleciona a resolução anterior.
 */
void JanelaAudioEVideo::resolucaoAnterior() {
    if (!capacidadesSistema.display_resolution) {
        definirMensagemStatus("Controle de resolucao indisponivel.", true);
        return;
    }
    if (indiceResolucaoAtual > 0) {
        indiceResolucaoAtual--;
    } else {
        indiceResolucaoAtual = resolucoes.size() - 1; // Wrap around
    }
    gerAudio.tocarSom("navegacao.wav");
}

/**
 * @brief Seleciona a próxima resolução.
 */
void JanelaAudioEVideo::resolucaoProxima() {
    if (!capacidadesSistema.display_resolution) {
        definirMensagemStatus("Controle de resolucao indisponivel.", true);
        return;
    }
    if (indiceResolucaoAtual < (int)resolucoes.size() - 1) {
        indiceResolucaoAtual++;
    } else {
        indiceResolucaoAtual = 0; // Wrap around
    }
    gerAudio.tocarSom("navegacao.wav");
}

/**
 * @brief Incrementa a escala da janela.
 */
void JanelaAudioEVideo::aumentarEscala() {
    if (!capacidadesSistema.display_scale) {
        definirMensagemStatus("Controle de escala indisponivel.", true);
        return;
    }
    if (escalaJanela < MAX_ESCALA) {
        escalaJanela = std::min(MAX_ESCALA, escalaJanela + PASSO_ESCALA);
        gerAudio.tocarSom("select.wav");
    }
}

/**
 * @brief Decrementa a escala da janela.
 */
void JanelaAudioEVideo::diminuirEscala() {
    if (!capacidadesSistema.display_scale) {
        definirMensagemStatus("Controle de escala indisponivel.", true);
        return;
    }
    if (escalaJanela > MIN_ESCALA) {
        escalaJanela = std::max(MIN_ESCALA, escalaJanela - PASSO_ESCALA);
        gerAudio.tocarSom("select.wav");
    }
}

/**
 * @brief Define a escala diretamente.
 */
void JanelaAudioEVideo::setEscala(float novaEscala) {
    if (!capacidadesSistema.display_scale) {
        definirMensagemStatus("Controle de escala indisponivel.", true);
        return;
    }
    escalaJanela = std::clamp(novaEscala, MIN_ESCALA, MAX_ESCALA);
}

void JanelaAudioEVideo::definirMensagemStatus(const std::string& mensagem, bool erro) {
    mensagemStatus = mensagem;
    mensagemErro = erro;
}

void JanelaAudioEVideo::desenharStatusOperacional(SDL_Renderer* renderer) {
    if (mensagemStatus.empty()) {
        return;
    }
    SDL_Color cor = mensagemErro ? SDL_Color{255, 120, 120, 255}
                                 : SDL_Color{120, 255, 120, 255};
    desenharTexto(renderer, mensagemStatus,
                  ConfigLayout::X(250), ConfigLayout::Y(220),
                  cor, ConfigLayout::F(20));
}

/**
 * @brief Move o foco para o elemento anterior.
 */
void JanelaAudioEVideo::navegarParaCima() {
    if (indiceFocado > 0) {
        // Move dois índices para cima (pula o par de botões)
        indiceFocado = std::max(0, indiceFocado - 2);
        gerAudio.tocarSom("navegacao.wav");
    }
}

/**
 * @brief Move o foco para o próximo elemento.
 */
void JanelaAudioEVideo::navegarParaBaixo() {
    if (indiceFocado < NUM_ELEMENTOS_FOCAVEIS - 1) {
        // Move dois índices para baixo (pula o par de botões)
        indiceFocado = std::min(NUM_ELEMENTOS_FOCAVEIS - 1, indiceFocado + 2);
        gerAudio.tocarSom("navegacao.wav");
    }
}

/**
 * @brief Move o foco para a esquerda (botão - do grupo).
 */
void JanelaAudioEVideo::navegarParaEsquerda() {
    // Se está no botão direito (+/>), move para o esquerdo (-/<)
    if (indiceFocado % 2 == 1) {
        indiceFocado--;
        gerAudio.tocarSom("navegacao.wav");
    }
}

/**
 * @brief Move o foco para a direita (botão + do grupo).
 */
void JanelaAudioEVideo::navegarParaDireita() {
    // Se está no botão esquerdo (-/<), move para o direito (+/>)
    if (indiceFocado % 2 == 0 && indiceFocado < NUM_ELEMENTOS_FOCAVEIS - 1) {
        indiceFocado++;
        gerAudio.tocarSom("navegacao.wav");
    }
}

/**
 * @brief Confirma a seleção do elemento focado (executa a ação do botão).
 */
void JanelaAudioEVideo::confirmarSelecao() {
    switch (indiceFocado) {
        case 0: diminuirVolume(); break;
        case 1: aumentarVolume(); break;
        case 2: dispositivoAnterior(); break;
        case 3: dispositivoProximo(); break;
        case 4: resolucaoAnterior(); break;
        case 5: resolucaoProxima(); break;
        case 6: diminuirEscala(); break;
        case 7: aumentarEscala(); break;
        case 8: diminuirBrilho(); break;
        case 9: aumentarBrilho(); break;
        case 10: aplicarAlteracoes(); break;
    }
}

/**
 * @brief Calcula o volume baseado na posição X do mouse.
 */
int JanelaAudioEVideo::calcularVolumeAPartirDoPonto(int mouseX) {
    int segmento = calcularSegmentoSlider(mouseX, areaBarraVolume, NUM_BARRAS_VOLUME);
    return segmento * (MAX_VOLUME / NUM_BARRAS_VOLUME);
}

/**
 * @brief Calcula a escala baseada na posição X do mouse.
 */
float JanelaAudioEVideo::calcularEscalaAPartirDoPonto(int mouseX) {
    int segmento = calcularSegmentoSlider(mouseX, areaBarraEscala, NUM_BARRAS_ESCALA);
    float passoVisual = (MAX_ESCALA - MIN_ESCALA) / NUM_BARRAS_ESCALA;
    return MIN_ESCALA + (segmento * passoVisual);
}

/**
 * @brief Calcula o brilho baseado na posição X do mouse.
 */
int JanelaAudioEVideo::calcularBrilhoAPartirDoPonto(int mouseX) {
    int segmento = calcularSegmentoSlider(mouseX, areaBarraBrilho, NUM_BARRAS_BRILHO);
    return segmento * (MAX_BRILHO / NUM_BARRAS_BRILHO);
}

/**
 * @brief Processa clique na barra de volume.
 */
bool JanelaAudioEVideo::processarCliqueBarraVolume(int mouseX, int mouseY, bool aplicarBackend) {
    if (!capacidadesSistema.volume_control) {
        return false;
    }
    if (mouseX >= areaBarraVolume.x && mouseX <= areaBarraVolume.x + areaBarraVolume.w &&
        mouseY >= areaBarraVolume.y && mouseY <= areaBarraVolume.y + areaBarraVolume.h) {
        int novoVolume = calcularVolumeAPartirDoPonto(mouseX);
        atualizarVolumeVisual(novoVolume);
        if (aplicarBackend && aplicarVolumeAtual()) {
            gerAudio.tocarSom("select.wav");
        }
        return true;
    }
    return false;
}

/**
 * @brief Processa clique na barra de escala.
 */
bool JanelaAudioEVideo::processarCliqueBarraEscala(int mouseX, int mouseY) {
    if (!capacidadesSistema.display_scale) {
        return false;
    }
    if (mouseX >= areaBarraEscala.x && mouseX <= areaBarraEscala.x + areaBarraEscala.w &&
        mouseY >= areaBarraEscala.y && mouseY <= areaBarraEscala.y + areaBarraEscala.h) {
        float novaEscala = calcularEscalaAPartirDoPonto(mouseX);
        setEscala(novaEscala);
        gerAudio.tocarSom("select.wav");
        return true;
    }
    return false;
}

/**
 * @brief Processa clique na barra de brilho.
 */
bool JanelaAudioEVideo::processarCliqueBarraBrilho(int mouseX, int mouseY, bool aplicarBackend) {
    if (!capacidadesSistema.brightness) {
        return false;
    }
    if (mouseX >= areaBarraBrilho.x && mouseX <= areaBarraBrilho.x + areaBarraBrilho.w &&
        mouseY >= areaBarraBrilho.y && mouseY <= areaBarraBrilho.y + areaBarraBrilho.h) {
        int novoBrilho = calcularBrilhoAPartirDoPonto(mouseX);
        atualizarBrilhoVisual(novoBrilho);
        if (aplicarBackend && aplicarBrilhoAtual()) {
            gerAudio.tocarSom("select.wav");
        }
        return true;
    }
    return false;
}

/**
 * @brief Processa eventos de input específicos para esta tela.
 */
bool JanelaAudioEVideo::processarEvento(SDL_Event& evento) {
    // Processa cliques do mouse
    if (evento.type == SDL_MOUSEBUTTONDOWN) {
        if (evento.button.button == SDL_BUTTON_LEFT) {
            int mouseX = evento.button.x;
            int mouseY = evento.button.y;
            
            // Verifica clique nas barras
            volumeAntesArrasto = volumeGeral;
            if (processarCliqueBarraVolume(mouseX, mouseY, false)) {
                arrastandoVolume = true;
                return true;
            }
            
            if (processarCliqueBarraEscala(mouseX, mouseY)) {
                arrastandoEscala = true;
                return true;
            }

            brilhoAntesArrasto = brilhoGeral;
            if (processarCliqueBarraBrilho(mouseX, mouseY, false)) {
                arrastandoBrilho = true;
                return true;
            }
            
            // Verifica clique nos botões
            if (capacidadesSistema.volume_control && btnVolumeDecremento && btnVolumeDecremento->contemPonto(mouseX, mouseY)) {
                diminuirVolume();
                return true;
            }
            if (capacidadesSistema.volume_control && btnVolumeIncremento && btnVolumeIncremento->contemPonto(mouseX, mouseY)) {
                aumentarVolume();
                return true;
            }
            
            if (capacidadesSistema.audio_select && btnDispositivoAnterior && btnDispositivoAnterior->contemPonto(mouseX, mouseY)) {
                dispositivoAnterior();
                return true;
            }
            if (capacidadesSistema.audio_select && btnDispositivoProximo && btnDispositivoProximo->contemPonto(mouseX, mouseY)) {
                dispositivoProximo();
                return true;
            }
            
            if (capacidadesSistema.display_resolution && btnResolucaoAnterior && btnResolucaoAnterior->contemPonto(mouseX, mouseY)) {
                resolucaoAnterior();
                return true;
            }
            if (capacidadesSistema.display_resolution && btnResolucaoProxima && btnResolucaoProxima->contemPonto(mouseX, mouseY)) {
                resolucaoProxima();
                return true;
            }
            
            if (capacidadesSistema.display_scale && btnEscalaDecremento && btnEscalaDecremento->contemPonto(mouseX, mouseY)) {
                diminuirEscala();
                return true;
            }
            if (capacidadesSistema.display_scale && btnEscalaIncremento && btnEscalaIncremento->contemPonto(mouseX, mouseY)) {
                aumentarEscala();
                return true;
            }

            if (capacidadesSistema.brightness && btnBrilhoDecremento && btnBrilhoDecremento->contemPonto(mouseX, mouseY)) {
                diminuirBrilho();
                return true;
            }
            if (capacidadesSistema.brightness && btnBrilhoIncremento && btnBrilhoIncremento->contemPonto(mouseX, mouseY)) {
                aumentarBrilho();
                return true;
            }


            //verifica clique no botao
            if (btnAplicar && btnAplicar->contemPonto(mouseX, mouseY)) {
                aplicarAlteracoes(); // <--- CHAMA AQUI
                return true;
            }
        }
    }
    
    // Processa soltar botão do mouse (fim do arrasto)
    if (evento.type == SDL_MOUSEBUTTONUP) {
        if (evento.button.button == SDL_BUTTON_LEFT) {
            bool aplicarVolume = arrastandoVolume;
            bool aplicarBrilho = arrastandoBrilho;

            arrastandoVolume = false;
            arrastandoEscala = false;
            arrastandoBrilho = false;

            if (aplicarVolume) {
                if (aplicarVolumeAtual()) {
                    gerAudio.tocarSom("select.wav");
                } else {
                    volumeGeral = volumeAntesArrasto;
                }
                return true;
            }

            if (aplicarBrilho) {
                if (aplicarBrilhoAtual()) {
                    gerAudio.tocarSom("select.wav");
                } else {
                    brilhoGeral = brilhoAntesArrasto;
                }
                return true;
            }
        }
    }
    
    // Processa arrasto
    if (evento.type == SDL_MOUSEMOTION) {
        int mouseX = evento.motion.x;
        int mouseY = evento.motion.y;
        
        if (arrastandoVolume) {
            if (processarCliqueBarraVolume(mouseX, mouseY, false)) {
                return true;
            }
        }
        
        if (arrastandoEscala) {
            if (processarCliqueBarraEscala(mouseX, mouseY)) {
                return true;
            }
        }

        if (arrastandoBrilho) {
            if (processarCliqueBarraBrilho(mouseX, mouseY, false)) {
                return true;
            }
        }
        
        // Atualiza hover dos botões
        if (btnVolumeDecremento) btnVolumeDecremento->handleMouseMotion(evento, 0, 0);
        if (btnVolumeIncremento) btnVolumeIncremento->handleMouseMotion(evento, 0, 0);
        if (btnDispositivoAnterior) btnDispositivoAnterior->handleMouseMotion(evento, 0, 0);
        if (btnDispositivoProximo) btnDispositivoProximo->handleMouseMotion(evento, 0, 0);
        if (btnResolucaoAnterior) btnResolucaoAnterior->handleMouseMotion(evento, 0, 0);
        if (btnResolucaoProxima) btnResolucaoProxima->handleMouseMotion(evento, 0, 0);
        if (btnEscalaDecremento) btnEscalaDecremento->handleMouseMotion(evento, 0, 0);
        if (btnEscalaIncremento) btnEscalaIncremento->handleMouseMotion(evento, 0, 0);
        if (btnBrilhoDecremento) btnBrilhoDecremento->handleMouseMotion(evento, 0, 0);
        if (btnBrilhoIncremento) btnBrilhoIncremento->handleMouseMotion(evento, 0, 0);
    }
    
    // Navegação por controle/teclado
    if (evento.type == SDL_KEYDOWN || evento.type == SDL_CONTROLLERBUTTONDOWN) {
        bool paraEsquerda = false;
        bool paraDireita = false;
        bool paraCima = false;
        bool paraBaixo = false;
        bool confirmar = false;
        
        if (evento.type == SDL_KEYDOWN) {
            switch (evento.key.keysym.sym) {
                case SDLK_LEFT:
                case SDLK_a:
                    paraEsquerda = true;
                    break;
                case SDLK_RIGHT:
                case SDLK_d:
                    paraDireita = true;
                    break;
                case SDLK_UP:
                case SDLK_w:
                    paraCima = true;
                    break;
                case SDLK_DOWN:
                case SDLK_s:
                    paraBaixo = true;
                    break;
                case SDLK_RETURN:
                case SDLK_SPACE:
                    confirmar = true;
                    break;
            }
        } else if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
            switch (evento.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    paraEsquerda = true;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    paraDireita = true;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    paraCima = true;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    paraBaixo = true;
                    break;
                case SDL_CONTROLLER_BUTTON_A:
                    confirmar = true;
                    break;
            }
        }
        
        if (paraEsquerda) {
            navegarParaEsquerda();
            return true;
        }
        if (paraDireita) {
            navegarParaDireita();
            return true;
        }
        if (paraCima) {
            navegarParaCima();
            return true;
        }
        if (paraBaixo) {
            navegarParaBaixo();
            return true;
        }
        if (confirmar) {
            confirmarSelecao();
            return true;
        }
    }
    
    // Navegação por analógico
    if (evento.type == SDL_CONTROLLERAXISMOTION) {
        Uint32 agora = SDL_GetTicks();
        if (agora - ultimoInputAnalogico < INTERVALO_ANALOGICO) {
            return false;
        }
        
        if (evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
            if (evento.caxis.value < -DEADZONE) {
                navegarParaEsquerda();
                ultimoInputAnalogico = agora;
                return true;
            } else if (evento.caxis.value > DEADZONE) {
                navegarParaDireita();
                ultimoInputAnalogico = agora;
                return true;
            }
        }
        
        if (evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
            if (evento.caxis.value < -DEADZONE) {
                navegarParaCima();
                ultimoInputAnalogico = agora;
                return true;
            } else if (evento.caxis.value > DEADZONE) {
                navegarParaBaixo();
                ultimoInputAnalogico = agora;
                return true;
            }
        }
    }
    
    return false;
}

/**
 * @brief Reseta o estado da janela para valores padrão.
 */
void JanelaAudioEVideo::resetar() {
    indiceFocado = SDL_NumJoysticks() > 0 ? 0 : -1;
    arrastandoVolume = false;
    arrastandoEscala = false;
    arrastandoBrilho = false;
    inicializarBotoes();
}

void JanelaAudioEVideo::aplicarAlteracoes() {
    definirMensagemStatus("Aplicando configuracoes...");

    // 1. Aplicar Áudio
    if (capacidadesSistema.audio_select && !dispositivos.empty() && indiceDispositivoAtual >= 0) {
        int idReal = dispositivos[indiceDispositivoAtual].id;
        if (idReal >= 0) {
            system_result audio = ::selecionar_dispositivo_audio_result(idReal);
            if (!audio.ok) {
                definirMensagemStatus(audio.mensagem.empty() ? "Falha ao definir audio." : audio.mensagem, true);
                return;
            }
        }
    }

    // ---------------------------------------------------------
    // 2. PREPARAÇÃO DE VÍDEO (Detectar Monitor)
    // ---------------------------------------------------------
    // Precisamos saber o nome do monitor (ex: HDMI-1, eDP-1) dinamicamente
    std::string nomeMonitor = "HDMI-1"; // Fallback padrão
    
    std::vector<DisplayOutput> displays;
    if (capacidadesSistema.display_info && ::listar_displays_result(displays).ok && !displays.empty()) {
        nomeMonitor = displays[0].name; // Pega o primeiro monitor conectado
    } else {
        std::cerr << "[UI] Aviso: Nenhum monitor detectado via backend. Tentando aplicar em " << nomeMonitor << "...\n";
    }

    // ---------------------------------------------------------
    // 3. APLICAR RESOLUÇÃO
    // ---------------------------------------------------------
    if (capacidadesSistema.display_resolution &&
        !resolucoes.empty() && indiceResolucaoAtual >= 0 && indiceResolucaoAtual < (int)resolucoes.size()) {
        Resolucao alvo = resolucoes[indiceResolucaoAtual];
        
        // A nova função pede (nome, largura, altura, refresh_rate)
        // Usamos 60.0f como padrão seguro, já que a UI ainda não escolhe Hz
        system_result resolucao = ::alterarResolucao_result(nomeMonitor, alvo.largura, alvo.altura, 60.0f);
        if (!resolucao.ok) {
            definirMensagemStatus(
                resolucao.mensagem.empty() ? "Falha ao definir resolucao." : resolucao.mensagem,
                true
            );
            return;
        }
    }

  //---------------------------------------------------------
    // 4. APLICAR ESCALA
    // ---------------------------------------------------------
    // A nova função pede apenas (nome, float escala)
    if (capacidadesSistema.display_scale) {
        system_result escala = ::alterarEscala_result(nomeMonitor, escalaJanela);
        if (!escala.ok) {
            definirMensagemStatus(
                escala.mensagem.empty() ? "Falha ao definir escala." : escala.mensagem,
                true
            );
            return;
        }
    }

    // ---------------------------------------------------------
    // 5. FEEDBACK
    // ---------------------------------------------------------
    definirMensagemStatus("Configuracoes aplicadas com sucesso.");
    gerAudio.tocarSom("select.wav");
}

/**
 * @brief Desenha informações sobre o Monitor e o Compositor (Wayland/X11).
 */
void JanelaAudioEVideo::desenharInfoSistema(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();

    static Uint32 ultimoRefresh = 0;
    static std::string sessaoCache = "unknown";
    static std::string nomeMonitor = "Desconhecido";
    Uint32 agora = SDL_GetTicks();
    if (agora - ultimoRefresh > 2000 || ultimoRefresh == 0) {
        sessaoCache = ::obter_tipo_sessao();
        std::vector<DisplayOutput> displays;
        if (capacidadesSistema.display_info && ::listar_displays_result(displays).ok && !displays.empty()) {
            nomeMonitor = displays[0].name;
        } else {
            nomeMonitor = "Indisponivel";
        }
        ultimoRefresh = agora;
    }

    // Formata o texto: "Monitor: HDMI-1 | Sessão: wayland"
    std::stringstream ss;
    ss << "Monitor: " << nomeMonitor << " | Sessão: " << sessaoCache;
    
    // Desenha logo abaixo do título (ajuste Y conforme necessário)
    desenharTexto(renderer, ss.str(), 
                  ConfigLayout::X(250), ConfigLayout::Y(200), // Posição
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(22)); // Fonte menor
}
