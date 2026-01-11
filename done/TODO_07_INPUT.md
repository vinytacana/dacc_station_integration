# 📋 TODO 07: Input Mapping (Navegação Horizontal)

**Objetivo:** Ajustar a lógica de entrada para corresponder ao novo layout linear.

**Arquivos Afetados:**
- `src/JanelaJogo.cpp` (Método `processarEventos` ou similar)

**Especificações Técnicas:**

1.  **Mapeamento Direcional:**
    *   **Esquerda (SDLK_LEFT):** `indiceSelecionado--`
    *   **Direita (SDLK_RIGHT):** `indiceSelecionado++`

2.  **Mapeamento Vertical (Ações):**
    *   **Cima (SDLK_UP):** Foco no botão "Jogar" (se houver) ou Ações do Sistema.
    *   **Baixo (SDLK_DOWN):** Retornar foco ao carrossel.

3.  **Bounds Checking (Circular):**
    Implementar navegação infinita:
    ```cpp
    if (indiceSelecionado < 0) indiceSelecionado = totalJogos - 1;
    if (indiceSelecionado >= totalJogos) indiceSelecionado = 0;
    ```

4.  **Botão de Ação:**
    *   **Enter / Joystick Button 0:** Acionar `GerenciadorJogos::executarJogo()`.

**Critério de Sucesso:**
Pressionar Esquerda/Direita navega suavemente pela lista. O loop funciona (ir para a direita no último item volta para o primeiro).
