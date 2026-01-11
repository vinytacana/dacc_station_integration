# 📋 TODO 02: Pipeline de Atmosfera (Fundo Dinâmico & Transições)

**Objetivo:** Implementar o fundo imersivo com transição suave (Fade In/Out) para evitar cortes bruscos.

**Arquivos Afetados:**
- `src/JanelaJogo.cpp`

**Especificações Técnicas:**

1.  **Estado de Transição:**
    Adicione variáveis à classe `JanelaJogo`:
    - `SDL_Texture* bgAtual;`
    - `SDL_Texture* bgProximo;`
    - `float transitionAlpha;` (0.0 a 255.0)

2.  **Lógica no Update():**
    Quando `indiceSelecionado` mudar:
    - Mova `bgAtual` para `bgProximo` (ou descarte o antigo).
    - Carregue o novo em `bgProximo`.
    - Resete `transitionAlpha = 0`.

    A cada frame:
    - Se `transitionAlpha < 255`, incremente (ex: `+= 5.0 * deltaTime`).

3.  **Lógica no Render():**
    - Desenhe `bgAtual` (O antigo) com Alpha total (ou decrescente).
    - Desenhe `bgProximo` (O novo) por cima com `SDL_SetTextureAlphaMod(bgProximo, (int)transitionAlpha)`.
    - *Nota:* Se for custoso manter duas texturas full-HD, use apenas uma e faça fade-to-black e black-to-new.

4.  **Overlay Escuro (Mantido):**
    Sempre desenhe o retângulo preto (Alpha ~200) por cima de todas as texturas de fundo para garantir contraste do texto.

**Critério de Sucesso:**
Ao trocar de jogo rapidamente, o fundo não "pisca". A transição é suave.