# Documentação da Implementação (.cpp)

A implementação do sistema foi reorganizada em módulos que refletem a estrutura dos cabeçalhos, separando claramente a lógica de baixo nível, as regras de negócio e a construção da interface gráfica.

---

## 1. Principais (Core)
Inicialização do motor e utilitários essenciais.

* **`GerenciarSDL.cpp`**: Implementa a inicialização segura da SDL2 e suas extensões. Contém a lógica de verificação de hardware (loop de detecção de controles) antes de iniciar o sistema principal.
* **`ConfigLayout.cpp`**: Centraliza os cálculos de escala responsiva. Suas funções transformam as coordenadas de design (base 1920x1280) em pixels reais da janela, garantindo que a UI se adapte corretamente a qualquer resolução de monitor.
* **`Utils.cpp`**: Fornece a implementação das funções de desenho de texto (`desenharTexto`) e o gerenciamento global dos caminhos de fontes do sistema.

---

## 2. Gerenciadores (Managers)
Sistemas de controle de recursos, estados e entradas.

* **`GerenciarInputs.cpp`**: Contém a lógica de "Navegação por Zonas". Implementa um sistema de índices para permitir que o controle (gamepad) salte entre banners, barras de ferramentas e o grid de jogos de forma intuitiva. Inclui filtros de *debounce* e zonas mortas (*deadzone*) para garantir precisão nos analógicos.
* **`GerenciadorImagens.cpp` & `GerenciadorFontes.cpp`**: Implementam o padrão *Flyweight*. Utilizam `std::map` para indexar ponteiros de memória, garantindo que o carregamento de arquivos do disco ocorra apenas uma vez por recurso, otimizando drasticamente o uso de VRAM e performance.
* **`GerenciadorTemas.cpp`**: Implementa o sistema de temas reativo via Singleton. Os componentes de UI consultam esta instância a cada frame para determinar suas cores, permitindo a transição visual instantânea (Claro/Escuro) sem necessidade de recarregar a tela.
* **`GerenciadorTexturasTexto.cpp`**: Gerencia o cache de texturas geradas dinamicamente a partir de strings (SDL_ttf), evitando a recriação custosa de superfícies de texto a cada ciclo de renderização.

---

## 3. GerenciadoresJogos (Lógica)
Regras de negócio da biblioteca.

* **`GerenciadorJogos.cpp`**: Implementa as operações de busca (linear e por substring) e a manutenção do vetor de jogos. Responsável por filtrar a biblioteca por categorias ou termos de pesquisa em tempo real.
* **`Jogo.cpp`**: Implementa a estrutura de dados e métodos de acesso para as informações dos jogos (metadados e caminhos de assets).

---

## 4. Arquivos (Persistência)
Manipulação de dados em disco.

* **`Arquivos.cpp`**: Implementa o *parser* que lê e grava os arquivos de texto da biblioteca, convertendo as strings serializadas do disco em objetos `Jogo` na memória.

---

## 5. Interface (UI)
Implementação visual e comportamental dos elementos gráficos.

### 5.1 Componentes
* **`Botao.cpp`**: Define o comportamento reativo dos elementos. Implementa detecção de colisão híbrida (AABB para retângulos e lógica de estado para navegação via controle).
* **`BotaoPesquisa.cpp`**: Implementa a lógica de buffer de texto e filtragem dinâmica. Gerencia a lista de sugestões que aparece enquanto o usuário digita.
* **`TecladoVirtual.cpp`**: Implementa uma matriz de colisão para mouse e um sistema de navegação por índices X/Y para gamepads, permitindo a entrada de texto sem teclado físico.
* **`Forma.cpp`**: Implementação básica para renderização de primitivas geométricas (retângulos, fundos).

### 5.2 Janelas
* **`GerenciarInterface.cpp`**: Orquestra o layout principal. Implementa o cálculo matemático do grid de jogos (usando operadores de divisão e módulo para linhas/colunas) e gerencia o *Hot-Swapping* de ícones ao trocar o tema.
* **`JanelaJogo.cpp`**: Centraliza a visualização do produto. Implementa o algoritmo de *Word Wrap* (quebra de linha automática) para descrições longas, o carrossel de capturas com técnica de *clipping* e a função crítica `executarJogo()` para lançar processos externos.
* **`JanelaConfiguracao.cpp`**: Gerencia uma segunda `SDL_Window` física. A implementação filtra eventos por ID de janela para evitar conflitos de input com a tela principal.
* **`Janela.cpp`**: Implementa popups modais informativos com posicionamento automático.

---

## Padrões e Dependências

### Dependências Utilizadas
* **SDL2 Core**: Gerenciamento de janelas, renderização e fila de eventos.
* **SDL2_image**: Decodificação de formatos de imagem (PNG/JPG) para texturas de hardware.
* **SDL2_ttf**: Rasterização de fontes TrueType para exibição de texto.
* **SDL2_gfx**: Primitivas gráficas avançadas e anti-aliasing (quando necessário).
* **Standard C++17**: Uso extensivo de `std::vector`, `std::map`, `std::unique_ptr` e `std::function`.

### Notas de Manutenção
Ao modificar a renderização em qualquer arquivo `.cpp` de interface, lembre-se de aplicar os offsets `scrollX` e `scrollY` negativos caso o elemento deva acompanhar o movimento da câmera principal (como no grid de jogos). Elementos fixos (como o cabeçalho) não devem receber esses offsets.