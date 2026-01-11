# 📋 TODO 05: Rodapé e Cabeçalho de Sistema (HUD)

**Objetivo:** Adicionar elementos fixos de interface que persistem independentemente do jogo selecionado (Relógio, Status, Legenda de Botões).

**Arquivos Afetados:**
- `src/JanelaJogo.cpp`

**Especificações Técnicas:**

1.  **Cabeçalho (Top Right):**
    *   Posição: Alinhado à direita (X ~ 1700), Y = 40.
    *   Conteúdo (Mockup): Texto "16:20" (pode usar `std::time` ou hardcoded por enquanto) e ícone de bateria/wifi (se houver assets) ou texto "[WIFI] [BAT]".
    *   Fonte: Pequena (20pt).

2.  **Rodapé (Bottom Bar):**
    *   Desenhar uma faixa preta no fundo da tela (altura ~60px).
    *   Alpha: 230 (quase sólida).

3.  **Legendas de Botões (Affordance):**
    *   Desenhar no canto inferior direito.
    *   Formato: `(Ícone/Letra) Ação`.
    *   Exemplo: `[A] SELECIONAR    [B] VOLTAR`.
    *   Dica: Se não tiver ícones de botões, use texto entre colchetes ou círculos desenhados com `SDL2_gfx`.

**Critério de Sucesso:**
A interface parece um "Sistema Operacional" e não apenas um visualizador de imagens. O usuário sabe quais botões apertar.
