#ifndef FORMA_HPP
#define FORMA_HPP

#pragma once

#include <SDL2/SDL.h>
#include <string>

namespace MeuProjeto {

/**
 * @class Forma
 * @brief Classe base para representar formas geométricas
 * 
 * Esta classe fornece funcionalidades básicas para desenhar e manipular
 * formas geométricas usando a biblioteca SDL2
 */
class Forma 
{
public:
    /**
     * @brief Construtor da classe Forma
     * @param x Posição X do canto superior esquerdo
     * @param y Posição Y do canto superior esquerdo
     * @param largura Largura da forma
     * @param altura Altura da forma
     * @param cor Cor da forma no formato SDL_Color
     * @param tipoForma Tipo da forma ("quadrado", "retangulo", "circulo", "triangulo")
     */
    Forma(int x, int y, int largura, int altura, SDL_Color cor, const std::string& tipoForma);
    
    /**
     * @brief Desenha a forma na tela
     * @param render Renderizador SDL onde a forma será desenhada
     */
    void desenhar(SDL_Renderer* render);
    
    /**
     * @brief Desenha a forma na tela com deslocamento
     * @param render Renderizador SDL onde a forma será desenhada
     * @param offsetX Deslocamento horizontal
     * @param offsetY Deslocamento vertical
     */
    void desenhar(SDL_Renderer* render, int offsetX, int offsetY);
    
    /**
     * @brief Altera a cor da forma
     * @param novaCor Nova cor para a forma
     */
    void mudarCor(SDL_Color novaCor);
    
    /**
     * @brief Move a forma para uma nova posição
     * @param novoX Nova coordenada X
     * @param novoY Nova coordenada Y
     */
    void mover(int novoX, int novoY);

private:
    SDL_Rect area;      ///< Área retangular que define a posição e dimensões da forma
    SDL_Color cor;      ///< Cor da forma
    std::string tipo;   ///< Tipo da forma geométrica
};

} // namespace MeuProjeto

#endif // FORMA_HPP