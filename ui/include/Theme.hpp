#ifndef THEME_HPP
#define THEME_HPP

#include <SDL2/SDL.h>

namespace MeuProjeto {

/**
 * @brief Centralizes all visual design tokens (colors, sizes, spacing).
 * Allows for easy "theming" and consistent aesthetics across the application.
 */
struct Theme {
    // --- Palette: Backgrounds ---
    static constexpr SDL_Color BG_PRIMARY   = {30, 30, 35, 255};   // Deep Grey (Main App BG)
    static constexpr SDL_Color BG_PANEL     = {40, 42, 48, 230};   // Lighter Grey (Panels/Cards) with transparency
    static constexpr SDL_Color BG_OVERLAY   = {10, 10, 12, 220};   // Almost Black (For dimming backgrounds)

    // --- Palette: Accents & States ---
    static constexpr SDL_Color ACCENT_COLOR = {0, 122, 204, 255};  // Professional Blue (Focus)
    static constexpr SDL_Color DANGER_COLOR = {220, 50, 50, 255};  // Red (Close/Delete)
    
    // --- Palette: Buttons ---
    static constexpr SDL_Color BTN_NORMAL   = {55, 55, 60, 255};
    static constexpr SDL_Color BTN_HOVER    = {75, 75, 85, 255};
    static constexpr SDL_Color BTN_ACTIVE   = {90, 90, 100, 255};

    // --- Palette: Text ---
    static constexpr SDL_Color TEXT_PRIMARY = {240, 240, 245, 255}; // Off-White
    static constexpr SDL_Color TEXT_SECONDARY= {160, 160, 170, 255}; // Grey Text

    // --- Geometry ---
    static constexpr int RADIUS_SMALL = 5;
    static constexpr int RADIUS_MEDIUM = 10;
    static constexpr int RADIUS_LARGE = 20;

    static constexpr int PADDING_SMALL = 10;
    static constexpr int PADDING_NORMAL = 20;
};

} // namespace MeuProjeto

#endif // THEME_HPP
