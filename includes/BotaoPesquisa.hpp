#ifndef BOTAOPESQUISA_HPP
#define BOTAOPESQUISA_HPP

#pragma once

#include "Botao.hpp"
#include "Utils.hpp"  // Para TipoFonte
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "Jogo.hpp"
#include "GerenciadorJogos.hpp"

namespace MeuProjeto {

/**
 * @class BotaoPesquisa
 * @brief Estende a classe Botao para criar um campo de input e exibir resultados de pesquisa dinamicamente.
 *
 * Esta classe gerencia a entrada de texto, lida com a busca de jogos e
 * renderiza os resultados abaixo do campo de input principal.
 */
class BotaoPesquisa : public Botao {
public:
    /**
     * @brief Construtor do BotaoPesquisa.
     *
     * @param renderer O renderizador SDL necessário para carregar texturas e iniciar input.
     * @param x Coordenada X superior esquerda.
     * @param y Coordenada Y superior esquerda.
     * @param largura Largura do campo de input.
     * @param altura Altura do campo de input.
     * @param texto Texto placeholder inicial (opcional).
     */
    BotaoPesquisa(SDL_Renderer* renderer, int x, int y, int largura, int altura, const std::string& texto = "");

    /**
     * @brief Destrutor. Para a entrada de texto do SDL.
     */
    ~BotaoPesquisa();

    /**
     * @brief Trata eventos de mouse, teclado e entrada de texto.
     *
     * @param evento A estrutura SDL_Event a ser tratada.
     * @param offsetX Deslocamento X da área de desenho.
     * @param offsetY Deslocamento Y da área de desenho.
     * @return Retorna @c true se o evento foi consumido.
     */
    bool tratarEvento(SDL_Event& evento, int offsetX, int offsetY) override;

    /**
     * @brief Desenha o campo de input e os resultados de pesquisa.
     *
     * @param renderer O renderizador SDL.
     * @param offsetX Deslocamento X.
     * @param offsetY Deslocamento Y.
     */
    void desenhar(SDL_Renderer* renderer, int offsetX, int offsetY) override;

    // --- Funções Públicas ---
    /**
     * @brief Retorna o jogo que foi clicado na lista de resultados.
     *
     * @return Ponteiro para o objeto @c Jogo, ou @c nullptr se nenhum foi clicado.
     */
    Jogo* getJogoClicado() const { return jogoClicado; }

    /**
     * @brief Limpa o sinal de clique no jogo (para o main loop não abrir a janela repetidamente).
     */
    void resetarJogoClicado() { jogoClicado = nullptr; }

    /**
     * @brief Define o tipo de fonte para o texto placeholder e de input.
     *
     * @param tipo O enumerador @c TipoFonte.
     */
    void setFontePlaceholder(TipoFonte tipo) { fontePlaceholder = tipo; }

    /**
     * @brief Define o tamanho da fonte para o texto placeholder e de input.
     *
     * @param tamanho O tamanho em pontos (pt).
     */
    void setTamanhoFontePlaceholder(int tamanho) { tamanhoPlaceholder = tamanho; }

    /**
     * @brief Define o tipo de fonte para os resultados da pesquisa.
     *
     * @param tipo O enumerador @c TipoFonte.
     */
    void setFonteResultados(TipoFonte tipo) { fonteResultados = tipo; }

    /**
     * @brief Define o tamanho da fonte para os resultados da pesquisa.
     *
     * @param tamanho O tamanho em pontos (pt).
     */
    void setTamanhoFonteResultados(int tamanho) { tamanhoResultados = tamanho; }

private:
    /**
     * @brief Ponteiro para o renderizador SDL.
     */
    SDL_Renderer* renderer;

    /**
     * @brief O texto atual digitado no campo de input.
     */
    std::string textoInput;

    /**
     * @brief Flag que indica se o campo de input está ativo e recebendo entrada.
     */
    bool inputAtivo;
    
    /**
     * @brief Índice do resultado de pesquisa atualmente sob o mouse (hover).
     */
    int resultadoSelecionado;

    /**
     * @brief Cor atual do botão (mantida por compatibilidade, mas a cor é determinada em desenhar).
     */
    SDL_Color corAtual;

    /**
     * @brief Ponteiro para sinalizar qual jogo foi clicado na lista de resultados.
     */
    Jogo* jogoClicado = nullptr; 

    // *** CONFIGURAÇÕES DE FONTE ***
    TipoFonte fontePlaceholder = TipoFonte::NORMAL;
    int tamanhoPlaceholder = 28;
    TipoFonte fonteResultados = TipoFonte::NEGRITO;
    int tamanhoResultados = 32;

    // Constantes de Layout
    const int ITEM_HEIGHT = 120; ///< Altura de cada item de resultado.
    const int ITEM_PADDING = 6;  ///< Espaçamento entre os itens de resultado.
    const int IMG_SIZE = 90;     ///< Tamanho da imagem de capa nos resultados.
    const int IMG_OFFSET_Y = (ITEM_HEIGHT - IMG_SIZE) / 2; ///< Offset Y para centralizar a imagem.
    const int IMG_OFFSET_X = 15; ///< Offset X da imagem.
    const int TEXT_OFFSET_Y = 10;  ///< Offset Y do texto.
    const int TEXT_OFFSET_X = IMG_OFFSET_X + IMG_SIZE + 20; ///< Offset X do texto.
    const int MAX_RESULTADOS = 3; ///< Número máximo de resultados a serem exibidos.

    /**
     * @brief Desenha a lista de resultados de pesquisa abaixo do campo de input.
     *
     * @param adjustedArea A área ajustada do campo de input principal.
     */
    void desenharResultados(const SDL_Rect& adjustedArea);

    /**
     * @brief Conta quantos jogos correspondem ao input atual.
     *
     * @param input O texto de busca.
     * @return O número de resultados encontrados.
     */
    int contarResultados(const std::string& input);
    
    // Funções de handle (não usadas na implementação atual, mas mantidas por convenção)
    bool handleMouseClick(SDL_Event& evento, int offsetX, int offsetY);
    void handleTextInput(SDL_Event& evento);
    bool handleKeyDown(SDL_Event& evento);
};

} // namespace MeuProjeto

#endif // BOTAOPESQUISA_HPP