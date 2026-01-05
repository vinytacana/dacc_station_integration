# DACC Station (GameMan)

**DACC Station** é um projeto de console de jogos acadêmico desenvolvido pelo Departamento de Ciência da Computação (DACC) da UNIR. Este repositório contém o código fonte para o hardware e software do console.

## Estrutura do Projeto

*   **`ui/`**: Interface Gráfica baseada em SDL2.
*   **`process-manager/`**: Serviço backend responsável por gerenciar a execução dos jogos.
*   **`logs/`**: Serviço centralizado de logs.
*   **`docs/`**: Documentação geral e relatórios de implementação.

## Como Compilar

O projeto utiliza `Makefiles` para gerenciamento de build. Certifique-se de ter as dependências instaladas (SDL2, g++, etc.).

Para compilar todo o sistema:

```bash
make
```

Para limpar os arquivos de build:

```bash
make clean
```

## Como Executar

Utilize o script de orquestração para iniciar todos os serviços na ordem correta:

```bash
./start_station.sh
```

## Documentação Recente

*   [Relatório de Implementação (Integração UI-PM)](docs/IMPLEMENTATION_REPORT.md) - Detalhes sobre a comunicação via Sockets e atualização de dados.
*   [Plano de Integração Gamescope](process-manager/docs/GAMESCOPE_INTEGRATION.md) - Roteiro para o uso do micro-compositor.
*   [GEMINI.md](GEMINI.md) - Contexto geral para assistentes de IA.
