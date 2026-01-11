# PLANO 01: Padronização e Migração de Estrutura (UI)

**Status:** Rascunho (Revisado)
**Data:** 11 Jan 2026
**Objetivo:** Migrar o código legado de `DACC-Station/` (antigo `ui-dacc`) para `ui/` seguindo a convenção de nomes em inglês e estrutura padrão do projeto, sem alterar o comportamento visual.

---

## 1. Mapeamento de Diretórios e Arquivos

A estrutura atual mistura português/inglês e ativos com código. A nova estrutura separa claramente responsabilidades e **achata** a hierarquia de código (flat structure).

### Código Fonte
| Origem (`DACC-Station/`) | Destino (`ui/`) | Notas |
| :--- | :--- | :--- |
| `cabecalhos/**/*.hpp` | `include/` | Mover **todos** os .hpp dos subdiretórios (`Arquivos`, `Gerenciadores`, `Interface`, `Principais`, `GerenciadoresJogos`) para a raiz de `include/`. |
| `implementacao/**/*.cpp` | `src/` | Mover **todos** os .cpp dos subdiretórios para a raiz de `src/`. |
| `main/main.cpp` | `src/main.cpp` | Ponto de entrada. |

**Arquivos Críticos Faltantes no Rascunho Anterior:**
*   `cabecalhos/Principais/ConfigLayout.hpp` -> `include/ConfigLayout.hpp`
*   `implementacao/Principais/ConfigLayout.cpp` -> `src/ConfigLayout.cpp`
*   `cabecalhos/Gerenciadores/GerenciadorTexturasTexto.hpp` -> `include/GerenciadorTexturasTexto.hpp`
*   `implementacao/Gerenciadores/GerenciadorTexturasTexto.cpp` -> `src/GerenciadorTexturasTexto.cpp`

### Ativos (Assets)
| Origem (`DACC-Station/arquivos/`) | Destino (`ui/assets/`) | Ação |
| :--- | :--- | :--- |
| `Jogos/**/*.txt` | `assets/data/` | Copiar arquivos de definição. |
| `Jogos/**/*.(jpg,png)` | `assets/images/` | **Atenção:** As imagens dos jogos devem ser movidas mantendo nomes únicos ou estrutura organizada. |
| `Icones/*.png` | `assets/images/` | Ícones gerais. |
| `Incones Modo Claro/*.{png,jpg}` | `assets/images/` | Renomear se necessário para evitar conflitos. |
| `Incones Modo Escuro/*.{png,jpg}` | `assets/images/` | Renomear se necessário. |
| `../Fontes/*.ttf` | `assets/fonts/` | Fontes Garet. |

---

## 2. Refatoração de Código Necessária

A movimentação e o achatamento de pastas quebrarão a compilação. É necessário um "Find & Replace" cirúrgico.

### A. Correção de Includes (Flattening)
Como `include/` agora é plano (não tem subpastas `Principais/`, `Interface/`, etc.), os includes devem ser limpos.

**Regra:** Remover prefixos de diretórios nos includes.
*   `#include "Principais/ConfigLayout.hpp"` -> `#include "ConfigLayout.hpp"`
*   `#include "Gerenciadores/GerenciadorImagens.hpp"` -> `#include "GerenciadorImagens.hpp"`
*   `#include "Interface/Componentes/Botao.hpp"` -> `#include "Botao.hpp"`
*   `#include "Interface/Janelas/Janela.hpp"` -> `#include "Janela.hpp"`
*   `#include "../cabecalhos/..."` -> `#include "..."`

### B. Correção de Caminhos de Assets
Atualizar as strings hardcoded no C++ e nos arquivos de dados para a nova estrutura `assets/`.

**Arquivos Alvo:** `src/*.cpp` e `include/*.hpp`
1.  **Fontes:**
    *   De: `"Fontes/"` ou `"../Fontes/"`
    *   Para: `"assets/fonts/"`
2.  **Imagens de UI:**
    *   De: `"arquivos/Icones/"`, `"arquivos/Incones Modo Claro/"`
    *   Para: `"assets/images/"`
    *   *Nota:* Verificar nomes de arquivo caso haja conflito entre Claro/Escuro (ex: `config.png` vs `config_dark.png`).
3.  **Dados de Jogos (.txt):**
    *   Os arquivos `.txt` contêm caminhos como `*arquivos/Jogos/Jogo1/...`.
    *   **Ação:** Executar script para substituir `arquivos/Jogos/` por `assets/images/` (ou caminho equivalente onde as imagens dos jogos foram colocadas).

---

## 3. Definição do Makefile (Build System)

O novo `Makefile` deve compilar a estrutura plana.

**Requisitos:**
*   **Compilador:** `g++` (C++17).
*   **Include Flags:** `-Iinclude` (Não precisa incluir subdiretórios pois foi achatado).
*   **Libs:** `-lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_gfx -lSDL2_mixer`.
*   **Source:** `SRCS = $(wildcard src/*.cpp)`.

---

## 4. Passos de Execução (Checklist Atualizado)

1. [ ] **Backup:** Garantir que `DACC-Station/` está seguro.
2. [ ] **Structure:** Criar `ui/include` e `ui/src`.
3. [ ] **Migrate Code:** Mover `.hpp` e `.cpp` eliminando as subpastas de origem (`Principais`, `Gerenciadores`, etc.).
4. [ ] **Migrate Assets:** Mover imagens e fontes para `ui/assets/`, resolvendo conflitos de nome.
5. [ ] **Refactor Includes:** Rodar `sed` ou substituir manualmente os includes para remover prefixos de pasta.
6. [ ] **Refactor Paths:** Atualizar caminhos de arquivos (`assets/...`) no código C++.
7. [ ] **Update Data:** Atualizar caminhos dentro dos arquivos `.txt` dos jogos.
8. [ ] **Build:** Compilar com `make` no diretório `ui/`.
9. [ ] **Verify:** Testar navegação, carregamento de imagens e responsividade (ConfigLayout).

---

## 5. Riscos e Mitigação

*   **Risco:** Perda de `ConfigLayout.hpp` causa quebra de layout.
    *   *Mitigação:* Confirmar migração explícita deste arquivo.
*   **Risco:** `GerenciadorTexturasTexto` esquecido causa queda de performance.
    *   *Mitigação:* Confirmar migração e uso em `Utils.cpp`.
*   **Risco:** Conflito de nomes de arquivos ao achatar pastas (ex: dois arquivos `Utils.hpp`?).
    *   *Mitigação:* Verificar unicidade antes de mover. (No projeto atual, parece seguro).