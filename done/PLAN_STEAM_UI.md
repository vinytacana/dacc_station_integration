# 🗺️ Plano de Refatoração de UI: "Project Hero"
**Objetivo:** Transição do layout "Grid de Arquivos" para "Interface de Console (Hero/Carousel)"
**Target Hardware:** Raspberry Pi 4 (SDL2 / GPU VideoCore VI)
**Data:** Janeiro 2026

---

## 1. Visão Arquitetural
O objetivo é abandonar a renderização de grade estática ($O(N)$ em tela) para um modelo focado no conteúdo selecionado (**Hero Layout**). A renderização deve seguir o "Algoritmo do Pintor" estrito para garantir a profundidade correta sem uso excessivo de *framebuffers*.

### Divisão de Viewport (1920x1080)
A tela será logicamente dividida em camadas (Layers):

1.  **Layer 0 (Background):** Wallpaper do jogo selecionado (Full HD, Dimmed).
2.  **Layer 1 (Hero Content):** Metadados, Logo e Título (Top-Left, ~70% da altura).
3.  **Layer 2 (Navigation):** Carrossel de Capas (Bottom-Strip, ~30% da altura).
4.  **Layer 3 (Overlay):** HUD de Sistema (Relógio, Status) e Prompts de Botão.

---

## 2. Detalhamento Técnico das Tarefas

### 🎨 Pilar 1: Atmosfera e Renderização (Pipeline Gráfico)
**Responsável:** Frontend (João Eduardo)

Para criar imersão sem custo computacional de shaders de blur em tempo real, utilizaremos **Alpha Blending** em camadas.

#### Pipeline de Renderização (Pseudo-código SDL2)
```cpp
void RenderFrame() {
    // 1. Limpar Tela
    SDL_RenderClear(renderer);

    // 2. Fundo Atmosférico (Otimizado)
    // Recupera textura do jogo focado atualmente (State Pattern)
    SDL_Texture* bgTexture = currentState->getSelectedGame()->getWallpaper();
    
    if (bgTexture) {
        // Desenha esticado para preencher 1920x1080
        SDL_RenderCopy(renderer, bgTexture, NULL, NULL);
    }

    // 3. Vignette/Dimmer (Essencial para leitura de texto)
    // Desenha um retângulo preto com Alpha sobre o fundo
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 15, 15, 20, 210); // Quase opaco (210/255)
    SDL_RenderFillRect(renderer, NULL); // Tela inteira

    // 4. Desenhar Carrossel e Elementos
    RenderCarousel();
    RenderHeroText();
    
    SDL_RenderPresent(renderer);
}
```

---

### 🏛️ Pilar 2: Layout "Hero" e Tipografia
**Responsável:** Frontend (João Eduardo)

A tipografia deve guiar o olho do usuário.

1.  **Título (Hero):**
    *   **Fonte:** `assets/fonts/Garet-Heavy.ttf`
    *   **Tamanho:** 64pt ou 72pt.
    *   **Posição:** Margem esquerda (100px), Topo (150px).
    *   **Cor:** Branco Puro ({255,255,255}).

2.  **Metadados (Sinopse/Gênero):**
    *   **Fonte:** `assets/fonts/Garet-Book.ttf`
    *   **Tamanho:** 28pt.
    *   **Posição:** Abaixo do título.
    *   **Cor:** Cinza Claro ({200,200,200}).
    *   **Truncamento:** Limitar a 3 linhas de texto para não poluir.

3.  **Rodapé (Affordance):**
    *   Barra fixa inferior com altura de 60px.
    *   Ícones de botões (A) (B) desenhados como sprites ou glifos de fonte.
    *   Texto: "Selecionar" e "Voltar".

---

### ↔️ Pilar 3: Navegação (O Carrossel)
**Responsável:** Frontend (João Eduardo)

Abandonar a lógica de matriz `[row][col]`. Adotar lógica de vetor linear `[index]`.

#### Lógica Visual
*   **Posicionamento Y:** Fixo na parte inferior (ex: Y=750).
*   **Item Selecionado:** Deve ter escala maior (1.1x) ou brilho/borda.
*   **Itens Inativos:** Devem ter opacidade reduzida (Alpha 150).

#### Implementação de Borda de Seleção (Focus Ring)
Em vez de desenhar retângulos, usar `GraphicsUtils::drawRoundedRectOutline` com espessura de 3px a 5px na cor de acento (Azul ou Branco) ao redor da capa selecionada.

---

## 3. Requisitos de Integração (Cross-Team)

### 📂 Infraestrutura de Arquivos (João Henrique)
Para suportar o layout rico, a estrutura de diretórios do jogo deve evoluir.
**Estrutura Proposta:**
```text
assets/data/
└── games/
    ├── jogo_id_01/
    │   ├── meta.json       (Nome, Descrição, Executável)
    │   ├── cover.jpg       (Vertical 600x900 - Para o Carrossel)
    │   └── wallpaper.jpg   (Horizontal 1920x1080 - Para o Fundo)
```
*   **Fallback:** Se `wallpaper.jpg` não existir, o sistema deve usar a `cover.jpg` esticada com um blur pré-processado ou apenas escurecida.

### ⚙️ Backend & IPC (Feliph Lima)
*   **Non-Blocking Loading:** O carregamento das texturas de alta resolução (Wallpaper) não pode travar o frame da UI.
*   **Sugestão:** Implementar um cache LRU (Least Recently Used) no `GerenciadorImagens` que mantém apenas os 3 wallpapers vizinhos carregados na VRAM, liberando os distantes.

### 🔋 System Status (Vinícius Tacaná)
*   **Mockups Iniciais:** Implementar funções "dummy" que retornam strings estáticas para a UI renderizar no topo direito.
    *   `System::getTime()` -> retorna "12:00"
    *   `System::getWifiStatus()` -> retorna true
*   Isso permite que o Frontend desenhe o layout final sem depender da implementação real do hardware agora.

---

## 4. Plano de Execução Imediata (Checklist)

- [ ] **Setup:** Confirmar que `assets/fonts` contém a família Garet.
- [ ] **Refatoração `JanelaJogo`:** Alterar `desenhar()` para usar o pipeline de camadas (Fundo -> Overlay -> Conteúdo).
- [ ] **Refatoração `main`:** Substituir o loop de grid duplo `for(x) for(y)` por um loop linear único `for(i)` no eixo X.
- [ ] **Polimento:** Ajustar valores Alpha do overlay para garantir legibilidade perfeita do texto branco.
