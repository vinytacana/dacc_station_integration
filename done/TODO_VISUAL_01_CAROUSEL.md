# TODO VISUAL 01: CARROSSEL E FOCO (ZOOM & GLOW)

## OBJETIVO
Implementar um feedback visual rico quando um jogo é selecionado no carrossel. O item focado deve aumentar de tamanho (Scale Up) e ter uma borda de destaque, criando profundidade em relação aos itens não selecionados.

## REFERÊNCIA TÉCNICA
- **Arquivo Alvo:** `src/JanelaJogo.cpp`
- **Método:** `desenharCarrossel()`

## ESPECIFICAÇÃO DE IMPLEMENTAÇÃO

### 1. Lógica de Renderização (Z-Index)
Atualmente, o loop desenha os jogos sequencialmente (0, 1, 2...). Isso causa um problema: se o item 1 crescer, ele será desenhado *atrás* do item 2.
**Solução:**
1. Desenhar todos os jogos **exceto** o selecionado no loop normal.
2. Desenhar o jogo selecionado **por último** (após o loop), para garantir que ele fique no topo (Z-Index maior).

### 2. Dimensões Dinâmicas
Defina constantes para tamanhos normais e expandidos.

```cpp
const int CARD_W_NORMAL = 200;
const int CARD_H_NORMAL = 300;
const int CARD_W_FOCUS = 230;  // 15% maior
const int CARD_H_FOCUS = 345;
const int CARD_SPACING = 20;   // Aumentar espaçamento base
```

### 3. Cálculo de Posição (Centralização)
Quando o card cresce, ele deve crescer a partir do centro, não do canto superior esquerdo.
- `x_draw = x_base - (w_focus - w_normal) / 2`
- `y_draw = y_base - (h_focus - h_normal) / 2` (Ele deve "subir" um pouco)

### 4. Estilo da Borda (Glow)
Substituir o `SDL_RenderDrawRect` simples.
- **Opção A (Simples):** Usar `roundedBoxRGBA` (SDL2_gfx) para desenhar um retângulo branco sólido arredondado *atrás* da textura da capa, levemente maior (border thickness).
- **Opção B (Textura):** Carregar uma imagem `assets/ui/selection_glow.png` (um quadrado branco com brilho difuso) e desenhá-la esticada atrás da capa selecionada.

### 5. Dimming (Escurecimento)
Os itens não selecionados devem ser desenhados com `SDL_SetTextureColorMod(tex, 150, 150, 150)` para ficarem mais escuros. O item selecionado deve ter `(255, 255, 255)`.

## PASSO A PASSO
1. [ ] Alterar loop em `desenharCarrossel` para pular o `indiceSelecionado`.
2. [ ] Implementar lógica de `ColorMod` (escurecer não selecionados).
3. [ ] Desenhar o `indiceSelecionado` isoladamente no final da função.
4. [ ] Aplicar lógica de escala e recentralização no item focado.
5. [ ] Adicionar borda arredondada ou glow atrás do item focado.
