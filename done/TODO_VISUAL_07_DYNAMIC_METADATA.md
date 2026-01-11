# TODO VISUAL 07: DYNAMIC METADATA & BUTTON ICONS

## OBJETIVO
Remover dados hardcoded da interface e adicionar ícones visuais nos botões de ação para melhorar a UX.

## REFERÊNCIA TÉCNICA
- **Arquivo Alvo:** `ui/src/JanelaJogo.cpp`, `ui/include/Jogo.hpp`

## ESPECIFICAÇÃO

### 1. Metadados Dinâmicos
Atualmente a string `"GENERICO • 2024 • 1 JOGADOR"` está hardcoded.
- **Ação:**
  - Adicionar campos na classe `Jogo` (ou parsear do `codigo`/`descricao` se necessário) para: `Ano`, `Players`, `Genero`.
  - Se não houver dados estruturados, tentar extrair do arquivo de texto ou usar defaults inteligentes.
  - Exibir os dados reais do jogo selecionado.

### 2. Ícones nos Botões (Input Hints)
O botão "JOGAR" exibe texto `(Enter)`.
- **Ação:**
  - Desenhar um círculo branco com a letra "A" (ou "Enter" estilizado) *dentro* do botão, à esquerda do texto "JOGAR".
  - Usar `aacircleRGBA` e texto centralizado pequeno.
  - Remover o texto auxiliar `(Enter)` da string principal do botão.
