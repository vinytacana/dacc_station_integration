# 📋 TODO 09: Gestão de Estado e IPC (Process Manager)

**Objetivo:** Garantir que a UI refleta o estado real do sistema (Rodando vs Ocioso) e envie comandos corretos.

**Arquivos Afetados:**
- `src/NetworkClient.cpp` (ou classe de Socket existente)
- `src/JanelaJogo.cpp`

**Especificações Técnicas:**

1.  **Monitoramento de Estado (Heartbeat/Polling):**
    A UI precisa saber se o `gamescope` ou um jogo está ativo.
    - Se o backend suportar push notification via socket, ouvir eventos.
    - Caso contrário, consultar estado periodicamente.

2.  **Lógica do Botão "Jogar/Retomar":**
    - Se `GameState == IDLE`: Texto do botão = "JOGAR" -> Envia comando `start`.
    - Se `GameState == RUNNING` e o jogo selecionado == jogo rodando: Texto = "RETOMAR" -> Envia comando `resume` ou foca a janela.
    - Se `GameState == RUNNING` e seleção != jogo rodando: Bloquear lançamento ou avisar "Fechar jogo atual?".

3.  **Payload JSON:**
    Confirmar formato de envio:
    ```json
    {
      "command": "start_game",
      "game_id": "game_xyz_123"
    }
    ```

**Critério de Sucesso:**
A UI não permite tentar abrir dois jogos ao mesmo tempo (se o backend não suportar). O botão principal reflete o contexto.
