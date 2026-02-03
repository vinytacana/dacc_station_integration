Este é um guia técnico detalhado sobre a estrutura, organização e finalidade dos ativos contidos na pasta `assets`. Esta pasta armazena todos os recursos estáticos necessários para a composição visual, sonora e de dados da interface, sendo essencial para o funcionamento dos gerenciadores de cache definidos no sistema.

### 1. Diretório de Imagens (`assets/images`)

O sistema de imagens é organizado para suportar a alternância dinâmica de temas e o gerenciamento de metadados visuais de jogos.

* **Ativos de Interface Global**:
* **jogoAleatorio.png**: Ícone utilizado no botão de seleção randômica de títulos.
* **mudarTema.png**: Recurso visual para o componente de alternância entre modo claro e escuro.


* **Subdiretório Temático: Modo Claro (`assets/images/light`)**:
* **ConfigClaro.png**: Ícone da engrenagem de configurações otimizado para fundos claros.
* **lupa_PesquisaClaro.png**: Elemento visual do campo de busca para o tema claro.
* **explicacaoBotoesClaro.jpg**: Textura informativa ou tutorial com esquema de cores claras.


* **Subdiretório Temático: Modo Escuro (`assets/images/dark`)**:
* **ConfigEscuro.png**: Versão do ícone de configurações para alta visibilidade em fundos escuros.
* **lupa_PesquisaEscuro.png**: Ícone de busca com contraste ajustado para o modo noturno.
* **explicacaoBotoesEscuro.jpg**: Guia visual informativo com paleta de cores escuras.


* **Gerenciamento de Jogos (`assets/images/games`)**:
* **Capas Principais**: Arquivos como `jogo1CapaTelaPrincipal.jpeg` e similares são utilizados na grade de exibição principal da biblioteca.
* **Fundos de Destaque**: Imagens como `jogo1FundoDestaque.jpeg` servem como o banner principal (hero) quando um jogo específico é selecionado.
* **Visualização Detalhada**: Arquivos como `jogo1CapaJanelaJogo.jpeg` são renderizados especificamente dentro da `JanelaJogo`.
* **Galeria de Capturas**: Imagens de `captura1.jpeg` a `captura4.jpeg` compõem o carrossel de screenshots demonstrativas do gameplay.



### 2. Diretório de Dados e Persistência (`assets/data`)

Este diretório contém a base de informações textuais que alimentam o `GerenciadorJogos`.

* **Arquivos de Definição de Jogos**: Arquivos individuais como `jogo1.txt` a `jogo5.txt` contêm as strings de nome, descrição curta e descrição longa.
* **Documentação de Estrutura**: O arquivo `Formatação dos dados dos jogos` detalha o padrão de organização interna desses arquivos de texto, garantindo que a classe `Arquivos` consiga realizar o parsing corretamente.

### 3. Diretório de Áudio (`assets/sounds`)

Recursos sonoros utilizados pelo `GerenciadorAudio` para fornecer feedback tátil-auditivo durante a navegação.

* **Feedback de Navegação**: O arquivo `navegacao.wav` é reproduzido durante a mudança de foco entre elementos da interface.
* **Interações de Sistema**:
* **abrir_config.wav** e **abrir_pesquisa.wav**: Acionados ao entrar em menus específicos.
* **fechar.wav**: Som de retorno ou encerramento de janelas.
* **mudar_tema.wav**: Feedback sonoro para a transição de cores da interface.


* **Ações de Execução**:
* **jogar.wav**: Executado no momento em que o comando de iniciar jogo é enviado ao `NetworkClient`.
* **aleatorio.wav**: Efeito sonoro para a função de sorteio de jogo.



### 4. Diretório de Fontes (`assets/fonts`)

Contém as fontes TrueType que compõem a tipografia da aplicação.

* **Garet-Heavy.ttf**: Fonte de peso elevado utilizada para títulos, destaques e textos em negrito através do `GerenciadorFontes`.
* **Garet-Book.ttf**: Fonte de leitura principal, utilizada para descrições longas e labels padrão do sistema.

### 5. Diretórios de Suporte e Legado (`assets/textures` e `assets/icons`)

Estes diretórios contêm ativos adicionais ou versões alternativas para contextos específicos de renderização.

* **Texturas Adicionais**: O diretório `assets/textures` armazena imagens de jogos (`jogo1.jpeg` a `jogo5.jpeg`) que podem ser usadas como miniaturas alternativas ou placeholders.
* **Ícones de Controle**: Contém variações como `configs2.png` e ícones de alternância de tema para diferentes escalas de interface.
