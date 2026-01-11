# PLANO DE MIGRAÇÃO: DACC-Station -> ui/

**Status:** Planejamento
**Origem:** `DACC-Station/` (Código Feature-Rich / Estrutura Legada)
**Destino:** `ui/` (Código Base / Estrutura Padronizada)
**Objetivo:** Consolidar as funcionalidades avançadas de `DACC-Station` (Pesquisa, Teclado, Layout Responsivo) dentro da estrutura de diretórios limpa e padronizada de `ui/`.

---

## 1. Análise de Diferenças e Mapeamento

O módulo `DACC-Station` contém a lógica mais recente da interface ("Update"), mas utiliza uma estrutura de pastas em português e profundamente aninhada. O módulo `ui/` possui a estrutura alvo (plana e em inglês/padronizada).

### 1.1. Mapeamento de Estrutura (De -> Para)

| Origem (`DACC-Station/`) | Destino (`ui/`) | Ação |
| :--- | :--- | :--- |
| **`cabecalhos/**/*.hpp`** | **`include/`** | **Achatar:** Mover todos os arquivos `.hpp` das subpastas (`Arquivos`, `Interface`, etc) diretamente para a raiz de `include/`. |
| **`implementacao/**/*.cpp`** | **`src/`** | **Achatar:** Mover todos os arquivos `.cpp` das subpastas diretamente para a raiz de `src/`. |
| `main/main.cpp` | `src/main.cpp` | Sobrescrever o entry point. |
| `arquivos/Icones/*.png` | `assets/images/` | Mover ícones gerais. |
| `arquivos/Incones Modo Claro/*` | `assets/images/` | Mover. **Atenção:** Verificar colisões de nome. |
| `arquivos/Incones Modo Escuro/*` | `assets/images/` | Mover. **Atenção:** Verificar colisões de nome. |
| `arquivos/Jogos/**/*.txt` | `assets/data/` | Mover arquivos de definição de jogos. |
| `arquivos/Jogos/**/*.(jpg\|png)` | `assets/images/` | Mover capas e backgrounds dos jogos. |
| `Fontes/*.ttf` | `assets/fonts/` | Mover fontes. |

### 1.2. Componentes Novos em DACC-Station
Estes arquivos são exclusivos ou versões muito modificadas que devem ser preservados:
*   `BotaoPesquisa` (.hpp/.cpp) - *Barra de busca com dropdown.*
*   `TecladoVirtual` (.hpp/.cpp) - *Teclado em tela.*
*   `JanelaConfiguracao` (.hpp/.cpp) - *Janela secundária.*
*   `ConfigLayout` (.hpp/.cpp) - *Cálculo de responsividade.*
*   `GerenciadorTexturasTexto` (.hpp/.cpp) - *Otimização de renderização.*

---

## 2. Refatoração de Código Necessária

A simples cópia dos arquivos quebrará a compilação devido às mudanças de caminhos.

### A. Correção de Includes (Flattening)
Como `ui/include` não terá subpastas, os `#include` no código devem ser atualizados.

*   **Padrão Antigo:** `#include "Principais/ConfigLayout.hpp"`, `#include "Interface/Componentes/Botao.hpp"`
*   **Novo Padrão:** `#include "ConfigLayout.hpp"`, `#include "Botao.hpp"`

**Ação:** Executar substituição global (Regex) em todos os `.cpp` e `.hpp` movidos para remover os prefixos de diretório nos includes.

### B. Atualização de Caminhos de Assets (Hardcoded Strings)
O código C++ carrega imagens usando strings hardcoded que apontam para a estrutura antiga.

*   **De:** `"arquivos/Incones Modo Claro/ConfigClaro.png"`
*   **Para:** `"assets/images/ConfigClaro.png"` (ou novo nome padronizado)

*   **De:** `"Fontes/Garet-Book.ttf"`
*   **Para:** `"assets/fonts/Garet-Book.ttf"`

### C. Atualização dos Arquivos de Dados (.txt)
Os arquivos de texto que definem os jogos (`jogo1.txt`, etc) contêm caminhos para as imagens.

*   **Conteúdo Atual:** `*arquivos/Jogos/Jogo1/capa.jpg`
*   **Conteúdo Alvo:** `*assets/images/jogo1/capa.jpg` (assumindo que as imagens dos jogos foram movidas para `assets/images/jogo1/`).

---

## 3. Estratégia de Execução

### Passo 1: Preparação do Ambiente `ui/`
1.  Limpar `ui/src/` e `ui/include/` (removendo código antigo/obsoleto, exceto `NetworkClient` se necessário preservar).
2.  Garantir existência de `ui/assets/images`, `ui/assets/data`, `ui/assets/fonts`.

### Passo 2: Migração e Achatamento
1.  Copiar todos os headers de `DACC-Station/cabecalhos/` recursivamente para `ui/include/`.
2.  Copiar todos os sources de `DACC-Station/implementacao/` recursivamente para `ui/src/`.
3.  Copiar `main.cpp`.

### Passo 3: Refatoração em Massa (Sed/Script)
1.  **Includes:** Remover qualquer texto seguido de `/` dentro de aspas em diretivas `#include`.
    *   Ex: `sed -i 's/#include ".*\/\(.*\)"/#include "\1"/' ui/src/*.cpp`
2.  **Assets:** Substituir ocorrências de `arquivos/` por `assets/` e `Fontes/` por `assets/fonts/`.

### Passo 4: Migração de Assets
1.  Copiar fontes.
2.  Copiar ícones e imagens de UI. *Sugestão:* Ao copiar "Incones Modo Claro" e "Escuro", talvez seja necessário renomear os arquivos para evitar sobreposição se eles tiverem o mesmo nome (ex: `config.png` em ambas as pastas).
    *   *Alternativa:* Manter subpastas em `assets/images/` (ex: `assets/images/light/` e `assets/images/dark/`) e atualizar o código C++ para refletir isso.

### Passo 5: Validação de Build
1.  Utilizar o `Makefile` existente em `ui/`. Ele já usa `wildcard`, então deve detectar os novos arquivos automaticamente.
2.  Rodar `make` e corrigir erros pontuais de compilação.

---

## 4. Notas Importantes sobre Dependências

*   **SDL2_gfx:** O novo código utiliza `roundedBoxRGBA` e `filledPolygonRGBA`. Garantir que `ui/Makefile` linka com `-lSDL2_gfx`.
*   **C++17:** O código usa `std::filesystem` (possivelmente) ou features modernas. Garantir flag `-std=c++17`.
