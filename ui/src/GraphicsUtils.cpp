#include "GraphicsUtils.hpp"

namespace MeuProjeto {

void GraphicsUtils::drawRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color) {
    roundedBoxRGBA(renderer, 
                   rect.x, rect.y, 
                   rect.x + rect.w, rect.y + rect.h, 
                   radius, 
                   color.r, color.g, color.b, color.a);
}

void GraphicsUtils::drawRoundedRectOutline(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color) {
    roundedRectangleRGBA(renderer, 
                         rect.x, rect.y, 
                         rect.x + rect.w, rect.y + rect.h, 
                         radius, 
                         color.r, color.g, color.b, color.a);
}

void GraphicsUtils::drawTextureCover(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect& targetRect, int alpha) {
    if (!texture) return;

    SDL_SetTextureAlphaMod(texture, alpha);
    SDL_RenderCopy(renderer, texture, NULL, &targetRect);
    SDL_SetTextureAlphaMod(texture, 255); // Reset
}

void GraphicsUtils::drawOverlay(SDL_Renderer* renderer, int width, int height, int alpha) {
    boxRGBA(renderer, 0, 0, width, height, 0, 0, 0, alpha);
}

} // namespace MeuProjeto
