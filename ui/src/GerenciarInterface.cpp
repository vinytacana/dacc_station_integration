#include "GerenciarInterface.hpp"
#include "GerenciadorJogos.hpp"
#include "GerenciadorImagens.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include "Utils.hpp"
#include <iostream>
#include <algorithm>

using namespace MeuProjeto;
using namespace std;

extern GerenciadorJogos gerenciadorJogos;
extern GerenciadorImagens gerImg;

GerenciarInterface::GerenciarInterface() {}

GerenciarInterface::~GerenciarInterface() {
    delete btnConfig; delete btnMudarTema; delete btnAleatorio; delete btnPesquisa;
    delete setaEsquerda; delete setaDireita; delete fundoCategorias; delete popupJanela;
    if (tecladoVirtual) delete tecladoVirtual;
}

bool GerenciarInterface::inicializar(SDL_Renderer* renderer) {
    if (!renderer) return false;
    popupJanela = new Janela(renderer);
    tecladoVirtual = new TecladoVirtual(); 

    // Carrega e Verifica: ConfigClaro
    iconeConfigClaro = gerImg.carregar(renderer, "assets/images/light/ConfigClaro.png");
    if (!iconeConfigClaro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/light/ConfigClaro.png" << endl;
        return false;
    }

    // Carrega e Verifica: ConfigEscuro
    iconeConfigEscuro = gerImg.carregar(renderer, "assets/images/dark/ConfigEscuro.png");
    if (!iconeConfigEscuro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/dark/ConfigEscuro.png" << endl;
        return false;
    }

    btnConfig = new Botao(renderer, 
        ConfigLayout::X(ConfigLayout::BOTAO_CONFIG_POS_X), ConfigLayout::Y(ConfigLayout::BOTAO_CONFIG_POS_Y), 
        ConfigLayout::F(ConfigLayout::BOTAO_CONFIG_LARGURA), ConfigLayout::F(ConfigLayout::BOTAO_CONFIG_ALTURA), 
        "", "Configurações", "", false);
    btnConfig->setIsRound(true);

    btnMudarTema = new Botao(renderer, 
        ConfigLayout::X(ConfigLayout::BOTAO_TEMA_POS_X), ConfigLayout::Y(ConfigLayout::BOTAO_TEMA_POS_Y), 
        ConfigLayout::F(ConfigLayout::BOTAO_TEMA_LARGURA), ConfigLayout::F(ConfigLayout::BOTAO_TEMA_ALTURA), 
        "assets/images/icons/mudarTema.png", "Mudar Tema", "", false);
    btnMudarTema->setIsRound(true);

    // Verifica imagem interna do botão Mudar Tema
    if (!btnMudarTema->getTexturaImagem()) {
        cerr << "[ERRO] Falha ao carregar imagem do botão: assets/images/icons/mudarTema.png" << endl;
        return false;
    }

    btnPesquisa = new BotaoPesquisa(renderer, 
        ConfigLayout::X(ConfigLayout::PESQUISA_POS_X), ConfigLayout::Y(ConfigLayout::PESQUISA_POS_Y), 
        ConfigLayout::F(ConfigLayout::PESQUISA_LARGURA), ConfigLayout::F(ConfigLayout::PESQUISA_ALTURA), 
        "Barra de Pesquisa");
    
    btnPesquisa->setFontePlaceholder(TipoFonte::NORMAL);
    btnPesquisa->setTamanhoFontePlaceholder(ConfigLayout::F(24));
    btnPesquisa->setFonteResultados(TipoFonte::NEGRITO);
    btnPesquisa->setTamanhoFonteResultados(ConfigLayout::F(32));

    // Carrega e Verifica: Lupas
    iconeLupaClaro = gerImg.carregar(renderer, "assets/images/light/lupa_PesquisaClaro.png");
    if (!iconeLupaClaro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/light/lupa_PesquisaClaro.png" << endl;
        return false;
    }

    iconeLupaEscuro = gerImg.carregar(renderer, "assets/images/dark/lupa_PesquisaEscuro.png");
    if (!iconeLupaEscuro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/dark/lupa_PesquisaEscuro.png" << endl;
        return false;
    }
    
    // Carrega e Verifica: Explicações
    explicacaoClaro = gerImg.carregar(renderer, "assets/images/light/explicacaoBotoesClaro.jpg");
    if (!explicacaoClaro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/light/explicacaoBotoesClaro.jpg" << endl;
        return false;
    }

    explicacaoEscuro = gerImg.carregar(renderer, "assets/images/dark/explicacaoBotoesEscuro.jpg");
    if (!explicacaoEscuro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/dark/explicacaoBotoesEscuro.jpg" << endl;
        return false;
    }

    vector<string> nomesCategorias = {"Aventura", "Casual", "Ação", "Rpg", "Estrategia", "Esportes"};
    
    for (const auto& nome : nomesCategorias) {
        categorias.push_back(Botao(0, 0, ConfigLayout::F(ConfigLayout::ITEM_CATEGORIA_LARGURA), ConfigLayout::F(ConfigLayout::ITEM_CATEGORIA_ALTURA), nome));
    }
    
    if (categorias.size() > 0) categorias[0].setTamanhoFonte(ConfigLayout::F(24));
    if (categorias.size() > 4) categorias[4].setTamanhoFonte(ConfigLayout::F(22));
    if (categorias.size() > 5) categorias[5].setTamanhoFonte(ConfigLayout::F(24));

    fundoCategorias = new Forma(
        ConfigLayout::X(ConfigLayout::PAINEL_CATEGORIA_POS_X), ConfigLayout::Y(ConfigLayout::PAINEL_CATEGORIA_POS_Y), 
        ConfigLayout::F(ConfigLayout::PAINEL_CATEGORIA_LARGURA), ConfigLayout::F(ConfigLayout::PAINEL_CATEGORIA_ALTURA), 
        {50, 50, 60, 255}, "retangulo");

    setaEsquerda = new Botao(ConfigLayout::X(220), ConfigLayout::Y(ConfigLayout::ITEM_CATEGORIA_POS_Y), ConfigLayout::F(40), ConfigLayout::F(80), "<");
    setaDireita = new Botao(ConfigLayout::X(960), ConfigLayout::Y(ConfigLayout::ITEM_CATEGORIA_POS_Y), ConfigLayout::F(40), ConfigLayout::F(80), ">");
    setaEsquerda->setFonte(TipoFonte::NORMAL);
    setaDireita->setFonte(TipoFonte::NORMAL);

    btnAleatorio = new Botao(
        ConfigLayout::X(ConfigLayout::BOTAO_ALEATORIO_POS_X), ConfigLayout::Y(ConfigLayout::BOTAO_ALEATORIO_POS_Y), 
        ConfigLayout::F(ConfigLayout::BOTAO_ALEATORIO_LARGURA), ConfigLayout::F(ConfigLayout::BOTAO_ALEATORIO_ALTURA), 
        "Escolher aleatoriamente");
    btnAleatorio->setTamanhoFonte(ConfigLayout::F(24));

    // Carrega e Verifica: Dados Aleatórios
    iconeDadoClaro = gerImg.carregar(renderer, "assets/images/light/jogoAleatorioClaro.png");
    if (!iconeDadoClaro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/light/jogoAleatorioClaro.png" << endl;
        return false;
    }

    iconeDadoEscuro = gerImg.carregar(renderer, "assets/images/dark/jogoAleatorioEscuro.png");
    if (!iconeDadoEscuro) {
        cerr << "[ERRO] Falha ao carregar: assets/images/dark/jogoAleatorioEscuro.png" << endl;
        return false;
    }

    int startX = ConfigLayout::X(ConfigLayout::DESTAQUE_MINI_POS_X_INICIAL);
    int startY = ConfigLayout::Y(ConfigLayout::DESTAQUE_MINI_POS_Y);
    int tamanho = ConfigLayout::F(ConfigLayout::DESTAQUE_MINI_TAMANHO);
    int espacamento = ConfigLayout::F(ConfigLayout::DESTAQUE_MINI_PASSO_TOTAL);

    vector<Jogo> todosJogos = gerenciadorJogos.listarJogos();
    
    destaques.clear(); 
    for (int i = 0; i < std::min(5, (int)todosJogos.size()); i++) {
        Jogo& jogo = todosJogos[i];
        destaques.push_back(Botao(renderer, startX + (i * espacamento), startY, tamanho, tamanho,
                                  jogo.getCapaTelaPrincipal(), jogo.getNome(), jogo.getDescricao(), true));
        
        // Verifica se a capa do jogo (destaque) carregou
        if (!destaques.back().getTexturaImagem()) {
            cerr << "[ERRO] Falha ao carregar capa do destaque: " << jogo.getNome() << endl;
            return false;
        }

        destaques.back().setParentJanela(popupJanela);
    }
    
    atualizarJogosPorCategoria("Todas", renderer);

    return true; // Sucesso
}

void GerenciarInterface::atualizarJogosPorCategoria(const std::string& categoria, SDL_Renderer* renderer) {
    jogos.clear();
    vector<Jogo*> jogosFiltrados = gerenciadorJogos.buscarPorCategoria(categoria);
    
    if (jogosFiltrados.empty()) return;

    const int COLUNAS = ConfigLayout::GRADE_JOGOS_COLUNAS;
    const int ALTURA_LINHA = ConfigLayout::F(ConfigLayout::GRADE_JOGOS_ALTURA_ITEM) + ConfigLayout::F(ConfigLayout::GRADE_JOGOS_ESPACO_Y); 

    for (int i = 0; i < 10; i++) { 
        Jogo* jogo = jogosFiltrados[i % jogosFiltrados.size()];
        
        int col = i % COLUNAS;
        int linha = i / COLUNAS;

        int larguraItem = ConfigLayout::F(ConfigLayout::GRADE_JOGOS_LARGURA_ITEM);
        int espacoX = ConfigLayout::F(ConfigLayout::GRADE_JOGOS_ESPACO_X);
        int alturaItem = ConfigLayout::F(ConfigLayout::GRADE_JOGOS_ALTURA_ITEM);
        int yInicial = ConfigLayout::Y(ConfigLayout::GRADE_JOGOS_Y_INICIAL);

        int posX = espacoX + (col * (larguraItem + espacoX));
        int posY = yInicial + (linha * ALTURA_LINHA); 
        
        int posXAbs = ConfigLayout::X(ConfigLayout::GRADE_JOGOS_ESPACO_X) + (col * (larguraItem + espacoX));
        
        jogos.push_back(Botao(renderer, posXAbs, posY, 
                              larguraItem, alturaItem,
                              jogo->getCapaTelaPrincipal(), jogo->getNome(), jogo->getDescricao(), false));
        jogos.back().setParentJanela(popupJanela);
    }
}

void GerenciarInterface::atualizarPosicoes(GerenciarScroll& estado) {
    for (auto& cat : categorias) { cat.area.x = -5000; cat.area.y = -5000; }
    
    int numCategorias = categorias.size();
    if (numCategorias == 0) return;
    
    int startIndex = (estado.categoriaIndex % numCategorias + numCategorias) % numCategorias;
  
    for (int i = 0; i < ConfigLayout::CATEGORIA_QTD_VISIVEL; ++i) {
        int idx = (startIndex + i) % numCategorias;
        int baseX = ConfigLayout::X(ConfigLayout::ITEM_CATEGORIA_POS_X_BASE);
        int passo = ConfigLayout::F(ConfigLayout::ITEM_CATEGORIA_PASSO);
        int posX = baseX + i * passo;
        
        categorias[idx].area.x = posX;
        categorias[idx].area.y = ConfigLayout::Y(ConfigLayout::ITEM_CATEGORIA_POS_Y);
    }
    
    if (setaEsquerda) setaEsquerda->area.y = ConfigLayout::Y(ConfigLayout::ITEM_CATEGORIA_POS_Y);
    if (setaDireita) setaDireita->area.y = ConfigLayout::Y(ConfigLayout::ITEM_CATEGORIA_POS_Y);
}

std::vector<Botao*>& GerenciarInterface::obterBotoesNavegaveis() {
    botoesNavegaveis.clear();
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

void GerenciarInterface::desenhar(SDL_Renderer* renderer, GerenciarScroll& estado) {
    if (!renderer) return;

    auto& tema = GerenciadorTemas::getInstance();
    
    SDL_Color btnNorm = tema.getCorBotaoNormal();
    SDL_Color btnHover = tema.getCorBotaoHover();
    SDL_Color btnPress = tema.getCorBotaoPressionado();
    
    btnConfig->setCor({0,0,0,0}, btnHover, btnPress);
    btnMudarTema->setCor({0,0,0,0}, btnHover, btnPress);
    btnAleatorio->setCor(btnNorm, btnHover, btnPress);

    if (btnConfig) {
        SDL_Texture* configTex = (tema.getTemaAtual() == TipoTema::CLARO) ? iconeConfigClaro : iconeConfigEscuro;
        btnConfig->setTexturaImagem(configTex);
    }
    
    if (btnMudarTema->getTexturaImagem()) {
        SDL_SetTextureColorMod(btnMudarTema->getTexturaImagem(), 255, 255, 255);
    }

    if (estado.imagensDestaques.size() > 0) {
        if (estado.currentDestaqueIndex >= 0 && estado.currentDestaqueIndex < (int)estado.imagensDestaques.size()) {
            SDL_Texture* bg = gerImg.carregar(renderer, estado.imagensDestaques[estado.currentDestaqueIndex]);
            if (bg) {
                SDL_Rect fundoRect = {
                    ConfigLayout::X(ConfigLayout::DESTAQUE_PRINCIPAL_POS_X) - estado.scrollX, 
                    ConfigLayout::Y(ConfigLayout::DESTAQUE_PRINCIPAL_POS_Y) - estado.scrollY, 
                    ConfigLayout::F(ConfigLayout::DESTAQUE_PRINCIPAL_LARGURA), 
                    ConfigLayout::F(ConfigLayout::DESTAQUE_PRINCIPAL_ALTURA)
                };
                SDL_RenderCopy(renderer, bg, NULL, &fundoRect);
            }
        }
    }

    if (fundoCategorias) {
        fundoCategorias->mudarCor(tema.getCorRetangulos());
        fundoCategorias->desenhar(renderer, -estado.scrollX, -estado.scrollY);
    }
    
    if (btnAleatorio) btnAleatorio->desenhar(renderer, -estado.scrollX, -estado.scrollY);
    
    desenharTexto(renderer, "Categorias", 
        ConfigLayout::X(ConfigLayout::TITULO_CATEGORIA_POS_X) - estado.scrollX, 
        ConfigLayout::Y(ConfigLayout::TITULO_CATEGORIA_POS_Y) - estado.scrollY, 
        tema.getCorTextoNegrito(), ConfigLayout::F(28), TipoFonte::NEGRITO);

    SDL_Texture* dadoAtual = (tema.getTemaAtual() == TipoTema::CLARO) ? iconeDadoClaro : iconeDadoEscuro;
    if (dadoAtual) {
        SDL_Rect rectDado = {
            ConfigLayout::X(ConfigLayout::BOTAO_ALEATORIO_POS_X) - estado.scrollX, 
            ConfigLayout::Y(ConfigLayout::BOTAO_ALEATORIO_POS_Y) - ConfigLayout::Y(20) - estado.scrollY, 
            ConfigLayout::F(150), ConfigLayout::F(150)
        };
        SDL_RenderCopy(renderer, dadoAtual, NULL, &rectDado);
    }

    int numCategorias = categorias.size();
    if (numCategorias > 0) {
        int startIndex = (estado.categoriaIndex % numCategorias + numCategorias) % numCategorias;
        for (int i = 0; i < ConfigLayout::CATEGORIA_QTD_VISIVEL; ++i) {
            int idx = (startIndex + i) % numCategorias;
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

    SDL_Color corFundoSeta;
    SDL_Color corTextoSeta;

    if (tema.getTemaAtual() == TipoTema::ESCURO) {
        corFundoSeta = {56, 9, 66, 255}; 
        corTextoSeta = tema.getCorTextoNegrito(); 
    } else {
        corFundoSeta = btnNorm; 
        corTextoSeta = tema.getCorTextoNegrito(); 
    }
    
    if (setaEsquerda) {
        setaEsquerda->setCor(corFundoSeta, btnHover, btnPress);
        setaEsquerda->setCorTexto(corTextoSeta);
        setaEsquerda->desenhar(renderer, -estado.scrollX, -estado.scrollY);
    }

    if (setaDireita) {
        setaDireita->setCor(corFundoSeta, btnHover, btnPress);
        setaDireita->setCorTexto(corTextoSeta);
        setaDireita->desenhar(renderer, -estado.scrollX, -estado.scrollY);
    }

    for (auto& jogo : jogos) jogo.desenhar(renderer, -estado.scrollX, -estado.scrollY);
    for (auto& dest : destaques) dest.desenhar(renderer, -estado.scrollX, -estado.scrollY);

    if (btnConfig) btnConfig->desenhar(renderer, 0, 0);
    if (btnPesquisa) btnPesquisa->desenhar(renderer, 0, 0); 
    if (btnMudarTema) btnMudarTema->desenhar(renderer, 0, 0);

    SDL_Texture* lupaAtual = (tema.getTemaAtual() == TipoTema::CLARO) ? iconeLupaClaro : iconeLupaEscuro;
    if (lupaAtual) {
        SDL_Rect rectLupa = {
            ConfigLayout::X(ConfigLayout::ICONE_LUPA_POS_X), 
            ConfigLayout::Y(ConfigLayout::ICONE_LUPA_POS_Y), 
            ConfigLayout::F(ConfigLayout::ICONE_LUPA_TAMANHO), 
            ConfigLayout::F(ConfigLayout::ICONE_LUPA_TAMANHO)
        }; 
        SDL_RenderCopy(renderer, lupaAtual, NULL, &rectLupa);
    }

    SDL_Texture* explicacaoAtual = (tema.getTemaAtual() == TipoTema::CLARO) ? explicacaoClaro : explicacaoEscuro;
    
    if (explicacaoAtual) {
        int w = 0, h = 0;
        SDL_QueryTexture(explicacaoAtual, NULL, NULL, &w, &h);
        
        int posY = ConfigLayout::Y(1080) - h;
        SDL_Rect rectExplicacao = { (ConfigLayout::larguraTela - w) / 2, posY, w, h };
        SDL_RenderCopy(renderer, explicacaoAtual, NULL, &rectExplicacao);
    }

    if (popupJanela && popupJanela->isPopupOpen()) {
        popupJanela->desenharPopup(-estado.scrollX, -estado.scrollY);
    }
    
    if (tecladoVirtual) tecladoVirtual->desenhar(renderer);
}