/**
 * @file JanelaJogo.cpp
 * @brief Implementação da interface de visualização e execução de um título individual.
 * 
 * Este arquivo coordena a exibição de metadados, galeria de imagens e a lógica 
 * de lançamento (launching) de processos externos.
 */

#include "JanelaJogo.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorImagens.hpp"
#include "GerenciadorTemas.hpp" 
#include <iostream>
#include <sstream>
#include "Utils.hpp"
#include <SDL2/SDL.h>
#include <cstdlib> 

using namespace MeuProjeto;

/** @brief Gerenciador global de imagens para acesso a texturas de capas e capturas. */
extern GerenciadorImagens gerImg;

/** @brief Instância estática para gerenciar as fontes exclusivas da janela de detalhes. */
static GerenciadorFontes gerenciadorFontesJanela;

/**
 * @brief Construtor da JanelaJogo.
 * 
 * @details **Lógica de Inicialização:**
 * 1. **Geometria:** Define as áreas fixas (Topo, Fundo, Imagem, Descrição e Capturas) baseadas na resolução 1920x1080.
 * 2. **Recursos:** Carrega a imagem principal e popula o vetor de texturas de capturas de tela.
 * 3. **Componentes:** Instancia botões (Jogar, Fechar, Setas) definindo fontes e cores iniciais.
 * 4. **Hibridismo:** Verifica se há um controle conectado para definir o foco visual inicial no botão "JOGAR".
 * 
 * @param renderer Renderizador SDL.
 * @param nomeJogo Nome do título.
 * @param imagemPath Caminho da imagem de destaque.
 * @param descricao Texto de sinopse.
 * @param capturasPaths Lista de caminhos para as screenshots.
 */
JanelaJogo::JanelaJogo(SDL_Renderer* renderer, const std::string& nomeJogo, const std::string& imagemPath,
                       const std::string& descricao, const std::vector<std::string>& capturasPaths)
    : renderer(renderer), imagemJogo(nullptr), nomeJogo(nomeJogo), descricao(descricao),
      botaoJogar(1080, 450, 770, 120, "JOGAR"),
      botaoFechar(1830, 10, 80, 80, "X"),
      capturaIndex(0),
      areaJanela{0, 0, 1920, 1080},
      areaTopo{0, 0, 1920, 100},
      areaFundo{50, 120, 1820, 560},
      areaImagem{70, 140, 990, 520},
      areaDescricao{1080, 140, 770, 350},
      areaFundoCapturas{0, 710, 1920, 370},
      setaEsquerda(30, CAPTURA_POS_Y, 120, CAPTURA_ALTURA, "<"),
      setaDireita(1770, CAPTURA_POS_Y, 120, CAPTURA_ALTURA, ">"),
      botaoJogarFocado(true), 
      ultimoTempoLB(0),
      ultimoTempoRB(0),
      animacaoSetaEsquerda(false),
      animacaoSetaDireita(false),
      tempoAnimacaoEsquerda(0),
      tempoAnimacaoDireita(0)
{
    // Carregamento da arte principal do jogo
    imagemJogo = gerImg.carregar(renderer, imagemPath.c_str());
    if (imagemJogo) {
        SDL_SetTextureBlendMode(imagemJogo, SDL_BLENDMODE_BLEND);
    }

    // Carregamento da galeria de screenshots
    for (const auto& path : capturasPaths) {
        SDL_Texture* tex = gerImg.carregar(renderer, path.c_str());
        if (tex) {
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            capturasTexturas.push_back(tex);
        }
    }

    // Estilização dos botões
    botaoJogar.setFonte(TipoFonte::NEGRITO);
    botaoJogar.setTamanhoFonte(42);

    botaoFechar.setFonte(TipoFonte::NORMAL);
    botaoFechar.setTamanhoFonte(48);
    botaoFechar.setCor({200, 20, 20, 255}, {250, 50, 50, 255}, {180, 0, 0, 255});

    setaEsquerda.setTamanhoFonte(56);
    setaDireita.setTamanhoFonte(56);
    
    // Auto-foco para Gamepad
    if (SDL_NumJoysticks() > 0) {
        botaoJogar.setFocado(true);
    }
}

JanelaJogo::~JanelaJogo() {
}

/**
 * @brief Processa eventos de mouse e teclado.
 * 
 * @return true se a janela deve ser fechada.
 */
bool JanelaJogo::tratarEvento(SDL_Event& evento, int offsetX, int offsetY) {
    botaoFechar.tratarEvento(evento, 0, 0);
    
    // Verifica clique no botão Fechar (X)
    if (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT) {
        if (botaoFechar.contemPonto(evento.button.x, evento.button.y)) return true;
    }

    // Verifica clique no botão Jogar
    if (botaoJogar.tratarEvento(evento, 0, 0)) {
        if (evento.type == SDL_MOUSEBUTTONUP) executarJogo();
    }

    // Controle do Carrossel de imagens via Mouse
    setaEsquerda.tratarEvento(evento, 0, 0);
    setaDireita.tratarEvento(evento, 0, 0);
    if (evento.type == SDL_MOUSEBUTTONUP) {
        int mouseX = evento.button.x;
        int mouseY = evento.button.y;
        int numCapturas = capturasTexturas.size();
        if (setaEsquerda.contemPonto(mouseX, mouseY) && numCapturas > 0) {
            capturaIndex = (capturaIndex - 1 + numCapturas) % numCapturas;
            animacaoSetaEsquerda = true;
            tempoAnimacaoEsquerda = SDL_GetTicks();
        }
        else if (setaDireita.contemPonto(mouseX, mouseY) && numCapturas > 0) {
            capturaIndex = (capturaIndex + 1) % numCapturas;
            animacaoSetaDireita = true;
            tempoAnimacaoDireita = SDL_GetTicks();
        }
    }

    // Tecla de atalho global para sair da visualização
    if (evento.type == SDL_KEYDOWN && evento.key.keysym.sym == SDLK_ESCAPE) return true;
    
    return false;
}

/**
 * @brief Processa eventos de botões do controle (Gamepad).
 */
bool JanelaJogo::tratarEventoControle(SDL_Event& evento) {
    if (SDL_NumJoysticks() == 0) return false;

    if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (evento.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_B: return true; // Botão B fecha a janela
            case SDL_CONTROLLER_BUTTON_A:
                if (botaoJogarFocado) botaoJogar.ativarPorControle();
                return false;
            case SDL_CONTROLLER_BUTTON_X: // Atalhos rápidos para iniciar o jogo
            case SDL_CONTROLLER_BUTTON_Y:
                executarJogo();
                return false;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: { // Navegação de capturas (LB)
                Uint32 tempoAtual = SDL_GetTicks();
                if (tempoAtual - ultimoTempoLB < DEBOUNCE_SHOULDER) return false;
                ultimoTempoLB = tempoAtual;
                if (!capturasTexturas.empty()) {
                    capturaIndex = (capturaIndex - 1 + capturasTexturas.size()) % capturasTexturas.size();
                    animacaoSetaEsquerda = true;
                    tempoAnimacaoEsquerda = tempoAtual;
                }
                return false;
            }
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: { // Navegação de capturas (RB)
                Uint32 tempoAtual = SDL_GetTicks();
                if (tempoAtual - ultimoTempoRB < DEBOUNCE_SHOULDER) return false;
                ultimoTempoRB = tempoAtual;
                if (!capturasTexturas.empty()) {
                    capturaIndex = (capturaIndex + 1) % capturasTexturas.size();
                    animacaoSetaDireita = true;
                    tempoAnimacaoDireita = tempoAtual;
                }
                return false;
            }
        }
    }
    else if (evento.type == SDL_CONTROLLERBUTTONUP) {
        if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
            if (botaoJogarFocado) {
                botaoJogar.desativarPorControle();
                executarJogo();
            }
            return false;
        }
    }
    return false;
}

/**
 * @brief Método principal de lançamento do jogo.
 * 
 * @details **Lógica de Execução Detalhada:**
 * Este método simula a funcionalidade de um launcher (como Steam ou Epic).
 * 1. **Montagem do Comando:** O sistema verifica o nome do jogo. Para fins de demonstração e teste de interface, nomes específicos foram mapeados para utilitários do sistema Linux:
 *    - "God of War" -> Inicia a calculadora (`gnome-calculator`).
 *    - "GTA V" -> Inicia o editor de texto (`gedit`).
 *    - "Minecraft" -> Inicia um novo terminal.
 * 2. **Comando Genérico:** Caso o jogo não esteja na lista de testes, ele tenta executar um arquivo binário com o nome exato do jogo no diretório atual (ex: `./"Nome Do Jogo" &`).
 * 3. **Processamento em Background:** O caractere `&` ao final do comando é crucial; ele informa ao sistema operacional para iniciar o processo de forma assíncrona, permitindo que o Launcher continue rodando sem travar enquanto o jogo está aberto.
 * 4. **Chamada de Sistema:** Utiliza `std::system()` para enviar a string ao shell do SO.
 * 5. **Feedback:** O resultado da chamada é verificado; um retorno `0` indica que o comando foi enviado com sucesso ao interpretador de comandos.
 * 
 * @note Em uma versão de produção, o comando seria o caminho absoluto do executável cadastrado no banco de dados.
 */
void JanelaJogo::executarJogo() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "[LAUNCHER] Preparando para iniciar: " << nomeJogo << std::endl;
    
    std::string comando;
    if (nomeJogo == "God of War") comando = "gnome-calculator &";
    else if (nomeJogo == "GTA V") comando = "gedit &";
    else if (nomeJogo == "Minecraft") comando = "x-terminal-emulator &";
    else comando = "./\"" + nomeJogo + "\" &";
    
    std::cout << "[SISTEMA] Comando: " << comando << std::endl;
    
    // Execução física no SO
    int resultado = system(comando.c_str());
    
    if (resultado == 0) {
        std::cout << "[SUCESSO] Processo enviado ao SO." << std::endl;
    } else {
        std::cerr << "[ERRO] Falha ao executar comando. Código: " << resultado << std::endl;
    }
    std::cout << "========================================\n" << std::endl;
}

/**
 * @brief Atualiza o estado das animações das setas baseando-se no tempo decorrido.
 */
void JanelaJogo::atualizarAnimacoes() {
    Uint32 tempoAtual = SDL_GetTicks();
    if (animacaoSetaEsquerda && tempoAtual - tempoAnimacaoEsquerda > DURACAO_ANIMACAO) animacaoSetaEsquerda = false;
    if (animacaoSetaDireita && tempoAtual - tempoAnimacaoDireita > DURACAO_ANIMACAO) animacaoSetaDireita = false;
}

/**
 * @brief Orquestra a renderização completa da tela de detalhes.
 * 
 * @details **Sincronização de Temas:**
 * A cada frame, o método consulta o `GerenciadorTemas` para atualizar as cores dos botões
 * e das setas. Isso garante que, se o tema for alterado globalmente, a Janela de Jogo
 * se adapte instantaneamente, inclusive as setas do carrossel que agora mudam de cor
 * conforme o estado de animação/clique.
 */
void JanelaJogo::desenhar(int offsetX, int offsetY) {
    atualizarAnimacoes();
    
    auto& tema = GerenciadorTemas::getInstance();
    
    // Atualiza botões principais conforme paleta do tema
    botaoJogar.setCor(tema.getCorBotaoNormal(), tema.getCorBotaoHover(), tema.getCorBotaoPressionado());
    
    SDL_Color corSetaBase = tema.getCorBotaoNormal(); 
    SDL_Color corSetaHover = tema.getCorBotaoHover();
    SDL_Color corSetaPress = tema.getCorBotaoPressionado();

    // Aplica lógica de animação de clique nas setas
    setaEsquerda.setCor(animacaoSetaEsquerda ? corSetaPress : corSetaBase, corSetaHover, corSetaPress);
    setaDireita.setCor(animacaoSetaDireita ? corSetaPress : corSetaBase, corSetaHover, corSetaPress);
    
    // Garante legibilidade do texto das setas (< >)
    setaEsquerda.setCorTexto(tema.getCorTextoNegrito());
    setaDireita.setCorTexto(tema.getCorTextoNegrito());

    // Chamadas de desenho em camadas
    desenharFundo();
    desenharTopo();
    desenharAreaFundo();
    desenharImagem(offsetX, offsetY);
    desenharDescricao(offsetX, offsetY);
    desenharCapturas(offsetX, offsetY);
    
    // Reseta qualquer recorte de renderização antes de desenhar botões flutuantes
    SDL_RenderSetClipRect(renderer, NULL);
    
    desenharBotoes(offsetX, offsetY);
}

void JanelaJogo::desenharFundo() {
    SDL_Color cor = GerenciadorTemas::getInstance().getCorFundo();
    SDL_SetRenderDrawColor(renderer, cor.r, cor.g, cor.b, cor.a);
    SDL_RenderFillRect(renderer, &areaJanela);
}

void JanelaJogo::desenharTopo() {
    SDL_Color cor = GerenciadorTemas::getInstance().getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, cor.r, cor.g, cor.b, cor.a);
    SDL_RenderFillRect(renderer, &areaTopo);
    
    SDL_Color corTexto = GerenciadorTemas::getInstance().getCorTextoNegrito();
    // Centralização manual do título no cabeçalho
    MeuProjeto::desenharTexto(renderer, nomeJogo, 960 - (nomeJogo.length() * 12), 26, corTexto, 48, MeuProjeto::TipoFonte::NEGRITO);
}

void JanelaJogo::desenharAreaFundo() {
    SDL_Color cor = GerenciadorTemas::getInstance().getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, cor.r, cor.g, cor.b, cor.a);
    SDL_RenderFillRect(renderer, &areaFundo);
}

void JanelaJogo::desenharImagem(int offsetX, int offsetY) {
    if (imagemJogo) {
        SDL_Rect dst = {areaImagem.x + offsetX, areaImagem.y + offsetY, areaImagem.w, areaImagem.h};
        SDL_RenderCopy(renderer, imagemJogo, nullptr, &dst);
    }
}

/**
 * @brief Renderiza a sinopse do jogo com algoritmo de Word Wrap.
 */
void JanelaJogo::desenharDescricao(int offsetX, int offsetY) {
    int fontSize = 24;
    int linhaAltura = 30;
    int x = areaDescricao.x + offsetX + 10;
    int y = areaDescricao.y + offsetY + 10;
    int maxWidth = areaDescricao.w - 20;

    std::stringstream ss(descricao);
    std::string linha;
    SDL_Color corTexto = GerenciadorTemas::getInstance().getCorTextoNormal();

    while (std::getline(ss, linha, '\n')) {
        std::vector<std::string> linhasQuebradas;
        std::string palavra;
        std::stringstream linhaStream(linha);
        std::string linhaAtual;
        
        while (std::getline(linhaStream, palavra, ' ')) {
            int larguraEstimada = (linhaAtual.length() + palavra.length()) * (fontSize / 2);
            if (larguraEstimada > maxWidth && !linhaAtual.empty()) {
                linhasQuebradas.push_back(linhaAtual);
                linhaAtual = palavra;
            } else {
                if (!linhaAtual.empty()) linhaAtual += " ";
                linhaAtual += palavra;
            }
        }
        if (!linhaAtual.empty()) linhasQuebradas.push_back(linhaAtual);
        
        for (const auto& textoLinha : linhasQuebradas) {
            if (y + linhaAltura > areaDescricao.y + areaDescricao.h) break;
            MeuProjeto::desenharTexto(renderer, textoLinha, x, y, corTexto, fontSize, MeuProjeto::TipoFonte::NORMAL);
            y += linhaAltura;
        }
    }
}

/**
 * @brief Renderiza o carrossel de capturas de tela.
 * 
 * @details **Lógica do Carrossel:**
 * 1. **Fundo:** Desenha o container inferior para as imagens.
 * 2. **Clipping:** Utiliza `SDL_RenderSetClipRect` para garantir que, durante a transição 
 *    entre imagens, elas não "vazem" para fora da área central (entre as setas).
 * 3. **Cálculo de Posição Relativa:** As imagens são posicionadas horizontalmente 
 *    baseadas no `capturaIndex`, permitindo um efeito de rolagem infinita simples.
 */
void JanelaJogo::desenharCapturas(int offsetX, int offsetY) {
    int numCapturas = static_cast<int>(capturasTexturas.size());
    SDL_Color corPainel = GerenciadorTemas::getInstance().getCorRetangulos();
    SDL_Color corTexto = GerenciadorTemas::getInstance().getCorTextoNormal();

    SDL_Rect bgCarrossel = {areaFundoCapturas.x + offsetX, areaFundoCapturas.y + offsetY, areaFundoCapturas.w, areaFundoCapturas.h};
    SDL_SetRenderDrawColor(renderer, corPainel.r, corPainel.g, corPainel.b, corPainel.a);
    SDL_RenderFillRect(renderer, &bgCarrossel);

    std::string titulo = "Capturas de Tela";
    if (numCapturas > 0 && SDL_NumJoysticks() > 0) {
        titulo += " (Use LB/RB para navegar)";
    }
    MeuProjeto::desenharTexto(renderer, titulo, 50, areaFundoCapturas.y + 10, corTexto, 28, MeuProjeto::TipoFonte::NEGRITO);
    
    if (numCapturas == 0) return;

    // Define área de recorte para as imagens não sobreporem as setas
    const int MARGEM_SETAS = 160;
    SDL_Rect clipRect = {MARGEM_SETAS, CAPTURA_POS_Y, 1920 - (MARGEM_SETAS * 2), CAPTURA_ALTURA};
    SDL_RenderSetClipRect(renderer, &clipRect);

    int centroX = 960;
    for (int i = 0; i < numCapturas; ++i) {
        int relPos = i - capturaIndex;
        // Lógica de wrapping (circular)
        if (relPos < -1) relPos += numCapturas;
        if (relPos > 1) relPos -= numCapturas;
        
        int posX = centroX + (relPos * CAPTURA_ESPACAMENTO) - (CAPTURA_LARGURA / 2);
        
        if (abs(relPos) <= 1) {
            SDL_Rect dst = {posX + offsetX, CAPTURA_POS_Y + offsetY, CAPTURA_LARGURA, CAPTURA_ALTURA};
            SDL_RenderCopy(renderer, capturasTexturas[i], nullptr, &dst);
        }
    }
    SDL_RenderSetClipRect(renderer, NULL); // Desativa o recorte
}

/**
 * @brief Desenha os botões interativos e indicadores de controle.
 */
void JanelaJogo::desenharBotoes(int offsetX, int offsetY) {
    // Desenha borda de foco visual para o botão JOGAR se estiver usando controle
    if (botaoJogarFocado && SDL_NumJoysticks() > 0) {
        SDL_Color corDest = GerenciadorTemas::getInstance().getCorDestaque();
        SDL_Rect bordaFoco = {
            botaoJogar.area.x + offsetX - 5, botaoJogar.area.y + offsetY - 5,
            botaoJogar.area.w + 10, botaoJogar.area.h + 10
        };
        SDL_SetRenderDrawColor(renderer, corDest.r, corDest.g, corDest.b, corDest.a);
        SDL_RenderDrawRect(renderer, &bordaFoco);
    }
    
    botaoJogar.desenhar(renderer, offsetX, offsetY);
    botaoFechar.desenhar(renderer, offsetX, offsetY);
    setaEsquerda.desenhar(renderer, offsetX, offsetY);
    setaDireita.desenhar(renderer, offsetX, offsetY);
    
    // Indicadores visuais de botões do controle (LB/RB)
    if (capturasTexturas.size() > 0 && SDL_NumJoysticks() > 0) {
        SDL_Color corTextoSec = GerenciadorTemas::getInstance().getCorTextoNegrito();
        MeuProjeto::desenharTexto(renderer, "LB", 55, CAPTURA_POS_Y - 35, corTextoSec, 20, MeuProjeto::TipoFonte::NEGRITO);
        MeuProjeto::desenharTexto(renderer, "RB", 1785, CAPTURA_POS_Y - 35, corTextoSec, 20, MeuProjeto::TipoFonte::NEGRITO);
    }
}