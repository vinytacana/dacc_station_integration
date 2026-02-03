# MÓDULO: LOGGING SYSTEM (dacc-logs)
# RESPONSÁVEL TÉCNICO: Feliph Lima

## 1. ESCOPO E OBJETIVO
Este diretório contém a biblioteca central de logging e o serviço de agregação de logs do DACC Station.
O objetivo é fornecer uma API unificada (`ILogger`) que permita que outros módulos (UI, Process Manager) enviem logs sem se preocupar com a escrita em disco ou concorrência.

**COMPONENTES CHAVE:**
1.  **Lib Client:** Linkada estaticamente nos outros módulos. Envia logs para um Socket ou Console.
2.  **Log Server:** Um processo daemon (opcional) que escuta em um Unix Domain Socket e centraliza a escrita em arquivo.
3.  **Writers:** Implementações de `ILogWriter` (Console, File, Socket).

---

## 2. TECH STACK & REQUISITOS
- **Linguagem:** C++ (Padrão C++17).
- **IPC:** Unix Domain Sockets (`sys/un.h`, `sys/socket.h`).
- **Concorrência:** `std::mutex`, `std::lock_guard` (Thread-Safety é mandatório).
- **Design Patterns:**
  - **Singleton:** Para o `LogManager`.
  - **Composite:** Para permitir múltiplos destinos (ex: Console + Arquivo ao mesmo tempo).
  - **Strategy:** Para os diferentes `ILogWriter`.

---

## 3. ARQUITETURA E REGRAS DE OURO

### Performance & Latência
- **Regra #1:** O cliente de log (que roda dentro do Jogo ou UI) deve ser o mais leve possível.
- **Regra #2:** Se o `SocketLogger` falhar ao conectar, ele deve degradar silenciosamente para `ConsoleWriter` ou `FallbackLogger`. Nunca deve crashar a aplicação principal.
- **Buffers:** Cuidado com alocações excessivas de `std::string`. Prefira passar referências `const std::string&` ou `std::string_view`.

### Estrutura de Mensagem
Padronize o formato da string antes de enviar para o socket:
`[TIMESTAMP] [LEVEL] [MODULE] Mensagem`

### Protocolo IPC
- Utilize sockets **STREAM** (`SOCK_STREAM`) para garantir a ordem, ou **DGRAM** se a performance for crítica e a perda de pacotes for aceitável (neste projeto, preferimos `SOCK_STREAM` com timeouts curtos).
- Caminho padrão do socket: `/tmp/dacc-station.sock` (Definido em headers comuns).

---

## 4. MODOS DE OPERAÇÃO (COMANDOS)

### Se eu digitar `/plan` (Modo Arquiteto):
- Foque na **Topologia**. Quem escreve onde?
- Analise riscos de **Deadlock** (ex: tentar logar dentro de um handler de sinal crítico).
- Verifique a estratégia de rotação de arquivos (log rotation).

### Se eu digitar `/code` (Modo Implementação):
- Garanta que todo destrutor (`~Class`) feche descritores de arquivo (RAII).
- Use `flock` ou mecanismos similares se múltiplos processos tentarem escrever no mesmo arquivo (caso não use o Log Server).
- Sempre envolva escritas de socket em blocos `try-catch` robustos.

### Se eu digitar `/test` (Modo QA):
- Sugira testes de carga: "O que acontece se 10 threads logarem ao mesmo tempo?"
- Teste a resiliência: "O que acontece se eu matar o processo do Log Server? O cliente trava?"

---

## 5. CONVENÇÕES DE CÓDIGO ESPECÍFICAS
1.  **Macros:** O uso de macros como `LOG_INFO("msg")` é permitido apenas se facilitar a captura de `__FILE__` e `__LINE__`.
2.  **Níveis de Log:** Implemente: `DEBUG`, `INFO`, `WARNING`, `ERROR`, `FATAL`.
3.  **Includes:** Evite incluir headers pesados no `ILogger.hpp`. Use *Forward Declarations* para manter o tempo de compilação baixo.