# 🗺️ Plano Mestre de Implementação: Interface "Hero" (DACC Station)

**Versão:** 1.0 (Consolidada)
**Objetivo:** Transformar o menu principal em uma interface imersiva estilo console ("Steam Big Picture"), onde o jogo selecionado domina a tela ("Hero") e a navegação ocorre via carrossel horizontal.
**Contexto Técnico:** C++17, SDL2, Raspberry Pi 4 (GPU VideoCore VI).

---

## 1. Arquitetura e Escopo

**Mudança Fundamental:**
Ao contrário da arquitetura anterior baseada em grid, o novo modelo centraliza o estado no **Loop Principal (`main.cpp`)**. A classe `JanelaJogo` será reservada apenas para a visualização detalhada pós-seleção.

### Gerenciamento de Estado
Substituir vetores dispersos de botões por um controle de índice centralizado:

```cpp
struct DashboardState {
    int selectedIndex = 0;          // Índice do jogo focado (0 a N-1)
    float carouselScrollX = 0.0f;   // Posição atual do scroll (para animação suave)
    float carouselTargetX = 0.0f;   // Posição alvo do scroll
    
    // Cache de Texturas (LRU simples)
    // Mantém na VRAM apenas: [Selected-1], [Selected], [Selected+1] e backgrounds recentes
};
```

---

## 2. Pipeline de Renderização (Z-Order Layers)

O loop de renderização (`Update` + `Draw`) deve seguir estritamente a ordem abaixo ("Algoritmo do Pintor") para garantir a profundidade correta sem uso custoso de Z-Buffers.

### 🎨 Layer 0: Atmosfera (Background)
O fundo define o "clima" do jogo selecionado.
*   **Fonte de Dados:** `Jogo::getCapa()` (ou `wallpaper.jpg` se disponível).
*   **Técnica:**
    *   `SDL_RenderCopy` ocupando 100% da tela (1920x1080).
    *   **Otimização Pi4:** Evitar shaders de blur em tempo real. Usar **Alpha Blending**.

### 🌑 Layer 1: Overlay (Dimmer/Vignette)
Essencial para garantir a legibilidade do texto branco sobre qualquer imagem de fundo.
*   **Técnica:** Desenhar um retângulo preto (`0,0,0`) cobrindo a tela inteira.
*   **Opacidade:** Alpha entre **180 e 220** (ajustável).
    ```cpp
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, NULL);
    ```

### 🦸 Layer 2: Hero Content (Metadados)
Informações textuais do jogo focado.
*   **Posição:** Quadrante Superior Esquerdo (Margem: 100px, Topo: 150px).
*   **Tipografia:**
    *   **Título:** `Garet-Heavy.ttf` (64pt, Branco Puro).
    *   **Info:** `Garet-Book.ttf` (24pt, Cinza `200,200,200`). Ex: "Ação | RPG".
    *   **Sinopse:** Texto com quebra de linha automática (max-width: 800px).
*   **Ação:** Ícone/Texto "Ⓐ JOGAR" logo abaixo da sinopse.

### 🎠 Layer 3: O Carrossel (Navegação)
Lista horizontal de "cartuchos" ou "capas" na parte inferior.
*   **Posição:** Rodapé (Y fixo: ~750px).
*   **Layout:**
    *   Desenhar apenas jogos visíveis: `[Selected - 2]` até `[Selected + 5]`.
    *   **Tamanho do Card:** ~300x400px (Portrait) ou ~300x170px (Landscape).
*   **Feedback Visual (Focus):**
    *   O item `selectedIndex` recebe uma borda branca espessa (3px a 5px).
    *   Pode ter um leve aumento de escala (1.1x).
    *   Itens não selecionados podem ter opacidade reduzida (Alpha 150).

---

## 3. Lógica de Input (Navegação)

A prioridade muda de "Mouse Hover" para "Controle Direcional" (D-Pad/Teclado), padrão de consoles.

**Mapeamento de Eventos (`SDL_KEYDOWN`):**
*   **Esquerda (`SDLK_LEFT`):** `selectedIndex--` (Circular ou Clamp em 0).
*   **Direita (`SDLK_RIGHT`):** `selectedIndex++` (Circular ou Clamp em Size-1).
*   **Confirmar (`SDLK_RETURN` / `SDLK_SPACE`):**
    *   Ação: Iniciar Jogo ou Abrir Detalhes (`JanelaJogo`).
*   **Cancelar (`SDLK_ESCAPE`):** Abrir Menu de Configurações ou Sair.

---

## 4. Integração entre Equipes (Contratos)

Para o sucesso da UI, os módulos adjacentes devem fornecer suporte específico:

### 📂 Filesystem & Infra (João Henrique)
Padronização da estrutura de assets para suportar a UI rica.
*   **Requisito:** `Jogo::getCapa()` deve retornar um caminho válido.
*   **Estrutura Ideal:**
    ```text
    assets/data/games/jogo_id/
    ├── cover.jpg       (300x400 - Carrossel)
    ├── wallpaper.jpg   (1920x1080 - Fundo/Hero)
    └── meta.json       (Dados)
    ```
*   **Fallback:** Se a imagem falhar, carregar `assets/textures/missing_cover.png`.

### ⚙️ Backend & Performance (Feliph Lima)
*   **Gerenciamento de Memória:** Carregar texturas 1080p a cada frame pode causar "stutter".
*   **Solução:** O `GerenciadorImagens` deve implementar um cache inteligente (LRU) para manter as texturas dos jogos vizinhos carregadas, evitando I/O de disco durante a navegação rápida.

### 🔋 System Status (Vinícius Tacaná)
*   Fornecer API (mesmo que mockada inicialmente) para status no topo direito.
    *   `System::getBatteryLevel()`
    *   `System::isWifiConnected()`

---

## 5. Implementação de Referência (Snippet)

Pseudo-código para o loop de renderização em `main.cpp`:

```cpp
// Variáveis Globais de Estado
int g_SelectedIndex = 0;
// ...

while (rodando) {
    // 1. INPUT
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_RIGHT) g_SelectedIndex++;
            if (e.key.keysym.sym == SDLK_LEFT) g_SelectedIndex--;
            // Validar limites...
        }
    }

    // 2. PREPARAR DADOS
    Jogo* jogoAtual = gerenciadorJogos.obter(g_SelectedIndex);
    SDL_Texture* bgTex = gerImg.carregar(renderer, jogoAtual->getCapa());

    // 3. RENDERIZAR (Z-ORDER)
    SDL_RenderClear(renderer);

    // [Layer 0] Background Hero
    SDL_RenderCopy(renderer, bgTex, NULL, NULL);

    // [Layer 1] Dimmer (Vidro Fumê)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 210); // Escuro e transparente
    SDL_Rect fullScreen = {0, 0, 1920, 1080};
    SDL_RenderFillRect(renderer, &fullScreen);

    // [Layer 2] Texto Hero
    desenharTexto(renderer, jogoAtual->getNome(), 100, 150, BRANCO, 72, NEGRITO);
    desenharTexto(renderer, jogoAtual->getDescricao(), 100, 250, CINZA, 28, NORMAL);
    desenharTexto(renderer, "PRESSIONE [ENTER] PARA JOGAR", 100, 450, ACCENT_COLOR, 24, NEGRITO);

    // [Layer 3] Carrossel
    int cardW = 300; 
    int cardH = 400; 
    int gap = 20;
    int startX = 100; // Margem esquerda fixa para o item selecionado? Ou centro?
    
    // Renderiza uma janela de jogos ao redor do selecionado
    for (int i = 0; i < totalJogos; i++) {
        // Lógica de posição relativa ao selecionado
        // Ex: O selecionado sempre fica em X=100. Os outros se deslocam.
        int visualX = 100 + (i - g_SelectedIndex) * (cardW + gap);
        
        // Culling: Só desenha se estiver na tela
        if (visualX + cardW > 0 && visualX < 1920) {
            SDL_Rect r = {visualX, 650, cardW, cardH};
            SDL_RenderCopy(renderer, obterTexturaCapa(i), NULL, &r);

            // Borda de Seleção
            if (i == g_SelectedIndex) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                GraphicsUtils::drawRectOutline(renderer, r, 4); // Borda grossa
            }
        }
    }

    SDL_RenderPresent(renderer);
}
```

## 6. Checklist de Execução Imediata

- [ ] **Limpeza:** Remover lógica antiga de grid (loops `x,y`) do `main.cpp`.
- [ ] **Input:** Implementar controle por teclado/D-Pad.
- [ ] **Visual:** Implementar renderização em camadas (Background -> Dimmer -> Texto -> Carrossel).
- [ ] **Assets:** Confirmar que as fontes `Garet-Heavy` e `Garet-Book` carregam corretamente.
