Este é um guia técnico detalhado sobre a estrutura e o funcionamento dos arquivos de cabeçalho localizados na pasta `include`. O projeto foi desenvolvido utilizando a biblioteca SDL2 e segue princípios de programação orientada a objetos em C++, com foco em performance, modularidade e responsividade.

### 1. Sistema de Layout e Responsividade

A base visual da aplicação é regida por um sistema de coordenadas virtuais que garante a consistência em diferentes resoluções de tela.

* **ConfigLayout.hpp**: Este arquivo define a arquitetura de design responsivo. O sistema utiliza uma resolução nativa de 1920x1280 como referência de design. Ele calcula fatores de escala global e offsets para centralizar o conteúdo, permitindo que coordenadas nativas sejam convertidas para coordenadas reais de tela através de funções estáticas como `X()`, `Y()` e `F()`.
* **Theme.hpp**: Atua como um dicionário de tokens de design, definindo constantes de tempo de compilação (`constexpr`) para cores de fundo, acentos, estados de botões, geometria de bordas e espaçamentos (paddings).

### 2. Gerenciamento de Recursos e Cache

Para otimizar o uso de memória e CPU, o projeto implementa diversos gerenciadores que evitam o carregamento redundante de arquivos do disco.

* **GerenciadorImagens.hpp**: Implementa um cache para texturas SDL. Ele garante que cada imagem seja carregada apenas uma vez; solicitações subsequentes para o mesmo caminho de arquivo retornam o ponteiro da textura já presente na memória.
* **GerenciadorFontes.hpp**: Gerencia o ciclo de vida de objetos `TTF_Font`. Utiliza um mapeamento interno que associa o caminho do arquivo e o tamanho da fonte a um `std::unique_ptr` com um destruidor customizado para a SDL_ttf.
* **GerenciadorTexturasTexto.hpp**: Reduz o custo de renderização de strings. Ele armazena texturas de texto já processadas, evitando a criação constante de superfícies temporárias se o conteúdo, a fonte, o tamanho e a cor do texto não mudarem.
* **GerenciadorAudio.hpp**: Centraliza o controle de áudio, permitindo o carregamento e a execução de efeitos sonoros (SFX) e músicas de fundo (BGM) com controle de volume independente.

### 3. Componentes de Interface do Usuário (UI)

A interface é composta por elementos modulares e interativos, projetados para suportar tanto mouse quanto controle.

* **Botao.hpp**: Classe base para elementos clicáveis. Gerencia estados de hover, clique e foco, além de suportar tanto rótulos de texto quanto texturas de imagem.
* **BotaoPesquisa.hpp**: Estende a funcionalidade de um botão para atuar como uma caixa de busca. Inclui lógica para filtragem dinâmica de resultados e gerenciamento de uma lista suspensa (dropdown) de sugestões.
* **TecladoVirtual.hpp**: Fornece uma interface de digitação na tela para dispositivos que não possuem teclado físico. Suporta navegação por grade e interação via gamepad.
* **Janela.hpp**: Gerencia popups de estilo overlay translúcido para exibir informações rápidas contextuais sobre elementos selecionados.
* **Forma.hpp**: Uma abstração simples para renderizar primitivas geométricas, como retângulos preenchidos ou apenas bordas, facilitando a criação de painéis de interface.
* **GraphicsUtils.hpp**: Fornece funções estáticas de alto nível para desenho complexo, incluindo retângulos com cantos arredondados, preenchimento de texturas mantendo a proporção (cover) e barras de status do sistema.

### 4. Gestão de Dados e Lógica de Negócio

* **Jogo.hpp**: Define o modelo de dados principal da aplicação. Armazena metadados como títulos, descrições curta e longa, códigos de identificação, caminhos de executáveis e coleções de capturas de tela.
* **GerenciadorJogos.hpp**: Administra o catálogo de jogos. Oferece funcionalidades de busca parcial por nome (case-insensitive), busca por categoria e acesso por índice ou código.
* **Arquivos.hpp**: Lida com a persistência de dados no sistema de arquivos, sendo responsável por serializar a biblioteca de jogos para salvamento e desserializar para carregamento inicial.

### 5. Integração com o Sistema e Rede

* **SystemStatus.hpp**: Um singleton que monitora e faz cache de informações do hardware, como o relógio do sistema, nível de carga de bateria e conectividade de rede WiFi.
* **NetworkClient.hpp**: Gerencia a comunicação entre o launcher e um gerenciador de processos externo via Unix Domain Sockets. É responsável por enviar comandos de inicialização de jogos e monitorar o status de execução.

### 6. Fluxo de Execução e Estados

* **GerenciarInterface.hpp**: Coordena a renderização de todos os componentes visuais mencionados acima, organizando a hierarquia de desenho e atualizando posições com base no scroll.
* **GerenciarInputs.hpp**: Centraliza o processamento de eventos da SDL. Traduz entradas de teclado, mouse e controles para ações na interface, gerenciando o sistema de foco de navegação.
* **GerenciarScroll.hpp**: Mantém o estado global de navegação, incluindo deslocamentos de tela (scroll X e Y), índices de categorias ativas e referências para janelas abertas no momento.
* **JanelaJogo.hpp**: Implementa a visualização detalhada de um jogo selecionado, contendo uma galeria de imagens interativa e os controles de lançamento do executável.
* **JanelaConfiguracao.hpp**: Gerencia uma janela secundária independente para ajustes de sistema e preferências do usuário.
* **GerenciarSDL.hpp**: Encapsula as funções de baixo nível para inicialização dos subsistemas de vídeo, áudio, imagem e fontes da SDL2.
* **Utils.hpp**: Contém utilitários gerais, como definições de tipos de fontes e atalhos para renderização de texto simples.