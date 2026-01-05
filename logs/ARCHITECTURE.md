# Arquitetura do Sistema de Log

Este documento descreve a arquitetura do sistema de log, que é projetado para ser um serviço modular, configurável e resiliente.

## Visão Geral

O sistema de log opera em um modelo cliente-servidor, onde o `LogServer` é um processo daemon independente e outros componentes do sistema (como o `ProcessManager`) atuam como clientes. A comunicação é feita via Unix Domain Sockets para alta performance e segurança.

## Componentes do Servidor (`LogServer`)

O `LogServer` é o núcleo do sistema de log. Ele é um executável independente com as seguintes características:

### 1. Configurabilidade via JSON

-   O comportamento do servidor é definido pelo arquivo `logs/config.json`.
-   **Configurações:**
    -   `server.socket_path`: Define o caminho no sistema de arquivos para o Unix Socket onde o servidor escutará por conexões.
    -   `output.type`: Determina o destino dos logs. Os valores suportados são `"file"` e `"console"`.
    -   `output.filepath`: Especifica o nome do arquivo de log quando `type` é `"file"`.

### 2. Arquitetura de Saída Modular (`ILogWriter`)

Para permitir diferentes destinos de log, o servidor utiliza uma arquitetura baseada em interfaces:

-   **`ILogWriter` (Interface):** Define o contrato para qualquer classe de escrita de log, com um único método `write(const std::string& message)`.
-   **`FileWriter` (Implementação):** Uma classe que implementa `ILogWriter` e escreve as mensagens de log para um arquivo especificado.
-   **`ConsoleWriter` (Implementação):** Uma classe que implementa `ILogWriter` e escreve as mensagens de log para a saída padrão (`std::cout`).

Ao iniciar, o `LogServer` lê o `config.json` e instancia a implementação `ILogWriter` apropriada, desacoplando a lógica do servidor da lógica de escrita.

### 3. Desligamento Gracioso (Graceful Shutdown)

-   O servidor captura os sinais `SIGINT` (Ctrl+C) e `SIGTERM` (`kill`).
-   Ao receber um desses sinais, ele sai do loop principal de forma controlada.
-   **Limpeza de Recursos:** Antes de terminar, o servidor fecha o file descriptor do socket e remove o arquivo de socket do sistema de arquivos (`unlink`). Isso previne erros de "Address already in use" em reinicializações.
-   **Loop de Eventos Não-Bloqueante:** Utiliza `epoll` com um timeout para esperar por novas conexões. Isso permite que o servidor responda a sinais de desligamento mesmo quando está ocioso, sem consumir CPU.

## Componentes do Cliente

O lado do cliente também é projetado para ser modular e resiliente, seguindo o Princípio da Inversão de Dependência.

### 1. `ILogger` (A Abstração do Cliente)

-   **Responsabilidade:** Definir o "contrato" de um logger do ponto de vista do cliente. É uma interface com uma única função virtual pura: `log(const std::string& message)`.
-   **Propósito:** Desacoplar os componentes de alto nível (como o `ProcessManager`) dos detalhes de implementação do log. O `ProcessManager` depende desta interface, não de uma classe concreta.

### 2. `SocketLogger` (A Implementação Primária)

-   **Responsabilidade:** Implementar a interface `ILogger` para se comunicar com o `LogServer` via Unix Socket.
-   **Composição:** Utiliza internamente a classe `UnixSocketClient`, que encapsula toda a lógica de baixo nível do socket (conexão, envio, etc.), seguindo o Princípio da Responsabilidade Única.

### 3. `FallbackLogger` (A Implementação de Contingência)

-   **Responsabilidade:** Implementar a interface `ILogger` para fornecer um mecanismo de log local e robusto quando o `LogServer` está indisponível.
-   **Funcionalidade:** Escreve mensagens de log tanto para o console (`stderr`) quanto para um arquivo de log local (ex: `pm_fallback.log`). Não possui dependências de rede.

### 4. `CompositeLogger` (O Padrão de Resiliência)

-   **Responsabilidade:** Implementar a interface `ILogger` e gerenciar a lógica de qual logger utilizar.
-   **Funcionalidade:** Contém uma instância do logger primário (`SocketLogger`) e do de fallback (`FallbackLogger`). Ele sempre tenta usar o primário. Se o primário não estiver disponível (falhou ao conectar), ele utiliza o de fallback automaticamente.

### Fluxo de Controle: Injeção de Dependência

-   O `ProcessManager` (e outros futuros serviços) não criam seu próprio logger.
-   O ponto de entrada da aplicação (`main`) é responsável por:
    1.  Criar o `FallbackLogger`.
    2.  Tentar criar o `SocketLogger`.
    3.  Criar um `CompositeLogger` com os dois anteriores.
    4.  "Injetar" o `CompositeLogger` no construtor do `ProcessManager`.

Isso garante que o `ProcessManager` sempre tenha um logger funcional, e que a falha do serviço de log não cause uma falha em cascata na aplicação.