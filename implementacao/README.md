Principais Blocos Lógicos
1. Motor de Interação (Input Handling)

GerenciarInputs.cpp: Contém a lógica de "Navegação por Zonas". Implementa um sistema de índices para permitir que o controle (gamepad) salte entre banners, barras de ferramentas e o grid de jogos. Inclui filtros de debounce e zonas mortas (deadzone) para garantir precisão no hardware.

TecladoVirtual.cpp: Implementa uma matriz de colisão para mouse e um sistema de navegação por índices X/Y para gamepads, permitindo a entrada de texto sem necessidade de teclado físico.

2. Orquestração de Interface e Layout

GerenciarInterface.cpp: Implementa o cálculo matemático do grid de jogos (usando operadores / e % para colunas e linhas). Gerencia o "Hot-Swapping" de texturas, trocando ícones de claro para escuro em tempo real conforme o estado do GerenciadorTemas.

JanelaJogo.cpp: Centraliza a visualização do produto. Implementa o carrossel de capturas de tela com técnica de clipping (recorte de renderização) e a função crítica executarJogo(), que utiliza chamadas de sistema assíncronas (&) para lançar os games.

Botao.cpp: Define o comportamento reativo dos elementos. Implementa detecção de colisão híbrida (AABB para retângulos e Distância Euclidiana para botões circulares).

3. Gerenciamento de Ativos e Memória

GerenciadorImagens.cpp & GerenciadorFontes.cpp: Implementam o padrão Flyweight. Utilizam std::map para indexar ponteiros de memória, garantindo que o IO de disco ocorra apenas uma vez por arquivo, otimizando drasticamente o uso de VRAM.

Utils.cpp: Contém a implementação da pipeline de texto: conversão de string UTF-8 -> Surface -> Texture -> Render Copy.

4. Persistência e Dados

Arquivos.cpp: Implementa um parser customizado baseado em prefixos (@, #, $, *, &, +, %). Este sistema permite carregar bibliotecas de jogos complexas a partir de arquivos de texto simples e leves.

GerenciadorJogos.cpp: Contém algoritmos de busca case-insensitive e filtros de categoria baseados em busca de substrings dentro de metadados.

Lógicas de Destaque
Algoritmo de Word Wrap (Quebra de Linha)

Implementado em Janela.cpp e JanelaJogo.cpp, o sistema processa strings longas de descrição, medindo a largura estimada de cada palavra e inserindo quebras de linha automáticas para garantir que o texto respeite o padding lateral dos painéis.

Sistema de Temas Reativo

Implementado através de um Singleton em GerenciadorTemas.cpp. Os componentes de UI não armazenam cores fixas; em vez disso, consultam a instância global a cada frame, permitindo a transição visual instantânea entre temas sem "flicker" ou recarregamento de tela.

Multi-Janelas SDL

Em JanelaConfiguracao.cpp, o código gerencia uma segunda SDL_Window e um segundo SDL_Renderer. A implementação garante que os IDs de janela sejam filtrados corretamente para que um clique na janela de configuração não dispare acidentalmente um botão na janela principal.

Dependências Utilizadas na Implementação

SDL2 Core: Janelas, renderização e eventos.

SDL2_image: Decodificação de PNG/JPG para texturas.

SDL2_ttf: Rasterização de fontes TrueType.

SDL2_gfx: Primitivas gráficas avançadas (círculos suaves e polígonos preenchidos).

Standard C++11/17: Uso de std::vector, std::map, std::unique_ptr e stringstream.

Notas de Manutenção:

Ao modificar a renderização em qualquer arquivo .cpp, certifique-se de aplicar o scrollX e scrollY negativos (offsets) caso o elemento deva acompanhar o movimento da câmera principal.