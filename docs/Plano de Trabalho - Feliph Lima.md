### **Programa Institucional de Bolsas de Extensão e de Cultura**

### **PLANO INDIVIDUAL DE TRABALHO**

### **DISCENTE: FELIPH DE MATOS MACÊDO LIMA**

### **CÓDIGO:** PJ053-2025

### **TÍTULO DA AÇÃO:** _DACC Station: Produção de Hardware e Software para o Desenvolvimento de um Console de Jogos_

### **PERÍODO DO PROJETO:** 01/02/2025 a 31/01/2026

### **ORIENTADOR(A):** Valmir Batista Prestes de Souza

### **PERÍODO DO PLANO:** 01/02/2025 a 31/01/2026

### **LOCAL DE TRABALHO:** Remoto e no Laboratório do Grupo de Pesquisa em Segurança, Algoritmos, Computação e Inovação na Universidade Federal de Rondônia, sala 102, bloco 2J

#### **Justificativa**

O Curso de Ciência da Computação promove atividades práticas por meio de metodologias que incentivam projetos dinâmicos, como jogos digitais. No entanto, muitos desses projetos ficam inacessíveis após a apresentação numa disciplina, projeto de pesquisa/extensão, sem uma forma eficaz de exposição permanente. A dificuldade em preparar ambientes para eventos limita o impacto dos trabalhos dos estudantes tanto para comunidade interna como externa.

O projeto DACC Station surge como uma oportunidade para conectar teoria e prática, preservando e exibindo os projetos desenvolvidos pelos discentes. Com a criação de um console de jogos que utiliza microcomputadores, modelagem 3D, eletrônica e desenvolvimento de jogos, o DACC Station oferece um ambiente de aprendizado colaborativo e acessível.

Nessa perspectiva, os discentes poderão estudar o ciclo de desenvolvimento de software, integrando conhecimentos em programação, sistemas operacionais e design de hardware. Essa experiência é essencial para uma formação completa, preparando-os para as demandas do mercado de trabalho, onde a integração entre hardware e software é cada vez mais exigida.

Logo, se faz necessário compreender os conceitos de Sistemas Operacionais, conjunto de operações, gerenciamento e compartilhamento de processamento e memória, sobreposição de janelas e interfaces, além de estudo sobre APIs e ferramentas de configuração do sistema.

**Objetivo Geral**

Integrar um console de jogos digital com hardware e software para gerenciar jogos/games desenvolvidos pelo Departamento Acadêmico de Ciências da Computação (DACC)

**Objetivos específicos**

- Desenvolver um sistema de gerenciamento de execução de jogos, garantindo a inicialização, monitoramento e encerramento adequados dos processos.
- Implementar um sistema de registro de jogatina, armazenando informações como tempo de jogo, frequência de uso e histórico de execução.
- Garantir a integração do executor de jogos com a aplicação principal do console, permitindo comunicação fluida entre os processos.
- Explorar e aplicar técnicas de gerenciamento de processos, garantindo eficiência e estabilidade na execução dos jogos.

**Atividades a serem desenvolvidas**

- **Estudo e levantamento teórico (PBL - Problem-Based Learning):** abordagens para criação e gerenciamento de processos e threads em C++ e Shell Script, analisando bibliotecas e comandos específicos, implementar testes práticos para avaliar o uso de recursos (CPU, RAM) em diferentes cenários de carga, comparar o desempenho e a aplicabilidade de cada abordagem, e produzir um relatório técnico detalhando os resultados obtidos e justificando a escolha da melhor solução para diferentes contextos de uso.
- **Implementação do executor de jogos:** criar um sistema para iniciar e finalizar jogos como processos filhos da aplicação principal, monitorar a execução do processo, garantindo que ele não cause falhas no sistema, implementar mecanismos para recuperação de falhas, evitando que um jogo mal comportado afete o funcionamento do console, e documentar o funcionamento do sistema, registrando decisões de design e testes realizados.
- **Registro de jogatina e telemetria:** criar um módulo para registrar o tempo de execução de cada jogo, implementar um histórico de sessões, permitindo ao usuário visualizar seus jogos recentes, otimizar a gravação desses dados no sistema de arquivos, garantindo baixo impacto no desempenho, e gerar relatórios sobre o impacto da telemetria no desempenho geral do console.
- **Integração com a aplicação principal do console:** garantir que o executor de jogos funcione de forma integrada com a aplicação principal, criar métodos de comunicação entre processos, permitindo que o menu interaja com jogos em execução e testar a estabilidade e o desempenho da comunicação entre os módulos.

| **Atividades**                                     | **Período**             |
| -------------------------------------------------- | ----------------------- |
| • Estudo e levantamento teórico.                   | 01/02/2025 - 30/04/2025 |
| ---                                                | ---                     |
| • Implementação do executor de jogos.              | 01/04/2025 - 31/07/2025 |
| ---                                                | ---                     |
| • Registro de jogatina e telemetria.<br><br>.      | 01/08/2025 - 31/10/2025 |
| ---                                                | ---                     |
| • Integração com a aplicação principal do console. | 01/11/2025 - 31/01/2026 |
| ---                                                | ---                     |

**Resultados esperados**

- Descrição técnica que compare e justifique a escolha da distribuição final.
- Desenvolvimento de um executor de jogos funcional e estável capaz de iniciar e finalizar jogos como processos filhos da aplicação principal.
- Aplicação de mecanismos de prevenção e recuperação de falhas, assim como de monitoramento do executor.
- Tudo isso buscando garantir o uso eficiente dos recursos a fim entregar uma experiência agradável, que seja compatível com o hardware escolhido, para o usuário final.