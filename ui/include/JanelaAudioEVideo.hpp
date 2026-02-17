/**
 * @file JanelaAudioEVideo.hpp
 * @brief Definição da classe JanelaAudioEVideo para gerenciamento de configurações de áudio e vídeo.
 * 
 * Este arquivo contém a interface para o submenu de configurações de áudio e vídeo,
 * incluindo controle de volume (slider com barras), seleção de dispositivos de saída,
 * resolução de tela e escala da janela.
 */

#ifndef JANELA_AUDIO_E_VIDEO_HPP
#define JANELA_AUDIO_E_VIDEO_HPP

#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <memory>
#include <string>
#include "Botao.hpp"
#include "GerenciadorImagens.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

class GerenciadorImagens;

/**
 * @struct DispositivoAudio
 * @brief Estrutura para armazenar informações sobre um dispositivo de áudio.
 */
struct DispositivoAudio {
    std::string nome;      /**< Nome do dispositivo de áudio. */
    int id;                /**< Identificador único do dispositivo. */
    
    /**
     * @brief Construtor da estrutura DispositivoAudio.
     * @param n Nome do dispositivo.
     * @param i ID do dispositivo.
     */
    DispositivoAudio(const std::string& n, int i) : nome(n), id(i) {}
};

/**
 * @struct Resolucao
 * @brief Estrutura para armazenar uma resolução de tela.
 */
struct Resolucao {
    int largura;    /**< Largura em pixels. */
    int altura;     /**< Altura em pixels. */
    
    /**
     * @brief Construtor da estrutura Resolucao.
     * @param l Largura.
     * @param a Altura.
     */
    Resolucao(int l, int a) : largura(l), altura(a) {}
    
    /**
     * @brief Retorna a resolução formatada como string.
     * @return String no formato "LARGURAxALTURA".
     */
    std::string toString() const;
};

/**
 * @class JanelaAudioEVideo
 * @brief Gerencia a interface de configurações de áudio e vídeo.
 * 
 * Esta classe renderiza e controla a interação com o menu de configurações
 * de áudio e vídeo, incluindo:
 * - Controle de volume geral (0-100%) com barra visual progressiva
 * - Seleção de dispositivo de saída de áudio
 * - Seleção de resolução de tela
 * - Controle de escala da janela (0.5x - 2.0x) com barra visual
 * - Navegação via controle ou mouse
 */
class JanelaAudioEVideo {
public:
    /**
     * @brief Construtor da classe JanelaAudioEVideo.
     */
    JanelaAudioEVideo(GerenciadorImagens* gerImgLocal);

    /**
     * @brief Destrutor da classe JanelaAudioEVideo.
     */
    ~JanelaAudioEVideo();

    /**
     * @brief Renderiza a interface de configurações de áudio e vídeo.
     * @param renderer Ponteiro para o renderizador SDL onde desenhar.
     */
    void desenhar(SDL_Renderer* renderer);

    /**
     * @brief Processa eventos de input específicos para esta tela.
     * @param evento Referência ao evento SDL capturado.
     * @return true se o evento foi processado e causou mudança de estado.
     */
    bool processarEvento(SDL_Event& evento);

    /**
     * @brief Reseta o estado da janela para valores padrão.
     */
    void resetar();

private:
    // CONFIGURAÇÕES DE ÁUDIO
    
    int volumeGeral = 80;                   /**< Volume geral (0-100). */
    const int MAX_VOLUME = 100;             /**< Volume máximo. */
    const int NUM_BARRAS_VOLUME = 20;       /**< Número de barras para representar o volume. */
    
    std::vector<DispositivoAudio> dispositivos; /**< Lista de dispositivos de áudio disponíveis. */
    int indiceDispositivoAtual = 0;         /**< Índice do dispositivo atualmente selecionado. */
    
    // CONFIGURAÇÕES DE VÍDEO
    
    std::vector<Resolucao> resolucoes;      /**< Lista de resoluções disponíveis. */
    int indiceResolucaoAtual = 0;           /**< Índice da resolução atualmente selecionada. */
    
    float escalaJanela = 1.0f;              /**< Escala da janela (0.5x - 2.0x). */
    const float MIN_ESCALA = 0.5f;          /**< Escala mínima. */
    const float MAX_ESCALA = 2.0f;          /**< Escala máxima. */
    const float PASSO_ESCALA = 0.1f;        /**< Incremento/decremento da escala. */
    const int NUM_BARRAS_ESCALA = 15;       /**< Número de barras para representar a escala. */
    
    // BOTÕES DE NAVEGAÇÃO
    
    std::unique_ptr<Botao> btnVolumeDecremento;  /**< Botão para diminuir volume. */
    std::unique_ptr<Botao> btnVolumeIncremento;  /**< Botão para aumentar volume. */
    
    std::unique_ptr<Botao> btnDispositivoAnterior;  /**< Botão para dispositivo anterior. */
    std::unique_ptr<Botao> btnDispositivoProximo;   /**< Botão para próximo dispositivo. */
    
    std::unique_ptr<Botao> btnResolucaoAnterior;    /**< Botão para resolução anterior. */
    std::unique_ptr<Botao> btnResolucaoProxima;     /**< Botão para próxima resolução. */
    
    std::unique_ptr<Botao> btnEscalaDecremento;  /**< Botão para diminuir escala. */
    std::unique_ptr<Botao> btnEscalaIncremento;  /**< Botão para aumentar escala. */
    std::unique_ptr<Botao> btnAplicar;
    // CONTROLE DE NAVEGAÇÃO
    
    int indiceFocado = -1;                  /**< Índice do elemento atualmente focado. */
    const int NUM_ELEMENTOS_FOCAVEIS = 9;   /**< Total de elementos navegáveis (4 grupos x 2 botões). */
    
    // Controle de Input de Periféricos
    Uint32 ultimoInputAnalogico = 0;        /**< Timestamp do último input analógico. */
    const Uint32 INTERVALO_ANALOGICO = 200; /**< Intervalo mínimo entre inputs (ms). */
    const int DEADZONE = 16000;             /**< Limiar de sensibilidade do analógico. */
    
    // ÁREAS DE INTERAÇÃO PARA MOUSE
    
    SDL_Rect areaBarraVolume;               /**< Área clicável da barra de volume. */
    SDL_Rect areaBarraEscala;               /**< Área clicável da barra de escala. */
    
    bool arrastandoVolume = false;          /**< Flag para controle de arrasto do volume. */
    bool arrastandoEscala = false;          /**< Flag para controle de arrasto da escala. */
    
    GerenciadorImagens* gerImgRef = nullptr; /**< Referência ao gerenciador de imagens isolado. */
    // IMAGEM EXPLICATIVA
    
    SDL_Texture* texturaExplicacao = nullptr; /**< Textura da imagem explicativa de rodapé. */
    

    /**
     * @brief Inicializa os botões e elementos interativos.
     */
    void inicializarBotoes();

    /**
     * @brief Inicializa a lista de dispositivos de áudio disponíveis.
     */
    void inicializarDispositivos();

    /**
     * @brief Inicializa a lista de resoluções disponíveis.
     */
    void inicializarResolucoes();

    /**
     * @brief Renderiza o cabeçalho da seção.
     * @param renderer Renderizador SDL.
     */
    void desenharCabecalho(SDL_Renderer* renderer);

    /**
     * @brief Renderiza o controle de volume com barra progressiva.
     * @param renderer Renderizador SDL.
     */
    void desenharControleVolume(SDL_Renderer* renderer);

    /**
     * @brief Renderiza o seletor de dispositivo de áudio.
     * @param renderer Renderizador SDL.
     */
    void desenharSeletorDispositivo(SDL_Renderer* renderer);

    /**
     * @brief Renderiza o seletor de resolução.
     * @param renderer Renderizador SDL.
     */
    void desenharSeletorResolucao(SDL_Renderer* renderer);

    /**
     * @brief Renderiza o controle de escala com barra progressiva.
     * @param renderer Renderizador SDL.
     */
    void desenharControleEscala(SDL_Renderer* renderer);

    /**
     * @brief Renderiza a imagem explicativa no rodapé da tela.
     * 
     * A imagem é exibida com altura fixa de 30px ocupando toda a largura
     * da tela, posicionada na parte inferior. A textura é carregada
     * automaticamente de acordo com o tema ativo (claro/escuro).
     * 
     * @param renderer Renderizador SDL onde a imagem será desenhada.
     */
    void desenharImagemExplicativa(SDL_Renderer* renderer);

    /**
     * @brief Desenha uma barra de progresso visual.
     * @param renderer Renderizador SDL.
     * @param x Posição X.
     * @param y Posição Y.
     * @param larguraTotal Largura total da área da barra.
     * @param altura Altura da barra.
     * @param numBarras Número de segmentos da barra.
     * @param barrasPreenchidas Quantos segmentos estão preenchidos.
     * @param corPreenchida Cor dos segmentos preenchidos.
     * @param corVazia Cor dos segmentos vazios.
     */
    void desenharBarraProgresso(SDL_Renderer* renderer, int x, int y, 
                                int larguraTotal, int altura, int numBarras, 
                                int barrasPreenchidas, SDL_Color corPreenchida, 
                                SDL_Color corVazia);

    /**
     * @brief Incrementa o volume.
     */
    void aumentarVolume();

    /**
     * @brief Decrementa o volume.
     */
    void diminuirVolume();

    /**
     * @brief Define o volume diretamente (usado para clique/arrasto).
     * @param novoVolume Novo valor de volume (0-100).
     */
    void setVolume(int novoVolume);

    /**
     * @brief Seleciona o dispositivo anterior na lista.
     */
    void dispositivoAnterior();

    /**
     * @brief Seleciona o próximo dispositivo na lista.
     */
    void dispositivoProximo();

    /**
     * @brief Seleciona a resolução anterior na lista.
     */
    void resolucaoAnterior();

    /**
     * @brief Seleciona a próxima resolução na lista.
     */
    void resolucaoProxima();

    /**
     * @brief Incrementa a escala da janela.
     */
    void aumentarEscala();

    /**
     * @brief Decrementa a escala da janela.
     */
    void diminuirEscala();

    /**
     * @brief Define a escala diretamente (usado para clique/arrasto).
     * @param novaEscala Nova escala (0.5 - 2.0).
     */
    void setEscala(float novaEscala);

    /**
     * @brief Move o foco para o elemento anterior.
     */
    void navegarParaCima();

    /**
     * @brief Move o foco para o próximo elemento.
     */
    void navegarParaBaixo();

    /**
     * @brief Move o foco para a esquerda (dentro do mesmo grupo).
     */
    void navegarParaEsquerda();

    /**
     * @brief Move o foco para a direita (dentro do mesmo grupo).
     */
    void navegarParaDireita();

    /**
     * @brief Confirma a seleção do elemento focado.
     */
    void confirmarSelecao();

    /**
     * @brief Processa clique na barra de volume.
     * @param mouseX Coordenada X do mouse.
     * @param mouseY Coordenada Y do mouse.
     * @return true se o clique foi na barra.
     */
    bool processarCliqueBarraVolume(int mouseX, int mouseY);

    /**
     * @brief Processa clique na barra de escala.
     * @param mouseX Coordenada X do mouse.
     * @param mouseY Coordenada Y do mouse.
     * @return true se o clique foi na barra.
     */
    bool processarCliqueBarraEscala(int mouseX, int mouseY);

    /**
     * @brief Calcula o volume baseado na posição X do mouse na barra.
     * @param mouseX Coordenada X do mouse.
     * @return Valor de volume calculado (0-100).
     */
    int calcularVolumeAPartirDoPonto(int mouseX);

    /**
     * @brief Calcula a escala baseada na posição X do mouse na barra.
     * @param mouseX Coordenada X do mouse.
     * @return Valor de escala calculado (0.5-2.0).
     */
    float calcularEscalaAPartirDoPonto(int mouseX);

    void aplicarAlteracoes();

    void desenharInfoSistema(SDL_Renderer* renderer);
};

} // namespace MeuProjeto

#endif // JANELA_AUDIO_E_VIDEO_HPP