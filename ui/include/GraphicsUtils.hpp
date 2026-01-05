#ifndef GRAPHICS_UTILS_HPP
#define GRAPHICS_UTILS_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <string>

namespace MeuProjeto {

class GraphicsUtils {
public:
    /**
     * @brief Draws a filled rounded rectangle.
     */
    static void drawRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color);

    /**
     * @brief Draws a rounded rectangle border (outline).
     */
    static void drawRoundedRectOutline(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color);

    /**
     * @brief Renders a texture filling the target area, maintaining aspect ratio (cover mode) or stretching.
     * Useful for backgrounds.
     */
    static void drawTextureCover(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect& targetRect, int alpha = 255);

    /**
     * @brief Draws a dimmer overlay (fullscreen rectangle with alpha).
     */
    static void drawOverlay(SDL_Renderer* renderer, int width, int height, int alpha);
};

} // namespace MeuProjeto

#endif // GRAPHICS_UTILS_HPP
