/**
 * @file Forma.cpp
 * @brief Implementação dos métodos da classe Forma para renderização de primitivas.
 * 
 * Este arquivo contém a lógica de desenho para diferentes formas geométricas,
 * permitindo a criação de elementos visuais básicos que compõem a interface 
 * do launcher de jogos.
 */

#include "Forma.hpp"
#include <SDL2/SDL2_gfxPrimitives.h>

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Construtor da classe Forma.
 * 
 * Inicializa as propriedades de estilo e geometria da forma.
 * 
 * @param x Coordenada horizontal inicial.
 * @param y Coordenada vertical inicial.
 * @param largura Largura da área da forma.
 * @param altura Altura da área da forma.
 * @param cor Cor inicial no formato SDL_Color (RGBA).
 * @param tipoForma String identificadora ("retangulo", "quadrado", "circulo", "triangulo").
 */
Forma::Forma(int x, int y, int largura, int altura, SDL_Color cor, const std::string& tipoForma)
    : cor(cor), tipo(tipoForma)
{
    area = {x, y, largura, altura};
}

/**
 * @brief Renderiza a forma na sua posição original.
 * 
 * Este método atua como um wrapper para a função de desenho com offset,
 * passando deslocamento zero por padrão.
 * 
 * @param render O ponteiro para o SDL_Renderer onde a forma será desenhada.
 */
void Forma::desenhar(SDL_Renderer* render) {
    desenhar(render, 0, 0); 
}

/**
 * @brief Renderiza a forma na tela aplicando cálculos de geometria baseados no tipo.
 * 
 * A função aplica um deslocamento (offset) às coordenadas originais e executa a 
 * lógica de desenho específica:
 * - **quadrado**: Calcula o menor lado para garantir a proporção 1:1.
 * - **retangulo**: Preenche a área total definida por SDL_Rect.
 * - **circulo**: Calcula o centro e o raio máximo permitido, utilizando a biblioteca SDL2_gfx.
 * - **triangulo**: Define três vértices baseados na área e utiliza SDL2_gfx para preenchimento.
 * 
 * @param render O ponteiro para o SDL_Renderer.
 * @param offsetX Deslocamento horizontal para renderização relativa.
 * @param offsetY Deslocamento vertical para renderização relativa.
 */
void Forma::desenhar(SDL_Renderer* render, int offsetX, int offsetY) {
    // Define a cor de desenho no renderer para as funções nativas da SDL
    SDL_SetRenderDrawColor(render, cor.r, cor.g, cor.b, cor.a);

    // Calcula a área ajustada com base nos deslocamentos fornecidos (scroll/posicionamento relativo)
    SDL_Rect adjustedArea = {area.x + offsetX, area.y + offsetY, area.w, area.h};

    if (tipo == "quadrado")
    {
        // Garante que a forma seja um quadrado perfeito usando a menor dimensão disponível
        int lado = (area.w < area.h) ? area.w : area.h;
        SDL_Rect quadrado = {adjustedArea.x, adjustedArea.y, lado, lado};
        SDL_RenderFillRect(render, &quadrado);
    }
    else if (tipo == "retangulo")
    {
        int raio = 20; // mesmo padrão visual dos outros retângulos do sistema

        roundedBoxRGBA(
            render,
            adjustedArea.x,
            adjustedArea.y,
            adjustedArea.x + adjustedArea.w,
            adjustedArea.y + adjustedArea.h,
            raio,
            cor.r,
            cor.g,
            cor.b,
            cor.a
            );
    }
    else if (tipo == "circulo")
    {
        // Cálculos para centralização e definição do raio
        int centroX = adjustedArea.x + adjustedArea.w / 2;
        int centroY = adjustedArea.y + adjustedArea.h / 2;
        int raio = (adjustedArea.w < adjustedArea.h ? adjustedArea.w : adjustedArea.h) / 2;
        
        // Uso da biblioteca SDL2_gfx para desenho de círculos suavizados e preenchidos
        filledCircleRGBA(render, centroX, centroY, raio, cor.r, cor.g, cor.b, cor.a);
    }
    else if (tipo == "triangulo")
    {
        // Define os três pontos do triângulo (Base esquerda, Base direita, Topo central)
        Sint16 vx[3] = {(Sint16)adjustedArea.x, (Sint16)(adjustedArea.x + adjustedArea.w), (Sint16)(adjustedArea.x + adjustedArea.w / 2)};
        Sint16 vy[3] = {(Sint16)(adjustedArea.y + adjustedArea.h), (Sint16)(adjustedArea.y + adjustedArea.h), (Sint16)adjustedArea.y};
        
        // Uso da biblioteca SDL2_gfx para renderizar o polígono preenchido
        filledPolygonRGBA(render, vx, vy, 3, cor.r, cor.g, cor.b, cor.a);
    }
}

/**
 * @brief Atualiza a cor interna da forma.
 * @param novaCor O novo struct SDL_Color a ser aplicado.
 */
void Forma::mudarCor(SDL_Color novaCor)
{
    cor = novaCor;
}

/**
 * @brief Altera a posição absoluta da forma no sistema de coordenadas.
 * @param novoX Nova coordenada horizontal.
 * @param novoY Nova coordenada vertical.
 */
void Forma::mover(int novoX, int novoY)
{
    area.x = novoX;
    area.y = novoY;
}