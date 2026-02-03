/**
 * @file JanelaAudioEVideo.cpp
 * @brief Implementação da interface de configurações de áudio e vídeo.
 *
 * Este arquivo contém toda a lógica visual e de interação para o submenu
 * de configurações de áudio e vídeo, incluindo controles de volume, dispositivos,
 * resolução e escala da janela.
 */

#include "JanelaAudioEVideo.hpp"
#include "functions.hpp"
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
JanelaAudioEVideo::JanelaAudioEVideo() {
    inicializarDispositivos();
    inicializarResolucoes();
    inicializarBotoes();

    volumeGeral = ::obter_volume_atual();
    
    // Inicializa áreas de interação das barras
    areaBarraVolume = {0, 0, 0, 0};
    areaBarraEscala = {0, 0, 0, 0};
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
    
    // Libera a textura explicativa (o gerenciador já cuida da limpeza)
    texturaExplicacao = nullptr;
}

/**
 * @brief Inicializa a lista de dispositivos de áudio com dados de exemplo.
 */
void JanelaAudioEVideo::inicializarDispositivos() {
dispositivos.clear();
    
    // 1. Chama o Backend (HardwareControl.cpp via functions.hpp)
    std::vector<device_audio> listaDoSistema = ::listar_dispositivos_audio();
    
    // Fallback se não encontrar nada
    if (listaDoSistema.empty()) {
        dispositivos.push_back(DispositivoAudio("Nenhum dispositivo encontrado", -1));
        indiceDispositivoAtual = 0;
        return;
    }

    // 2. Converte os dados do backend para a estrutura da UI
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
    
    std::cout << "[UI] Carregados " << dispositivos.size() << " dispositivos de áudio.\n";
}

/**
 * @brief Inicializa a lista de resoluções disponíveis.
 */
void JanelaAudioEVideo::inicializarResolucoes() {
    resolucoes.clear();
    
    // Adiciona resoluções comuns
    resolucoes.push_back(Resolucao(1920, 1080));  // Full HD
    resolucoes.push_back(Resolucao(1280, 720));   // HD
    resolucoes.push_back(Resolucao(2560, 1440));  // QHD
    resolucoes.push_back(Resolucao(3840, 2160));  // 4K
    resolucoes.push_back(Resolucao(1366, 768));   // WXGA
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
        ConfigLayout::X(250), ConfigLayout::Y(320),
        ConfigLayout::X(60), ConfigLayout::Y(60),
        "-"
    );
    btnVolumeDecremento->setCor(btnNormal, btnHover, btnPress);
    btnVolumeDecremento->setRetanguloBordasArredondadas(15);

    btnVolumeIncremento = std::make_unique<Botao>(
        ConfigLayout::X(1260), ConfigLayout::Y(320),
        ConfigLayout::X(60), ConfigLayout::Y(60),
        "+"
    );
    btnVolumeIncremento->setCor(btnNormal, btnHover, btnPress);
    btnVolumeIncremento->setRetanguloBordasArredondadas(15);

    // Botões de Dispositivo
    btnDispositivoAnterior = std::make_unique<Botao>(
        ConfigLayout::X(250), ConfigLayout::Y(480),
        ConfigLayout::X(60), ConfigLayout::Y(60),
        "<"
    );
    btnDispositivoAnterior->setCor(btnNormal, btnHover, btnPress);
    btnDispositivoAnterior->setRetanguloBordasArredondadas(15);

    btnDispositivoProximo = std::make_unique<Botao>(
        ConfigLayout::X(1260), ConfigLayout::Y(480),
        ConfigLayout::X(60), ConfigLayout::Y(60),
        ">"
    );
    btnDispositivoProximo->setCor(btnNormal, btnHover, btnPress);
    btnDispositivoProximo->setRetanguloBordasArredondadas(15);

    // Botões de Resolução
    btnResolucaoAnterior = std::make_unique<Botao>(
        ConfigLayout::X(250), ConfigLayout::Y(640),
        ConfigLayout::X(60), ConfigLayout::Y(60),
        "<"
    );
    btnResolucaoAnterior->setCor(btnNormal, btnHover, btnPress);
    btnResolucaoAnterior->setRetanguloBordasArredondadas(15);

    btnResolucaoProxima = std::make_unique<Botao>(
        ConfigLayout::X(1260), ConfigLayout::Y(640),
        ConfigLayout::X(60), ConfigLayout::Y(60),
        ">"
    );
    btnResolucaoProxima->setCor(btnNormal, btnHover, btnPress);
    btnResolucaoProxima->setRetanguloBordasArredondadas(15);

    // Botões de Escala
    btnEscalaDecremento = std::make_unique<Botao>(
        ConfigLayout::X(250), ConfigLayout::Y(800),
        ConfigLayout::X(60), ConfigLayout::Y(60),
        "-"
    );
    btnEscalaDecremento->setCor(btnNormal, btnHover, btnPress);
    btnEscalaDecremento->setRetanguloBordasArredondadas(15);

    btnEscalaIncremento = std::make_unique<Botao>(
        ConfigLayout::X(1260), ConfigLayout::Y(800),
        ConfigLayout::X(60), ConfigLayout::Y(60),
        "+"
    );
    btnEscalaIncremento->setCor(btnNormal, btnHover, btnPress);
    btnEscalaIncremento->setRetanguloBordasArredondadas(15);

    // Inicializa foco se houver controle conectado
    if (SDL_NumJoysticks() > 0) {
        indiceFocado = 0;
    } else {
        indiceFocado = -1;
    }

    // --- NOVO: Botão Aplicar ---
    // Posicionado lá embaixo, centralizado ou à direita
    btnAplicar = std::make_unique<Botao>(
        ConfigLayout::X(1200), ConfigLayout::Y(920), // Posição X, Y
        ConfigLayout::X(200), ConfigLayout::Y(60),   // Largura, Altura
        "Aplicar"
    );
    btnAplicar->setCor(tema.getCorBotaoNormal(), tema.getCorBotaoHover(), tema.getCorBotaoPressionado());
    btnAplicar->setRetanguloBordasArredondadas(15);
    // ---------------------------
}

/**
 * @brief Renderiza toda a interface de configurações de áudio e vídeo.
 */
void JanelaAudioEVideo::desenhar(SDL_Renderer* renderer) {
    if (!renderer) return;

    desenharCabecalho(renderer);
    desenharControleVolume(renderer);
    desenharSeletorDispositivo(renderer);
    desenharSeletorResolucao(renderer);
    desenharControleEscala(renderer);
    desenharImagemExplicativa(renderer);
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
    int larguraTela = ConfigLayout::X(1525);
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
                  ConfigLayout::X(250), ConfigLayout::Y(250), 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(32));
    
    // Aplica foco aos botões se necessário (índices 0 e 1)
    if (indiceFocado == 0 && SDL_NumJoysticks() > 0) {
        btnVolumeDecremento->setFocado(true);
    } else {
        btnVolumeDecremento->setFocado(false);
    }
    
    if (indiceFocado == 1 && SDL_NumJoysticks() > 0) {
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
    int barraX = ConfigLayout::X(330);
    int barraY = ConfigLayout::Y(320);
    int barraLargura = ConfigLayout::X(910);
    int barraAltura = ConfigLayout::Y(60);
    
    areaBarraVolume = {barraX, barraY, barraLargura, barraAltura};
    
    // Desenha a barra de volume
    SDL_Color corPreenchida = tema.getCorDestaque();
    SDL_Color corVazia = tema.getCorBotaoNormal();
    
    desenharBarraProgresso(renderer, barraX, barraY, barraLargura, barraAltura,
                          NUM_BARRAS_VOLUME, barrasPreenchidas, 
                          corPreenchida, corVazia);
    
    // Texto do percentual
    std::stringstream ss;
    ss << volumeGeral << "%";
    desenharTexto(renderer, ss.str(), 
                  ConfigLayout::X(1350), ConfigLayout::Y(335), 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(28));
}

/**
 * @brief Renderiza o seletor de dispositivo de áudio.
 */
void JanelaAudioEVideo::desenharSeletorDispositivo(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Label
    desenharTexto(renderer, "Dispositivo de Saída:", 
                  ConfigLayout::X(250), ConfigLayout::Y(410), 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(32));
    
    // Aplica foco aos botões se necessário (índices 2 e 3)
    if (indiceFocado == 2 && SDL_NumJoysticks() > 0) {
        btnDispositivoAnterior->setFocado(true);
    } else {
        btnDispositivoAnterior->setFocado(false);
    }
    
    if (indiceFocado == 3 && SDL_NumJoysticks() > 0) {
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
                  ConfigLayout::X(340), ConfigLayout::Y(495), 
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(28));
}

/**
 * @brief Renderiza o seletor de resolução.
 */
void JanelaAudioEVideo::desenharSeletorResolucao(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Label
    desenharTexto(renderer, "Resolução:", 
                  ConfigLayout::X(250), ConfigLayout::Y(570), 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(32));
    
    // Aplica foco aos botões se necessário (índices 4 e 5)
    if (indiceFocado == 4 && SDL_NumJoysticks() > 0) {
        btnResolucaoAnterior->setFocado(true);
    } else {
        btnResolucaoAnterior->setFocado(false);
    }
    
    if (indiceFocado == 5 && SDL_NumJoysticks() > 0) {
        btnResolucaoProxima->setFocado(true);
    } else {
        btnResolucaoProxima->setFocado(false);
    }
    
    // Desenha botões
    btnResolucaoAnterior->desenhar(renderer);
    btnResolucaoProxima->desenhar(renderer);
    
    // Texto da resolução atual
    std::string resolucao = resolucoes[indiceResolucaoAtual].toString();
    desenharTexto(renderer, resolucao, 
                  ConfigLayout::X(340), ConfigLayout::Y(655), 
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(28));
}

/**
 * @brief Renderiza o controle de escala com barra progressiva.
 */
void JanelaAudioEVideo::desenharControleEscala(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Label
    desenharTexto(renderer, "Escala da Janela:", 
                  ConfigLayout::X(250), ConfigLayout::Y(730), 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(32));
    
    // Aplica foco aos botões se necessário (índices 6 e 7)
    if (indiceFocado == 6 && SDL_NumJoysticks() > 0) {
        btnEscalaDecremento->setFocado(true);
    } else {
        btnEscalaDecremento->setFocado(false);
    }
    
    if (indiceFocado == 7 && SDL_NumJoysticks() > 0) {
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
    int barraX = ConfigLayout::X(330);
    int barraY = ConfigLayout::Y(800);
    int barraLargura = ConfigLayout::X(910);
    int barraAltura = ConfigLayout::Y(60);
    
    areaBarraEscala = {barraX, barraY, barraLargura, barraAltura};
    
    // Desenha a barra de escala
    SDL_Color corPreenchida = tema.getCorDestaque();
    SDL_Color corVazia = tema.getCorBotaoNormal();
    
    desenharBarraProgresso(renderer, barraX, barraY, barraLargura, barraAltura,
                          NUM_BARRAS_ESCALA, barrasPreenchidas, 
                          corPreenchida, corVazia);
    
    // Texto da escala
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << escalaJanela << "x";
    desenharTexto(renderer, ss.str(), 
                  ConfigLayout::X(1350), ConfigLayout::Y(815), 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(28));
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
    
    // Carrega a imagem apropriada baseada no tema atual
    std::string caminhoImagem;

    if (tema.getTemaAtual() == TipoTema::CLARO) {
        caminhoImagem = "assets/images/light/explicacaoBotoesJanelaAudioEVideoClaro.jpg";

        std::cout << "[AUDIO_E_VIDEO] Carregando imagem CLARA: " << caminhoImagem << std::endl;
    } else {
        caminhoImagem = "assets/images/dark/explicacaoBotoesJanelaAudioEVideoEscuro.jpg";
        std::cout << "[AUDIO_E_VIDEO] Carregando imagem ESCURA: " << caminhoImagem << std::endl;
    }
    
    // Carrega a textura usando o gerenciador (com cache)
    texturaExplicacao = gerenciadorImagens.carregar(renderer, caminhoImagem);
    
    if (!texturaExplicacao) {
        return;
    }
    
    // 1. Define a altura fixa da imagem e largura total da tela
    int larguraTela = ConfigLayout::X(1525); 
    int alturaImagem = ConfigLayout::Y(30);
    
    // 2. Define a posição Y como (Altura da Tela - Altura da Imagem)
    int posY = ConfigLayout::Y(1080) - alturaImagem;
    
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
    // 1. Atualiza a variável interna para feedback visual imediato
    if (volumeGeral < MAX_VOLUME) {
        volumeGeral = std::min(MAX_VOLUME, volumeGeral + 5);
        
        // 2. Toca o som da UI
        gerAudio.tocarSom("select.wav");
        
        // 3. LOGICA REAL: Chama a função do backend (wpctl)
        ::aumentar_volume(); // Chama a função global do functions.hpp

        
        std::cout << "[AUDIO] Volume UI: " << volumeGeral << "% (Comando enviado)" << std::endl;
    }
}

/**
 * @brief Decrementa o volume.
 */
void JanelaAudioEVideo::diminuirVolume() {

    if (volumeGeral > 0) {
        volumeGeral = std::max(0, volumeGeral - 5);
        gerAudio.tocarSom("select.wav");
        ::diminuir_volume(); 
        std::cout << "[AUDIO] Volume UI: " << volumeGeral << "% (Comando enviado)" << std::endl;
    }
}

/**
 * @brief Define o volume diretamente.
 */
void JanelaAudioEVideo::setVolume(int novoVolume) {
    volumeGeral = std::clamp(novoVolume, 0, MAX_VOLUME);
    ::definir_volume(volumeGeral);
    std::cout << "[AUDIO] Volume ajustado para: " << volumeGeral << "%" << std::endl;
}

/**
 * @brief Seleciona o dispositivo anterior.
 */
void JanelaAudioEVideo::dispositivoAnterior() {
    if (indiceDispositivoAtual > 0) {
        indiceDispositivoAtual--;
    } else {
        indiceDispositivoAtual = dispositivos.size() - 1; // Wrap around
    }
    gerAudio.tocarSom("navegacao.wav");
    std::cout << "[AUDIO] Dispositivo: " << dispositivos[indiceDispositivoAtual].nome << std::endl;
}

/**
 * @brief Seleciona o próximo dispositivo.
 */
void JanelaAudioEVideo::dispositivoProximo() {
    if (indiceDispositivoAtual < (int)dispositivos.size() - 1) {
        indiceDispositivoAtual++;
    } else {
        indiceDispositivoAtual = 0; // Wrap around
    }
    gerAudio.tocarSom("navegacao.wav");
    std::cout << "[AUDIO] Dispositivo: " << dispositivos[indiceDispositivoAtual].nome << std::endl;
}

/**
 * @brief Seleciona a resolução anterior.
 */
void JanelaAudioEVideo::resolucaoAnterior() {
    if (indiceResolucaoAtual > 0) {
        indiceResolucaoAtual--;
    } else {
        indiceResolucaoAtual = resolucoes.size() - 1; // Wrap around
    }
    gerAudio.tocarSom("navegacao.wav");
    std::cout << "[VIDEO] Resolução: " << resolucoes[indiceResolucaoAtual].toString() << std::endl;
}

/**
 * @brief Seleciona a próxima resolução.
 */
void JanelaAudioEVideo::resolucaoProxima() {
    if (indiceResolucaoAtual < (int)resolucoes.size() - 1) {
        indiceResolucaoAtual++;
    } else {
        indiceResolucaoAtual = 0; // Wrap around
    }
    gerAudio.tocarSom("navegacao.wav");
    std::cout << "[VIDEO] Resolução: " << resolucoes[indiceResolucaoAtual].toString() << std::endl;
}

/**
 * @brief Incrementa a escala da janela.
 */
void JanelaAudioEVideo::aumentarEscala() {
    if (escalaJanela < MAX_ESCALA) {
        escalaJanela = std::min(MAX_ESCALA, escalaJanela + PASSO_ESCALA);
        gerAudio.tocarSom("select.wav");
        std::cout << "[VIDEO] Escala: " << escalaJanela << "x" << std::endl;
    }
}

/**
 * @brief Decrementa a escala da janela.
 */
void JanelaAudioEVideo::diminuirEscala() {
    if (escalaJanela > MIN_ESCALA) {
        escalaJanela = std::max(MIN_ESCALA, escalaJanela - PASSO_ESCALA);
        gerAudio.tocarSom("select.wav");
        std::cout << "[VIDEO] Escala: " << escalaJanela << "x" << std::endl;
    }
}

/**
 * @brief Define a escala diretamente.
 */
void JanelaAudioEVideo::setEscala(float novaEscala) {
    escalaJanela = std::clamp(novaEscala, MIN_ESCALA, MAX_ESCALA);
    std::cout << "[VIDEO] Escala ajustada para: " << escalaJanela << "x" << std::endl;
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
    }
}

/**
 * @brief Calcula o volume baseado na posição X do mouse.
 */
int JanelaAudioEVideo::calcularVolumeAPartirDoPonto(int mouseX) {
    int posicaoRelativa = mouseX - areaBarraVolume.x;
    float percentual = (float)posicaoRelativa / areaBarraVolume.w;
    return (int)(percentual * MAX_VOLUME);
}

/**
 * @brief Calcula a escala baseada na posição X do mouse.
 */
float JanelaAudioEVideo::calcularEscalaAPartirDoPonto(int mouseX) {
    int posicaoRelativa = mouseX - areaBarraEscala.x;
    float percentual = (float)posicaoRelativa / areaBarraEscala.w;
    return MIN_ESCALA + (percentual * (MAX_ESCALA - MIN_ESCALA));
}

/**
 * @brief Processa clique na barra de volume.
 */
bool JanelaAudioEVideo::processarCliqueBarraVolume(int mouseX, int mouseY) {
    if (mouseX >= areaBarraVolume.x && mouseX <= areaBarraVolume.x + areaBarraVolume.w &&
        mouseY >= areaBarraVolume.y && mouseY <= areaBarraVolume.y + areaBarraVolume.h) {
        int novoVolume = calcularVolumeAPartirDoPonto(mouseX);
        setVolume(novoVolume);
        gerAudio.tocarSom("select.wav");
        return true;
    }
    return false;
}

/**
 * @brief Processa clique na barra de escala.
 */
bool JanelaAudioEVideo::processarCliqueBarraEscala(int mouseX, int mouseY) {
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
 * @brief Processa eventos de input específicos para esta tela.
 */
bool JanelaAudioEVideo::processarEvento(SDL_Event& evento) {
    // Processa cliques do mouse
    if (evento.type == SDL_MOUSEBUTTONDOWN) {
        if (evento.button.button == SDL_BUTTON_LEFT) {
            int mouseX = evento.button.x;
            int mouseY = evento.button.y;
            
            // Verifica clique nas barras
            if (processarCliqueBarraVolume(mouseX, mouseY)) {
                arrastandoVolume = true;
                return true;
            }
            
            if (processarCliqueBarraEscala(mouseX, mouseY)) {
                arrastandoEscala = true;
                return true;
            }
            
            // Verifica clique nos botões
            if (btnVolumeDecremento && btnVolumeDecremento->contemPonto(mouseX, mouseY)) {
                diminuirVolume();
                return true;
            }
            if (btnVolumeIncremento && btnVolumeIncremento->contemPonto(mouseX, mouseY)) {
                aumentarVolume();
                return true;
            }
            
            if (btnDispositivoAnterior && btnDispositivoAnterior->contemPonto(mouseX, mouseY)) {
                dispositivoAnterior();
                return true;
            }
            if (btnDispositivoProximo && btnDispositivoProximo->contemPonto(mouseX, mouseY)) {
                dispositivoProximo();
                return true;
            }
            
            if (btnResolucaoAnterior && btnResolucaoAnterior->contemPonto(mouseX, mouseY)) {
                resolucaoAnterior();
                return true;
            }
            if (btnResolucaoProxima && btnResolucaoProxima->contemPonto(mouseX, mouseY)) {
                resolucaoProxima();
                return true;
            }
            
            if (btnEscalaDecremento && btnEscalaDecremento->contemPonto(mouseX, mouseY)) {
                diminuirEscala();
                return true;
            }
            if (btnEscalaIncremento && btnEscalaIncremento->contemPonto(mouseX, mouseY)) {
                aumentarEscala();
                return true;
            }
        }
    }
    
    // Processa soltar botão do mouse (fim do arrasto)
    if (evento.type == SDL_MOUSEBUTTONUP) {
        if (evento.button.button == SDL_BUTTON_LEFT) {
            arrastandoVolume = false;
            arrastandoEscala = false;
        }
    }
    
    // Processa arrasto
    if (evento.type == SDL_MOUSEMOTION) {
        int mouseX = evento.motion.x;
        int mouseY = evento.motion.y;
        
        if (arrastandoVolume) {
            if (processarCliqueBarraVolume(mouseX, mouseY)) {
                return true;
            }
        }
        
        if (arrastandoEscala) {
            if (processarCliqueBarraEscala(mouseX, mouseY)) {
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
    inicializarBotoes();
}