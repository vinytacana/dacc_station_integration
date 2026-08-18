/**
 * @file JanelaJogo.cpp
 * @brief Implementação da interface de visualização e execução de um título individual.
 *
 * Este arquivo contém a lógica operacional da classe JanelaJogo, gerenciando o ciclo de vida
 * dos elementos visuais, a carga de recursos gráficos, a organização espacial responsiva
 * e o tratamento de interações multiplataforma (mouse, teclado e controle).
 */

#include "JanelaJogo.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorImagens.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include "Utils.hpp"
#include "NetworkClient.hpp"
#include "GerenciadorAudio.hpp"
#include <iostream>
#include <sstream>
#include <SDL2/SDL.h>
#include <cstdlib> 
#include <unistd.h> 

using namespace MeuProjeto;

/** @brief Instância externa do gerenciador de imagens para carregamento de texturas. 
 */
extern GerenciadorImagens gerImg;

/** @brief Gerenciador estático de fontes para otimização de renderização de texto na janela. 
 */
static GerenciadorFontes gerenciadorFontesJanela;

/**
 * @brief Construtor da classe JanelaJogo.
 *
 * Inicializa a estrutura da janela detalhada do jogo realizando as seguintes operações:
 * 1. Define o mapeamento de áreas de layout (topo, fundo, imagem, descrição e capturas) 
 * utilizando o sistema de coordenadas responsivas da classe ConfigLayout.
 * 2. Instancia e configura os botões de ação ("JOGAR", "X" e setas de navegação).
 * 3. Carrega a textura principal do jogo e o vetor de texturas das capturas de tela, 
 * aplicando o modo de mesclagem SDL_BLENDMODE_BLEND para suporte a transparência.
 * 4. Configura propriedades tipográficas (fonte e tamanho) e cromáticas para os botões.
 * 5. Gerencia o estado inicial de foco para dispositivos de controle.
 *
 * @param renderer Ponteiro para o renderizador SDL_Renderer.
 * @param nomeJogo Nome do título a ser exibido.
 * @param imagemPath Caminho do arquivo da imagem de capa.
 * @param descricao Texto descritivo sobre o jogo.
 * @param capturasPaths Lista de caminhos para as imagens da galeria.
 */
JanelaJogo::JanelaJogo(SDL_Renderer* renderer, const std::string& codigo, const std::string& nomeJogo, const std::string& imagemPath,
                       const std::string& descricaoLonga, const std::vector<std::string>& capturasPaths)
    : renderer(renderer), 
      codigoJogo(codigo),
      imagemJogo(nullptr), 
      nomeJogo(nomeJogo), 
      descricaoLonga(descricaoLonga),
      botaoJogar(
          ConfigLayout::X(ConfigLayout::JANELA_JOGAR_POS_X), 
          ConfigLayout::Y(ConfigLayout::JANELA_JOGAR_POS_Y), 
          ConfigLayout::X(ConfigLayout::JANELA_JOGAR_LARGURA), 
          ConfigLayout::Y(ConfigLayout::JANELA_JOGAR_ALTURA), 
          "JOGAR"
      ),
      botaoFechar(
          ConfigLayout::X(ConfigLayout::JANELA_FECHAR_POS_X), 
          ConfigLayout::Y(ConfigLayout::JANELA_FECHAR_POS_Y), 
          ConfigLayout::F(ConfigLayout::JANELA_FECHAR_TAM), 
          ConfigLayout::F(ConfigLayout::JANELA_FECHAR_TAM), 
          "X"
      ),
      areaJanela{0, 0, ConfigLayout::larguraTela, ConfigLayout::alturaTela},
      areaTopo{0, 0, ConfigLayout::larguraTela, ConfigLayout::Y(ConfigLayout::JANELA_TOPO_ALTURA)},
      areaFundo{
          ConfigLayout::X(ConfigLayout::JANELA_PAINEL_POS_X), 
          ConfigLayout::Y(ConfigLayout::JANELA_PAINEL_POS_Y), 
          ConfigLayout::X(ConfigLayout::JANELA_PAINEL_LARGURA), 
          ConfigLayout::Y(ConfigLayout::JANELA_PAINEL_ALTURA)
      },
      areaImagem{
          ConfigLayout::X(ConfigLayout::JANELA_CAPA_POS_X), 
          ConfigLayout::Y(ConfigLayout::JANELA_CAPA_POS_Y), 
          ConfigLayout::X(ConfigLayout::JANELA_CAPA_LARGURA), 
          ConfigLayout::Y(ConfigLayout::JANELA_CAPA_ALTURA)
      },
      areaDescricao{
          ConfigLayout::X(ConfigLayout::JANELA_DESC_POS_X), 
          ConfigLayout::Y(ConfigLayout::JANELA_DESC_POS_Y), 
          ConfigLayout::X(ConfigLayout::JANELA_DESC_LARGURA), 
          ConfigLayout::Y(ConfigLayout::JANELA_DESC_ALTURA)
      },
      areaFundoCapturas{
          0, 
          ConfigLayout::Y(ConfigLayout::JANELA_CARROSSEL_BG_POS_Y), 
          ConfigLayout::larguraTela, 
          ConfigLayout::Y(ConfigLayout::JANELA_CARROSSEL_BG_ALTURA)
      },
      setaEsquerda(
          ConfigLayout::X(ConfigLayout::JANELA_SETA_ESQ_POS_X), 
          ConfigLayout::Y(ConfigLayout::JANELA_SETA_POS_Y), 
          ConfigLayout::X(ConfigLayout::JANELA_SETA_LARGURA), 
          ConfigLayout::Y(ConfigLayout::JANELA_SETA_ALTURA), 
          "<"
      ),
      setaDireita(
          ConfigLayout::X(ConfigLayout::JANELA_SETA_DIR_POS_X), 
          ConfigLayout::Y(ConfigLayout::JANELA_SETA_POS_Y), 
          ConfigLayout::X(ConfigLayout::JANELA_SETA_LARGURA), 
          ConfigLayout::Y(ConfigLayout::JANELA_SETA_ALTURA), 
          ">"
      ),
      capturaIndex(0),
      botaoJogarFocado(true), 
      ultimoTempoLB(0), 
      ultimoTempoRB(0),
      animacaoSetaEsquerda(false), 
      animacaoSetaDireita(false),
      tempoAnimacaoEsquerda(0), 
      tempoAnimacaoDireita(0),
      capturaLargura(ConfigLayout::X(ConfigLayout::JANELA_CAPTURA_LARGURA)),
      capturaAltura(ConfigLayout::Y(ConfigLayout::JANELA_CAPTURA_ALTURA)),
      capturaPosY(ConfigLayout::Y(ConfigLayout::JANELA_CAPTURA_POS_Y)),
      capturaEspacamento(ConfigLayout::X(ConfigLayout::JANELA_CAPTURA_GAP)),
      margemSetas(ConfigLayout::X(ConfigLayout::JANELA_SETA_MARGEM))
{
    this->tempoAbertura = SDL_GetTicks();
    imagemJogo = gerImg.carregar(renderer, imagemPath.c_str());
    if (imagemJogo) SDL_SetTextureBlendMode(imagemJogo, SDL_BLENDMODE_BLEND);

    for (const auto& path : capturasPaths) {
        SDL_Texture* tex = gerImg.carregar(renderer, path.c_str());
        if (tex) {
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            capturasTexturas.push_back(tex);
        }
    }

    botaoJogar.setFonte(TipoFonte::NEGRITO);
    botaoJogar.setTamanhoFonte(ConfigLayout::F(ConfigLayout::JANELA_JOGAR_FONT));
    botaoJogar.setRetanguloBordasArredondadas(20);

    botaoFechar.setFonte(TipoFonte::NORMAL);
    botaoFechar.setTamanhoFonte(ConfigLayout::F(ConfigLayout::JANELA_FECHAR_FONT));
    botaoFechar.setCor({200, 20, 20, 255}, {250, 50, 50, 255}, {180, 0, 0, 255});
    botaoFechar.setIsRound(true);

    setaEsquerda.setTamanhoFonte(ConfigLayout::F(ConfigLayout::JANELA_SETA_FONT));
    setaDireita.setTamanhoFonte(ConfigLayout::F(ConfigLayout::JANELA_SETA_FONT));

    setaEsquerda.setRetanguloBordasArredondadas(20);
    setaDireita.setRetanguloBordasArredondadas(20);
    
    if (SDL_NumJoysticks() > 0) botaoJogar.setFocado(true);
}

/**
 * @brief Destrutor da classe JanelaJogo.
 */
JanelaJogo::~JanelaJogo() {}

/**
 * @brief Processa eventos de entrada baseados em Mouse e Teclado.
 *
 * Implementa a lógica de:
 * 1. Detecção de clique no botão de fechamento.
 * 2. Ativação da execução do jogo ao clicar no botão "JOGAR".
 * 3. Navegação sequencial pela galeria de capturas através das setas direcionais, 
 * acionando os sinalizadores de animação e marcas temporais.
 * 4. Tratamento da tecla ESC para sinalizar o fechamento da janela.
 *
 * @param evento Referência para a estrutura SDL_Event.
 * @return true se o evento solicita o fechamento da janela (clique em fechar ou ESC), false caso contrário.
 */
bool JanelaJogo::tratarEvento(SDL_Event& evento, int, int) {
    botaoFechar.tratarEvento(evento, 0, 0);
    if (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT) {
        if (botaoFechar.contemPonto(evento.button.x, evento.button.y)){
            gerAudio.tocarSom("fechar.wav");
            return true;
        } 
    }
    if (botaoJogar.tratarEvento(evento, 0, 0)) {
        if (evento.type == SDL_MOUSEBUTTONUP) executarJogo();
    }
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
            gerAudio.tocarSom("navegacao.wav");
        }
        else if (setaDireita.contemPonto(mouseX, mouseY) && numCapturas > 0) {
            capturaIndex = (capturaIndex + 1) % numCapturas;
            animacaoSetaDireita = true;
            tempoAnimacaoDireita = SDL_GetTicks();
            gerAudio.tocarSom("navegacao.wav");
        }
    }
    if (evento.type == SDL_KEYDOWN && evento.key.keysym.sym == SDLK_ESCAPE){
        gerAudio.tocarSom("fechar.wav");
        return true;
    } 
    return false;
}

/**
 * @brief Processa eventos provenientes de controladores de jogo (Gamepads).
 *
 * Realiza o mapeamento de botões físicos para ações da interface:
 * - Botão B: Sinaliza retorno/fechamento.
 * - Botão A: Ativa o botão de "JOGAR" (com lógica de pressionamento visual).
 * - Botões X/Y: Execução direta do título.
 * - Left Shoulder (L1/LB) e Right Shoulder (R1/RB): Navegação na galeria com debounce 
 * para evitar disparos acidentais múltiplos.
 *
 * @param evento Referência para a estrutura SDL_Event.
 * @return true se o evento solicita o fechamento/retorno, false para eventos de consumo interno.
 */
bool JanelaJogo::tratarEventoControle(SDL_Event& evento) {
    if (SDL_NumJoysticks() == 0) return false;

    if (SDL_GetTicks() - this->tempoAbertura < 250) {
        return false;
    }

    if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (evento.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_B: 
                gerAudio.tocarSom("fechar.wav");
                return true;
            case SDL_CONTROLLER_BUTTON_A:
                if (botaoJogarFocado) botaoJogar.ativarPorControle();
                return false;
            case SDL_CONTROLLER_BUTTON_X:
            case SDL_CONTROLLER_BUTTON_Y:
                executarJogo();
                return false;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: {
                Uint32 tempoAtual = SDL_GetTicks();
                if (tempoAtual - ultimoTempoLB < DEBOUNCE_SHOULDER) return false;
                ultimoTempoLB = tempoAtual;
                if (!capturasTexturas.empty()) {
                    capturaIndex = (capturaIndex - 1 + capturasTexturas.size()) % capturasTexturas.size();
                    animacaoSetaEsquerda = true;
                    tempoAnimacaoEsquerda = tempoAtual;
                    gerAudio.tocarSom("navegacao.wav");
                }
                return false;
            }
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: {
                Uint32 tempoAtual = SDL_GetTicks();
                if (tempoAtual - ultimoTempoRB < DEBOUNCE_SHOULDER) return false;
                ultimoTempoRB = tempoAtual;
                if (!capturasTexturas.empty()) {
                    capturaIndex = (capturaIndex + 1) % capturasTexturas.size();
                    animacaoSetaDireita = true;
                    tempoAnimacaoDireita = tempoAtual;
                    gerAudio.tocarSom("navegacao.wav");
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

void JanelaJogo::executarJogo() {
    gerAudio.tocarSom("jogar.wav");
    std::cout << "\n[LAUNCHER] Iniciando: " << nomeJogo << std::endl;
    
    std::string path = codigoJogo.find('/') != std::string::npos
        ? caminho_absoluto_projeto(codigoJogo)
        : caminho_absoluto_projeto("games/" + codigoJogo + ".sh"); 

    NetworkClient& network = NetworkClient::getInstance();
    if (!network.sendStartGame(codigoJogo, path)) {
        std::cerr << "[LAUNCHER] Falha ao enviar inicio do jogo: "
                  << network.getLastLaunchError() << std::endl;
    }

    
}

/**
 * @brief Atualiza o estado lógico das animações de feedback visual.
 * Verifica, através de marcas temporais (SDL_GetTicks), se o tempo decorrido desde 
 * o início das animações das setas de navegação ultrapassou a constante DURACAO_ANIMACAO. 
 * Em caso positivo, redefine os sinalizadores booleanos para o estado falso.
 */
void JanelaJogo::atualizarAnimacoes() {
    Uint32 tempoAtual = SDL_GetTicks();
    if (animacaoSetaEsquerda && tempoAtual - tempoAnimacaoEsquerda > DURACAO_ANIMACAO) animacaoSetaEsquerda = false;
    if (animacaoSetaDireita && tempoAtual - tempoAnimacaoDireita > DURACAO_ANIMACAO) animacaoSetaDireita = false;
}

/**
 * @brief Orquestra o ciclo completo de renderização da janela de detalhes.
 * Este método coordena a sequência de desenho de todas as camadas da interface:
 * 1. Atualiza estados de animação.
 * 2. Sincroniza as cores dos componentes (botões e setas) com o GerenciadorTemas.
 * 3. Define as cores de feedback para as setas baseando-se no estado de animação.
 * 4. Chama os métodos especializados para renderizar fundo, topo, descrição, imagens e capturas.
 * 5. Garante a limpeza de áreas de recorte (ClipRect) antes do desenho final dos botões.
 * @param offsetX Deslocamento horizontal global para a renderização.
 * @param offsetY Deslocamento vertical global para a renderização.
 */
void JanelaJogo::desenhar(int offsetX, int offsetY) {
    atualizarAnimacoes();
    auto& tema = GerenciadorTemas::getInstance();
    
    botaoJogar.setCor(tema.getCorBotaoNormal(), tema.getCorBotaoHover(), tema.getCorBotaoPressionado());
    
    SDL_Color corSetaBase = tema.getCorBotaoNormal(); 
    SDL_Color corSetaHover = tema.getCorBotaoHover();
    SDL_Color corSetaPress = tema.getCorBotaoPressionado();

    setaEsquerda.setCor(animacaoSetaEsquerda ? corSetaPress : corSetaBase, corSetaHover, corSetaPress);
    setaDireita.setCor(animacaoSetaDireita ? corSetaPress : corSetaBase, corSetaHover, corSetaPress);
    
    setaEsquerda.setCorTexto(tema.getCorTextoNegrito());
    setaDireita.setCorTexto(tema.getCorTextoNegrito());

    desenharFundo();
    desenharTopo();
    desenharAreaFundo();
    desenharImagem(offsetX, offsetY);
    desenharDescricao(offsetX, offsetY);
    desenharCapturas(offsetX, offsetY);
    
    SDL_RenderSetClipRect(renderer, NULL);
    desenharBotoes(offsetX, offsetY);
}

/**
 * @brief Renderiza a camada base de fundo da janela.
 * Preenche toda a área da janela com a cor de fundo definida pelo tema atual.
 */
void JanelaJogo::desenharFundo() {
    SDL_Color cor = GerenciadorTemas::getInstance().getCorFundo();
    SDL_SetRenderDrawColor(renderer, cor.r, cor.g, cor.b, cor.a);
    SDL_RenderFillRect(renderer, &areaJanela);
}

/**
 * @brief Renderiza a barra superior e o título do jogo.
 * Desenha o retângulo do topo e realiza o cálculo de centralização horizontal 
 * para o nome do jogo, utilizando uma estimativa de largura baseada na escala X.
 */
void JanelaJogo::desenharTopo() {
    SDL_Color cor = GerenciadorTemas::getInstance().getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, cor.r, cor.g, cor.b, cor.a);
    SDL_RenderFillRect(renderer, &areaTopo);
    
    SDL_Color corTexto = GerenciadorTemas::getInstance().getCorTextoNegrito();
    
    int centroTela = ConfigLayout::larguraTela / 2;
    int larguraTextoEstimada = nomeJogo.length() * ConfigLayout::X(12); 
    
    MeuProjeto::desenharTexto(renderer, nomeJogo, 
        centroTela - larguraTextoEstimada, 
        ConfigLayout::Y(ConfigLayout::JANELA_TITULO_POS_Y), 
        corTexto, 
        ConfigLayout::F(ConfigLayout::JANELA_TITULO_FONT), 
        MeuProjeto::TipoFonte::NEGRITO);
}

/**
 * @brief Renderiza o painel central de informações.
 * Desenha o contêiner retangular que servirá de base para a descrição e a imagem de capa.
 */
void JanelaJogo::desenharAreaFundo() {
    SDL_Color cor = GerenciadorTemas::getInstance().getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, cor.r, cor.g, cor.b, cor.a);
    SDL_RenderFillRect(renderer, &areaFundo);
}

/**
 * @brief Renderiza a imagem de capa do jogo.
 * @param offsetX Deslocamento horizontal.
 * @param offsetY Deslocamento vertical.
 */
void JanelaJogo::desenharImagem(int offsetX, int offsetY) {
    if (imagemJogo) {
        SDL_Rect dst = {areaImagem.x + offsetX, areaImagem.y + offsetY, areaImagem.w, areaImagem.h};
        SDL_RenderCopy(renderer, imagemJogo, nullptr, &dst);
    }
}

/**
 * @brief Processa e renderiza o texto de descrição com quebra de linha automática.
 * Implementa um algoritmo de diagramação que:
 * 1. Divide o texto original por quebras de linha explícitas ('\n').
 * 2. Realiza o "Word Wrap" (quebra por palavra) baseando-se na largura estimada 
 * dos caracteres em relação à largura máxima permitida da área de descrição.
 * 3. Respeita os limites verticais da área, interrompendo a renderização caso 
 * o texto exceda a altura disponível.
 * @param offsetX Deslocamento horizontal.
 * @param offsetY Deslocamento vertical.
 */
void JanelaJogo::desenharDescricao(int offsetX, int offsetY) {
    int fontSize = ConfigLayout::F(ConfigLayout::JANELA_DESC_FONT);
    int linhaAltura = ConfigLayout::Y(ConfigLayout::JANELA_DESC_LINE_H);
    int padding = ConfigLayout::X(ConfigLayout::JANELA_DESC_PAD);
    
    int x = areaDescricao.x + offsetX + padding;
    int y = areaDescricao.y + offsetY + padding;
    int maxWidth = areaDescricao.w - (padding * 2);

    std::stringstream ss(descricaoLonga);
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
 * Desenha o contêiner inferior e as miniaturas das capturas. Utiliza a função 
 * SDL_RenderSetClipRect para garantir que as imagens da galeria fiquem confinadas 
 * entre as setas de navegação. O algoritmo calcula a posição relativa das capturas 
 * para exibir a imagem atual centralizada e as adjacentes nas extremidades.
 * @param offsetX Deslocamento horizontal.
 * @param offsetY Deslocamento vertical.
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
    
    int paddingX = ConfigLayout::X(ConfigLayout::JANELA_CARROSSEL_TIT_POS_X);
    int paddingY = ConfigLayout::Y(ConfigLayout::JANELA_CARROSSEL_TIT_POS_Y);
    int fontTitle = ConfigLayout::F(ConfigLayout::JANELA_CARROSSEL_TIT_FONT);
    
    MeuProjeto::desenharTexto(renderer, titulo, paddingX, areaFundoCapturas.y + paddingY, corTexto, fontTitle, MeuProjeto::TipoFonte::NEGRITO);
    
    if (numCapturas == 0) return;

    // Definição da área de recorte para prevenir transbordamento visual sobre as setas
    SDL_Rect clipRect = {
        margemSetas, 
        capturaPosY, 
        ConfigLayout::larguraTela - (margemSetas * 2), 
        capturaAltura
    };
    SDL_RenderSetClipRect(renderer, &clipRect);

    int centroX = ConfigLayout::larguraTela / 2;
    
    for (int i = 0; i < numCapturas; ++i) {
        int relPos = i - capturaIndex;
        // Lógica para carrossel infinito (wrap-around)
        if (relPos < -1) relPos += numCapturas;
        if (relPos > 1) relPos -= numCapturas;
        
        int posX = centroX + (relPos * capturaEspacamento) - (capturaLargura / 2);
        
        // Renderiza apenas os itens vizinhos imediatos para otimização
        if (abs(relPos) <= 1) {
            SDL_Rect dst = {posX + offsetX, capturaPosY + offsetY, capturaLargura, capturaAltura};
            SDL_RenderCopy(renderer, capturasTexturas[i], nullptr, &dst);
        }
    }
    SDL_RenderSetClipRect(renderer, NULL); 
}

/**
 * @brief Renderiza os botões e os indicadores de controle físico.
 * Além de renderizar os objetos de botão, este método desenha um retângulo 
 * de destaque ao redor do botão "JOGAR" se este possuir o foco de controle. 
 * Se houver um gamepad conectado, exibe as etiquetas textuais "LB" e "RB" 
 * acima das setas de navegação para orientar o usuário.
 * @param offsetX Deslocamento horizontal.
 * @param offsetY Deslocamento vertical.
 */
void JanelaJogo::desenharBotoes(int offsetX, int offsetY) {
    
    botaoJogar.desenhar(renderer, offsetX, offsetY);
    botaoFechar.desenhar(renderer, offsetX, offsetY);
    setaEsquerda.desenhar(renderer, offsetX, offsetY);
    setaDireita.desenhar(renderer, offsetX, offsetY);
    
    if (capturasTexturas.size() > 0 && SDL_NumJoysticks() > 0) {
        SDL_Color corTextoSec = GerenciadorTemas::getInstance().getCorTextoNegrito();
        int fontLB = ConfigLayout::F(ConfigLayout::JANELA_LABEL_FONT);
        int posLabelY = capturaPosY - ConfigLayout::Y(ConfigLayout::JANELA_LABEL_OFFSET_Y);
        
        MeuProjeto::desenharTexto(renderer, "LB", ConfigLayout::X(ConfigLayout::JANELA_LABEL_POS_X_LB), posLabelY, corTextoSec, fontLB, MeuProjeto::TipoFonte::NEGRITO);
        MeuProjeto::desenharTexto(renderer, "RB", ConfigLayout::X(ConfigLayout::JANELA_LABEL_POS_X_RB), posLabelY, corTextoSec, fontLB, MeuProjeto::TipoFonte::NEGRITO);
    }
}
