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
 * @brief Representa a janela de visualização detalhada de um jogo (tela de detalhes ou loja).
 *
 * Exibe a imagem principal, descrição, capturas de tela em um carrossel e botões
 * para interagir (Jogar/Fechar).
 */
class JanelaJogo {
public:
    /**
     * @brief Construtor da Janela de Jogo.
     *
     * @param renderer O renderizador SDL principal.
     * @param nomeJogo O nome do jogo.
     * @param imagemPath Caminho para a imagem principal/capa.
     * @param descricao A descrição longa do jogo.
     * @param capturasPaths Vetor de caminhos para as capturas de tela.
     */
    JanelaJogo(SDL_Renderer* renderer, const std::string& nomeJogo, const std::string& imagemPath,
               const std::string& descricao, const std::vector<std::string>& capturasPaths);

    /**
     * @brief Destrutor.
     */
    ~JanelaJogo();

    /**
     * @brief Desenha todos os elementos da janela (fundo, imagens, texto, carrossel e botões).
     *
     * @param offsetX Deslocamento X da viewport (padrão 0).
     * @param offsetY Deslocamento Y da viewport (padrão 0).
     */
    void desenhar(int offsetX = 0, int offsetY = 0);

    /**
     * @brief Trata eventos de mouse e teclado, incluindo cliques nos botões e setas.
     *
     * @param evento A estrutura SDL_Event.
     * @param offsetX Deslocamento X.
     * @param offsetY Deslocamento Y.
     * @return Retorna @c true se o evento indica que a janela deve ser fechada (ex: clique no 'X' ou ESC).
     */
    bool tratarEvento(SDL_Event& evento, int offsetX, int offsetY);
    
    /**
     * @brief Inicia o processo de execução do jogo.
     *
     * Esta função deve conter a lógica para chamar o executável ou script de lançamento.
     */
    void executarJogo();

private:
    SDL_Renderer* renderer; ///< O renderizador SDL.
    SDL_Texture* imagemJogo; ///< Textura da imagem principal/capa.
    std::vector<SDL_Texture*> capturasTexturas; ///< Vetor de texturas das capturas de tela.
    std::string nomeJogo; ///< Nome do jogo.
    std::string descricao; ///< Descrição longa do jogo.

    Botao botaoJogar; ///< Botão principal para iniciar o jogo.
    Botao botaoFechar; ///< Botão 'X' para fechar a janela.

    SDL_Rect areaJanela; ///< Área total da janela.
    SDL_Rect areaTopo; ///< Área do cabeçalho.
    SDL_Rect areaFundo; ///< Área do painel principal (capa + descrição).
    SDL_Rect areaImagem; ///< Área onde a capa do jogo é desenhada.
    SDL_Rect areaDescricao; ///< Área do texto de descrição.
    SDL_Rect areaFundoCapturas; ///< Área do fundo do carrossel de capturas.
    
    Botao setaEsquerda; ///< Botão para navegar à esquerda no carrossel.
    Botao setaDireita; ///< Botão para navegar à direita no carrossel.
    
    int capturaIndex; ///< Índice da captura de tela central no carrossel.

    // Constantes de layout do carrossel de capturas
    static constexpr int CAPTURA_LARGURA = 480; ///< Largura da captura de tela no carrossel.
    static constexpr int CAPTURA_ALTURA = 270; ///< Altura da captura de tela no carrossel.
    static constexpr int CAPTURA_POS_Y = 790; ///< Posição Y do carrossel.
    static constexpr int CAPTURA_ESPACAMENTO = 520; ///< Espaçamento entre as capturas no carrossel.

    // Funções de desenho auxiliares
    void desenharFundo();
    void desenharTopo();
    void desenharAreaFundo();
    void desenharImagem(int offsetX, int offsetY);
    void desenharDescricao(int offsetX, int offsetY);
    void desenharCapturas(int offsetX, int offsetY);
    void desenharBotoes(int offsetX, int offsetY);
};

} // namespace MeuProjeto

#endif // JANELA_JOGO_HPP