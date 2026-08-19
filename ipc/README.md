# IPC compartilhado

O módulo `ipc` fornece framing e envio confiável para os sockets Unix usados pela UI,
pelo Process Manager e pelo sistema de logs.

## Framing

- Cada mensagem é UTF-8 e termina com `\n`.
- `ipc::sendMessage()` acrescenta o delimitador quando necessário.
- Cada frame pode ter no máximo 64 KiB, sem contar o delimitador.
- `ipc::FrameReader` mantém fragmentos entre chamadas e separa mensagens concatenadas.
- O envio usa prazo monotônico, trata escritas parciais e não gera `SIGPIPE`.
- `SendStatus::Desynced` indica que apenas parte do frame foi enviada. O chamador
  deve fechar o socket imediatamente e nunca reutilizar esse stream.

## Protocolo de execução de jogos

A UI inicia um jogo com:

```json
{
  "action": "start",
  "request_id": "ui-123-456-1",
  "game_id": "ashley",
  "path": "/caminho/absoluto/jogo"
}
```

`request_id` e `game_id` são obrigatórios. Comandos sem `request_id` são
registrados e descartados sem iniciar processo ou alterar estado.

Após confirmar que `execvp()` foi executado, o Process Manager responde:

```json
{
  "event": "game_started",
  "request_id": "ui-123-456-1",
  "game_id": "ashley",
  "pid": 1234
}
```

Se o `fork()`, o pipe de confirmação ou o `execvp()` falhar, a resposta é:

```json
{
  "event": "game_start_failed",
  "request_id": "ui-123-456-1",
  "game_id": "ashley",
  "error_code": 2,
  "message": "No such file or directory"
}
```

Quando o processo termina:

```json
{
  "event": "game_finished",
  "request_id": "ui-123-456-1",
  "game_id": "ashley",
  "pid": 1234,
  "exit_status": 0
}
```

`exit_status` contém o código normal de saída ou `128 + sinal` quando o processo
termina por sinal. A UI descarta eventos sem `request_id` e correlaciona cada mudança
de estado exclusivamente com a requisição correspondente.
