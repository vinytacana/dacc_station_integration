# 📋 TODO 03: Hierarquia Tipográfica (Hero Text)

**Objetivo:** Exibir o título e informações do jogo em destaque na área superior ("Hero Area"), usando as fontes carregadas.

**Arquivos Afetados:**
- `src/JanelaJogo.cpp`
- `src/GerenciadorFontes.cpp` (se necessário carregar tamanhos novos)

**Especificações Técnicas:**

1.  **Carregamento de Fontes:**
    Garanta que o `GerenciadorFontes` carregou:
    - `Garet-Heavy.ttf` tamanho **64** ou **72** (Para Títulos).
    - `Garet-Book.ttf` tamanho **24** ou **28** (Para Metadados).

2.  **Posicionamento (Coordenadas Absolutas):**
    *   **Título:** X = 100, Y = 150.
    *   **Metadados:** X = 100, Y = 250 (Abaixo do título).

3.  **Renderização de Texto (SDL_ttf):**
    Use uma cor **Branco Puro ({255, 255, 255})** para o título.
    Use uma cor **Cinza ({200, 200, 200})** para os metadados (Gênero, Ano, etc).

4.  **Tratamento de Dados Faltantes:**
    Se o objeto `Jogo` não tiver descrição ou gênero preenchidos (lidos do `.txt`), não desenhe nada ou desenhe uma string padrão ("Informações indisponíveis"). Evite crashes por string nula.

**Critério de Sucesso:**
O nome do jogo selecionado aparece enorme e nítido no canto superior esquerdo. O texto "salta" da tela devido ao contraste com o fundo escurecido (feito no TODO 02).
