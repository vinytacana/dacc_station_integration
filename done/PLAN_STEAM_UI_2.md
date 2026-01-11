   # 🛠️  Plano Técnico de Implementação: Interface "Hero" (Steam-like)

   **Objetivo:** Substituir o grid atual do Menu Principal (`main.cpp`) por um layout "Hero" onde o jogo selecionado domina a tela e a lista de jogos se torna um carrossel
   horizontal no rodapé.

   ---

   ## 1. Arquitetura e Estrutura de Dados
   **Correção de Escopo:** A alteração deve ocorrer no **Loop Principal (`main.cpp`)**, pois `JanelaJogo` apenas exibe um jogo por vez.

   ### Novos Estados Necessários (em `main.cpp`)
   Substituir o vetor disperso `jogos` (botoes) por um controle centralizado de índice.
  // Variáveis de Estado Globais (main.cpp)
  int g_GameSelectedIndex = 0;       // Índice do jogo focado atualmente
  float g_CarouselScrollX = 0.0f;    // Scroll suave do carrossel (animado)
  float g_CarouselTargetX = 0.0f;    // Alvo do scroll


   ## 2. Camadas de Renderização (Z-Order)

   O loop de renderização (`Update` + `Draw`) deve seguir estritamente esta ordem para garantir o efeito visual correto:

   ### Camada 0: Atmosfera (Background)
   *   **Fonte de Dados:** `gerenciadorJogos.obterJogoPorIndice(g_GameSelectedIndex)->getCapa()`
   *   **Técnica SDL:**
       *   Carregar a textura do jogo selecionado.
       *   Usar `SDL_RenderCopy` com `NULL` no `dstRect` para preencher 100% da tela (1920x1080).
       *   **Otimização:** Não aplicar Blur via CPU (lento no Pi4).
       *   **Overlay:** Desenhar um `SDL_Rect` preto tela cheia `(0,0,0)` com Alpha `180-220` (`SDL_SetRenderDrawBlendMode` = `SDL_BLENDMODE_BLEND`) sobre a imagem para
   garantir leitura do texto.

   ### Camada 1: Hero Content (Metadados)
   *   **Posição:** Quadrante Superior Esquerdo (Margem: 80px).
   *   **Tipografia:**
       *   **Título:** `Garet-Heavy.ttf` (Tam: 64pt, Cor: Branca).
       *   **Info:** `Garet-Book.ttf` (Tam: 24pt, Cor: Cinza Claro `200,200,200`). Ex: "Ação | 20h jogadas".
       *   **Sinopse:** Texto com quebra de linha automática (limite largura 800px).
   *   **Ação Principal:** Exibir prompt "Ⓐ JOGAR" (ou Enter) logo abaixo da sinopse.

   ### Camada 2: O Carrossel (Lista Horizontal)
   *   **Posição:** Rodapé (Y: 750px até 1080px).
   *   **Lógica de Renderização:**
       *   Não desenhe *todos* os jogos. Desenhe apenas `[Selected - 2]` até `[Selected + 5]`.
       *   **Tamanho do Card:** ~300x170px (Landscape) ou ~200x300px (Portrait) - *Decidir formato das capas*.
       *   **Destaque (Selected):**
           *   O item no índice `g_GameSelectedIndex` deve ter uma borda branca espessa (3px) desenhada com `SDL_RenderDrawRect`.
           *   Opcional: Leve aumento de escala (Scale 1.1x) no render copy.

   ---

   ## 3. Lógica de Input e Navegação

   Substituir a detecção de mouse (hover) por lógica de controle direcional (D-Pad/Teclado), que é o padrão para consoles.

   **No Loop de Eventos (`SDL_KEYDOWN`):**
   *   `SDLK_RIGHT`: `g_GameSelectedIndex++` (com proteção de limites `size()-1`).
   *   `SDLK_LEFT`: `g_GameSelectedIndex--` (com proteção `0`).
   *   `SDLK_RETURN` (Enter):
       *   Chamar `criarJanelaDeJogoReal(...)` ou iniciar o jogo diretamente.
       *   *Dica:* Se quiser manter o `JanelaJogo` como um "Detalhe Profundo", o Enter abre ele. Se quiser lançar direto, o Enter chama `NetworkClient::sendStartGame`.

   ---

   ## 4. Integração entre Times (Contratos)

   ### Para: João Henrique (Infra/Filesystem)
   *   **Padronização de Assets:** Precisamos garantir que o método `getCapa()` retorne caminhos válidos.
   *   **Fallback:** Se um jogo não tiver capa (`.jpg`/`.png`), o sistema deve carregar `assets/textures/placeholder_cover.png` para não quebrar o loop de renderização.

   ### Para: Vinícius Tacaná (System)
   *   **Header do Sistema (Opcional):**
       *   Criar função `SystemUtils::getBatteryStatus()` e `SystemUtils::getWifiStatus()`.
       *   O João Eduardo apenas chamará essas funções para desenhar ícones pequenos no canto superior direito (Top-Right Anchor).

   ### Para: Feliph Lima (Backend)
   *   **Carregamento Assíncrono (Futuro):**
       *   Atualmente, carregar a textura "Hero" de alta resolução (`1080p`) a cada movimento de seta pode causar *stutter* (engasgo).
       *   **Solução Imediata:** Manter cache das últimas 5 texturas carregadas em `GerenciadorImagens`.
       *   **Solução Ideal:** Thread separada carregando a textura do "próximo" jogo antecipadamente.

   ---

   ## 5. Exemplo de Código (Snippet para main.cpp)
  // Pseudo-código para o novo loop de renderização

  // 1. Fundo Hero
  SDL_Texture* bgTex = gerImg.carregar(renderer, jogoSelecionado->getCapa());
  SDL_RenderCopy(renderer, bgTex, NULL, NULL); // Tela cheia

  // 2. Dimmer (Escurecer fundo)
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200); // Alpha 200
  SDL_Rect fullscreen = {0,0,1920,1080};
  SDL_RenderFillRect(renderer, &fullscreen);

  // 3. Texto Hero
  desenharTexto(renderer, jogoSelecionado->getNome(), 100, 100, BRANCO, 64, NEGRITO);
  desenharTexto(renderer, jogoSelecionado->getDescricao(), 100, 200, CINZA, 24, NORMAL);

  // 4. Carrossel
  int cardW = 300;
  int cardH = 400;
  int gap = 20;
  int startX = 100; // Margem esquerda

  for (int i = 0; i < totalJogos; i++) {
      // Lógica simples de posição relativa
      int posX = startX + (i - g_GameSelectedIndex) * (cardW + gap);

      // Só desenha se estiver na tela
      if (posX + cardW > 0 && posX < 1920) {
          SDL_Rect r = {posX, 600, cardW, cardH};
          SDL_RenderCopy(renderer, capas[i], NULL, &r);

          // Borda de seleção
          if (i == g_GameSelectedIndex) {
              SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
              SDL_RenderDrawRect(renderer, &r);
          }
      }
  }