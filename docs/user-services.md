# Etapa 4: serviços de usuário (desenvolvimento)

As units versionadas estão em `systemd/user/*.in`. O instalador substitui
`@DACC_ROOT@` pela raiz absoluta deste checkout. Não há UID, HOME, display ou
`/run/user/N` fixo. O `WorkingDirectory` é a raiz do projeto: LogServer lê
`process-manager/config.json` e grava `data/logs/`; a UI encontra assets a
partir do binário; o PM lê `process-manager/games.json` relativo à localização
de `bin/process-manager`. O catálogo permanece server-side. Não use links para
os binários em outro layout sem rever esses caminhos.

Após `make`, execute `scripts/install-user-services.sh --dry-run` e depois,
se apropriado, `scripts/install-user-services.sh --install`. A instalação é
idempotente, recusa units existentes diferentes e não habilita nem inicia
serviços. `--uninstall` remove somente as quatro units ainda idênticas às
geradas pelo instalador e recarrega o manager; pare os serviços antes de
desinstalar. Para validar sem instalar: `scripts/test-user-services.sh`.
Para exercitar crash/restart e shutdown com jogo falso na sessão de usuário,
instale as units e execute `scripts/test-user-services-live.sh --live`; esse
teste altera temporariamente o ambiente do manager e para PM/LogServer ao sair.
Nenhum comando precisa de sudo. As units instaladas vivem em `~/.config/systemd/user/` (ou
`$XDG_CONFIG_HOME/systemd/user/`).

## Inicialização e sessão gráfica

Inicie `systemctl --user start dacc-station.target` dentro da sessão labwc.
O exemplo `systemd/labwc/autostart.example` contém apenas esse comando; copie
a linha manualmente para o autostart real após verificar a sessão. Não habilite
`dacc-ui.service` em `default.target`: ela não deve iniciar fora do Wayland.
`ExecCondition` exige `WAYLAND_DISPLAY`, `XDG_RUNTIME_DIR`,
`XDG_SESSION_TYPE=wayland` e o socket do compositor. O diagnóstico imprime
somente esses três valores no journal.

A documentação atual do labwc informa que, no backend DRM e com uma sessão
D-Bus ativa, ele importa `WAYLAND_DISPLAY`, `DISPLAY`, `XDG_CURRENT_DESKTOP`
e `XDG_SESSION_TYPE` para o systemd de usuário. Em backend aninhado isso pode
estar desabilitado. Verifique a versão instalada e compare
`systemctl --user show-environment` com o ambiente da sessão antes de adicionar
qualquer importação manual. Neste checkout de desenvolvimento não há labwc
instalado; o teste final dessa integração fica para uma sessão labwc real.

`After=` ordena jobs, mas não prova socket pronto. As units usam `Type=exec`,
sem `sd_notify` nem libsystemd: PM tolera LogServer ausente, e seu sink de log
tenta reconectar de forma limitada; UI tolera PM ausente na inicialização,
detecta desconexão e tenta reconectar no próximo pedido de launch. Nenhum
`Requires=` une os três componentes. `Wants=` inicia a dependência sem derrubar
o consumidor se ela falhar. O target coordena start/stop via `Wants=` e
`PartOf=`; parar o target para os serviços, mas parar uma unit isolada não para
automaticamente as outras.

## Recovery, shutdown e diagnóstico

As três services usam `Restart=on-failure`, `RestartSec=2s`,
`StartLimitIntervalSec=30s` e `StartLimitBurst=5`. Uma parada explícita não
reinicia. Para falhas repetidas, consulte `systemctl --user status` e execute
`systemctl --user reset-failed UNIT` somente após corrigir a causa.

O PM usa `KillMode=mixed`: systemd envia SIGTERM primeiro apenas ao PID
principal, permitindo o shutdown interno TERM → KILL do grupo do jogo. Após
`TimeoutStopSec=5s`, `SendSIGKILL=yes` elimina o cgroup remanescente, inclusive
descendentes que saíram do grupo tradicional. O PM conserva a verificação de
PGID, subreaper, reap e `cleanup_incomplete_` da Etapa 3. A contenção por
cgroup é uma segunda barreira, não substitui essas proteções. LogServer e UI
usam `KillMode=control-group` e o mesmo timeout.

Logs de startup, stdout e stderr ficam no journal:

```
journalctl --user -u dacc-log.service -b
journalctl --user -u dacc-process-manager.service -b
journalctl --user -u dacc-ui.service -b
```

LogServer continua gravando o arquivo configurado em
`process-manager/config.json`; o PM/UI ainda podem emitir a mesma mensagem
para LogServer e stdout. Isso é duplicação intencional para diagnóstico quando
LogServer cai; uma política de logs unificada fica para etapa posterior.

`NoNewPrivileges=yes` foi aplicado às três units. `PrivateTmp`, proteção de
filesystem, restrições de devices/capabilities e filtros de syscalls não foram
ativados cegamente: jogos, SDL, GPU, controle, áudio, PipeWire e BlueZ ainda
precisam de testes em sessão e hardware reais. O serviço roda como o próprio
usuário, sem sudo, e usa os sockets privados de `$XDG_RUNTIME_DIR` da Etapa 3.

## Ainda pendente no Raspberry Pi OS 64-bit

Validar a versão de labwc e seu import de ambiente, abertura de janela SDL,
GPU/DRM, gamepad/input, PipeWire/WirePlumber, NetworkManager, BlueZ, permissões
do usuário dedicado, catálogo/paths instalados, journal persistente ou volátil,
e shutdown com jogos reais. Nada aqui configura autologin ou instala serviços
globalmente.
