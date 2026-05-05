# DACC Config Backend

`config-dacc` e o backend de configuracao da DACC Station. Ele fornece uma API C++ para que a UI controle audio, video, rede, bateria e Bluetooth sem depender diretamente de comandos de terminal ou de parsing de texto bruto.

O objetivo deste modulo e transformar operacoes de sistema Linux em contratos previsiveis, tipados e seguros para a interface grafica.

## Responsabilidades

O modulo e responsavel por:

- executar comandos de sistema com controle de erro;
- detectar ferramentas disponiveis;
- ler informacoes simples do sistema, como bateria;
- listar e selecionar dispositivos de audio;
- listar monitores, resolucoes e escala;
- alterar resolucao e escala;
- listar redes Wi-Fi;
- detectar conexao Wi-Fi e conexao cabeada;
- conectar, desconectar e ligar/desligar Wi-Fi;
- consultar estado do Bluetooth;
- escanear, parear, confiar, conectar, desconectar e remover dispositivos Bluetooth;
- normalizar resultados de erro com `system_result`.

Ele nao deve:

- renderizar UI;
- decidir layout visual;
- iniciar jogos;
- manter estado visual de telas;
- expor saida bruta de comandos como contrato principal.

## Arquivos

- `functions.hpp`: contrato publico consumido pela UI.
- `ConfigCommand.cpp`: execucao de comandos e utilitarios basicos do sistema.
- `HardwareControl.cpp`: audio, video, bateria e rede.
- `BluetoothControl.cpp`: API publica de Bluetooth e regras de alto nivel.
- `BluetoothInternal.cpp`: sessoes `bluetoothctl`, parser, polling e funcoes auxiliares.
- `BluetoothInternal.hpp`: contrato interno do modulo Bluetooth.
- `Makefile`: build da biblioteca estatica e testes.
- `tests/test_bluetooth_parser.cpp`: testes do parser de eventos Bluetooth.

## Build e Testes

Compilar somente o backend:

```bash
make -C config-dacc
```

Rodar testes do backend:

```bash
make -C config-dacc test
```

Limpar artefatos:

```bash
make -C config-dacc clean
```

O build gera `libdacc-config.a`, linkado pela UI.

## Contrato Publico

Todo contrato publico fica em `functions.hpp`. Esse arquivo e a fronteira entre a UI e o backend.

### `system_result`

`system_result` e o contrato unico para operacoes de configuracao que podem falhar:

```cpp
struct system_result {
    bool ok = false;
    std::string mensagem;
    std::string codigo;
    std::string detalhes;
};
```

Campos:

- `ok`: `true` quando a operacao terminou com sucesso.
- `mensagem`: mensagem pronta para UI ou log de alto nivel.
- `codigo`: codigo estavel, usado para diferenciar tipos de falha.
- `detalhes`: saida bruta, stderr/stdout ou contexto tecnico.

Por que foi unificado:

- audio, video, Wi-Fi e Bluetooth tinham structs duplicadas com os mesmos campos;
- um contrato unico reduz repeticao e facilita evolucao;
- a UI pode ter tratamento uniforme para sucesso, erro e fallback;
- logs e mensagens ficam consistentes entre modulos.

Regras de uso:

- novas operacoes mutaveis devem retornar `system_result`;
- `codigo` deve ser estavel e legivel;
- `mensagem` deve ser compreensivel para usuario/desenvolvedor;
- `detalhes` deve conter informacao tecnica suficiente para debug;
- funcoes antigas que retornam `bool` podem existir por compatibilidade, mas a UI deve preferir `_result`.

### `command_result`

`command_result` representa o resultado bruto de um comando:

```cpp
struct command_result {
    bool ok = false;
    int exit_code = -1;
    std::string stdout_output;
    std::string stderr_output;
    std::string mensagem;
};
```

Ele e usado internamente para converter saida de ferramentas Linux em `system_result` ou em structs especializadas.

## Modulo Sistema e Execucao

Implementacao: `ConfigCommand.cpp` e parte de `HardwareControl.cpp`.

### `bool comando_existe(const std::string& cmd)`

Verifica se um comando esta disponivel no `PATH`.

Como funciona:

- le a variavel de ambiente `PATH`;
- percorre cada diretorio;
- monta o caminho completo;
- verifica se o arquivo existe e e executavel.

Por que existe:

- evita tentar usar ferramentas ausentes;
- permite escolher fallback;
- ajuda a gerar erros mais claros.

### `std::string exec_command(const char* cmd)`

Executa um comando textual e retorna a saida como string.

Uso esperado:

- comandos simples e controlados;
- consultas internas onde nao ha entrada de usuario.

Observacao:

- por receber string de comando, pode envolver shell;
- para argumentos variaveis, prefira `exec_command_args_result`.

### `command_result exec_command_result(const std::string& cmd)`

Executa um comando simples e retorna `command_result`.

Como funciona:

- captura stdout e stderr;
- preserva codigo de saida;
- preenche `ok` conforme sucesso do comando.

Uso esperado:

- leituras de estado;
- chamadas onde shell e aceitavel;
- comandos sem argumentos vindos diretamente da UI.

### `command_result exec_command_args_result(const std::vector<std::string>& args)`

Executa um comando com argumentos separados.

Como funciona:

- usa processo filho;
- chama `execvp`;
- passa argumentos como vetor, sem concatenar shell;
- captura saidas;
- interpreta status de saida.

Por que e importante:

- evita injecao de shell;
- lida melhor com SSIDs, nomes de conexao e caminhos com espacos;
- deve ser a escolha padrao para comandos com dados dinamicos.

### `long long obter_tempo_ms()`

Retorna tempo monotonicamente crescente em milissegundos.

Uso:

- timeouts;
- comparacao de intervalos;
- tarefas assicronas ou polling.

### `int obter_bateria()`

Le o nivel de bateria do sistema.

Como funciona:

- tenta ler `/sys/class/power_supply/BAT0/capacity`;
- retorna valor percentual quando disponivel;
- retorna `-1` quando nao ha bateria ou a leitura falha.

Por que retorna `-1`:

- desktops e algumas Stations podem nao ter bateria;
- a UI pode esconder o indicador ou mostrar estado desconhecido;
- falha de bateria nao deve derrubar a interface.

## Modulo Audio

Implementacao: `HardwareControl.cpp`.

### Struct `device_audio`

```cpp
struct device_audio {
    int id;
    std::string descricao;
    bool padrao;
};
```

Campos:

- `id`: identificador usado pelo sistema de audio.
- `descricao`: nome legivel do dispositivo.
- `padrao`: indica se e o dispositivo atual.

### `void aumentar_volume()`

Aumenta o volume em um incremento padrao.

Uso:

- atalhos de UI;
- controles fisicos;
- ajustes simples sem abrir a tela completa.

### `void diminuir_volume()`

Diminui o volume em um incremento padrao.

Mesma finalidade de `aumentar_volume`, mas no sentido inverso.

### `int obter_volume_atual()`

Consulta o volume atual.

Como funciona:

- tenta ler do sistema de audio disponivel;
- interpreta saidas de ferramentas como `wpctl`, `pactl` ou `amixer`;
- converte para percentual.

Por que o parsing e no backend:

- saidas variam por ferramenta;
- a UI so precisa de um numero;
- erros de ambiente ficam isolados.

### `void definir_volume(int valor_int)`

Define o volume em percentual.

Comportamento:

- recebe valor inteiro;
- limita a faixa esperada;
- tenta aplicar usando a ferramenta disponivel.

Estratégia:

- prioriza ferramentas modernas quando possivel;
- usa fallback para manter compatibilidade com maquinas diferentes.

### `std::vector<device_audio> listar_dispositivos_audio()`

Lista saidas de audio disponiveis.

Como funciona:

- consulta o sistema de audio;
- identifica dispositivos e descricoes;
- marca o dispositivo padrao.

Uso na UI:

- preencher a lista de dispositivos na janela Audio/Video;
- permitir troca de saida sem expor comandos ao usuario.

### `void selecionar_dispositivo_audio(int id)`

Seleciona dispositivo de audio e ignora detalhes do resultado.

Motivo de existir:

- compatibilidade com chamadas antigas;
- uso simples quando a chamada nao precisa exibir erro.

Preferencia:

- em UI nova, use `selecionar_dispositivo_audio_result`.

### `system_result selecionar_dispositivo_audio_result(int id)`

Seleciona dispositivo de audio e retorna sucesso/falha detalhado.

Retornos comuns:

- `ok`: dispositivo definido.
- `audio_select_failed`: falha ao definir dispositivo.

Uso na UI:

- aplicar alteracoes e mostrar mensagem clara em caso de erro.

### `void imprimir_dispositivos_audio()`

Funcao auxiliar para debug via terminal.

Nao deve ser usada como fonte de dados da UI.

## Modulo Video

Implementacao: `HardwareControl.cpp`.

### Structs

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

`DisplayMode` descreve uma resolucao/taxa disponivel. `DisplayOutput` descreve uma saida fisica/logica de video.

### `std::string obter_tipo_sessao()`

Detecta o tipo de sessao grafica.

Fontes comuns:

- `XDG_SESSION_TYPE`
- variaveis de ambiente relacionadas a Wayland/X11

Por que importa:

- X11 e Wayland usam ferramentas diferentes;
- escala em GNOME tem caminho proprio;
- aplicar comando errado pode falhar silenciosamente.

### `void verificarSessao()`

Funcao de apoio/debug para inspecionar a sessao atual.

### `std::vector<DisplayOutput> obter_info_displays()`

Retorna monitores conectados, modos e escala atual quando possivel.

Uso:

- preencher opcoes de resolucao na UI;
- saber qual modo esta ativo;
- evitar apresentar resolucoes inexistentes.

### `void listar_resolucao()`

Funcao auxiliar para imprimir resolucoes no terminal.

### `bool alterarResolucao(...)`

Aplica resolucao e retorna apenas sucesso/falha.

Existe por compatibilidade. Para UI, prefira `alterarResolucao_result`.

### `system_result alterarResolucao_result(...)`

Aplica resolucao com retorno detalhado.

Parametros:

- `saida`: nome do monitor/saida.
- `width`: largura.
- `height`: altura.
- `rate`: taxa de atualizacao desejada.

Comportamento:

- monta comando conforme sessao;
- executa com argumentos separados quando aplicavel;
- retorna erro mapeado se a ferramenta falhar.

### `bool alterarEscala(...)`

Aplica escala e retorna apenas booleano.

Existe por compatibilidade.

### `system_result alterarEscala_result(...)`

Aplica escala com retorno detalhado.

Como decide:

- GNOME pode usar `gsettings`;
- X11 pode usar `xrandr`;
- Wayland/wlroots pode usar `wlr-randr`.

Por que escala e delicada:

- compositores diferentes usam modelos diferentes;
- nem toda sessao permite escala por saida;
- falhas precisam ser comunicadas sem travar a UI.

### `void aumentar_brilho()` e `void diminuir_brilho()`

Ajustam brilho por incremento.

Dependem de ferramentas/ambiente disponiveis.

## Modulo Rede

Implementacao: `HardwareControl.cpp`.

### Struct `wifi_network`

```cpp
struct wifi_network {
    std::string ssid;
    int sinal;
    std::string seguranca;
    bool em_uso;
};
```

Campos:

- `ssid`: nome da rede.
- `sinal`: intensidade do sinal.
- `seguranca`: tipo de seguranca informado pelo NetworkManager.
- `em_uso`: indica se e a rede atual.

### Struct `wifi_adapter_status`

```cpp
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
```

Uso:

- informar estado do radio Wi-Fi;
- indicar se ha adaptador disponivel;
- propagar estado de conexao cabeada para a UI de rede.

### Struct `network_connection_status`

```cpp
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

Uso:

- representar a conexao primaria;
- manter detalhes separados de Wi-Fi e Ethernet;
- permitir que `SystemStatus` e `JanelaRede` tomem decisoes leves.

### `void listar_wifi()`

Lista redes no terminal.

Funcao auxiliar/debug. Para UI, use `listar_wifi_parsed`.

### `std::vector<wifi_network> listar_wifi_parsed()`

Escaneia redes Wi-Fi e retorna structs.

Como funciona:

- chama `nmcli` em modo tabular;
- usa parser que respeita caracteres escapados;
- trata SSIDs com `:` corretamente;
- preenche sinal, seguranca e indicador de rede atual.

Por que o parser e necessario:

- `nmcli -t` separa campos por `:`;
- SSIDs tambem podem conter `:`;
- parsing simples por split quebraria nomes reais de rede.

### `wifi_adapter_status obter_status_wifi()`

Retorna estado do Wi-Fi e informacoes de conexao.

Inclui:

- radio ligado/desligado;
- disponibilidade;
- conexao Wi-Fi;
- conexao cabeada;
- nomes de dispositivos e conexoes.

### `network_connection_status obter_status_conexao_rede()`

Detecta conexao de rede ativa.

Como funciona:

- consulta `nmcli device status`;
- identifica dispositivos `wifi` e `ethernet`;
- marca Ethernet como conexao primaria quando conectada;
- ainda preserva dados de Wi-Fi se existir.

Por que Ethernet tem prioridade:

- se cabo esta conectado, a rede ja esta funcional;
- scan Wi-Fi deixa de ser necessario para a experiencia principal;
- a UI pode renderizar painel mais leve.

### `bool wifi_conectado()`

Retorna se ha Wi-Fi conectado.

Observacao:

- nao significa que ha qualquer rede;
- para saber se ha cabo ou rede primaria, use `obter_status_conexao_rede`.

### `system_result definir_estado_wifi_result(bool ligar)`

Liga ou desliga o radio Wi-Fi.

Retornos comuns:

- `ok`
- `wifi_toggle_failed`

Depois de ligar Wi-Fi, a UI pode disparar novo sync para recarregar redes.

### `void conectar_wifi(...)`

Conecta a uma rede e descarta detalhes.

Funcao de compatibilidade.

### `system_result conectar_wifi_result(...)`

Conecta a uma rede Wi-Fi.

Parametros:

- `ssid`: nome da rede.
- `senha`: senha; pode ser vazia para rede aberta ou rede ja conhecida.

Mapeamento de falhas:

- autenticacao incorreta;
- rede indisponivel;
- Wi-Fi desligado;
- falha generica do NetworkManager.

### `void desconectar_wifi(...)`

Desconecta e descarta detalhes.

Funcao de compatibilidade.

### `system_result desconectar_wifi_result(...)`

Derruba uma conexao Wi-Fi pelo identificador/nome da conexao.

Uso:

- tela de rede;
- acoes administrativas futuras.

## Modulo Bluetooth

Implementacoes:

- `BluetoothControl.cpp`
- `BluetoothInternal.cpp`
- `BluetoothInternal.hpp`

Bluetooth e o modulo mais sensivel porque `bluetoothctl` nao foi desenhado como API de maquina. Ele e uma ferramenta interativa, com prompt, eventos assicronos e codigos ANSI.

### Struct `device_bt`

```cpp
struct device_bt {
    std::string mac;
    std::string nome;
    std::string icon;
    bool conectado = false;
    bool pareado = false;
    bool confiavel = false;
};
```

Campos:

- `mac`: endereco do dispositivo.
- `nome`: nome exibido ao usuario.
- `icon`: categoria informada pelo BlueZ.
- `conectado`: estado atual.
- `pareado`: se ja foi pareado.
- `confiavel`: se foi marcado como trusted.

### Struct `bluetooth_adapter_status`

```cpp
struct bluetooth_adapter_status {
    bool powered = false;
    bool soft_blocked = false;
    bool hard_blocked = false;
    bool controller_disponivel = true;
    std::string show_output;
    std::string rfkill_output;
};
```

Representa o estado do adaptador e bloqueios `rfkill`.

### Struct `bluetooth_state_snapshot`

```cpp
struct bluetooth_state_snapshot {
    bluetooth_adapter_status adapter;
    std::vector<device_bt> conhecidos;
    std::vector<device_bt> pareados;
};
```

Snapshot tecnico do estado conhecido.

### Struct `bluetooth_ui_snapshot`

```cpp
struct bluetooth_ui_snapshot {
    bluetooth_adapter_status adapter;
    std::vector<device_bt> desconhecidos_conectados;
    std::vector<device_bt> pareados;
    std::vector<device_bt> escaneados_filtrados;
    bool vindo_do_cache = false;
    std::string erro_codigo;
    std::string erro_mensagem;
    std::string erro_detalhes;
};
```

Snapshot orientado para UI.

Por que existe:

- a tela nao precisa conhecer todos os detalhes do BlueZ;
- evita scans repetidos;
- separa listas que tem comportamento visual diferente;
- permite mostrar estado de erro sem quebrar renderizacao.

### `bool obter_estado_bluetooth()`

Retorna se Bluetooth esta ligado.

Uso:

- indicadores simples;
- compatibilidade.

### `bluetooth_adapter_status obter_status_bluetooth()`

Consulta status do adaptador.

Como funciona:

- executa `bluetoothctl show`;
- consulta `rfkill`;
- detecta ausencia de controller;
- diferencia bloqueio fisico e logico.

### `bluetooth_state_snapshot obter_estado_bluetooth_completo()`

Retorna adaptador, conhecidos e pareados.

Uso:

- sincronizacao interna;
- base para snapshots mais amigaveis.

### `bluetooth_ui_snapshot obter_estado_bluetooth_ui()`

Retorna snapshot leve para UI sem necessariamente fazer scan novo.

Importancia:

- evita travar tela;
- permite abrir configuracao rapidamente;
- usa cache quando adequado.

### `bluetooth_ui_snapshot obter_estado_bluetooth_ui_com_scan(int segundos)`

Retorna snapshot para UI com scan ativo por tempo definido.

Uso:

- botao de buscar dispositivos;
- atualizacao manual;
- descoberta de novos dispositivos.

### `bool definir_estado_bt(bool ligar)`

Liga/desliga Bluetooth e retorna apenas booleano.

Funcao de compatibilidade.

### `system_result definir_estado_bt_result(bool ligar)`

Liga/desliga Bluetooth com retorno detalhado.

Fluxo ao ligar:

1. tenta desbloquear via `rfkill`;
2. valida adaptador;
3. envia `power on`;
4. verifica saida;
5. retorna `system_result`.

Fluxo ao desligar:

1. tenta validar quando necessario;
2. envia `power off`;
3. retorna sucesso/falha.

### `std::vector<device_bt> listar_dispositivos_bluetooth_conhecidos()`

Lista dispositivos conhecidos pelo BlueZ.

### `std::vector<device_bt> listar_dispositivos_bluetooth_pareados()`

Lista dispositivos pareados.

### `std::vector<device_bt> scan_dispositivos_bluetooth(int segundos)`

Escaneia dispositivos por um periodo.

Como funciona:

- abre sessao `bluetoothctl`;
- liga scan;
- coleta eventos;
- parseia stream;
- desliga scan;
- retorna lista ordenada/filtrada.

### `system_result parear_bluetooth(const std::string& mac)`

Pareia um dispositivo.

Fluxo:

1. garante Bluetooth desbloqueado;
2. valida adaptador;
3. prepara agent;
4. executa `pair`;
5. verifica sucesso/falha;
6. retorna mensagem normalizada.

### `system_result confiar_bluetooth(const std::string& mac)`

Marca dispositivo como confiavel (`trust`).

Por que e importante:

- permite reconexao automatica;
- reduz necessidade de repetir pareamento.

### `system_result parear_confiar_conectar_bluetooth(const std::string& mac)`

Fluxo completo para dispositivos novos:

1. parear;
2. confiar;
3. conectar.

Se qualquer etapa falhar, retorna o `system_result` daquela etapa.

### `system_result conectar_bluetooth_result(const std::string& mac)`

Conecta dispositivo pareado/conhecido.

Possui tratamento para pareamento obsoleto:

- identifica falhas de autenticacao ou chave antiga;
- remove dispositivo antigo quando seguro;
- tenta parear/confiar/conectar novamente.

### `bool conectar_bluetooth(const std::string& mac)`

Wrapper booleano para compatibilidade.

### `system_result desconectar_bluetooth_result(const std::string& mac)`

Desconecta dispositivo.

Tratamentos:

- dispositivo ja desconectado;
- dispositivo indisponivel;
- timeout de operacao;
- falha generica do `bluetoothctl`.

### `bool desconectar_bluetooth(const std::string& mac)`

Wrapper booleano para compatibilidade.

### `system_result remover_bluetooth(const std::string& mac)`

Remove dispositivo do BlueZ.

Fluxo:

1. consulta estado inicial;
2. se conectado, tenta desconectar;
3. executa `remove`;
4. verifica saida;
5. retorna `system_result`.

### `void parsing_bluetooth_stream(...)`

Parser central de eventos do Bluetooth.

Assinatura:

```cpp
void parsing_bluetooth_stream(
    std::istream& input,
    std::unordered_map<std::string, device_bt>& mapa,
    std::string& ultimo_mac_context
);
```

Responsabilidades:

- remover codigos ANSI;
- detectar linhas com MAC;
- manter contexto do ultimo dispositivo;
- preencher nome, icon, conectado, pareado e confiavel;
- lidar com eventos parciais emitidos durante scan/conexao.

Por que recebe `ultimo_mac_context`:

- `bluetoothctl` frequentemente imprime propriedades em linhas seguintes;
- nem toda linha repete o MAC;
- o parser precisa saber a qual dispositivo aquela propriedade pertence.

## Internals do Bluetooth

### `forkpty`

O backend usa PTY porque `bluetoothctl` espera terminal interativo. Sem PTY, algumas operacoes podem nao emitir eventos do mesmo jeito ou podem se comportar de forma diferente.

### Marcadores de sucesso/falha

Operacoes interativas usam listas de marcadores:

- sucesso: textos esperados na saida;
- falha: textos que indicam erro conhecido.

Isso permite encerrar operacoes antes do timeout quando o resultado ja e conhecido.

### Polling de estado

Depois de conectar/desconectar/parear, o hardware pode demorar alguns instantes para refletir estado final. O backend consulta o dispositivo algumas vezes antes de declarar falha definitiva.

## Relacao com a UI

As telas da UI que consomem diretamente `config-dacc` sao:

- `JanelaRede`
- `JanelaBluetooth`
- `JanelaAudioEVideo`
- `SystemStatus`

Regras:

- UI deve consumir structs e `system_result`;
- UI nao deve parsear `nmcli`, `bluetoothctl`, `wpctl` ou `xrandr`;
- operacoes lentas devem acontecer em tarefa de segundo plano na UI;
- backend deve retornar detalhes suficientes para debug;
- mensagens exibidas podem usar `mensagem`, com fallback local se vazia.

## Codigos de Erro

Os codigos nao sao uma enum formal, mas devem ser tratados como contrato estavel.

Padroes usados:

- `ok`
- `audio_device_not_found`
- `audio_select_failed`
- `display_output_not_found`
- `display_scale_failed`
- `display_resolution_failed`
- `wifi_auth_failed`
- `wifi_not_found`
- `wifi_disabled`
- `network_manager_unavailable`
- `wifi_toggle_failed`
- `wifi_connect_failed`
- `wifi_disconnect_failed`
- `adapter_unavailable`
- `adapter_blocked`
- `adapter_hard_blocked`
- `adapter_soft_blocked`
- `rfkill_unblock_failed`
- `power_failed`
- `pair_failed`
- `pair_not_reflected`
- `trust_failed`
- `trust_not_reflected`
- `connect_failed`
- `connect_not_reflected`
- `disconnect_failed`
- `disconnect_not_reflected`
- `remove_failed`
- `remove_not_reflected`
- `device_unavailable`
- `device_not_connected`
- `authentication_failed`
- `operation_timeout`
- `bluetoothctl_timeout`
- `bluetoothctl_failed`
- `bluetoothctl_signaled`
- `forkpty_failed`
- `write_failed`
- `read_failed`
- `stale_pairing_remove_failed`

Ao adicionar novos codigos:

- use snake_case;
- prefira nomes especificos;
- mantenha compatibilidade com a UI;
- documente quando a UI precisar comportamento especial.

## Guia para Novas Funcoes

Ao adicionar uma nova funcao de configuracao:

1. Declare o contrato em `functions.hpp`.
2. Se a operacao puder falhar, retorne `system_result`.
3. Use `exec_command_args_result` quando houver argumentos dinamicos.
4. Coloque parsing no backend, nao na UI.
5. Retorne structs tipadas para listas/estado.
6. Preencha `codigo`, `mensagem` e `detalhes`.
7. Adicione teste se houver parser ou regra nao trivial.
8. Rode `make -C config-dacc` e `make test`.
9. Rode build limpo da UI se `functions.hpp` mudou.

## Validacao Recomendada

Depois de alterar este modulo:

```bash
make -C config-dacc
make -C config-dacc test
make -C ui clean
make
./scripts/start_station.sh
```

Checklist manual:

- abrir tela de rede;
- testar modo cabeado;
- testar Wi-Fi quando disponivel;
- abrir Audio/Video;
- abrir Bluetooth;
- executar scan Bluetooth curto;
- confirmar que a UI nao trava durante operacoes lentas;
- verificar logs em caso de erro.

## Principios de Manutencao

- Contratos publicos devem ser pequenos e estaveis.
- Parsing de ferramentas externas deve ficar perto da chamada que gerou a saida.
- A UI deve receber dados prontos para renderizar.
- Erros esperados de hardware nao devem virar excecoes fatais.
- Fallbacks sao aceitaveis quando preservam funcionamento em hardware variado.
- Detalhes tecnicos devem ir para `detalhes`, nao para texto principal de UI.
- Mudancas em Bluetooth devem ser testadas com cuidado, porque o hardware responde de forma assíncrona.
