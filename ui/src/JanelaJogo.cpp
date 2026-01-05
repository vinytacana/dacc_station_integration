#include "JanelaJogo.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorImagens.hpp"
#include <iostream>
#include <sstream>
#include "Utils.hpp"
#include "Theme.hpp"
#include "GraphicsUtils.hpp"
#include "NetworkClient.hpp" // Added NetworkClient
#include <SDL2/SDL.h>

// Removed raw socket headers as they are now encapsulated in NetworkClient
#include "json.hpp"

using json = nlohmann::json;
using namespace MeuProjeto;

extern GerenciadorImagens gerImg;
// Variável estática não utilizada, mas mantida no escopo original
static GerenciadorFontes gerenciadorFontesJanela;

/**
 * @brief Construtor da Janela de Jogo.
 */
JanelaJogo::JanelaJogo(SDL_Renderer* renderer, const std::string& nomeJogo, const std::string& imagemPath,
                       const std::string& descricao, const std::vector<std::string>& capturasPaths,
                       const std::string& pathExecutavel)
    : renderer(renderer), imagemJogo(nullptr), nomeJogo(nomeJogo), descricao(descricao), pathExecutavel(pathExecutavel),
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
      setaDireita(1770, CAPTURA_POS_Y, 120, CAPTURA_ALTURA, ">")
{
    // Carrega a imagem principal
    imagemJogo = gerImg.carregar(renderer, imagemPath.c_str());
    if (imagemJogo) {
        SDL_SetTextureBlendMode(imagemJogo, SDL_BLENDMODE_BLEND);
    }

    // Carrega as texturas das capturas de tela
    for (const auto& path : capturasPaths) {
        SDL_Texture* tex = gerImg.carregar(renderer, path.c_str());
        if (tex) {
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            capturasTexturas.push_back(tex);
        }
    }

    // Configurar cores dos botões usando o THEME
    // Botão Jogar: Azul Steam (Accent)
    botaoJogar.setCor(Theme::ACCENT_COLOR, 
                      {Theme::ACCENT_COLOR.r, Theme::ACCENT_COLOR.g, Theme::ACCENT_COLOR.b, 200}, // Hover mais claro/transparente
                      {Theme::ACCENT_COLOR.r, Theme::ACCENT_COLOR.g, Theme::ACCENT_COLOR.b, 150}); // Click

    // Botão Fechar: Vermelho Perigo
    botaoFechar.setCor(Theme::DANGER_COLOR,
                       {255, 80, 80, 255},
                       {180, 0, 0, 255});

    // Setas: Neutras
    setaEsquerda.setCor(Theme::BTN_NORMAL, Theme::BTN_HOVER, Theme::BTN_ACTIVE);
    setaDireita.setCor(Theme::BTN_NORMAL, Theme::BTN_HOVER, Theme::BTN_ACTIVE);

    // Configurar fontes dos botões
    botaoJogar.setFonte(TipoFonte::NEGRITO);
    botaoJogar.setTamanhoFonte(42);

    botaoFechar.setFonte(TipoFonte::NORMAL);
    botaoFechar.setTamanhoFonte(48);

    setaEsquerda.setFonte(TipoFonte::NORMAL);
    setaEsquerda.setTamanhoFonte(56);

    setaDireita.setFonte(TipoFonte::NORMAL);
    setaDireita.setTamanhoFonte(56);
}

/**
 * @brief Destrutor.
 */
JanelaJogo::~JanelaJogo() {
    std::cout << "[DEBUG JanelaJogo] Destruidor chamado." << std::endl;
}

/**
 * @brief Trata eventos de mouse e teclado.
 *
 * @param evento A estrutura SDL_Event.
 * @param offsetX Deslocamento X.
 * @param offsetY Deslocamento Y.
 * @return Retorna @c true se o evento indica que a janela deve ser fechada.
 */
bool JanelaJogo::tratarEvento(SDL_Event& evento, int offsetX, int offsetY) {
    // BOTÃO FECHAR (X)
    botaoFechar.tratarEvento(evento, 0, 0);
    if (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT) {
        int mouseX = evento.button.x;
        int mouseY = evento.button.y;
        
        if (botaoFechar.contemPonto(mouseX, mouseY)) {
            std::cout << "  [DEBUG] ✓ Botão X clicado - fechando janela!" << std::endl;
            return true;
        }
    }
    
    // BOTÃO JOGAR 
    if (botaoJogar.tratarEvento(evento, 0, 0)) {
        if (evento.type == SDL_MOUSEBUTTONUP) {
            std::cout << "  [DEBUG] ✓ Botão JOGAR clicado!" << std::endl;
            // *** NOVA FUNCIONALIDADE: Executar o jogo ***
            executarJogo();
            // Não fecha a janela, apenas executa o jogo
        }
    }

    // SETAS (Carrossel)
    setaEsquerda.tratarEvento(evento, 0, 0);
    setaDireita.tratarEvento(evento, 0, 0);
    
    if (evento.type == SDL_MOUSEBUTTONUP) {
        int mouseX = evento.button.x;
        int mouseY = evento.button.y;
        
        int numCapturas = capturasTexturas.size();
        
        if (setaEsquerda.contemPonto(mouseX, mouseY) && numCapturas > 0) {
            // Navegação circular para a esquerda
            capturaIndex = (capturaIndex - 1 + numCapturas) % numCapturas;
        }
        else if (setaDireita.contemPonto(mouseX, mouseY) && numCapturas > 0) {
            // Navegação circular para a direita
            capturaIndex = (capturaIndex + 1) % numCapturas;
        }
    }

    // ===== TECLA ESC =====
    if (evento.type == SDL_KEYDOWN && evento.key.keysym.sym == SDLK_ESCAPE) {
        std::cout << "  [DEBUG] ESC pressionado - fechando janela!" << std::endl;
        return true;
    }

    return false;
}

/**
 * @brief Inicia o processo de execução do jogo.
 *
 * Esta função deve conter a lógica de chamada do sistema operacional
 * ou API de launcher para iniciar o jogo.
 */
void JanelaJogo::executarJogo() {
    std::cout << "[LAUNCHER] Tentando iniciar o jogo: " << nomeJogo << std::endl;
    
    if (pathExecutavel.empty()) {
        std::cerr << "[LAUNCHER ERRO] Caminho do executável não definido para " << nomeJogo << std::endl;
        return;
    }

    // Usando NetworkClient (Singleton) para enviar o comando
    // Por enquanto, hardcoded parameters para Gamescope (podem vir de config futura)
    NetworkClient::getInstance().sendStartGame(
        nomeJogo, 
        pathExecutavel, 
        1280, 720, 60, false // useGamescope=false inicial
    );
}

/**
 * @brief Desenha todos os elementos da janela.
 */
void JanelaJogo::desenhar(int offsetX, int offsetY) {
    desenharFundo();
    //desenharTopo(); // Topo removido ou integrado ao design limpo
    desenharAreaFundo();
    desenharImagem(offsetX, offsetY);
    desenharDescricao(offsetX, offsetY);
    desenharCapturas(offsetX, offsetY);
    
    // Limpa o clip rect antes de desenhar os botões que ficam por cima
    SDL_RenderSetClipRect(renderer, NULL); 
    
    desenharBotoes(offsetX, offsetY);
}

/**
 * @brief Desenha o fundo da janela (Ambient Backdrop).
 */
void JanelaJogo::desenharFundo() {
    // 1. Preenche com a cor base (caso a imagem falhe)
    SDL_SetRenderDrawColor(renderer, Theme::BG_PRIMARY.r, Theme::BG_PRIMARY.g, Theme::BG_PRIMARY.b, 255);
    SDL_RenderFillRect(renderer, &areaJanela);

    // 2. Desenha a imagem do jogo esticada no fundo
    if (imagemJogo) {
        GraphicsUtils::drawTextureCover(renderer, imagemJogo, areaJanela, 255);
    }

    // 3. Aplica o overlay escuro (Vidro fumê)
    // Se tiver imagem, escurece bastante (220/255). Se não, escurece pouco.
    int overlayAlpha = imagemJogo ? 235 : 50; 
    GraphicsUtils::drawOverlay(renderer, areaJanela.w, areaJanela.h, overlayAlpha);
}

/**
 * @brief Desenha o cabeçalho e o nome do jogo.
 */
void JanelaJogo::desenharTopo() {
    // Mantido por compatibilidade, mas idealmente seria removido ou redesenhado
    SDL_SetRenderDrawColor(renderer, Theme::BG_PANEL.r, Theme::BG_PANEL.g, Theme::BG_PANEL.b, Theme::BG_PANEL.a);
    SDL_RenderFillRect(renderer, &areaTopo);
}

/**
 * @brief Desenha a área de fundo do painel principal.
 */
void JanelaJogo::desenharAreaFundo() {
    // Retângulo arredondado translúcido para segurar o conteúdo
    GraphicsUtils::drawRoundedRect(renderer, areaFundo, Theme::RADIUS_LARGE, Theme::BG_PANEL);

    // Nome do jogo desenhado acima/dentro da área
    int fontSize = 52; // Fonte maior
    int xPos = areaFundo.x + 30; // Alinhado a esquerda agora, mais moderno
    MeuProjeto::desenharTexto(renderer, nomeJogo, xPos, areaFundo.y - 30, Theme::TEXT_PRIMARY, fontSize, MeuProjeto::TipoFonte::NEGRITO);
}

/**
 * @brief Desenha a imagem principal/capa do jogo.
 */
void JanelaJogo::desenharImagem(int offsetX, int offsetY) {
    if (imagemJogo) {
        SDL_Rect dst = {areaImagem.x + offsetX, areaImagem.y + offsetY, areaImagem.w, areaImagem.h};
        // Opcional: Borda na imagem
        // GraphicsUtils::drawRoundedRectOutline(renderer, dst, 0, Theme::TEXT_SECONDARY);
        SDL_RenderCopy(renderer, imagemJogo, nullptr, &dst);
    }
}

/**
 * @brief Desenha a descrição com quebra de linha.
 */
void JanelaJogo::desenharDescricao(int offsetX, int offsetY) {
    int fontSize = 24;
    int linhaAltura = 32; // Mais entrelinha para leitura
    
    int x = areaDescricao.x + offsetX + 10;
    int y = areaDescricao.y + offsetY + 10;
    int maxWidth = areaDescricao.w - 20;

    std::stringstream ss(descricao);
    std::string linhaOriginal;

    // Processa linha a linha, respeitando quebras de linha manuais (\n)
    while (std::getline(ss, linhaOriginal, '\n')) {
        std::vector<std::string> linhasQuebradas;
        std::string palavra;
        std::stringstream linhaStream(linhaOriginal);
        std::string linhaAtual;
        
        // Quebra de linha automática (por palavra)
        while (std::getline(linhaStream, palavra, ' ')) {
            // Estimativa de largura usando proporção simples
            int larguraEstimada = (int)(linhaAtual.length() + palavra.length()) * (fontSize / 2);
            
            if (larguraEstimada > maxWidth && !linhaAtual.empty()) {
                linhasQuebradas.push_back(linhaAtual);
                linhaAtual = palavra;
            } else {
                if (!linhaAtual.empty()) linhaAtual += " ";
                linhaAtual += palavra;
            }
        }
        
        if (!linhaAtual.empty()) {
            linhasQuebradas.push_back(linhaAtual);
        }
        
        // Desenha as linhas resultantes
        for (const auto& textoLinha : linhasQuebradas) {
            if (y + linhaAltura > areaDescricao.y + areaDescricao.h) break;
         
            MeuProjeto::desenharTexto(renderer, textoLinha, x, y, Theme::TEXT_PRIMARY, fontSize, MeuProjeto::TipoFonte::NORMAL);
            y += linhaAltura;
        }
    }
}

/**
 * @brief Desenha o carrossel circular de capturas de tela.
 */
void JanelaJogo::desenharCapturas(int offsetX, int offsetY) {
    int numCapturas = static_cast<int>(capturasTexturas.size());

    // Fundo do Carrossel (Arredondado agora)
    SDL_Rect bgCarrossel = {areaFundoCapturas.x + offsetX + 20, areaFundoCapturas.y + offsetY, areaFundoCapturas.w - 40, areaFundoCapturas.h};
    GraphicsUtils::drawRoundedRect(renderer, bgCarrossel, Theme::RADIUS_MEDIUM, Theme::BG_PANEL);

    // Título
    MeuProjeto::desenharTexto(renderer, "Capturas de Tela", 60, areaFundoCapturas.y + 15, Theme::TEXT_SECONDARY, 28 ,MeuProjeto::TipoFonte::NEGRITO);
    
    if (numCapturas == 0) {
        return;
    }

    const int MARGEM_SETAS = 160; // Espaço para as setas
    // Área visível do carrossel (recorte)
    SDL_Rect clipRect = {MARGEM_SETAS, CAPTURA_POS_Y, 1920 - (MARGEM_SETAS * 2), CAPTURA_ALTURA};
    SDL_RenderSetClipRect(renderer, &clipRect); // Aplica o recorte

    int centroX = 960; // Centro da tela
    
    for (int i = 0; i < numCapturas; ++i) {
        // Calcula posição relativa ao índice central
        int relPos = i - capturaIndex;
        
        // Ajuste para carrossel circular (mostra apenas vizinhos imediatos)
        if (relPos < -1) relPos += numCapturas;
        if (relPos > 1) relPos -= numCapturas;
        
        // Posição X centralizada com espaçamento
        int posX = centroX + (relPos * CAPTURA_ESPACAMENTO) - (CAPTURA_LARGURA / 2);
        
        // Desenha apenas as capturas visíveis (central, esquerda e direita)
        if (abs(relPos) <= 1) {
            SDL_Rect dst = {
                posX + offsetX,
                CAPTURA_POS_Y + offsetY,
                CAPTURA_LARGURA,
                CAPTURA_ALTURA
            };
            
            SDL_RenderCopy(renderer, capturasTexturas[i], nullptr, &dst);
            
            // Borda fina na screenshot
            // GraphicsUtils::drawRoundedRectOutline(renderer, dst, 0, {255,255,255,50});
        }
    }
    
    SDL_RenderSetClipRect(renderer, NULL); // Remove o recorte
}

/**
 * @brief Desenha todos os botões (Jogar, Fechar, Setas).
 */
void JanelaJogo::desenharBotoes(int offsetX, int offsetY) {
    botaoJogar.desenhar(renderer, offsetX, offsetY);
    botaoFechar.desenhar(renderer, offsetX, offsetY);
    setaEsquerda.desenhar(renderer, offsetX, offsetY);
    setaDireita.desenhar(renderer, offsetX, offsetY);
}