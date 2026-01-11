# TODO VISUAL 06: GAME LOGOS

## OBJETIVO
Implementar a exibição de logotipos (imagens PNG com transparência) no lugar do texto simples do título do jogo, quando disponível. Isso aproxima a interface da experiência de consoles modernos.

## REFERÊNCIA TÉCNICA
- **Arquivo Alvo:** `ui/src/JanelaJogo.cpp`
- **Método:** `desenharInfo()`

## ESPECIFICAÇÃO
1. **Sistema de Arquivos:**
   - Verificar existência de arquivo: `ui/assets/logos/{game_id}.png`.

2. **Lógica de Renderização:**
   - **Se existir logo:**
     - Carregar textura (cachear via `GerenciadorImagens` se possível).
     - Renderizar na posição do Título, mantendo a proporção (aspect ratio).
     - Altura máxima: ~120px. Largura máxima: ~400px.
   - **Se NÃO existir logo:**
     - Manter o comportamento atual (Renderizar texto com fonte `Garet-Heavy` 72pt).

3. **Fallback:**
   - Se o carregamento da imagem falhar, fallback imediato para texto.
