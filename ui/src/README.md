Este é um guia técnico detalhado sobre as implementações contidas na pasta `src`. O código-fonte materializa a arquitetura definida nos arquivos de cabeçalho, focando em alta performance para sistemas embarcados ou desktops, utilizando padrões de projeto como Singleton, Factory e Cache.

### 1. Núcleo de Inicialização e Ciclo de Vida

O controle de baixo nível e a entrada do sistema são gerenciados por classes que abstraem a complexidade da API SDL2.

* **main.cpp**: Atua como o ponto de entrada e orquestrador global. Ele inicializa o sistema de coordenadas responsivo através da `ConfigLayout`, carrega as fontes e imagens iniciais, e instancia os gerenciadores de áudio e jogos. O loop principal é implementado aqui, dividindo cada quadro em fases de processamento de eventos, atualização de lógica e renderização.
* **GerenciarSDL.cpp**: Implementa a inicialização robusta de todos os subsistemas SDL (Vídeo, Áudio, Gamepad, Imagens e Fontes). Uma funcionalidade crítica implementada aqui é a verificação de hardware, que pode bloquear a inicialização até que um controle compatível seja detectado, garantindo que o usuário tenha meios de navegação.
* **ConfigLayout.cpp**: Define as variáveis de estado do layout. Ele calcula a escala global comparando a janela atual com a resolução nativa de 1920x1280, definindo os deslocamentos (offsets) necessários para centralizar a interface em monitores com diferentes proporções de tela.

### 2. Gerenciamento de Recursos e Otimização de Memória

A pasta `src` contém lógicas de cache agressivas para manter a fluidez da interface a 60 FPS, minimizando acessos ao disco e processamento de CPU.

* **GerenciadorImagens.cpp**: Implementa um sistema de cache de texturas. Ao solicitar uma imagem, a classe verifica se ela já existe na memória de vídeo (VRAM). Se não, utiliza a `SDL_image` para carregar o arquivo, cria a textura e a armazena para reutilização imediata.
* **GerenciadorTexturasTexto.cpp**: Resolve um gargalo comum na SDL2 (a lentidão do SDL_ttf). Ele renderiza strings para texturas e as armazena em um mapa. Se o texto, a fonte ou a cor não mudarem entre os quadros, o sistema apenas redesenha a textura existente na GPU, economizando ciclos de processamento caros.
* **GerenciadorFontes.cpp**: Controla o carregamento de objetos `TTF_Font`. Ele utiliza chaves compostas (caminho do arquivo + tamanho) para garantir que cada variante de fonte seja carregada apenas uma vez.
* **GerenciadorAudio.cpp**: Gerencia a `SDL_mixer`. Ele carrega efeitos sonoros curtos (WAV) em cache e gerencia o streaming de músicas de fundo (MP3/OGG), permitindo controle de volume e interrupção de faixas de forma centralizada.

### 3. Lógica de Dados e Persistência

* **GerenciadorJogos.cpp**: Implementa a inteligência de busca da biblioteca. Utiliza algoritmos de comparação de strings para permitir buscas parciais e ignorar diferenças entre maiúsculas e minúsculas. Também organiza os ponteiros dos jogos para permitir a filtragem rápida por categorias como "Ação" ou "Aventura".
* **Arquivos.cpp**: Contém o *parser* que lê os arquivos de texto na pasta de ativos. Ele extrai as informações estruturadas (nome, descrições, caminhos de executáveis) e as converte em objetos da classe `Jogo`.
* **Jogo.cpp**: Define os métodos de acesso (getters e setters) para o modelo de dados, garantindo a integridade das informações de cada título individual.

### 4. Componentes da Interface do Usuário (UI)

A implementação da UI foca em modularidade e suporte nativo a múltiplos métodos de entrada.

* **GerenciarInterface.cpp**: É a classe mais extensa da UI. Ela coordena a criação e o desenho de todos os elementos: banners de destaque, a grade de jogos e as setas de navegação. Ela integra o sistema de scroll para que os elementos mudem de posição suavemente conforme o usuário navega.
* **Botao.cpp**: Gerencia a lógica de interação. Detecta a colisão do mouse, responde a eventos de foco do controle e altera as cores ou texturas conforme o estado (Normal, Hover, Pressionado).
* **BotaoPesquisa.cpp**: Implementa uma caixa de texto interativa. Ela monitora o input do usuário e dispara consultas em tempo real ao `GerenciadorJogos`, atualizando dinamicamente uma lista de sugestões suspensa.
* **JanelaJogo.cpp**: Controla a tela de detalhes. Implementa a lógica da galeria de fotos (permitindo trocar imagens com os botões L1/R1 do controle) e prepara o ambiente para a execução do jogo selecionado.
* **TecladoVirtual.cpp**: Implementa uma interface de entrada completa. Gerencia uma grade de caracteres que permite ao usuário "digitar" usando apenas um controle direcional e um botão de confirmação.

### 5. Processamento de Entrada e Integração Externa

* **GerenciarInputs.cpp**: Centraliza todos os eventos de hardware. Ele traduz comandos complexos de gamepads (eixos analógicos e botões) em ações de interface. Implementa o sistema de "navegação por foco", que move a seleção entre os botões de forma lógica (cima, baixo, esquerda, direita).
* **NetworkClient.cpp**: Gerencia a comunicação inter-processos. Utiliza sockets Unix para enviar comandos de inicialização para um gerenciador externo, permitindo que o launcher monitore se o jogo ainda está rodando ou se foi encerrado.
* **SystemStatus.cpp**: Realiza chamadas periódicas ao sistema operacional para atualizar o relógio, o nível de bateria e a força do sinal WiFi, mantendo esses dados em cache para exibição na barra superior.
