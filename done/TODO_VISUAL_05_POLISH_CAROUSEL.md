# TODO VISUAL 05: CAROUSEL GLOW & POLISH

## OBJETIVO
Melhorar o acabamento visual do item selecionado no carrossel. Atualmente, usa-se linhas grossas (`SDL_RenderDrawRect`) que criam um visual "serrilhado". O objetivo é implementar um brilho suave (Glow) e bordas arredondadas reais.

## REFERÊNCIA TÉCNICA
- **Arquivo Alvo:** `ui/src/JanelaJogo.cpp`
- **Método:** `desenharCarrossel()`

## ESPECIFICAÇÃO
1. **Soft Glow:**
   - Substituir o loop de `SDL_RenderDrawRect` por `roundedBoxRGBA` (SDL2_gfx) desenhado *atrás* da capa.
   - Usar Alpha baixo (ex: 100-150) e um tamanho levemente maior que a capa (ex: +10px em cada lado) para simular brilho difuso.
   - Cor: Branco ou Azul Claro (Theme Accent).

2. **Bordas Arredondadas na Capa (Máscara):**
   - (Opcional) As capas atualmente são retângulos retos. Investigar uso de máscara ou renderização com `roundedBoxRGBA` texturizado se possível, ou apenas aceitar o retângulo mas garantir que o Glow siga o formato.

3. **Animação de Pulso:**
   - Fazer o Alpha do Glow oscilar levemente (ex: de 100 a 180) usando uma função seno baseada no tempo (`SDL_GetTicks()`).
