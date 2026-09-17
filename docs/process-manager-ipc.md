# Process Manager e IPC local

Os sockets locais ficam em `$XDG_RUNTIME_DIR/dacc-station/`:

- `process-manager.sock`
- `log-server.sock`

O diretório deve pertencer ao usuário efetivo e ter modo `0700`; os sockets e
arquivos de lock usam `0600`. O servidor valida tipo, proprietário e inode e
somente remove o socket que ele próprio criou. Clientes e servidores aceitam
apenas peers do mesmo UID por `SO_PEERCRED`.

Em desenvolvimento, quando `XDG_RUNTIME_DIR` não existe, `DACC_RUNTIME_DIR`
pode apontar para um diretório absoluto, já existente, privado (`0700`) e do
usuário atual. Não há fallback para `/tmp` nem criação de diretório global. O
uso de caminhos ancorados exige `/proc/self/fd`, disponível no Linux desktop e
no Raspberry Pi OS; sua ausência produz `proc_fd_unavailable` em vez de voltar
a um caminho não ancorado.

## Lançamento de jogo

O cliente envia um frame JSON terminado por nova linha com apenas a identidade:

```json
{"action":"start","request_id":"ui-...","game_id":"run-sync"}
```

`path`, `argv`, `cwd` e `graphics` enviados pelo cliente são recusados com
`client_command_forbidden`. O servidor resolve `game_id` no catálogo
`process-manager/games.json`. Cada entrada contém `id`, `argv` estruturado e
`cwd`; não há concatenação nem execução por shell implícita. Para testes ou
desenvolvimento, o daemon pode receber outro catálogo server-side por
`DACC_GAME_CATALOG`.

O protocolo correlaciona `game_started`, `game_start_failed` e `game_finished`
por `request_id` e `game_id`. IDs são limitados a 128 caracteres alfanuméricos,
ponto, hífen e sublinhado; frames são limitados a 64 KiB; `request_id`
duplicado não executa o jogo novamente.

## Lifecycle

Existe no máximo um jogo ativo. O filho cria um grupo com PGID igual ao seu PID
antes de `exec`, e o Process Manager atua como subreaper. Término, desconexão do
cliente e shutdown usam `SIGTERM` no grupo, grace period limitado e `SIGKILL`.
O líder não é coletado antes do último sinal de grupo, evitando reutilização de
PID/PGID; depois, líder e descendentes adotados do mesmo grupo são coletados.
Se a propriedade do PID/PGID ou o reap não puder ser provado, o PM entra em
`process_cleanup_incomplete` e bloqueia novos launches.
