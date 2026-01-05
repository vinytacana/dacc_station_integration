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

    // Obtém dimensões originais da textura
    int texW, texH;
    if (SDL_QueryTexture(texture, NULL, NULL, &texW, &texH) != 0) return;

    float texAspect = static_cast<float>(texW) / static_cast<float>(texH);
    float targetAspect = static_cast<float>(targetRect.w) / static_cast<float>(targetRect.h);

    SDL_Rect srcRect;

    if (texAspect > targetAspect) {
        // Textura é mais larga que a área de destino (Letterbox horizontal -> Crop laterais)
        srcRect.h = texH;
        srcRect.w = static_cast<int>(texH * targetAspect);
        srcRect.x = (texW - srcRect.w) / 2;
        srcRect.y = 0;
    } else {
        // Textura é mais alta que a área de destino (Crop topo/fundo)
        srcRect.w = texW;
        srcRect.h = static_cast<int>(texW / targetAspect);
        srcRect.x = 0;
        srcRect.y = (texH - srcRect.h) / 2;
    }

    SDL_SetTextureAlphaMod(texture, alpha);
    SDL_RenderCopy(renderer, texture, &srcRect, &targetRect);
    SDL_SetTextureAlphaMod(texture, 255); // Reseta para não afetar outros draws
}

void GraphicsUtils::drawOverlay(SDL_Renderer* renderer, int width, int height, int alpha) {
    // Garante que o blend mode está ativo para transparência
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    boxRGBA(renderer, 0, 0, width, height, 0, 0, 0, alpha);
}

} // namespace MeuProjeto