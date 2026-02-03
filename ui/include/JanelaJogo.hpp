/**
 * @file JanelaJogo.hpp
 * @brief Definição da classe JanelaJogo.
 *
 * Este arquivo contém a especificação da classe JanelaJogo, responsável por gerenciar
 * e exibir a interface detalhada de um título selecionado na biblioteca. A classe
 * engloba a exibição de metadados, galeria de capturas de tela e controles de execução.
 */

#ifndef JANELA_JOGO_HPP
#define JANELA_JOGO_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include "Botao.hpp"

namespace MeuProjeto {

/**
 * @class JanelaJogo
 * @brief Gerencia a interface de visualização detalhada de um jogo específico.
 *
 * A classe JanelaJogo é responsável pela composição visual da página de um jogo,
 * incluindo a renderização de texturas de capa, descrições textuais, uma galeria 
 * iterável de capturas de tela e botões de ação. Implementa lógica de navegação 
 * via mouse e controle, além de suportar um layout responsivo adaptável.
 */
class JanelaJogo {
public:
    /**
     * @brief Construtor da classe JanelaJogo.
     * 
     * Inicializa a interface detalhada carregando texturas, configurando áreas de 
     * recorte e preparando os componentes de interação.
     * 
     * @param renderer Ponteiro para o renderizador SDL_Renderer.
     * @param nomeJogo String contendo o título do jogo.
     * @param imagemPath Caminho do sistema de arquivos para a imagem principal.
     * @param descricao Texto contendo as informações detalhadas do jogo.
     * @param capturasPaths Vetor de strings com os caminhos para as capturas de tela.
     */
    JanelaJogo(SDL_Renderer* renderer, const std::string& codigo, const std::string& nomeJogo, const std::string& imagemPath,
               const std::string& descricaoLonga, const std::vector<std::string>& capturasPaths);

    /**
     * @brief Destrutor da classe JanelaJogo.
     * 
     * Realiza a liberação das texturas carregadas para o jogo e suas capturas, 
     * garantindo a integridade da memória de vídeo.
     */
    ~JanelaJogo();

    /**
     * @brief Renderiza todos os elementos da janela detalhada.
     * 
     * @param offsetX Deslocamento horizontal para a renderização.
     * @param offsetY Deslocamento vertical para a renderização.
     */
    void desenhar(int offsetX = 0, int offsetY = 0);

    /**
     * @brief Processa eventos de entrada (mouse e janela) para a interface.
     * 
     * @param evento Referência para o SDL_Event capturado.
     * @param offsetX Deslocamento horizontal do componente.
     * @param offsetY Deslocamento vertical do componente.
     * @return true se o evento resultou em uma interação consumida, false caso contrário.
     */
    bool tratarEvento(SDL_Event& evento, int offsetX, int offsetY);

    /**
     * @brief Inicia o processo de execução do título selecionado.
     */
    void executarJogo();

    /**
     * @brief Processa entradas específicas de dispositivos de controle (gamepads).
     * 
     * Trata a navegação entre capturas de tela e o foco nos botões de ação 
     * utilizando os botões de ombro (shoulders) e direcionais.
     * 
     * @param evento Referência para o SDL_Event.
     * @return true se a entrada do controle foi processada.
     */
    bool tratarEventoControle(SDL_Event& evento);

private:
    SDL_Renderer* renderer; /**< Referência ao renderizador SDL. */
    std::string codigoJogo;; /**< Código identificador do jogo. */
    SDL_Texture* imagemJogo; /**< Textura da imagem de capa/principal do jogo. */
    std::vector<SDL_Texture*> capturasTexturas; /**< Vetor de texturas das capturas de tela da galeria. */
    std::string nomeJogo; /**< Nome do título para exibição no topo. */
    std::string descricaoLonga; /**< Conteúdo descritivo do jogo. */
    
    Botao botaoJogar; /**< Componente de botão para iniciar o jogo. */
    Botao botaoFechar; /**< Componente de botão para encerrar a visualização detalhada. */
    
    SDL_Rect areaJanela; /**< Retângulo definindo os limites totais da janela. */
    SDL_Rect areaTopo; /**< Retângulo definindo a região superior (título). */
    SDL_Rect areaFundo; /**< Retângulo definindo o plano de fundo principal. */
    SDL_Rect areaImagem; /**< Retângulo definindo o posicionamento da imagem de capa. */
    SDL_Rect areaDescricao; /**< Retângulo definindo a zona de renderização da descrição. */
    SDL_Rect areaFundoCapturas; /**< Retângulo definindo o contêiner da galeria de capturas. */
    Uint32 tempoAbertura;
    
    Botao setaEsquerda; /**< Botão de navegação para captura de tela anterior. */
    Botao setaDireita; /**< Botão de navegação para próxima captura de tela. */
    
    int capturaIndex; /**< Índice da captura de tela atualmente visível na galeria. */
    
    bool botaoJogarFocado; /**< Indica se o botão de execução possui o foco de navegação. */
    Uint32 ultimoTempoLB; /**< Marca temporal para controle de debounce do botão Left Shoulder. */
    Uint32 ultimoTempoRB; /**< Marca temporal para controle de debounce do botão Right Shoulder. */
    
    bool animacaoSetaEsquerda; /**< Sinalizador de ativação do efeito visual na seta esquerda. */
    bool animacaoSetaDireita; /**< Sinalizador de ativação do efeito visual na seta direita. */
    Uint32 tempoAnimacaoEsquerda; /**< Cronômetro interno para a duração da animação esquerda. */
    Uint32 tempoAnimacaoDireita; /**< Cronômetro interno para a duração da animação direita. */
    
    int capturaLargura; /**< Dimensão horizontal calculada para as capturas de tela. */
    int capturaAltura; /**< Dimensão vertical calculada para as capturas de tela. */
    int capturaPosY; /**< Posição vertical calculada para o alinhamento da galeria. */
    int capturaEspacamento; /**< Distância entre elementos da galeria de capturas. */
    int margemSetas; /**< Espaçamento lateral para definição de recorte dinâmico das setas. */

    /**
     * @brief Intervalo de tempo (ms) para evitar múltiplas entradas consecutivas nos gatilhos.
     */
    static constexpr Uint32 DEBOUNCE_SHOULDER = 300;

    /**
     * @brief Tempo total de duração (ms) para as animações de feedback visual.
     */
    static constexpr Uint32 DURACAO_ANIMACAO = 200;

    /**
     * @brief Renderiza a camada de fundo da interface detalhada.
     */
    void desenharFundo();

    /**
     * @brief Renderiza a seção superior, incluindo o título do jogo.
     */
    void desenharTopo();

    /**
     * @brief Renderiza a área de base que compõe o plano de fundo dos detalhes.
     */
    void desenharAreaFundo();

    /**
     * @brief Renderiza a imagem principal/capa do jogo com os devidos deslocamentos.
     * @param offsetX Deslocamento horizontal.
     * @param offsetY Deslocamento vertical.
     */
    void desenharImagem(int offsetX, int offsetY);

    /**
     * @brief Processa e desenha o texto de descrição do jogo.
     * @param offsetX Deslocamento horizontal.
     * @param offsetY Deslocamento vertical.
     */
    void desenharDescricao(int offsetX, int offsetY);

    /**
     * @brief Renderiza a galeria de capturas de tela e gerencia o recorte visual.
     * @param offsetX Deslocamento horizontal.
     * @param offsetY Deslocamento vertical.
     */
    void desenharCapturas(int offsetX, int offsetY);

    /**
     * @brief Renderiza os botões interativos (Jogar, Fechar e Setas).
     * @param offsetX Deslocamento horizontal.
     * @param offsetY Deslocamento vertical.
     */
    void desenharBotoes(int offsetX, int offsetY);

    /**
     * @brief Atualiza o estado lógico das animações baseando-se no tempo decorrido.
     */
    void atualizarAnimacoes();
};

} // namespace MeuProjeto

#endif // JANELA_JOGO_HPP