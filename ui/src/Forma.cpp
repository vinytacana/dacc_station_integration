#include "Forma.hpp"
#include <SDL2/SDL2_gfxPrimitives.h>

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Construtor da classe Forma
 * 
 * Inicializa uma forma geométrica com posição, dimensões, cor e tipo especificados.
 * Utiliza lista de inicialização para membros e define a área retangular da forma.
 * 
 * @param x Posição X do canto superior esquerdo da forma
 * @param y Posição Y do canto superior esquerdo da forma
 * @param largura Largura da forma em pixels
 * @param altura Altura da forma em pixels
 * @param cor Cor da forma no formato SDL_Color (RGBA)
 * @param tipoForma Tipo da forma geométrica ("quadrado", "retangulo", "circulo", "triangulo")
 */
Forma::Forma(int x, int y, int largura, int altura, SDL_Color cor, const std::string& tipoForma)
    : cor(cor), tipo(tipoForma)
{
    area = {x, y, largura, altura};
}

/**
 * @brief Desenha a forma na tela sem deslocamento
 * 
 * Método sobrecarregado que chama a versão com deslocamento usando valores zero.
 * 
 * @param render Ponteiro para o renderizador SDL onde a forma será desenhada
 */
void Forma::desenhar(SDL_Renderer* render) 
{
    desenhar(render, 0, 0); // Chama o método com deslocamento padrão
}

/**
 * @brief Desenha a forma na tela com deslocamento aplicado
 * 
 * Renderiza a forma geométrica baseada no seu tipo, aplicando um deslocamento
 * nas coordenadas. Suporta quadrados, retângulos, círculos e triângulos.
 * 
 * @param render Ponteiro para o renderizador SDL onde a forma será desenhada
 * @param offsetX Deslocamento horizontal a ser aplicado na posição X
 * @param offsetY Deslocamento vertical a ser aplicado na posição Y
 */
void Forma::desenhar(SDL_Renderer* render, int offsetX, int offsetY) 
{
    // Define a cor de desenho no renderizador
    SDL_SetRenderDrawColor(render, cor.r, cor.g, cor.b, cor.a);

    // Aplica o deslocamento à área da forma
    SDL_Rect adjustedArea = {area.x + offsetX, area.y + offsetY, area.w, area.h};
    
    // Desenha a forma baseada no tipo especificado
    if (tipo == "quadrado")
    {
        // Para quadrado, usa o menor lado entre largura e altura
        int lado = (area.w < area.h) ? area.w : area.h;
        SDL_Rect quadrado = {adjustedArea.x, adjustedArea.y, lado, lado};
        SDL_RenderFillRect(render, &quadrado);
    }
    else if (tipo == "retangulo")
    {
        // Desenha retângulo preenchendo toda a área
        SDL_RenderFillRect(render, &adjustedArea);
    }
    else if (tipo == "circulo")
    {
        // Calcula centro e raio do círculo
        int centroX = adjustedArea.x + adjustedArea.w / 2;
        int centroY = adjustedArea.y + adjustedArea.h / 2;
        int raio = (adjustedArea.w < adjustedArea.h ? adjustedArea.w : adjustedArea.h) / 2;
        
        // Usa função da SDL2_gfx para desenhar círculo preenchido
        filledCircleRGBA(render, centroX, centroY, raio, cor.r, cor.g, cor.b, cor.a);
    }
    else if (tipo == "triangulo")
    {
        // Define os vértices do triângulo (base na parte inferior)
        Sint16 vx[3] = {
            (Sint16)adjustedArea.x, 
            (Sint16)(adjustedArea.x + adjustedArea.w), 
            (Sint16)(adjustedArea.x + adjustedArea.w / 2)
        };
        Sint16 vy[3] = {
            (Sint16)(adjustedArea.y + adjustedArea.h), 
            (Sint16)(adjustedArea.y + adjustedArea.h), 
            (Sint16)adjustedArea.y
        };
        
        // Usa função da SDL2_gfx para desenhar polígono preenchido
        filledPolygonRGBA(render, vx, vy, 3, cor.r, cor.g, cor.b, cor.a);
    }
}

/**
 * @brief Altera a cor da forma
 * 
 * Permite modificar a cor de renderização da forma geometrica.
 * 
 * @param novaCor Nova cor a ser aplicada à forma, no formato SDL_Color
 */
void Forma::mudarCor(SDL_Color novaCor)
{
    cor = novaCor;
}

/**
 * @brief Move a forma para uma nova posição
 * 
 * Atualiza as coordenadas X e Y da forma sem alterar suas dimensões.
 * 
 * @param novoX Nova coordenada X para a posição da forma
 * @param novoY Nova coordenada Y para a posição da forma
 */
void Forma::mover(int novoX, int novoY)
{
    area.x = novoX;
    area.y = novoY;
}