Este guia técnico descreve as especificações de dimensões e proporções para os ativos visuais contidos na pasta `assets`, fundamentado nas constantes de design definidas no sistema de layout responsivo do projeto. O launcher utiliza uma resolução nativa de **1920x1280 pixels** como base para todos os cálculos de interface.

### 1. Ativos de Jogos (assets/images/games)

Estes arquivos representam os metadados visuais de cada título e são os elementos de maior destaque na interface.

* **Capas da Tela Principal (`*CapaTelaPrincipal.jpeg`)**:
* **Dimensão Alvo**: 290 x 435 pixels.
* **Proporção**: Aproximadamente 2:3 (padrão vertical de capas de jogos físicos).
* **Uso**: Renderizadas na grade de jogos (`GRADE_JOGOS`). O sistema organiza estes itens em 5 colunas com espaçamento horizontal de 72 pixels.


* **Fundos de Destaque (`*FundoDestaque.jpeg`)**:
* **Dimensão Alvo**: 1862 x 597 pixels.
* **Uso**: Atuam como o banner principal (Hero Banner) no topo da tela inicial.
* **Observação**: Devem possuir alta resolução para cobrir a largura quase total da tela nativa (1920 pixels) sem perda de qualidade.


* **Capas de Janela de Detalhes (`*CapaJanelaJogo.jpeg`)**:
* **Dimensão Alvo**: 960 x 540 pixels.
* **Proporção**: 16:9.
* **Uso**: Exibidas no lado esquerdo da `JanelaJogo` quando um título é selecionado.


* **Capturas de Tela/Screenshots (`captura*.jpeg`)**:
* **Dimensão Alvo**: 500 x 250 pixels.
* **Uso**: Compõem o carrossel de imagens na parte inferior da tela de detalhes. O sistema utiliza um espaçamento de 520 pixels entre cada captura para permitir a navegação horizontal.



### 2. Elementos de Interface e Ícones (assets/images/light e assets/images/dark)

Estes ativos são ícones funcionais que se adaptam ao tema selecionado e possuem tamanhos rígidos para garantir a precisão do clique.

* **Ícones de Configuração (`Config*.png`)**:
* **Dimensão Alvo**: 80 x 80 pixels.
* **Posicionamento**: Canto superior esquerdo, alinhado à margem de 32 pixels da borda da tela.


* **Ícones de Pesquisa/Lupa (`lupa_Pesquisa*.png`)**:
* **Dimensão Alvo**: 50 x 50 pixels.
* **Uso**: Inseridos dentro da barra de pesquisa (que possui 600x80 pixels). O ícone possui uma margem interna de 14 pixels em relação à borda da barra.


* **Botões de Tema e Aleatório (`mudarTema.png`, `jogoAleatorio.png`)**:
* **Mudar Tema**: 80 x 80 pixels (mesma escala do botão de configurações).
* **Jogo Aleatório**: Segue as dimensões de um card de jogo padrão (290 x 435 pixels) para manter a simetria na grade.



### 3. Ativos Informativos e de Suporte

* **Texturas de Explicação (`explicacaoBotoes*.jpg`)**:
* **Dimensão Sugerida**: 1920 x 140 pixels (baseada no `FUNDO_TOPO_ALTURA`).
* **Uso**: Funcionam como banners de instrução que podem cobrir a largura total do cabeçalho da aplicação.


* **Setas de Navegação**:
* **Categorias**: 50 x 90 pixels.
* **Carrossel de Capturas**: 120 x 270 pixels.



### 4. Especificações Técnicas de Renderização

Para garantir que as imagens não sofram distorção, o motor de renderização utiliza o método `drawTextureCover`. Este método mantém a proporção original da imagem e corta o excesso para preencher o retângulo alvo definido pelas constantes acima. Todas as imagens são escalonadas pelo `escalaGlobal` calculado no início da execução, permitindo que ativos de 1920x1280 caibam proporcionalmente em telas 720p, 1080p ou 4K.
