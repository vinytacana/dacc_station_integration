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
JanelaAudioEVideo::JanelaAudioEVideo(GerenciadorImagens* gerImgLocal) : gerImgRef(gerImgLocal) {
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
    
    // 1. Busca info do sistema
    std::vector<DisplayOutput> displays = ::obter_info_displays();
    
    if (displays.empty()) {
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
        ConfigLayout::F(250), ConfigLayout::F(320),
        ConfigLayout::F(60), ConfigLayout::F(60),
        "-"
    );
    btnVolumeDecremento->setCor(btnNormal, btnHover, btnPress);
    btnVolumeDecremento->setRetanguloBordasArredondadas(15);

    btnVolumeIncremento = std::make_unique<Botao>(
        ConfigLayout::F(1260), ConfigLayout::F(320),
        ConfigLayout::F(60), ConfigLayout::F(60),
        "+"
    );
    btnVolumeIncremento->setCor(btnNormal, btnHover, btnPress);
    btnVolumeIncremento->setRetanguloBordasArredondadas(15);

    // Botões de Dispositivo
    btnDispositivoAnterior = std::make_unique<Botao>(
        ConfigLayout::F(250), ConfigLayout::F(480),
        ConfigLayout::F(60), ConfigLayout::F(60),
        "<"
    );
    btnDispositivoAnterior->setCor(btnNormal, btnHover, btnPress);
    btnDispositivoAnterior->setRetanguloBordasArredondadas(15);

    btnDispositivoProximo = std::make_unique<Botao>(
        ConfigLayout::F(1260), ConfigLayout::F(480),
        ConfigLayout::F(60), ConfigLayout::F(60),
        ">"
    );
    btnDispositivoProximo->setCor(btnNormal, btnHover, btnPress);
    btnDispositivoProximo->setRetanguloBordasArredondadas(15);

    // Botões de Resolução
    btnResolucaoAnterior = std::make_unique<Botao>(
        ConfigLayout::F(250), ConfigLayout::F(640),
        ConfigLayout::F(60), ConfigLayout::F(60),
        "<"
    );
    btnResolucaoAnterior->setCor(btnNormal, btnHover, btnPress);
    btnResolucaoAnterior->setRetanguloBordasArredondadas(15);

    btnResolucaoProxima = std::make_unique<Botao>(
        ConfigLayout::F(1260), ConfigLayout::F(640),
        ConfigLayout::F(60), ConfigLayout::F(60),
        ">"
    );
    btnResolucaoProxima->setCor(btnNormal, btnHover, btnPress);
    btnResolucaoProxima->setRetanguloBordasArredondadas(15);

    // Botões de Escala
    btnEscalaDecremento = std::make_unique<Botao>(
        ConfigLayout::F(250), ConfigLayout::F(800),
        ConfigLayout::F(60), ConfigLayout::F(60),
        "-"
    );
    btnEscalaDecremento->setCor(btnNormal, btnHover, btnPress);
    btnEscalaDecremento->setRetanguloBordasArredondadas(15);

    btnEscalaIncremento = std::make_unique<Botao>(
        ConfigLayout::F(1260), ConfigLayout::F(800),
        ConfigLayout::F(60), ConfigLayout::F(60),
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
        ConfigLayout::F(1200), ConfigLayout::F(920), // Posição X, Y
        ConfigLayout::F(200), ConfigLayout::F(60),   // Largura, Altura
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
    desenharInfoSistema(renderer);
    desenharControleVolume(renderer);
    desenharSeletorDispositivo(renderer);
    desenharSeletorResolucao(renderer);
    desenharControleEscala(renderer);
    desenharImagemExplicativa(renderer);
    

    if (btnAplicar) {
        // Verifica se o foco está nele (índice 8)
        if (indiceFocado == 8 && SDL_NumJoysticks() > 0) {
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
    int larguraTela = ConfigLayout::F(1525);
    int posX = (larguraTela - larguraEstimada) / 2;
    
    desenharTexto(renderer, titulo, 
                  posX, ConfigLayout::F(150), 
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
                  ConfigLayout::F(250), ConfigLayout::F(250), 
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
    int barraX = ConfigLayout::F(330);
    int barraY = ConfigLayout::F(320);
    int barraLargura = ConfigLayout::F(910); // USAR F PARA LARGURA
    int barraAltura = ConfigLayout::F(60);   // USAR F PARA ALTURA
    
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
                  ConfigLayout::F(1350), ConfigLayout::F(335), 
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
                  ConfigLayout::F(250), ConfigLayout::F(410), 
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
                  ConfigLayout::F(340), ConfigLayout::F(495), 
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
                  ConfigLayout::F(250), ConfigLayout::F(570), 
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
                  ConfigLayout::F(340), ConfigLayout::F(655), 
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
                  ConfigLayout::F(250), ConfigLayout::F(730), 
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
    int barraX = ConfigLayout::F(330);
    int barraY = ConfigLayout::F(800);
    int barraLargura = ConfigLayout::F(910); // USAR F PARA LARGURA
    int barraAltura = ConfigLayout::F(60);   // USAR F PARA ALTURA
    
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
                  ConfigLayout::F(1350), ConfigLayout::F(815), 
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
    } else {
        caminhoImagem = "assets/images/dark/explicacaoBotoesJanelaAudioEVideoEscuro.jpg";
    }
    
    // Carrega a textura usando o gerenciador fornecido (com cache isolado)
    if (gerImgRef) {
        texturaExplicacao = gerImgRef->carregar(renderer, caminhoImagem);
    }
    
    if (!texturaExplicacao) {
        return;
    }
    
    // 1. Define a altura fixa da imagem e largura total da tela
    int larguraTela = ConfigLayout::F(1525); 
    int alturaImagem = ConfigLayout::F(30);
    
    // 2. Define a posição Y como (Altura da Tela - Altura da Imagem)
    int posY = ConfigLayout::F(1080) - alturaImagem;
    
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
        case 8: aplicarAlteracoes(); break;
    }
}

/**
 * @brief Calcula o volume baseado no quadrado (segmento) clicado.
 */
int JanelaAudioEVideo::calcularVolumeAPartirDoPonto(int mouseX) {
    if (areaBarraVolume.w <= 0) return 0;

    // 1. Calcula a posição relativa dentro da barra
    int posicaoRelativa = mouseX - areaBarraVolume.x;
    
    // 2. Determina a largura de um segmento individual (incluindo o espaço de design)
    float larguraSegmento = (float)areaBarraVolume.w / NUM_BARRAS_VOLUME;
    
    // 3. Calcula qual o índice do segmento (0 a NUM_BARRAS_VOLUME-1)
    int indiceSegmento = (int)(posicaoRelativa / larguraSegmento);
    
    // 4. Converte para volume (cada segmento representa um degrau)
    // Se clicou no primeiro pixel do primeiro segmento, volume = (0 + 1) * passo
    int novoVolume = (indiceSegmento + 1) * (MAX_VOLUME / NUM_BARRAS_VOLUME);

    return std::clamp(novoVolume, 0, MAX_VOLUME);
}

/**
 * @brief Calcula a escala baseada no quadrado (segmento) clicado.
 */
float JanelaAudioEVideo::calcularEscalaAPartirDoPonto(int mouseX) {
    if (areaBarraEscala.w <= 0) return MIN_ESCALA;

    int posicaoRelativa = mouseX - areaBarraEscala.x;
    float larguraSegmento = (float)areaBarraEscala.w / NUM_BARRAS_ESCALA;
    int indiceSegmento = (int)(posicaoRelativa / larguraSegmento);
    
    float percentual = (float)indiceSegmento / (NUM_BARRAS_ESCALA - 1);
    float escala = MIN_ESCALA + (percentual * (MAX_ESCALA - MIN_ESCALA));

    return std::clamp(escala, MIN_ESCALA, MAX_ESCALA);
}

/**
 * @brief Processa clique na barra de volume.
 */
bool JanelaAudioEVideo::processarCliqueBarraVolume(int mouseX, int mouseY) {
    if (mouseX >= areaBarraVolume.x && mouseX <= areaBarraVolume.x + areaBarraVolume.w &&
        mouseY >= areaBarraVolume.y && mouseY <= areaBarraVolume.y + areaBarraVolume.h) {
        
        int novoVolume = calcularVolumeAPartirDoPonto(mouseX);
        volumeGeral = std::clamp(novoVolume, 0, MAX_VOLUME); // Atualiza apenas visualmente durante o arrasto
        
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
            if (arrastandoVolume) {
                ::definir_volume(volumeGeral); // Aplica no hardware apenas ao soltar
                gerAudio.tocarSom("select.wav");
            }
            if (arrastandoEscala) {
                // Escala só é aplicada ao clicar em 'Aplicar', mas aqui podemos tocar um som
                gerAudio.tocarSom("select.wav");
            }
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

void JanelaAudioEVideo::aplicarAlteracoes() {
    std::cout << "=== Aplicando Configurações ===" << std::endl;

    // 1. Aplicar Áudio
    if (!dispositivos.empty() && indiceDispositivoAtual >= 0) {
        int idReal = dispositivos[indiceDispositivoAtual].id;
        ::selecionar_dispositivo_audio(idReal);
        std::cout << "Áudio definido para ID: " << idReal << std::endl;
    }

    // ---------------------------------------------------------
    // 2. PREPARAÇÃO DE VÍDEO (Detectar Monitor)
    // ---------------------------------------------------------
    // Precisamos saber o nome do monitor (ex: HDMI-1, eDP-1) dinamicamente
    std::string nomeMonitor = "HDMI-1"; // Fallback padrão
    
    std::vector<DisplayOutput> displays = ::obter_info_displays();
    if (!displays.empty()) {
        nomeMonitor = displays[0].name; // Pega o primeiro monitor conectado
    } else {
        std::cerr << "[UI] Aviso: Nenhum monitor detectado via backend. Tentando aplicar em " << nomeMonitor << "...\n";
    }

    // ---------------------------------------------------------
    // 3. APLICAR RESOLUÇÃO
    // ---------------------------------------------------------
    if (!resolucoes.empty() && indiceResolucaoAtual >= 0 && indiceResolucaoAtual < (int)resolucoes.size()) {
        Resolucao alvo = resolucoes[indiceResolucaoAtual];
        
        // A nova função pede (nome, largura, altura, refresh_rate)
        // Usamos 60.0f como padrão seguro, já que a UI ainda não escolhe Hz
        bool sucesso = ::alterarResolucao(nomeMonitor, alvo.largura, alvo.altura, 60.0f);
        
        if (sucesso)
            std::cout << "[UI] Resolução definida para: " << alvo.toString() << " em " << nomeMonitor << std::endl;
        else
            std::cerr << "[UI] Falha ao definir resolução.\n";
    }

  //---------------------------------------------------------
    // 4. APLICAR ESCALA
    // ---------------------------------------------------------
    // A nova função pede apenas (nome, float escala)
    bool scaleSucesso = ::alterarEscala(nomeMonitor, escalaJanela);
    
    if (scaleSucesso)
        std::cout << "[UI] Escala definida para: " << escalaJanela << "x" << std::endl;
    else
        std::cerr << "[UI] Falha ao definir escala.\n";

    // ---------------------------------------------------------
    // 5. FEEDBACK
    // ---------------------------------------------------------
    gerAudio.tocarSom("select.wav");
}

/**
 * @brief Desenha informações sobre o Monitor e o Compositor (Wayland/X11).
 */
void JanelaAudioEVideo::desenharInfoSistema(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Obter informações do backend (cachear isso numa variável de classe seria melhor para performance)
    std::string sessao = ::obter_tipo_sessao(); // "wayland" ou "x11"
    
    // Pega o nome do monitor (fallback se vazio)
    std::string nomeMonitor = "Desconhecido";
    auto displays = ::obter_info_displays();
    if (!displays.empty()) {
        nomeMonitor = displays[0].name; // Ex: "HDMI-1"
    }

    // Formata o texto: "Monitor: HDMI-1 | Sessão: wayland"
    std::stringstream ss;
    ss << "Monitor: " << nomeMonitor << " | Sessão: " << sessao;
    
    // Desenha logo abaixo do título (ajuste Y conforme necessário)
    desenharTexto(renderer, ss.str(), 
                  ConfigLayout::F(250), ConfigLayout::F(200), // Posição
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(22)); // Fonte menor
}