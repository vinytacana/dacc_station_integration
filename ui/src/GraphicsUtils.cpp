#include "GraphicsUtils.hpp"
#include "SystemStatus.hpp" // Adicionado TODO 08
#include <ctime>
#include <iomanip>
#include <sstream>

namespace MeuProjeto {
// ... (keep drawRoundedRect and others)

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
    
    // Desenhar apenas na metade inferior para garantir leitura do texto e carrossel
    // sem escurecer a arte principal no topo.
    int startY = height / 2;
    boxRGBA(renderer, 0, startY, width, height, 0, 0, 0, alpha);
    
    // Opcional: Uma faixa mais suave logo acima para transição (simulando gradiente)
    boxRGBA(renderer, 0, startY - 50, width, startY, 0, 0, 0, alpha / 2);
}

void GraphicsUtils::drawSystemTopBar(SDL_Renderer* renderer, int screenWidth, TTF_Font* font) {
    // 1. Obter dados do SystemStatus (TODO 05/08)
    SystemData data = SystemStatus::getInstance().getCachedData();
    
    if (!font) return;

    SDL_Color white = {220, 220, 220, 255};
    int rightMargin = 100; // Increased margin for Avatar
    int topMargin = 25;

    // --- AVATAR (Far Right) ---
    int avatarRadius = 24; // Slightly larger
    int avatarX = screenWidth - 50;
    int avatarY = topMargin + 10;
    
    // Border (White, thicker)
    aacircleRGBA(renderer, avatarX, avatarY, avatarRadius + 2, 255, 255, 255, 255);
    aacircleRGBA(renderer, avatarX, avatarY, avatarRadius + 1, 255, 255, 255, 255);
    
    // Fill (Dark Blue/Grey placeholder)
    filledCircleRGBA(renderer, avatarX, avatarY, avatarRadius, 40, 45, 60, 255);
    
    // Initial "U"
    if(font) {
         SDL_Color uCol = {200, 200, 200, 255};
         SDL_Surface* sU = TTF_RenderUTF8_Blended(font, "U", uCol);
         if(sU) {
             SDL_Texture* tU = SDL_CreateTextureFromSurface(renderer, sU);
             SDL_Rect rU = {avatarX - sU->w/2, avatarY - sU->h/2, sU->w, sU->h};
             SDL_RenderCopy(renderer, tU, NULL, &rU);
             SDL_FreeSurface(sU);
             SDL_DestroyTexture(tU);
         }
    }

    // --- CLOCK ---
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, data.currentTime.c_str(), white);
    if(surf) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
        // Position left of Avatar (AvatarX - Radius - Padding - TextWidth)
        int clockX = avatarX - avatarRadius - 30 - surf->w;
        
        SDL_Rect dst = {clockX, topMargin, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &dst);
        
        int clockW = surf->w;
        SDL_FreeSurface(surf);
        SDL_DestroyTexture(tex);

        // --- BATTERY (Left of Clock) ---
        int batW = 40;
        int batH = 20;
        int batX = clockX - 50; // 50px gap from clock
        int batY = topMargin + 5; 

        // Outline
        roundedRectangleRGBA(renderer, batX, batY, batX + batW, batY + batH, 4, 200, 200, 200, 255);
        // Tip
        boxRGBA(renderer, batX + batW, batY + 6, batX + batW + 4, batY + batH - 6, 200, 200, 200, 255);
        // Fill (Dynamic)
        int fillW = (int)(batW * (data.batteryLevel / 100.0f));
        if(fillW > batW - 6) fillW = batW - 6;
        if(fillW < 0) fillW = 0;
        
        boxRGBA(renderer, batX + 3, batY + 3, batX + 3 + fillW, batY + batH - 3, 255, 255, 255, 255);

        // --- WIFI (Left of Battery) ---
        if (data.wifiConnected) {
            int wifiX = batX - 40;
            int wifiY = batY + 18; // Base of the arc
            
            // Simple Arcs
            for(int r = 4; r <= 16; r += 6) {
                 arcRGBA(renderer, wifiX, wifiY, r, 225, 315, 255, 255, 255, 255);
                 if(r > 4) arcRGBA(renderer, wifiX, wifiY, r-1, 225, 315, 255, 255, 255, 255); // Thickness
            }
            filledCircleRGBA(renderer, wifiX, wifiY, 2, 255, 255, 255, 255);
        }
    }
}
void GraphicsUtils::drawBottomNavHints(SDL_Renderer* renderer, int screenWidth, int screenHeight, TTF_Font* font) {
    if (!font) return;

    int barHeight = 70;
    int yStart = screenHeight - barHeight;

    // Gradient Background (Dark at bottom)
    // boxRGBA is flat. Let's just do a dark rect.
    boxRGBA(renderer, 0, yStart, screenWidth, screenHeight, 20, 23, 26, 240); // Steam Dark Blue/Grey

    // Helper Lambda for Prompt
    auto drawPrompt = [&](int& xRight, const std::string& key, const std::string& action) {
        // 1. Text
        SDL_Color textCol = {200, 200, 200, 255};
        SDL_Surface* sText = TTF_RenderUTF8_Blended(font, action.c_str(), textCol);
        if(!sText) return;
        
        xRight -= (sText->w + 30); // Move cursor left
        
        SDL_Texture* tText = SDL_CreateTextureFromSurface(renderer, sText);
        SDL_Rect rText = {xRight, yStart + (barHeight - sText->h)/2, sText->w, sText->h};
        SDL_RenderCopy(renderer, tText, NULL, &rText);
        
        // 2. Button Circle
        int btnSize = 32;
        int btnX = xRight - btnSize - 10;
        int btnY = yStart + (barHeight - btnSize)/2;

        filledCircleRGBA(renderer, btnX + btnSize/2, btnY + btnSize/2, btnSize/2, 255, 255, 255, 255);
        
        // 3. Key Letter
        SDL_Color keyCol = {20, 20, 20, 255};
        SDL_Surface* sKey = TTF_RenderUTF8_Blended(font, key.c_str(), keyCol);
        if(sKey) {
            SDL_Texture* tKey = SDL_CreateTextureFromSurface(renderer, sKey);
            SDL_Rect rKey = {
                btnX + (btnSize - sKey->w)/2,
                btnY + (btnSize - sKey->h)/2 - 1, // Slight visual adjustment
                sKey->w, sKey->h
            };
            SDL_RenderCopy(renderer, tKey, NULL, &rKey);
            SDL_FreeSurface(sKey);
            SDL_DestroyTexture(tKey);
        }

        SDL_FreeSurface(sText);
        SDL_DestroyTexture(tText);
        
        // Update Cursor
        xRight = btnX; 
    };

    // Draw Right-Aligned Prompts
    int cursorX = screenWidth - 40;
    drawPrompt(cursorX, "B", "VOLTAR");
    drawPrompt(cursorX, "A", "SELECIONAR");
    
    // Left-Aligned "STEAM MENU"
    int leftX = 40;
    int btnSize = 80; 
    // Just text for "STEAM MENU" or "OPTIONS"
    SDL_Color menuCol = {150, 150, 150, 255};
    SDL_Surface* sMenu = TTF_RenderUTF8_Blended(font, "OPÇÕES", menuCol);
    if(sMenu) {
        // Draw Hamburger Icon or similar? Just text for now.
        // Circle with "="
        int cX = leftX + 16;
        int cY = yStart + barHeight/2;
        filledCircleRGBA(renderer, cX, cY, 16, 60, 60, 60, 255);
        lineRGBA(renderer, cX-8, cY-4, cX+8, cY-4, 255, 255, 255, 255);
        lineRGBA(renderer, cX-8, cY, cX+8, cY, 255, 255, 255, 255);
        lineRGBA(renderer, cX-8, cY+4, cX+8, cY+4, 255, 255, 255, 255);

        SDL_Texture* tMenu = SDL_CreateTextureFromSurface(renderer, sMenu);
        SDL_Rect rMenu = {leftX + 45, yStart + (barHeight - sMenu->h)/2, sMenu->w, sMenu->h};
        SDL_RenderCopy(renderer, tMenu, NULL, &rMenu);
        
        SDL_FreeSurface(sMenu);
        SDL_DestroyTexture(tMenu);
    }
}

} // namespace MeuProjeto
