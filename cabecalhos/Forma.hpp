/**
 * @file Forma.hpp
 * @brief Definição da classe Forma para renderização de primitivas gráficas.
 * 
 * Este arquivo contém a classe Forma, que facilita o desenho de retângulos 
 * e outras formas geométricas básicas utilizando a API da SDL2, permitindo 
 * a criação de painéis, botões e fundos customizados.
 */

#ifndef FORMA_HPP
#define FORMA_HPP

#pragma once

#include <SDL2/SDL.h>
#include <string>

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class Forma
 * @brief Representa uma forma geométrica renderizável na interface.
 * 
 * A classe Forma encapsula propriedades de geometria (posição e tamanho), 
 * cor e estilo (tipo). Ela permite que elementos visuais sejam manipulados 
 * como objetos, facilitando a alteração de cores e posições dinamicamente 
 * durante a execução do launcher.
 */
class Forma {
public:
    /**
     * @brief Construtor da classe Forma.
     * 
     * @param x Coordenada X inicial.
     * @param y Coordenada Y inicial.
     * @param largura Largura da forma em pixels.
     * @param altura Altura da forma em pixels.
     * @param cor Estrutura SDL_Color definindo a cor inicial (RGBA).
     * @param tipoForma String que define o comportamento do desenho (ex: "retangulo", "preenchido").
     */
    Forma(int x, int y, int largura, int altura, SDL_Color cor, const std::string& tipoForma);

    /**
     * @brief Renderiza a forma na tela na sua posição original.
     * 
     * @param render O renderizador SDL onde a forma será desenhada.
     */
    void desenhar(SDL_Renderer* render);

    /**
     * @brief Renderiza a forma na tela com um deslocamento (offset).
     * 
     * Útil para desenhar formas dentro de containers que se movem, como 
     * listas de jogos roláveis ou janelas de popup que possuem coordenadas relativas.
     * 
     * @param render O renderizador SDL.
     * @param offsetX Deslocamento horizontal adicional.
     * @param offsetY Deslocamento vertical adicional.
     */
    void desenhar(SDL_Renderer* render, int offsetX, int offsetY);

    /**
     * @brief Atualiza a cor da forma.
     * 
     * @param novaCor Nova estrutura SDL_Color a ser aplicada.
     */
    void mudarCor(SDL_Color novaCor);

    /**
     * @brief Altera a posição da forma no espaço da tela.
     * 
     * @param novoX Nova coordenada X.
     * @param novoY Nova coordenada Y.
     */
    void mover(int novoX, int novoY);

private:
    SDL_Rect area;     /**< Estrutura SDL que armazena posição (x, y) e dimensões (w, h). */
    SDL_Color cor;     /**< Cor atual da forma no formato RGBA. */
    std::string tipo;  /**< Identificador do estilo de renderização da forma. */
};

} // namespace MeuProjeto

#endif // FORMA_HPP