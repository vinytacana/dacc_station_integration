# DACC Station OS

O DACC Station OS e um launcher/console academico desenvolvido para a DACC Station. O projeto integra uma interface grafica em SDL2, um backend de configuracao do sistema Linux, um gerenciador de processos para execucao de jogos e um servidor centralizado de logs.

Esta documentacao descreve a arquitetura atual da versao estabilizada, os modulos principais, como eles se comunicam e o motivo das principais decisoes tecnicas.

## Visao Geral

O sistema foi organizado para separar responsabilidades:

- `ui/`: interface grafica, navegacao, telas de configuracao, carregamento de jogos e comunicacao com o Process Manager.
- `config-dacc/`: biblioteca C++ que encapsula operacoes de audio, video, rede, bateria e Bluetooth.
- `process-manager/`: servico responsavel por iniciar jogos e notificar a UI quando processos terminam.
- `logs/`: servidor e cliente de logs via socket Unix, usado pelos processos principais.
- `scripts/`: scripts de operacao, principalmente inicializacao do ambiente.
- `games/`: scripts e binarios/JARs de jogos usados pelo launcher.
- `docs/`: planos e documentos auxiliares do projeto.

O fluxo padrao de execucao e:

1. `scripts/start_station.sh` inicia o Log Server.
2. O script inicia o Process Manager, que abre `/tmp/gameman.sock`.
3. A UI SDL2 e iniciada.
4. A UI consulta `config-dacc` para informacoes de sistema e configuracoes.
5. Quando o usuario inicia um jogo, a UI envia um JSON para o Process Manager.
6. O Process Manager cria o processo do jogo e informa a UI quando ele termina.

## Build e Execucao

### Dependencias principais

O projeto espera um ambiente Linux com:

- `g++` com suporte a C++17
- `make`
- SDL2 e extensoes: `SDL2`, `SDL2_image`, `SDL2_ttf`, `SDL2_mixer`, `SDL2_gfx`
- `nlohmann/json`
- `spdlog`
- `nmcli`/NetworkManager para rede
- `bluetoothctl`/BlueZ para Bluetooth
- `wpctl`, `pactl` ou `amixer` para audio
- `xrandr`, `wlr-randr` ou `gsettings` para video/escala
- `mpv` para reproducao do video de introducao, quando disponivel

Em Ubuntu/Debian, a base de desenvolvimento da UI normalmente e:

```bash
sudo apt update
sudo apt install build-essential git
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libsdl2-gfx-dev
sudo apt install nlohmann-json3-dev
```

Dependencias especificas de hardware/configuracao variam conforme a distribuicao.

### Compilar tudo

```bash
make
```

O `Makefile` raiz compila, nesta ordem:

1. `logs`
2. `config-dacc`
3. `process-manager`
4. `ui`

Os binarios finais ficam em `bin/`.

### Limpar build

```bash
make clean
```

Quando houver mudancas em headers compartilhados, especialmente `config-dacc/functions.hpp`, e recomendavel fazer build limpo da UI:

```bash
make -C ui clean
make
```

O motivo e que a UI depende dos contratos de `config-dacc`; recompilar do zero evita objetos antigos linkados com definicoes desatualizadas.

### Rodar a station

```bash
./scripts/start_station.sh
```

O script inicia os servicos na ordem correta e encerra os filhos quando a UI fecha ou quando recebe sinal de interrupcao.

## Modulo `config-dacc`

O `config-dacc` e uma biblioteca estatica (`libdacc-config.a`) que atua como camada de traducao entre a UI e o sistema operacional. A UI nao executa `nmcli`, `bluetoothctl`, `wpctl` ou `xrandr` diretamente; ela chama funcoes C++ e recebe structs tipadas.

Arquivo publico principal:

- `config-dacc/functions.hpp`

Implementacoes principais:

- `config-dacc/ConfigCommand.cpp`
- `config-dacc/HardwareControl.cpp`
- `config-dacc/BluetoothControl.cpp`
- `config-dacc/BluetoothInternal.cpp`
- `config-dacc/BluetoothInternal.hpp`

### Contrato `system_result`

Todas as operacoes mutaveis de configuracao que podem falhar retornam `system_result`:

```cpp
struct system_result {
    bool ok = false;
    std::string mensagem;
    std::string codigo;
    std::string detalhes;
};
```

Campos:

- `ok`: indica sucesso ou falha.
- `mensagem`: texto amigavel para UI ou log.
- `codigo`: identificador estavel do tipo de resultado, como `ok`, `wifi_auth_failed`, `audio_select_failed`.
- `detalhes`: saida bruta ou contexto tecnico para debug.

Por que existe:

- evita quatro contratos duplicados para audio, video, Wi-Fi e Bluetooth;
- permite que a UI trate erros de forma uniforme;
- separa mensagem amigavel de detalhe tecnico;
- deixa o backend livre para mapear erros de ferramentas diferentes sem vazar complexidade para as telas.

### Modulo de sistema e comandos

Funcoes principais:

- `bool comando_existe(const std::string& cmd)`
- `std::string exec_command(const char* cmd)`
- `command_result exec_command_result(const std::string& cmd)`
- `command_result exec_command_args_result(const std::vector<std::string>& args)`
- `long long obter_tempo_ms()`
- `int obter_bateria()`

`command_result` representa a execucao de comandos:

```cpp
struct command_result {
    bool ok = false;
    int exit_code = -1;
    std::string stdout_output;
    std::string stderr_output;
    std::string mensagem;
};
```

`exec_command_result` e usado para comandos simples, normalmente informativos. `exec_command_args_result` e preferido para comandos com argumentos vindos de UI ou entrada externa, porque usa `execvp` com vetor de argumentos e evita interpolacao de shell.

`obter_bateria()` le `/sys/class/power_supply/BAT0/capacity`. Se nao houver bateria ou o arquivo nao puder ser lido, retorna `-1`, permitindo que a UI esconda ou degrade o indicador sem quebrar.

### Modulo de audio

Funcoes publicas:

- `void aumentar_volume()`
- `void diminuir_volume()`
- `int obter_volume_atual()`
- `void definir_volume(int valor_int)`
- `std::vector<device_audio> listar_dispositivos_audio()`
- `void selecionar_dispositivo_audio(int id)`
- `system_result selecionar_dispositivo_audio_result(int id)`
- `void imprimir_dispositivos_audio()`

Struct:

```cpp
struct device_audio {
    int id;
    std::string descricao;
    bool padrao;
};
```

Como funciona:

- `definir_volume` tenta controlar volume com ferramentas comuns do Linux moderno e tradicional.
- `listar_dispositivos_audio` interpreta a saida do sistema de audio e identifica dispositivos disponiveis.
- `selecionar_dispositivo_audio_result` troca o dispositivo padrao e retorna `system_result`.

Por que a implementacao e tolerante:

- diferentes maquinas podem usar PipeWire/WirePlumber, PulseAudio ou ALSA;
- a Station pode rodar em hardware academico, desktop ou mini PC;
- falhas de comando precisam voltar para a UI como mensagem controlada, nao como crash.

### Modulo de video

Funcoes publicas:

- `std::string obter_tipo_sessao()`
- `void verificarSessao()`
- `std::vector<DisplayOutput> obter_info_displays()`
- `void listar_resolucao()`
- `bool alterarResolucao(const std::string& saida, int width, int height, float rate)`
- `system_result alterarResolucao_result(const std::string& saida, int width, int height, float rate)`
- `bool alterarEscala(const std::string& saida, float escala)`
- `system_result alterarEscala_result(const std::string& saida, float escala)`
- `void aumentar_brilho()`
- `void diminuir_brilho()`

Structs:

```cpp
struct DisplayMode {
    int width;
    int height;
    float refresh_rate;
    bool is_current;
};

struct DisplayOutput {
    std::string name;
    bool connected;
    std::vector<DisplayMode> modes;
    DisplayMode current_mode;
    float current_scale;
};
```

Como funciona:

- o backend detecta tipo de sessao grafica;
- em X11, tende a usar `xrandr`;
- em Wayland/wlroots, pode usar `wlr-randr`;
- em GNOME, escala pode depender de `gsettings`.

Por que ha funcoes `bool` e funcoes `_result`:

- as funcoes `bool` preservam compatibilidade com chamadas antigas;
- as funcoes `_result` sao o contrato preferido para UI nova, porque explicam a falha.

### Modulo de rede

Funcoes publicas:

- `void listar_wifi()`
- `std::vector<wifi_network> listar_wifi_parsed()`
- `wifi_adapter_status obter_status_wifi()`
- `network_connection_status obter_status_conexao_rede()`
- `bool wifi_conectado()`
- `system_result definir_estado_wifi_result(bool ligar)`
- `void conectar_wifi(const std::string& ssid, const std::string& senha)`
- `system_result conectar_wifi_result(const std::string& ssid, const std::string& senha)`
- `void desconectar_wifi(const std::string& id)`
- `system_result desconectar_wifi_result(const std::string& id)`

Structs:

```cpp
struct wifi_network {
    std::string ssid;
    int sinal;
    std::string seguranca;
    bool em_uso;
};

struct wifi_adapter_status {
    bool enabled = false;
    bool disponivel = true;
    bool conectado_wifi = false;
    bool conectado_cabeado = false;
    std::string dispositivo_cabeado;
    std::string conexao_cabeada;
    std::string dispositivo_wifi;
    std::string conexao_wifi;
    std::string output;
};

struct network_connection_status {
    bool conectado = false;
    bool wifi_conectado = false;
    bool cabeado_conectado = false;
    std::string tipo;
    std::string dispositivo;
    std::string conexao;
    std::string dispositivo_wifi;
    std::string conexao_wifi;
    std::string dispositivo_cabeado;
    std::string conexao_cabeada;
};
```

Como funciona:

- `listar_wifi_parsed` usa `nmcli -t` e faz parsing dos campos escapados.
- `obter_status_conexao_rede` consulta dispositivos conectados e diferencia Wi-Fi de Ethernet.
- se ha rede cabeada ativa, ela e tratada como conexao primaria.
- `obter_status_wifi` inclui tanto informacoes do radio Wi-Fi quanto indicacao de cabo.

Por que o modo cabeado existe:

- quando a Station esta em Ethernet, fazer scan Wi-Fi frequente e desnecessario;
- scan de Wi-Fi pode ser lento, causar travamentos ou poluir a UI;
- a UI pode mostrar um painel leve de rede cabeada e evitar renderizar listas pesadas.

### Modulo Bluetooth

Arquivos:

- `BluetoothControl.cpp`: API publica e regras de alto nivel.
- `BluetoothInternal.cpp`: sessoes `bluetoothctl`, parser e funcoes auxiliares.
- `BluetoothInternal.hpp`: funcoes internas compartilhadas.

Funcoes publicas:

- `bool obter_estado_bluetooth()`
- `bluetooth_adapter_status obter_status_bluetooth()`
- `bluetooth_state_snapshot obter_estado_bluetooth_completo()`
- `bluetooth_ui_snapshot obter_estado_bluetooth_ui()`
- `bluetooth_ui_snapshot obter_estado_bluetooth_ui_com_scan(int segundos = 10)`
- `bool definir_estado_bt(bool ligar)`
- `system_result definir_estado_bt_result(bool ligar)`
- `std::vector<device_bt> listar_dispositivos_bluetooth_conhecidos()`
- `std::vector<device_bt> listar_dispositivos_bluetooth_pareados()`
- `std::vector<device_bt> scan_dispositivos_bluetooth(int segundos = 10)`
- `system_result parear_bluetooth(const std::string& mac)`
- `system_result confiar_bluetooth(const std::string& mac)`
- `system_result parear_confiar_conectar_bluetooth(const std::string& mac)`
- `system_result conectar_bluetooth_result(const std::string& mac)`
- `bool conectar_bluetooth(const std::string& mac)`
- `system_result desconectar_bluetooth_result(const std::string& mac)`
- `bool desconectar_bluetooth(const std::string& mac)`
- `system_result remover_bluetooth(const std::string& mac)`
- `void parsing_bluetooth_stream(...)`

Structs:

```cpp
struct device_bt {
    std::string mac;
    std::string nome;
    std::string icon;
    bool conectado = false;
    bool pareado = false;
    bool confiavel = false;
};

struct bluetooth_adapter_status {
    bool powered = false;
    bool soft_blocked = false;
    bool hard_blocked = false;
    bool controller_disponivel = true;
    std::string show_output;
    std::string rfkill_output;
};
```

Como funciona:

- `bluetoothctl` e interativo, entao o backend usa PTY para simular um terminal real;
- a saida do `bluetoothctl` e um fluxo com eventos, cores ANSI e linhas contextuais;
- `parsing_bluetooth_stream` remove ANSI, acompanha o ultimo MAC visto e preenche `device_bt`;
- operacoes lentas usam polling para confirmar se o estado realmente mudou;
- erros conhecidos sao normalizados em `system_result`.

Por que existe `bluetooth_ui_snapshot`:

- a UI precisa de uma fotografia coerente do estado;
- separar conhecidos, pareados e escaneados reduz regra visual na tela;
- o snapshot pode vir do cache, evitando scans caros o tempo todo.

## Modulo `ui`

A UI e uma aplicacao SDL2. Ela renderiza o launcher, as telas de configuracao e a navegacao por mouse/teclado/controle.

Arquivos e classes importantes:

- `main.cpp`: inicializacao SDL, intro, loop principal.
- `GerenciarSDL`: inicializacao e encerramento dos recursos SDL.
- `GerenciarInterface`: composicao visual da tela principal.
- `GerenciarInputs`: roteamento de eventos de teclado, mouse e controle.
- `GerenciadorJogos`: descoberta e carregamento de jogos.
- `GerenciadorImagens`: cache de texturas por renderer.
- `GerenciadorFontes`: cache de fontes.
- `GerenciadorTexturasTexto`: cache/renderizacao de textos.
- `SystemStatus`: coleta hora, bateria e estado de rede para a barra superior.
- `GraphicsUtils`: primitivas de desenho e icones de status.
- `JanelaConfiguracao`: hub das telas de configuracao.
- `JanelaRede`: configuracao de Wi-Fi/Ethernet.
- `JanelaBluetooth`: Bluetooth, scan, pareamento e conexao.
- `JanelaAudioEVideo`: volume, dispositivo de audio, resolucao e escala.
- `JanelaInfosSistema`: tela informativa do sistema.
- `TecladoVirtual`: entrada de texto sem teclado fisico.
- `NetworkClient`: cliente Unix socket para o Process Manager.

### Janela de rede

A tela de rede consome `config-dacc` e possui dois modos:

- modo Wi-Fi: lista redes, permite senha, conexao e toggle do radio;
- modo cabeado: detecta Ethernet ativa, exibe conexao/dispositivo e evita scan pesado de Wi-Fi.

Essa separacao existe para melhorar estabilidade: em maquinas cabeadas, a rede ja esta resolvida e a UI nao precisa gastar tempo renderizando listas de redes sem necessidade.

### Janela Bluetooth

A tela de Bluetooth usa tarefas em segundo plano para operacoes lentas. Isso evita congelar a UI durante scan, pareamento ou conexao.

Ela consome snapshots do backend e separa:

- dispositivos desconhecidos conectados;
- dispositivos pareados;
- dispositivos escaneados.

As operacoes retornam `system_result`, permitindo mensagens amigaveis como timeout, falha de autenticacao ou adaptador indisponivel.

### Janela Audio e Video

A tela de audio/video lista dispositivos reais do sistema e aplica alteracoes usando o backend. Alteracoes que podem falhar usam `system_result`, entao a UI consegue exibir uma mensagem adequada sem depender da saida bruta de terminal.

### SystemStatus

`SystemStatus` e responsavel por dados leves de barra superior:

- horario atual;
- bateria;
- Wi-Fi conectado;
- rede cabeada conectada.

O modulo protege consultas de rede e bateria para que falhas de ambiente nao derrubem a UI.

### Cache de imagens por renderer

Texturas SDL pertencem ao `SDL_Renderer` que as criou. Por isso, o cache de imagens considera renderer e caminho. Quando uma janela de configuracao fecha e seu renderer e destruido, as texturas daquele renderer tambem precisam ser liberadas.

Essa regra evita reusar textura vinculada a renderer destruido, que pode causar icones sumindo, renderizacao incorreta ou falhas de memoria.

## Modulo `process-manager`

O Process Manager e o servico que inicia jogos fora do processo da UI. Ele existe para que a interface continue separada da vida util dos jogos.

Socket:

- caminho: `/tmp/gameman.sock`
- protocolo: JSON sobre Unix socket

Mensagem principal recebida:

```json
{
  "action": "start",
  "id": "run-sync",
  "path": "/caminho/para/jogo.sh",
  "graphics": {
    "use_gamescope": false,
    "width": 1280,
    "height": 720,
    "fps": 60
  }
}
```

Quando um jogo termina, o Process Manager envia para os clientes conectados:

```json
{
  "event": "game_finished",
  "pid": 12345,
  "status": 0
}
```

Como funciona internamente:

- cria um socket Unix server;
- usa `epoll` para observar conexoes e mensagens;
- usa `signalfd` para receber `SIGCHLD`;
- cria processos com `fork`;
- executa jogos com `execvp`;
- rastreia PIDs em execucao e tempo total;
- notifica clientes quando um filho termina.

### `ApplicationDefinition`

Representa uma aplicacao/jogo:

- `id`: identificador logico usado pela UI;
- `path`: executavel ou script;
- `type`: tipo da aplicacao;
- `renderWidth`/`renderHeight`: resolucao interna;
- `outputWidth`/`outputHeight`: resolucao de saida;
- `refreshRate`: FPS alvo;
- `useGamescope`: indica se deve envolver o jogo com `gamescope`;
- `fullscreen`: indica fullscreen quando gamescope estiver ativo.

Se o caminho termina com `.sh`, o Process Manager executa via `/bin/bash`. Isso reduz problemas quando scripts externos nao possuem permissao de execucao.

## Modulo `logs`

O modulo de logs centraliza mensagens dos componentes principais.

Componentes:

- `LogServer`: processo servidor que escuta um socket Unix.
- `LogManager`: singleton usado pelos programas para obter logger configurado.
- `UnixSocketClient`: cliente socket usado pelo sink de logs.
- `UnixSocketSink`: sink spdlog que envia mensagens ao servidor.

Como funciona:

1. O Log Server le `config.json`.
2. Ele abre o socket configurado, normalmente `/tmp/dacc-station.sock`.
3. Clientes inicializam `LogManager`.
4. Logs sao enviados por socket ao servidor.
5. O servidor escreve em console ou arquivo rotativo, conforme configuracao.

Por que existe:

- processos separados conseguem compartilhar uma saida de log;
- a UI e o Process Manager nao precisam escrever arquivos diretamente;
- se o socket falhar, o sistema pode degradar para fallback/console.

## Scripts

### `scripts/start_station.sh`

Script operacional principal. Ele:

- identifica a raiz do projeto;
- inicia o Log Server;
- inicia o Process Manager;
- inicia a UI;
- encerra servicos filhos ao finalizar.

Use este script para testes reais, porque ele reproduz a ordem correta dos servicos.

### `scripts/check_compatibility.sh`

Script auxiliar para verificar ambiente e dependencias.

### `scripts/optimize_assets.sh`

Script auxiliar para otimizacao de assets.

## Testes e validacao

Testes unitarios atuais do backend:

```bash
make test
```

No momento, a cobertura automatizada esta concentrada no parser de Bluetooth. Para alteracoes em UI/config, a validacao recomendada e:

```bash
make -C config-dacc
make test
make -C ui clean
make
./scripts/start_station.sh
```

Checklist manual recomendado:

- abrir e fechar notificacoes varias vezes;
- conferir se bateria continua renderizando;
- abrir rede com cabo conectado;
- abrir rede sem cabo ou com Wi-Fi;
- abrir Audio/Video;
- abrir Bluetooth;
- abrir Informacoes do Sistema;
- iniciar um jogo e confirmar retorno para a UI;
- fechar a Station e verificar encerramento dos servicos.

## Fluxo Git Atual

Fluxo recomendado para estabilizacoes:

1. criar uma branch de trabalho a partir de `main`;
2. manter commits pequenos por assunto;
3. rodar build/testes;
4. publicar branch;
5. abrir PR para `main`;
6. revisar `Files changed`;
7. fazer merge;
8. atualizar `main` local com `git pull --ff-only origin main`.

Branches de backup podem ser mantidas temporariamente ate a versao estabilizada ser usada por algum tempo.

## Convencoes de Desenvolvimento

- Use `system_result` para operacoes de configuracao que podem falhar.
- Prefira `exec_command_args_result` para comandos com argumentos variaveis.
- Evite colocar regras de parsing na UI; parsing de sistema pertence ao `config-dacc`.
- Operacoes lentas de hardware devem rodar fora do loop principal da UI.
- Texturas SDL devem ser liberadas antes do renderer correspondente.
- Mudancas em headers compartilhados pedem rebuild limpo.
- Jogos e configuracao devem permanecer desacoplados: a UI pede execucao ao Process Manager, nao executa jogos diretamente.

## Documentacao Especifica

- `config-dacc/README.md`: detalhes completos do backend de configuracao.
- `ui/README`: detalhes da interface SDL2.
- `ui/assets/README.md`: organizacao de assets.
- `docs/`: planos de trabalho e documentos auxiliares.
