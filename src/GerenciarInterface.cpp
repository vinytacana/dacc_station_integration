/**
 * @file GerenciarInterface.cpp
 * @brief Implementação do orquestrador visual da biblioteca de jogos.
 * 
 * Este arquivo contém a lógica de construção da interface, gerenciamento de 
 * layouts dinâmicos (grades e carrosséis) e a renderização em camadas de 
 * todos os componentes da UI.
 */

#include "GerenciarInterface.hpp"
#include "GerenciadorJogos.hpp"
#include "GerenciadorImagens.hpp"
#include "GerenciadorTemas.hpp" 
#include "Utils.hpp"
#include <iostream>
#include <algorithm>

using namespace MeuProjeto;
using namespace std;

/** @brief Referência externa ao banco de dados de jogos. */
extern GerenciadorJogos gerenciadorJogos;
/** @brief Referência externa ao sistema de cache de texturas. */
extern GerenciadorImagens gerImg;

GerenciarInterface::GerenciarInterface() {}

/**
 * @brief Destrutor da classe.
 * 
 * Realiza a limpeza rigorosa da memória heap, deletando todos os componentes 
 * instanciados dinamicamente (botões, formas e sub-gerenciadores).
 */
GerenciarInterface::~GerenciarInterface() {
    delete btnConfig; delete btnMudarTema; delete btnAleatorio; delete btnPesquisa;
    delete setaEsquerda; delete setaDireita; delete fundoCategorias; delete popupJanela;
    if (tecladoVirtual) delete tecladoVirtual;
}

/**
 * @brief Inicializa a infraestrutura da interface do usuário.
 * 
 * @details **Processo de Inicialização:**
 * 1. **Sub-sistemas:** Cria as instâncias de `Janela` (popups) e `TecladoVirtual`.
 * 2. **Ativos por Tema:** Pré-carrega ícones em suas versões Claro e Escuro para troca rápida.
 * 3. **Barra de Ferramentas:** Instancia os botões de Configuração, Tema e a Barra de Pesquisa.
 * 4. **Categorias:** Cria a lista de botões de filtro e o fundo decorativo.
 * 5. **Destaques (Hero section):** Mapeia os primeiros 5 jogos da biblioteca para os banners grandes superiores.
 * 6. **Grid Inicial:** Invoca a primeira renderização da lista de jogos com o filtro "Todas".
 * 
 * @param renderer Renderizador SDL principal.
 */
void GerenciarInterface::inicializar(SDL_Renderer* renderer) {
    if (!renderer) return;
    popupJanela = new Janela(renderer);
    tecladoVirtual = new TecladoVirtual(); 

    // Carregamento de ativos bi-temáticos
    iconeConfigClaro = gerImg.carregar(renderer, "assets/images/ConfigClaro.png");
    iconeConfigEscuro = gerImg.carregar(renderer, "assets/images/ConfigsEscuro.png");

    // Configuração dos botões superiores
    btnConfig = new Botao(renderer, 35, 5, 130, 180, "", "Configurações", "", false);
    btnConfig->setIsRound(true);

    btnMudarTema = new Botao(renderer, 1730, 27, 120, 120, "assets/images/mudarTema.png", "Mudar Tema", "", false);
    btnMudarTema->setIsRound(true);

    btnPesquisa = new BotaoPesquisa(renderer, 456, 37, 1008, 98, "Barra de Pesquisa");
    btnPesquisa->setFontePlaceholder(TipoFonte::NORMAL);
    btnPesquisa->setTamanhoFontePlaceholder(24);
    btnPesquisa->setFonteResultados(TipoFonte::NEGRITO);
    btnPesquisa->setTamanhoFonteResultados(32);

    iconeLupaClaro = gerImg.carregar(renderer, "assets/images/lupa_PesquisaClaro.png");
    iconeLupaEscuro = gerImg.carregar(renderer, "assets/images/lupa_PesquisaEscuro.png");
    
    // Imagens de guia para controles (Gamepad)
    explicacaoClaro = gerImg.carregar(renderer, "assets/images/explicacaoBotoesClaro.jpg");
    explicacaoEscuro = gerImg.carregar(renderer, "assets/images/explicacaoBotoesEscuro.jpg");

    // Inicialização da Barra de Categorias
    vector<string> nomesCategorias = {"Aventura", "Casual", "Ação", "Rpg", "Estrategia", "Esportes"};
    for (const auto& nome : nomesCategorias) {
        categorias.push_back(Botao(0, 0, CATEGORIA_LARGURA, CATEGORIA_ALTURA, nome));
    }
    
    // Ajustes finos de tipografia para nomes longos
    if (categorias.size() > 0) categorias[0].setTamanhoFonte(24);
    if (categorias.size() > 4) categorias[4].setTamanhoFonte(22);
    if (categorias.size() > 5) categorias[5].setTamanhoFonte(24);

    fundoCategorias = new Forma(30, 920, 980, 100, {50, 50, 60, 255}, "retangulo");

    setaEsquerda = new Botao(220, CATEGORIA_POS_Y, 40, 80, "<");
    setaDireita = new Botao(960, CATEGORIA_POS_Y, 40, 80, ">");
    setaEsquerda->setFonte(TipoFonte::NORMAL);
    setaDireita->setFonte(TipoFonte::NORMAL);

    btnAleatorio = new Botao(1200, 920, 600, 100, "Escolher aleatoriamente");
    btnAleatorio->setTamanhoFonte(24);
    
    iconeDadoClaro = gerImg.carregar(renderer, "assets/images/jogoAleatorioClaro.png");
    iconeDadoEscuro = gerImg.carregar(renderer, "assets/images/jogoAleatorioEsuro.png");

    // Construção da Seção de Destaques
    int startX = 70, startY = 597, tamanho = 300, espacamento = 372;
    vector<Jogo> todosJogos = gerenciadorJogos.listarJogos();
    
    destaques.clear(); 
    for (int i = 0; i < std::min(5, (int)todosJogos.size()); i++) {
        Jogo& jogo = todosJogos[i];
        destaques.push_back(Botao(renderer, startX + (i * espacamento), startY, tamanho, tamanho,
                                  jogo.getCapaTelaPrincipal(), jogo.getNome(), jogo.getDescricao(), true));
        destaques.back().setParentJanela(popupJanela);
    }
    
    // Popula a grade principal de jogos
    atualizarJogosPorCategoria("Todas", renderer);
}

/**
 * @brief reconstrói a grade de jogos baseada em um filtro de categoria.
 * 
 * @details **Lógica do Grid de 4 Colunas:**
 * 1. Limpa o vetor atual de botões de jogos.
 * 2. Busca no banco de dados apenas os jogos que pertencem à @a categoria.
 * 3. Utiliza as constantes `JOGO_LARGURA`, `JOGO_ALTURA` e `JOGO_GAP` para calcular 
 *    a posição (X, Y) de cada item.
 * 4. O cálculo de linha/coluna usa os operadores de divisão (/) e resto (%).
 * 
 * @param categoria String de filtro.
 * @param renderer Renderizador para carregar as capas.
 */
void GerenciarInterface::atualizarJogosPorCategoria(const std::string& categoria, SDL_Renderer* renderer) {
    jogos.clear();
    vector<Jogo*> jogosFiltrados = gerenciadorJogos.buscarPorCategoria(categoria);
    
    if (jogosFiltrados.empty()) return;

    const int COLUNAS = 4;
    const int ALTURA_LINHA = JOGO_ALTURA + 50; // 50px de respiro vertical entre linhas

    for (int i = 0; i < QTD_JOGOS_LISTA; i++) {
        // Usa o operador módulo para repetir jogos caso a categoria tenha menos que 8 itens
        Jogo* jogo = jogosFiltrados[i % jogosFiltrados.size()];
        
        int col = i % COLUNAS;
        int linha = i / COLUNAS;

        int posX = JOGO_GAP + (col * (JOGO_LARGURA + JOGO_GAP));
        int posY = JOGO_Y + (linha * ALTURA_LINHA); 
        
        jogos.push_back(Botao(renderer, posX, posY, JOGO_LARGURA, JOGO_ALTURA,
                              jogo->getCapaTelaPrincipal(), jogo->getNome(), jogo->getDescricao(), false));
        jogos.back().setParentJanela(popupJanela);
    }
}

/**
 * @brief Atualiza as posições dos botões de categorias para simular um carrossel horizontal.
 * 
 * Move as categorias não visíveis para coordenadas fora da tela (-5000) e posiciona
 * as 4 categorias visíveis conforme o `categoriaIndex` definido no estado.
 * 
 * @param estado Referência ao controle de scroll e índices.
 */
void GerenciarInterface::atualizarPosicoes(GerenciarScroll& estado) {
    // Esconde todas inicialmente
    for (auto& cat : categorias) { cat.area.x = -5000; cat.area.y = -5000; }
    
    int numCategorias = categorias.size();
    if (numCategorias == 0) return;

    // Calcula o ponto de partida do carrossel (garantindo índice positivo)
    int startIndex = (estado.categoriaIndex % numCategorias + numCategorias) % numCategorias;
    
    for (int i = 0; i < estado.maxCategoriasVisiveis; ++i) {
        int idx = (startIndex + i) % numCategorias;
        int posX = POSICAO_BASE_X + i * CATEGORIA_ESPACAMENTO;
        categorias[idx].area.x = posX;
        categorias[idx].area.y = CATEGORIA_POS_Y;
    }
    
    // Sincroniza a posição das setas de navegação
    if (setaEsquerda) setaEsquerda->area.y = CATEGORIA_POS_Y;
    if (setaDireita) setaDireita->area.y = CATEGORIA_POS_Y;
}

/**
 * @brief Consolida todos os botões interativos em uma lista plana para o processador de input.
 * 
 * A ordem de inserção aqui define a ordem de tabulação/navegação do controle.
 * @return Referência ao vetor de ponteiros de botões.
 */
std::vector<Botao*>& GerenciarInterface::obterBotoesNavegaveis() {
    botoesNavegaveis.clear();
    // Ordem: Destaques -> Barra Superior -> Categorias -> Aleatório -> Grade de Jogos
    for (auto& destaque : destaques) botoesNavegaveis.push_back(&destaque);
    botoesNavegaveis.push_back(btnConfig);
    botoesNavegaveis.push_back(btnPesquisa);
    botoesNavegaveis.push_back(btnMudarTema);
    for (auto& categoria : categorias) botoesNavegaveis.push_back(&categoria);
    botoesNavegaveis.push_back(btnAleatorio);
    for (auto& jogo : jogos) botoesNavegaveis.push_back(&jogo);
    return botoesNavegaveis;
}

Botao* GerenciarInterface::getBotaoCategoria(int indice) {
    if (indice >= 0 && indice < (int)categorias.size()) return &categorias[indice];
    return nullptr;
}

/**
 * @brief Orquestra a renderização frame-a-frame da interface.
 * 
 * @details **Fluxo de Renderização:**
 * 1. **Atualização Visual:** Sincroniza as cores dos botões base com o tema atual.
 * 2. **Hero Banner:** Desenha a imagem de fundo grande (fundo do destaque selecionado).
 * 3. **Barra de Categorias:** Desenha o fundo, o texto "Categorias", os ícones decorativos (dado) 
 *    e os botões de filtro visíveis (aplicando cores de destaque para a categoria ativa ou focada).
 * 4. **Listas Dinâmicas:** Renderiza o grid de jogos e as miniaturas de destaques.
 * 5. **Sobreposições (Overlay):** Desenha a barra de pesquisa, configurações e o ícone de lupa 
 *    com coordenadas fixas (não sofrem scroll).
 * 6. **Interatividade:** Desenha as imagens de ajuda do controle (Gamepad) se um controle estiver conectado.
 * 7. **Popups e Teclado:** Por último, desenha elementos modais que ficam acima de tudo.
 * 
 * @param renderer O renderizador SDL.
 * @param estado O estado global de navegação e scroll.
 */
void GerenciarInterface::desenhar(SDL_Renderer* renderer, GerenciarScroll& estado) {
    if (!renderer) return;

    auto& tema = GerenciadorTemas::getInstance();
    
    // Sincronização de paleta para os botões do sistema
    SDL_Color btnNorm = tema.getCorBotaoNormal();
    SDL_Color btnHover = tema.getCorBotaoHover();
    SDL_Color btnPress = tema.getCorBotaoPressionado();
    
    btnConfig->setCor({0,0,0,0}, btnHover, btnPress);
    btnMudarTema->setCor({0,0,0,0}, btnHover, btnPress);
    btnAleatorio->setCor(btnNorm, btnHover, btnPress);

    // Swap dinâmico de ícones baseados no tema
    if (btnConfig) {
        SDL_Texture* configTex = (tema.getTemaAtual() == TipoTema::CLARO) ? iconeConfigClaro : iconeConfigEscuro;
        btnConfig->setTexturaImagem(configTex);
    }
    
    // Reseta cor de modulação do tema para garantir cores originais dos ativos
    if (btnMudarTema->getTexturaImagem()) {
        SDL_SetTextureColorMod(btnMudarTema->getTexturaImagem(), 255, 255, 255);
    }

    // 1. Desenho do Banner de Fundo (Seção Hero)
    if (estado.imagensDestaques.size() > 0) {
        if (estado.currentDestaqueIndex >= 0 && estado.currentDestaqueIndex < (int)estado.imagensDestaques.size()) {
            SDL_Texture* bg = gerImg.carregar(renderer, estado.imagensDestaques[estado.currentDestaqueIndex]);
            if (bg) {
                // Aplica scroll negativo para mover o fundo junto com a página
                SDL_Rect fundoRect = {30 - estado.scrollX, 180 - estado.scrollY, 1862, 597};
                SDL_RenderCopy(renderer, bg, NULL, &fundoRect);
            }
        }
    }

    // 2. Elementos Decorativos de Categorias
    if (fundoCategorias) {
        fundoCategorias->mudarCor(tema.getCorRetangulos());
        fundoCategorias->desenhar(renderer, -estado.scrollX, -estado.scrollY);
    }
    
    if (btnAleatorio) btnAleatorio->desenhar(renderer, -estado.scrollX, -estado.scrollY);
    
    desenharTexto(renderer, "Categorias", 40 - estado.scrollX, 950 - estado.scrollY, 
                 tema.getCorTextoNegrito(), 28, TipoFonte::NEGRITO);

    SDL_Texture* dadoAtual = (tema.getTemaAtual() == TipoTema::CLARO) ? iconeDadoClaro : iconeDadoEscuro;
    if (dadoAtual) {
        SDL_Rect rectDado = {1200 - estado.scrollX, 900 - estado.scrollY, 150, 150};
        SDL_RenderCopy(renderer, dadoAtual, NULL, &rectDado);
    }

    // 3. Renderização do Carrossel de Categorias
    int numCategorias = categorias.size();
    if (numCategorias > 0) {
        int startIndex = (estado.categoriaIndex % numCategorias + numCategorias) % numCategorias;
        for (int i = 0; i < estado.maxCategoriasVisiveis; ++i) {
            int idx = (startIndex + i) % numCategorias;
            
            // Lógica de destaque visual (Prioridade: Selecionado > Focado por Gatilho > Normal)
            if (idx == estado.categoriaAtivaIndex) {
                categorias[idx].setCor(tema.getCorBotaoPressionado(), btnHover, btnPress);
            } else if (idx == estado.categoriaFocadaPorGatilho) {
                categorias[idx].setCor(tema.getCorDestaque(), btnHover, btnPress);
            } else {
                categorias[idx].setCor(btnNorm, btnHover, btnPress);
            }

            if (categorias[idx].area.y > 0) {
                categorias[idx].desenhar(renderer, -estado.scrollX, -estado.scrollY);
            }
        }
    }

    // 4. Desenho das Setas de Navegação
    SDL_Color corFundoSeta = (tema.getTemaAtual() == TipoTema::ESCURO) ? SDL_Color{56, 9, 66, 255} : btnNorm;
    
    if (setaEsquerda) {
        setaEsquerda->setCor(corFundoSeta, btnHover, btnPress);
        setaEsquerda->setCorTexto(tema.getCorTextoNegrito());
        setaEsquerda->desenhar(renderer, -estado.scrollX, -estado.scrollY);
    }

    if (setaDireita) {
        setaDireita->setCor(corFundoSeta, btnHover, btnPress);
        setaDireita->setCorTexto(tema.getCorTextoNegrito());
        setaDireita->desenhar(renderer, -estado.scrollX, -estado.scrollY);
    }

    // 5. Renderização das Listas de Jogos (Grid e Destaques)
    for (auto& jogo : jogos) jogo.desenhar(renderer, -estado.scrollX, -estado.scrollY);
    for (auto& dest : destaques) dest.desenhar(renderer, -estado.scrollX, -estado.scrollY);

    // 6. Barra Superior Fixa (Overlay que não sofre scroll)
    if (btnConfig) btnConfig->desenhar(renderer, 0, 0);
    if (btnPesquisa) btnPesquisa->desenhar(renderer, 0, 0); 
    if (btnMudarTema) btnMudarTema->desenhar(renderer, 0, 0);

    SDL_Texture* lupaAtual = (tema.getTemaAtual() == TipoTema::CLARO) ? iconeLupaClaro : iconeLupaEscuro;
    if (lupaAtual) {
        SDL_Rect rectLupa = {1350, 40, 100, 100}; 
        SDL_RenderCopy(renderer, lupaAtual, NULL, &rectLupa);
    }

    // 7. Renderização de Ajuda de Hardware (Gamepad Helper)
    if (SDL_NumJoysticks() > 0) {
        SDL_Texture* explicacaoAtual = (tema.getTemaAtual() == TipoTema::CLARO) ? explicacaoClaro : explicacaoEscuro;
        if (explicacaoAtual) {
            int w = 0, h = 0;
            SDL_QueryTexture(explicacaoAtual, NULL, NULL, &w, &h);
            int posY = 1080 - h; // Alinha ao rodapé da área visível
            SDL_Rect rectExplicacao = { (1920 - w) / 2, posY, w, h };
            SDL_RenderCopy(renderer, explicacaoAtual, NULL, &rectExplicacao);
        }
    }

    // 8. Elementos Modais Superiores (Popups e Teclado)
    if (popupJanela && popupJanela->isPopupOpen()) {
        popupJanela->desenharPopup(-estado.scrollX, -estado.scrollY);
    }
    
    if (tecladoVirtual) tecladoVirtual->desenhar(renderer);
}