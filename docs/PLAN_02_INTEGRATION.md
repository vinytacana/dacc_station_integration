# PLANO 02: Integração de Sistemas (IPC & Logging)

**Status:** Pendente (Depende do PLANO 01)
**Data:** 10 Jan 2026
**Objetivo:** Conectar a UI (agora padronizada) ao `ProcessManager` e ao `LogServer` do DACC Station, permitindo o lançamento real de jogos e telemetria.

---

## 1. Importação de Classes de Infraestrutura

Não vamos reescrever a roda. Vamos importar (ou linkar) as classes robustas já testadas no protótipo `mono-dacc-station`.

**Arquivos a Importar (Cópia ou Symlink):**
*   Do `mono-dacc-station/ui/include/` para `dacc-station/ui/include/`:
    *   `NetworkClient.hpp`: Cliente de Socket para falar com o Process Manager.
    *   `SystemStatus.hpp`: Lógica de relógio/bateria (se já não existir no `ui-dacc` original, adaptar).
*   Do `mono-dacc-station/ui/src/` para `dacc-station/ui/src/`:
    *   `NetworkClient.cpp`: Implementação do protocolo JSON.

**Dependências Externas:**
*   Biblioteca de Logs: `dacc-station/logs/libdacc-logs.a` (Já deve estar compilada).
*   Biblioteca JSON: `dacc-station/process-manager/third-party/json.hpp`.

---

## 2. Refatoração da Lógica de Lançamento (Launching)

Atualmente, `ui-dacc` usa `std::system()` diretamente em `JanelaJogo.cpp`. Isso é inseguro e não gerenciado.

**Mudanças em `src/JanelaJogo.cpp`:**
1.  **Include:** Adicionar `#include "NetworkClient.hpp"`.
2.  **Método `executarJogo()`:**
    *   *Antigo:* `system(comando.c_str());`
    *   *Novo:*
        ```cpp
        // Exemplo
        NetworkClient::getInstance().sendStartGame(
            jogo.getId(),
            jogo.getExecutavel() // Agora deve ser o caminho absoluto
        );
        ```
3.  **Feedback Visual:**
    *   O botão "JOGAR" deve mudar para "RODANDO..." se `NetworkClient::isGameRunning()` for verdadeiro.

---

## 3. Refatoração de Logging

Substituir `std::cout` e `std::cerr` pelo sistema de log centralizado.

**Mudanças em `src/main.cpp`:**
1.  **Inicialização:**
    ```cpp
    LogManager::getInstance().initialize("UI", "/tmp/dacc-station.sock");
    LOG_INFO("UI Iniciada com sucesso.");
    ```

**Mudanças Gerais:**
*   `std::cerr << "Erro..."` -> `LOG_ERROR("Erro...");`
*   `std::cout << "Carregando..."` -> `LOG_INFO("Carregando...");`

---

## 4. Integração com o Loop Principal (`src/main.cpp`)

O loop SDL precisa verificar mensagens recebidas do socket (ex: "Jogo fechou").

**Código a Inserir no Loop:**
```cpp
// Antes do SDL_RenderPresent
NetworkClient::getInstance().checkEvents(); // Verifica se o backend mandou algo
```

---

## 5. Passos de Execução (Checklist)

1. [ ] **Copy:** Trazer `NetworkClient` (.hpp/.cpp) do protótipo.
2. [ ] **Deps:** Atualizar `Makefile` para incluir diretórios de `json.hpp` e linkar `-ldacc-logs`.
3. [ ] **Main:** Inicializar `LogManager` no início do `main()`.
4. [ ] **Logic:** Alterar `JanelaJogo::executarJogo` para usar IPC.
5. [ ] **UI Feedback:** Atualizar botão "Jogar" baseando-se no estado do `NetworkClient`.
6. [ ] **Test:** Iniciar `log-server`, `process-manager` e depois a nova `dacc-ui`. Verificar se o log aparece no arquivo central e se o jogo abre via PM.
