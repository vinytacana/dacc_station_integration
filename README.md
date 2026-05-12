# DACC Station (GameMan)

**DACC Station** e um projeto de console de jogos academico desenvolvido pelo Departamento de Ciencia da Computacao (DACC) da UNIR. Este repositorio contem o codigo-fonte dos principais componentes de software do console, incluindo a interface grafica, os servicos de execucao, o backend de configuracao do sistema e o sistema centralizado de logs.

## Estrutura do Projeto

* **`ui/`**: Interface grafica baseada em SDL2, responsavel pela navegacao, exibicao dos jogos, telas de configuracao e interacao com o usuario.
* **`config-dacc/`**: Biblioteca e CLI de configuracao do sistema, com funcoes para audio, video, rede, bateria e Bluetooth.
* **`process-manager/`**: Servico backend responsavel por iniciar jogos, acompanhar processos e se comunicar com a interface.
* **`logs/`**: Servico centralizado de logs usado pelos modulos principais.
* **`scripts/`**: Scripts de operacao do projeto, incluindo a inicializacao orquestrada da station.
* **`games/`**: Jogos, scripts e binarios usados pelo launcher.
* **`docs/`**: Documentacao geral, planos de trabalho e registros de implementacao.

## Pre-requisitos / Dependencias

### Dependencias de Build

Necessarias para executar `make` e compilar os componentes C++/SDL2:

* Compiladores C/C++: `gcc`, `g++` ou `gcc-c++`
* `make`
* SDL2 e extensoes: `SDL2`, `SDL2_image`, `SDL2_ttf`, `SDL2_gfx` e `SDL2_mixer`

Em distribuicoes Debian, Ubuntu e Raspberry Pi OS, os pacotes usados sao:

```bash
gcc g++ make libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-gfx-dev libsdl2-mixer-dev
```

Em Fedora, os pacotes usados sao:

```bash
gcc gcc-c++ make SDL2-devel SDL2_image-devel SDL2_ttf-devel SDL2_gfx-devel SDL2_mixer-devel
```

### Dependencias de Runtime

Necessarias no sistema operacional hospedeiro para que o `config-dacc` controle recursos de hardware quando eles estiverem disponiveis:

* Rede: `network-manager` / `NetworkManager` (`nmcli`)
* Bluetooth: `bluez`, `bluez-tools` quando aplicavel (`bluetoothctl`) e `rfkill`
* Audio: `pipewire`, `wireplumber`, `pulseaudio-utils` (`pactl`) e `alsa-utils` (`amixer`)
* Video: `x11-xserver-utils` ou `xorg-x11-server-utils` (`xrandr`) e `wlr-randr`
* Brilho: `brightnessctl`

## Como Instalar

Para preparar a maquina automaticamente em sistemas com `apt-get` ou `dnf`, execute:

```bash
./scripts/install_dependencies.sh
```

O script detecta o gerenciador de pacotes disponivel, instala as dependencias de build e runtime, e encerra com erro se alguma etapa de instalacao falhar.

## Como Compilar

O projeto utiliza `Makefiles` para gerenciamento de build. Antes de compilar, instale as dependencias com `./scripts/install_dependencies.sh` ou siga a lista de pacotes acima.

Para compilar todo o sistema:

```bash
make
```

Para limpar os arquivos de build:

```bash
make clean
```

Os binarios gerados ficam em `bin/`.

## Como Executar

Utilize o script de orquestracao para iniciar os servicos na ordem correta:

```bash
./scripts/start_station.sh
```

O script inicia o servidor de logs, o Process Manager e a interface grafica.

## Documentacao

* [README do config-dacc](config-dacc/README.md)
* [README da UI](ui/README)
* [Documentacao de assets da UI](ui/assets/README.md)
