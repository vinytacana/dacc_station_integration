#ifndef GRAPHICS_UTILS_HPP
#define GRAPHICS_UTILS_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>
#include <string>

namespace MeuProjeto {

/**
 * @class GraphicsUtils
 * @brief Classe utilitária para operações gráficas com SDL2
 * 
 * Esta classe fornece métodos estáticos para renderização de elementos
 * gráficos comuns na interface do usuário, incluindo formas geométricas,
 * texturas, overlays e barras de status do sistema.
 * 
 * Todos os métodos são estáticos e não requerem instanciação da classe.
 */
class GraphicsUtils {
public:
    /**
     * @brief Desenha um retângulo arredondado preenchido
     * 
     * Renderiza um retângulo com cantos arredondados completamente preenchido
     * com a cor especificada. Utiliza a biblioteca SDL2_gfx para criar os
     * cantos suaves.
     * 
     * @param renderer Ponteiro para o SDL_Renderer onde o retângulo será desenhado
     * @param rect Estrutura SDL_Rect definindo posição (x, y) e dimensões (w, h) do retângulo
     * @param radius Raio dos cantos arredondados em pixels (valores maiores = cantos mais arredondados)
     * @param color Cor do preenchimento no formato SDL_Color (RGBA)
     */
    static void drawRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color);

    /**
     * @brief Desenha apenas a borda de um retângulo arredondado
     * 
     * Renderiza somente o contorno de um retângulo com cantos arredondados,
     * sem preenchimento interno. Útil para criar bordas em elementos de UI.
     * 
     * @param renderer Ponteiro para o SDL_Renderer onde a borda será desenhada
     * @param rect Estrutura SDL_Rect definindo posição (x, y) e dimensões (w, h) do retângulo
     * @param radius Raio dos cantos arredondados em pixels
     * @param color Cor da borda no formato SDL_Color (RGBA)
     */
    static void drawRoundedRectOutline(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color);

    /**
     * @brief Renderiza uma textura preenchendo a área alvo, mantendo proporção ou esticando
     * 
     * Esta função desenha uma textura em uma área retangular específica. Pode operar
     * em modo "cover" (mantendo aspect ratio e cortando o excesso) ou modo stretch
     * (esticando para preencher completamente). Especialmente útil para renderizar
     * imagens de fundo (backgrounds) que precisam cobrir toda a tela.
     * 
     * @param renderer Ponteiro para o SDL_Renderer onde a textura será renderizada
     * @param texture Ponteiro para a SDL_Texture que contém a imagem a ser desenhada
     * @param targetRect Área retangular onde a textura deve ser desenhada
     * @param alpha Nível de transparência da textura (0-255), onde 0 é totalmente transparente
     *              e 255 é totalmente opaco. Valor padrão é 255 (opaco)
     */
    static void drawTextureCover(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect& targetRect, int alpha = 255);

    /**
     * @brief Desenha uma camada de sobreposição escurecida sobre a tela
     * 
     * Renderiza um retângulo semi-transparente que cobre toda a tela, criando
     * um efeito de "dimmer" ou escurecimento. Comumente usado para criar efeitos
     * de modal, menus sobrepostos ou para destacar elementos específicos da interface
     * enquanto escurece o resto da tela.
     * 
     * @param renderer Ponteiro para o SDL_Renderer onde o overlay será desenhado
     * @param width Largura da tela em pixels
     * @param height Altura da tela em pixels
     * @param alpha Nível de opacidade do overlay (0-255). Valores mais altos resultam
     *              em escurecimento mais intenso. Valores típicos: 128-200
     */
    static void drawOverlay(SDL_Renderer* renderer, int width, int height, int alpha);

    /**
     * @brief Desenha a barra de status do sistema no topo da tela
     * 
     * Renderiza a barra superior da interface contendo informações do sistema como
     * relógio (hora atual), nível de bateria e status da conexão WiFi. Esta barra
     * é tipicamente exibida de forma persistente no topo da aplicação, similar
     * a interfaces de sistemas operacionais móveis ou consoles.
     * 
     * @param renderer Ponteiro para o SDL_Renderer onde a barra será desenhada
     * @param screenWidth Largura total da tela em pixels (usado para posicionar elementos)
     * @param font Ponteiro para a fonte TTF_Font usada para renderizar o texto da barra
     */
    static void drawSystemTopBar(SDL_Renderer* renderer, int screenWidth, TTF_Font* font);

    /**
     * @brief Desenha a barra de navegação inferior com dicas de controles
     * 
     * Renderiza uma barra na parte inferior da tela exibindo hints (dicas) dos
     * botões do controle/gamepad e suas respectivas funções. Funciona como um
     * guia contextual para o usuário, mostrando quais botões estão disponíveis
     * e o que cada um faz na tela atual.
     * 
     * Exemplo: "A - Selecionar  B - Voltar  X - Opções"
     * 
     * @param renderer Ponteiro para o SDL_Renderer onde a barra será desenhada
     * @param screenWidth Largura total da tela em pixels
     * @param screenHeight Altura total da tela em pixels (usada para posicionar a barra no fundo)
     * @param font Ponteiro para a fonte TTF_Font usada para renderizar o texto das dicas
     */
    static void drawBottomNavHints(SDL_Renderer* renderer, int screenWidth, int screenHeight, TTF_Font* font);
};

} // namespace MeuProjeto

#endif // GRAPHICS_UTILS_HPP