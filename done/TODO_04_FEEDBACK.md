# 📋 TODO 04: Feedback Visual de Seleção (Focus Ring)

**Objetivo:** Destacar claramente qual item do carrossel está selecionado atualmente, usando bordas e efeitos de escala/brilho.

**Arquivos Afetados:**
- `src/JanelaJogo.cpp`
- `include/GraphicsUtils.hpp` (Utilizar `drawRoundedRectOutline`)

**Especificações Técnicas:**

1.  **Lógica dentro do Loop do Carrossel:**
    Durante a iteração dos jogos para desenhar as capas inferiores:

    ```cpp
    bool isSelected = (i == indiceSelecionado);
    
    if (isSelected) {
        // 1. Desenhar a capa ligeiramente maior (Opcional) ou na posição padrão
        // 2. Desenhar borda de destaque
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Branco
        
        // Desenhar retangulos concêntricos para simular espessura
        for(int w=0; w<4; w++) {
             SDL_Rect outline = {rect.x - w, rect.y - w, rect.w + (w*2), rect.h + (w*2)};
             SDL_RenderDrawRect(renderer, &outline);
        }
    } else {
        // Desenhar capa com Alpha reduzido (Dimmed)
        SDL_SetTextureAlphaMod(texture, 150); 
        SDL_RenderCopy(...)
        SDL_SetTextureAlphaMod(texture, 255); // Resetar para não afetar outros
    }
    ```

2.  **Highlight Color:**
    Pode-se usar Branco ou uma cor temática definida em `Theme.hpp` (ex: `Theme::COLOR_ACCENT`).

**Critério de Sucesso:**
Ao navegar, é óbvio qual jogo está ativo. Os jogos não selecionados parecem estar "mais longe" ou "apagados".
