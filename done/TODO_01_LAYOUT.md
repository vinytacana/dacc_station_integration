# 📋 TODO 01: Reestruturação do Layout (Grid -> Master/Detail)

**Objetivo:** Abandonar o loop de renderização de matriz e implementar um Carrossel Horizontal com lógica de "Câmera Virtual".

**Arquivos Afetados:**
- `src/JanelaJogo.cpp`

**Especificações Técnicas:**

1.  **Definição de Constantes:**
    ```cpp
    const int HERO_SECTION_HEIGHT = 720;
    const int CAROUSEL_Y = 750;
    const int COVER_WIDTH = 200;
    const int COVER_HEIGHT = 300;
    const int SPACING = 20;
    const int SCREEN_WIDTH = 1920;
    ```

2.  **Lógica de Câmera (Offset X):**
    Em vez de desenhar os jogos em posições fixas, calcule a posição baseada no índice selecionado. O objetivo é manter o item selecionado em uma posição agradável (ex: 15% da tela à esquerda ou centralizado).
    
    ```cpp
    // Exemplo de lógica de deslocamento
    int targetX = 100; // Onde queremos que o selecionado apareça
    int cameraOffsetX = targetX - (indiceSelecionado * (COVER_WIDTH + SPACING));
    ```

3.  **Loop de Renderização com Culling (Performance):**
    Não desenhe todos os 1000 jogos se apenas 6 cabem na tela.
    
    ```cpp
    for (int i = 0; i < totalJogos; i++) {
        int drawX = (i * (COVER_WIDTH + SPACING)) + cameraOffsetX;
        
        // Culling: Se estiver fora da tela, pule
        if (drawX + COVER_WIDTH < 0 || drawX > SCREEN_WIDTH) continue;
        
        // Desenhar capa em (drawX, CAROUSEL_Y)...
    }
    ```

**Critério de Sucesso:**
O carrossel desliza suavemente. Jogos fora da tela não consomem ciclos de GPU (`SDL_RenderCopy` não é chamado para eles).