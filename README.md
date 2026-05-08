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

## Como Compilar

O projeto utiliza `Makefiles` para gerenciamento de build. Certifique-se de ter as dependencias instaladas, como `g++`, `make`, SDL2 e suas extensoes (`SDL2_image`, `SDL2_ttf`, `SDL2_mixer`, `SDL2_gfx`).

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
