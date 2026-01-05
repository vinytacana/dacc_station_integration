# Diagrama de Classes do Sistema de Log

Este documento contém um diagrama de classes UML, gerado com Mermaid, que descreve a arquitetura do sistema de log. O diagrama foi baseado na análise dos arquivos-fonte localizados no diretório `logs/`.

## Diagrama UML

O diagrama abaixo ilustra as principais classes, interfaces e suas relações, como herança, composição e dependência.

```mermaid
classDiagram
    direction TD

    %% --- Client Side ---
    namespace Client {
        class ILogger {
            <<Interface>>
            +log(message: string)* void
        }

        class CompositeLogger {
            -primary_logger_: unique_ptr~ILogger~
            -fallback_logger_: unique_ptr~ILogger~
            +log(message: string) void
        }

        class SocketLogger {
            -_socket_client_: unique_ptr~UnixSocketClient~
            +log(message: string) void
        }

        class FallbackLogger {
            -console_writer_: ConsoleWriter
            -file_writer_: FileWriter
            +log(message: string) void
        }
    }

    %% --- Server & Writers ---
    namespace ServerAndWriters {
        class ILogWriter {
            <<Interface>>
            +write(message: string)* void
        }

        class FileWriter {
            -file_: ofstream
            +write(message: string) void
        }

        class ConsoleWriter {
            +write(message: string) void
        }

        class LogServer {
            +start() void
        }
    }

    %% --- Utilities ---
    namespace Utilities {
        class UnixSocketClient {
            -fd_: int
            -mtx_: mutex
            +send(data: string) void
        }

        class LogManager {
            <<static>>
            +logError(context: string, includeErrno: bool) void
        }
    }

    %% --- Notes (fora do namespace para evitar parse issues) ---
    note for ServerAndWriters.LogServer "Main server process. Uses an ILogWriter implementation."

    %% --- Relationships (sempre fora dos namespaces) ---
    Client.ILogger <|-- Client.CompositeLogger
    Client.ILogger <|-- Client.SocketLogger
    Client.ILogger <|-- Client.FallbackLogger
    Client.CompositeLogger o-- Client.ILogger : manages

    ServerAndWriters.ILogWriter <|-- ServerAndWriters.FileWriter
    ServerAndWriters.ILogWriter <|-- ServerAndWriters.ConsoleWriter
    ServerAndWriters.LogServer ..> ServerAndWriters.ILogWriter : uses

    Client.SocketLogger --o Utilities.UnixSocketClient : uses
    Client.FallbackLogger --o ServerAndWriters.ConsoleWriter : uses
    Client.FallbackLogger --o ServerAndWriters.FileWriter : uses
    Utilities.UnixSocketClient ..> Utilities.LogManager : uses for error logging
    ServerAndWriters.FileWriter ..> Utilities.LogManager : uses for error logging


```
