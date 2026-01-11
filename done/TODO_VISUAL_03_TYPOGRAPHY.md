# TODO VISUAL 03: TIPOGRAFIA E METADADOS

## OBJETIVO
Melhorar a legibilidade e a hierarquia visual dos textos. Adicionar ícones ou separadores visuais para os metadados (Gênero, Ano, Players) para que não pareçam apenas texto solto.

## REFERÊNCIA TÉCNICA
- **Arquivo Alvo:** `src/JanelaJogo.cpp`
- **Método:** `desenharDetalhesJogo()`
- **Fonts:** `Garet-Heavy.ttf` (Títulos), `Garet-Book.ttf` (Corpo).

## ESPECIFICAÇÃO DE IMPLEMENTAÇÃO

### 1. Hierarquia de Fontes
Definir tamanhos claros no `GerenciadorFontes`:
- **Título:** 64pt ou 72pt (Garet-Heavy). Cor: Branco (255, 255, 255). Sombra leve (drop shadow) para contraste.
- **Subtítulo/Metadados:** 24pt (Garet-Book). Cor: Cinza Claro (200, 200, 200). UPPERCASE.
- **Descrição:** 18pt ou 20pt (Garet-Book). Cor: Cinza Médio (170, 170, 170). Quebra de linha automática (wrap).

### 2. Formatação de Metadados
Em vez de apenas imprimir "Aventura, RPG", estruturar visualmente.
- **Separador:** Usar caractere " • " ou " | " com cor mais escura entre os itens.
  - Ex: "RPG  •  2023  •  1 Jogador"
- **Ícones (Opcional/Futuro):** Desenhar pequenos ícones (relógio, controle, calendário) antes de cada dado. Por enquanto, usar texto bem espaçado.

### 3. Tratamento da Descrição
- Limitar a largura do texto da descrição para ~40% da tela (não deixar esticar até a direita toda).
- Adicionar "..." se o texto for muito longo (truncagem manual ou renderizar apenas as primeiras 3 linhas).

### 4. Logo vs Texto (Feature)
Verificar se existe um arquivo `assets/logos/{game_id}.png`.
- **Se existir:** Desenhar a logo em vez do texto do título.
- **Se não existir:** Desenhar o texto (fallback atual).

## PASSO A PASSO
1. [ ] Ajustar cores e tamanhos em `JanelaJogo::desenharDetalhesJogo`.
2. [ ] Implementar concatenação de metadados com separadores (" • ").
3. [ ] Definir `SDL_Rect` de limite para o texto da descrição para forçar quebra de linha (`TTF_RenderText_Blended_Wrapped`).
4. [ ] (Opcional) Implementar verificação de existência de logo PNG.
