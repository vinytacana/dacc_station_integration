#ifndef BOTAO_HPP
#define BOTAO_HPP

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include "Utils.hpp" // Para TipoFonte

// Declaração antecipada (Forward Declaration) para evitar inclusão cíclica
namespace MeuProjeto {
    class Janela; 
}

#include "GerenciadorImagens.hpp"

namespace MeuProjeto {

/**
 * @class Botao
 * @brief Classe que representa um botão interativo na interface gráfica SDL.
 *
 * O botão pode ser baseado em texto ou imagem, suporta diferentes estados
 * (padrão, hover, pressionado) e pode acionar popups de descrição.
 */
class Botao {
public:
    /**
     * @brief Construtor padrão para um botão baseado em texto ou cor.
     *
     * @param x Coordenada X superior esquerda.
     * @param y Coordenada Y superior esquerda.
     * @param largura Largura do botão.
     * @param altura Altura do botão.
     * @param texto O texto a ser exibido no botão (opcional).
     */
    Botao(int x, int y, int largura, int altura, const std::string& texto = "");

    /**
     * @brief Construtor para um botão baseado em imagem.
     *
     * @param renderer O renderizador SDL.
     * @param x Coordenada X superior esquerda.
     * @param y Coordenada Y superior esquerda.
     * @param largura Largura do botão.
     * @param altura Altura do botão.
     * @param imagemPath O caminho para o arquivo de imagem.
     * @param nome O nome associado ao jogo (para o popup).
     * @param desc A descrição associada ao jogo (para o popup).
     * @param isDestaque Indica se o botão é um destaque (muda a transparência).
     */
    Botao(SDL_Renderer* renderer, int x, int y, int largura, int altura, 
          const std::string& imagemPath, const std::string& nome = "", 
          const std::string& desc = "", bool isDestaque = false);
    
    /**
     * @brief Destrutor virtual.
     */
    virtual ~Botao();

    /**
     * @brief Trata eventos de mouse (movimento, clique) para atualizar o estado do botão.
     *
     * @param evento A estrutura SDL_Event a ser tratada.
     * @param offsetX Deslocamento X da área de desenho (útil para botões em rolagem).
     * @param offsetY Deslocamento Y da área de desenho.
     * @return Retorna @c true se o evento foi consumido pelo botão, @c false caso contrário.
     */
    virtual bool tratarEvento(SDL_Event& evento, int offsetX, int offsetY);
    
    /**
     * @brief Desenha o botão na posição absoluta (sem offset).
     *
     * @param renderer O renderizador SDL.
     */
    virtual void desenhar(SDL_Renderer* renderer); 
    
    /**
     * @brief Desenha o botão com um deslocamento (offset) aplicado.
     *
     * @param renderer O renderizador SDL.
     * @param offsetX Deslocamento X.
     * @param offsetY Deslocamento Y.
     */
    virtual void desenhar(SDL_Renderer* renderer, int offsetX, int offsetY);

    /**
     * @brief Define a textura da imagem do botão.
     *
     * @param textura O ponteiro para a textura SDL.
     */
    void setTexturaImagem(SDL_Texture* textura);

    /**
     * @brief Define as cores dos estados do botão.
     *
     * @param corPadrao Cor normal.
     * @param corHover Cor quando o mouse está sobre o botão.
     * @param corPressionado Cor quando o botão está pressionado.
     */
    void setCor(const SDL_Color& corPadrao, const SDL_Color& corHover, const SDL_Color& corPressionado);

    /**
     * @brief Define a janela pai para manipulação de popups.
     *
     * @param janela O ponteiro para a janela pai.
     */
    void setParentJanela(Janela* janela);

    /**
     * @brief Define se o botão deve ser desenhado como um círculo (arredondado).
     *
     * @param round Se @c true, o botão é desenhado como um círculo.
     */
    void setIsRound(bool round);
    
    /**
     * @brief Define o tipo de fonte para o texto do botão.
     *
     * @param tipo O enumerador @c TipoFonte.
     */
    void setFonte(TipoFonte tipo) { tipoFonte = tipo; }
    
    /**
     * @brief Define o tamanho da fonte para o texto do botão.
     *
     * @param tamanho O tamanho em pontos (pt).
     */
    void setTamanhoFonte(int tamanho) { tamanhoFonte = tamanho; }
    
    /**
     * @brief Obtém o tipo de fonte atual.
     *
     * @return O enumerador @c TipoFonte.
     */
    TipoFonte getFonte() const { return tipoFonte; }
    
    /**
     * @brief Obtém o tamanho da fonte atual.
     *
     * @return O tamanho da fonte.
     */
    int getTamanhoFonte() const { return tamanhoFonte; }
    
    /**
     * @brief Verifica se o botão está no estado pressionado.
     *
     * @return Retorna @c true se o botão estiver pressionado, @c false caso contrário.
     */
    bool isPressed() const { return pressionado; }
    
    /**
     * @brief Obtém a textura de imagem associada ao botão.
     *
     * @return O ponteiro para a textura SDL.
     */
    SDL_Texture* getTexturaImagem() const { return texturaImagem; }

    /**
     * @brief Verifica se um ponto (x, y) está contido na área do botão.
     *
     * @param x Coordenada X do ponto.
     * @param y Coordenada Y do ponto.
     * @return Retorna @c true se o ponto estiver dentro da área, @c false caso contrário.
     */
    bool contemPonto(int x, int y);
    
    /**
     * @brief Define manualmente o estado de seleção (hover) do botão.
     * Útil para navegação via teclado/controle.
     * @param selecionado true para ativar o visual de hover.
     */
    void setSelecionado(bool selecionado);

    /**
     * @brief Verifica se o botão está manualmente selecionado.
     */
    bool estaSelecionado() const;

    /**
     * @brief A área retangular do botão.
     */
    SDL_Rect area;

    /**
     * @brief A cor padrão do botão.
     */
    SDL_Color corPadrao;

    /**
     * @brief A cor do botão no estado hover.
     */
    SDL_Color corHover;

    /**
     * @brief A cor do botão no estado pressionado.
     */
    SDL_Color corPressionado;

    /**
     * @brief O texto a ser exibido no botão.
     */
    std::string texto;
    
    /**
     * @brief Define o texto a ser exibido no botão.
     *
     * @param txt O novo texto.
     */
    void setTexto(const std::string& txt) { texto = txt; }
    
    /**
     * @brief Define o caminho da imagem (útil para recarregar).
     *
     * @param path O novo caminho da imagem.
     */
    void setImagemPath(const std::string& path) { imagemPath = path; }

    /**
     * @brief Obtém o nome do jogo/item associado.
     *
     * @return O nome do jogo/item.
     */
    const std::string& getNomeJogo() const { return nomeJogo; }

    /**
     * @brief Obtém a descrição do jogo/item associado.
     *
     * @return A descrição do jogo/item.
     */
    const std::string& getDescJogo() const { return descJogo; }

    /**
     * @brief Obtém o caminho da imagem atual.
     *
     * @return O caminho da imagem.
     */
    const std::string& getImagemPath() const { return imagemPath; }

    /**
     * @brief Obtém a área do botão.
     *
     * @return A estrutura @c SDL_Rect.
     */
    const SDL_Rect& getArea() const { return area; }

protected:
    /**
     * @brief Textura da imagem do botão.
     */
    SDL_Texture* texturaImagem;

    /**
     * @brief Estado de hover (mouse sobre o botão).
     */
    bool hover;

    /**
     * @brief Estado de pressionado (botão esquerdo do mouse clicado).
     */
    bool pressionado;
    
    /**
     * @brief Tipo de fonte selecionado para o texto.
     */
    TipoFonte tipoFonte;

    /**
     * @brief Tamanho da fonte selecionado para o texto.
     */
    int tamanhoFonte;

    /**
     * @brief Determina a cor de fundo atual com base nos estados (pressionado/hover).
     *
     * @return A estrutura @c SDL_Color.
     */
    SDL_Color determineCurrentColor();

private:
    /**
     * @brief Trata o evento de movimento do mouse (atualiza hover e popups).
     */
    bool handleMouseMotion(SDL_Event& evento, int offsetX, int offsetY);

    /**
     * @brief Trata o evento de botão do mouse para baixo (atualiza pressionado).
     */
    bool handleMouseButtonDown(SDL_Event& evento, int offsetX, int offsetY);

    /**
     * @brief Trata o evento de botão do mouse para cima (finaliza o clique).
     */
    bool handleMouseButtonUp(SDL_Event& evento, int offsetX, int offsetY);

    /**
     * @brief Renderiza o texto centralizado na área do botão.
     */
    void renderizarTextoCentralizado(SDL_Renderer* renderer, const SDL_Rect& area);
    
    /**
     * @brief O caminho do arquivo de imagem.
     */
    std::string imagemPath;

    /**
     * @brief Ponteiro para a janela pai (para popups).
     */
    Janela* parentJanela;

    /**
     * @brief Nome do jogo associado.
     */
    std::string nomeJogo;

    /**
     * @brief Descrição do jogo associado.
     */
    std::string descJogo;

    /**
     * @brief Indica se o botão é circular.
     */
    bool isRound = false;

    /**
     * @brief Indica se o botão é um destaque.
     */
    bool isDestaque = false;
};

} // namespace MeuProjeto

#endif // BOTAO_HPP