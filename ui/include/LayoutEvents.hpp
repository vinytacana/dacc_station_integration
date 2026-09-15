#ifndef LAYOUT_EVENTS_HPP
#define LAYOUT_EVENTS_HPP

#include <SDL2/SDL.h>

namespace MeuProjeto {

inline bool eventoAtualizaLayout(const SDL_Event& evento, Uint32 windowId) {
    if (evento.type != SDL_WINDOWEVENT || evento.window.windowID != windowId) {
        return false;
    }
    if (evento.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
        evento.window.event == SDL_WINDOWEVENT_RESIZED) {
        return true;
    }
#if SDL_VERSION_ATLEAST(2, 0, 18)
    return evento.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED;
#else
    return false;
#endif
}

} // namespace MeuProjeto

#endif
