# Documentação Técnica: DACC Config Backend

Este diretório contém a biblioteca `dacc-config`, responsável por abstrair a complexidade do hardware Linux para a interface do usuário. O objetivo principal é fornecer uma API C++ segura que gerencie Áudio, Vídeo, Wi-Fi e Bluetooth sem que o desenvolvedor da UI precise conhecer comandos de terminal ou lidar com parsing de texto bruto.

---

## 🏗 Arquitetura Geral e Tecnologias

O backend opera como uma camada de tradução. Ele utiliza chamadas de sistema para invocar utilitários padrão do Linux, captura a saída de texto dessas ferramentas, processa essa informação (Parsing) e a devolve em estruturas de dados (Structs) organizadas.

### Ferramentas de Base:
- **Execução:** `forkpty` (para emular terminais), `popen` e `execvp`.
- **Áudio:** `WirePlumber` (`wpctl`), `PulseAudio` (`pactl`) e `ALSA` (`amixer`).
- **Rede:** `NetworkManager` (`nmcli`).
- **Bluetooth:** `BlueZ` (`bluetoothctl`).
- **Vídeo:** `X11` (`xrandr`) e `Wayland` (`wlr-randr`).

---

## 🛠 Módulo: Sistema e Execução (`ConfigCommand.cpp`)

Este é o módulo base. Todas as outras funções dependem dele para interagir com o sistema operacional.

### Funções:

#### `bool comando_existe(const std::string& cmd)`
- **O que faz:** Verifica se um programa está instalado e disponível no sistema.
- **Detalhes:** Ela percorre as pastas listadas na variável de ambiente `PATH` (como `/usr/bin`, `/usr/local/bin`) e verifica se o arquivo existe e é executável.
- **Por que é importante:** Antes de tentar mudar o volume ou ligar o Wi-Fi, o sistema verifica se as ferramentas necessárias estão lá para evitar "crashes".

#### `command_result exec_command_result(const std::string& cmd)`
- **O que faz:** Executa um comando simples de shell e captura tudo o que ele imprime na tela.
- **Lógica:** Usa `popen` com redirecionamento `2>&1` (para capturar erros também).
- **Uso:** Ideal para comandos rápidos e informativos, como verificar o nível da bateria.

#### `command_result exec_command_args_result(const std::vector<std::string>& args)`
- **O que faz:** Executa um comando passando uma lista de argumentos de forma isolada.
- **Lógica:** Esta é a função mais segura. Ela usa `fork()` para criar um novo processo e `execvp()` para rodar o comando. Isso impede "Injeção de Shell" (onde caracteres maliciosos como `;` ou `&` poderiam executar comandos indesejados).
- **Tratamento de Erros:** Captura o código de saída (Exit Code). Se for `0`, a operação foi um sucesso.

#### `int obter_bateria()`
- **O que faz:** Lê o status da bateria diretamente do kernel Linux.
- **Detalhes:** Ele tenta abrir os arquivos em `/sys/class/power_supply/BAT0/capacity`. É muito mais rápido do que rodar um comando externo.

---

## 🔊 Módulo: Áudio (`HardwareControl.cpp`)

Gerencia o sistema de som de forma agnóstica (funciona em quase qualquer distribuição Linux).

### Funções:

#### `void definir_volume(int valor_int)`
- **O que faz:** Define o volume de 0 a 100%.
- **Diferencial:** Ela é "multi-sistema". Tenta primeiro via `wpctl` (moderno), se falhar tenta `pactl` e por último `amixer`. Isso garante que o som funcione tanto no Raspberry Pi quanto em um PC Desktop.

#### `std::vector<device_audio> listar_dispositivos_audio()`
- **O que faz:** Retorna todos os fones de ouvido, caixas de som e saídas HDMI disponíveis.
- **Lógica:** Processa a saída do `wpctl status`. Ela identifica qual dispositivo tem um asterisco (`*`), marcando-o como o dispositivo atual/padrão no sistema.

---

## 🌐 Módulo: Wi-Fi (`HardwareControl.cpp`)

Interface direta com o `NetworkManager`.

### Funções:

#### `std::vector<wifi_network> listar_wifi_parsed()`
- **O que faz:** Escaneia o ar em busca de redes Wi-Fi.
- **Complexidade:** Usa o comando `nmcli -t` (modo tabular). O desafio aqui é que nomes de redes Wi-Fi podem conter dois pontos (`:`), o que quebraria um parser simples.
- **Solução:** Criamos a função `split_nmcli_escaped_fields` que entende caracteres de escape (`\:`), garantindo que nomes de redes complexos sejam lidos corretamente.

#### `system_result conectar_wifi_result(const string &ssid, const string &senha)`
- **O que faz:** Tenta realizar a autenticação em uma rede.
- **Tratamento de Erros:** A função não apenas diz "falhou", ela analisa a mensagem do sistema para dizer se o erro foi "Senha Incorreta", "Rede Fora de Alcance" ou "Wi-Fi Desativado".

---

## 🔵 Módulo: Bluetooth (`BluetoothInternal.cpp` & `BluetoothControl.cpp`)

Este é o módulo mais complexo do projeto, pois o `bluetoothctl` é uma ferramenta feita para humanos digitarem, não para programas lerem.

### 🧠 O Grande Desafio: O Parser de Stream

Diferente de outros comandos que você roda e eles terminam, o Bluetooth precisa de uma **sessão interativa**.

#### 1. Emulação de Terminal (`forkpty`)
Para que o `bluetoothctl` aceite comandos, ele precisa achar que está em um terminal real. Usamos `forkpty` para criar um terminal virtual (PTY). Isso nos permite enviar comandos como `scan on` e receber o fluxo de dados constante de volta.

#### 2. A Função `parsing_bluetooth_stream` (A Joia da Coroa)
Esta função lê cada linha que o Bluetooth "cospe" no terminal.
- **Remoção de ANSI:** O Bluetooth envia cores (códigos como `\033[32m`). O parser limpa tudo isso para ter apenas texto puro.
- **Contexto de MAC:** Quando o sistema diz `Device 00:11:22... Connected: yes`, o parser guarda esse endereço MAC. Se a próxima linha disser apenas `Name: Meu Fone`, o parser sabe que esse nome pertence ao MAC da linha anterior.
- **Identificação de Propriedades:** Ele identifica palavras-chave como `Paired`, `Trusted`, `Connected`, `Icon` e `RSSI` para preencher a estrutura `device_bt`.

#### 3. Sincronização e Polling
Como o Bluetooth é lento (hardware físico), criamos a função `aguardar_estado_dispositivo`. Ela envia um comando (ex: conectar) e fica "vigiando" o dispositivo por alguns segundos até que o parser confirme que o estado realmente mudou no hardware.

### Funções Principais de Bluetooth:

- `scan_dispositivos_bluetooth(segundos)`: Abre a sessão, liga o scan, coleta tudo o que o parser encontrar e fecha a sessão.
- `parear_bluetooth(mac)`: Lida com a segurança. Ele primeiro desbloqueia o adaptador, tenta o pareamento e depois marca o dispositivo como "Confiavel" (Trusted) para que ele reconecte automaticamente no futuro.
- `definir_estado_bt_result(bool ligar)`: Gerencia o rádio Bluetooth. Se for ligar, ele também verifica se não há um bloqueio físico (`rfkill`) impedindo a ativação.

---

## 🖥 Módulo: Vídeo (`HardwareControl.cpp`)

#### `system_result alterarEscala_result(const string &saida, float escala)`
- **O que faz:** Altera o tamanho da interface (Zoom).
- **Lógica:** No Linux, isso muda dependendo do ambiente. No GNOME, usamos `gsettings`. Em outros (como o DACC Station puro), usamos `xrandr` ou `wlr-randr`. A função detecta automaticamente qual tecnologia usar através da variável `XDG_SESSION_TYPE`.

---

## 📝 Como contribuir ou estender

1.  **Novas Estruturas:** Sempre adicione novas structs em `functions.hpp`.
2.  **Novos Comandos:** Se precisar de uma nova ferramenta, use `exec_command_args_result` para segurança.
3.  **Logs:** O backend está preparado para retornar mensagens detalhadas em `system_result`. Sempre preencha o campo `detalhes` com a saída bruta do erro para facilitar o debug na UI.

---
*Documento gerado para a equipe de desenvolvimento do DACC Station.*
