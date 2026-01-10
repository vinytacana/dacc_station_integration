/**
 * @file Botao.cpp
 * @brief Implementação detalhada da classe Botao.
 * 
 * Este arquivo gerencia a lógica de estados (hover, clique, foco), a integração
 * com o motor de temas e a renderização híbrida de texto e imagem.
 */

#include "Interface/Componentes/Botao.hpp"
#include "Interface/Janelas/Janela.hpp"
#include "Gerenciadores/GerenciadorFontes.hpp"
#include "Gerenciadores/GerenciadorImagens.hpp"
#include "Principais/Utils.hpp" 
#include "Gerenciadores/GerenciadorTemas.hpp"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <iostream>

using namespace MeuProjeto;

/** 
 * @brief Referência ao gerenciador global de imagens definido no main.cpp. 
 * Utilizado para evitar o recarregamento de texturas idênticas.
 */
extern GerenciadorImagens gerImg;

/**
 * @brief Construtor para Botão de Texto.
 * 
 * Inicializa um botão focado em exibir rótulos textuais. 
 * @note Este construtor define @a usaTemaPadrao como @b true, o que significa que
 * as cores do botão mudarão automaticamente quando o usuário alternar entre o tema Claro e Escuro.
 * 
 * @param x Posição X absoluta.
 * @param y Posição Y absoluta.
 * @param largura Dimensão horizontal.
 * @param altura Dimensão vertical.
 * @param texto Conteúdo textual do botão.
 */
Botao::Botao(int x, int y, int largura, int altura, const std::string& texto) 
    : area{x, y, largura, altura}, texturaImagem(nullptr), texto(texto), 
      corPadrao({0,0,0,0}), corHover({0,0,0,0}), corPressionado({0,0,0,0}),
      usaTemaPadrao(true), 
      hover(false), pressionado(false), focadoPorControle(false), parentJanela(nullptr), nomeJogo(""), descJogo(""), 
      isRound(false), isDestaque(false),
      tipoFonte(TipoFonte::NEGRITO), tamanhoFonte(32),
      onClickCallback(nullptr)
{
}

/**
 * @brief Construtor para Botão de Imagem (Capas/Banners).
 * 
 * Inicializa um botão que exibe uma textura. Por padrão, o fundo é transparente
 * para não interferir na arte do jogo.
 * @note Este construtor define @a usaTemaPadrao como @b false, pois botões de imagem
 * geralmente possuem identidade visual fixa ou dependem da arte do jogo.
 * 
 * @param renderer O renderizador SDL.
 * @param x Posição X.
 * @param y Posição Y.
 * @param largura Largura.
 * @param altura Altura.
 * @param imagemPath Caminho do arquivo de imagem.
 * @param nome Nome do jogo (usado no popup).
 * @param desc Descrição do jogo (usada no popup).
 * @param isDestaque Se true, aplica transparência especial de banner.
 */
Botao::Botao(SDL_Renderer* renderer, int x, int y, int largura, int altura,
             const std::string& imagemPath, const std::string& nome,
             const std::string& desc, bool isDestaque)
    : area{x, y, largura, altura}, texturaImagem(nullptr), texto(""),
      corPadrao({0,0,0,0}), corHover({96, 97, 99, 255}), corPressionado({131, 131, 133, 255}),
      usaTemaPadrao(false), 
      hover(false), pressionado(false), focadoPorControle(false), parentJanela(nullptr),
      nomeJogo(nome), descJogo(desc), imagemPath(imagemPath), 
      isRound(false), isDestaque(isDestaque),
      tipoFonte(TipoFonte::NEGRITO), tamanhoFonte(32),
      onClickCallback(nullptr)
{
    if (renderer && !imagemPath.empty()) {
        // Solicita a textura ao cache global
        texturaImagem = gerImg.carregar(renderer, imagemPath);
        if (texturaImagem) {
            // Habilita canal alpha para transparências
            SDL_SetTextureBlendMode(texturaImagem, SDL_BLENDMODE_BLEND);
        }
    }
}

Botao::~Botao() {
    // A textura não é destruída aqui pois pertence ao cache global (gerImg)
}

/**
 * @brief Define uma cor de texto que ignora o tema.
 */
void Botao::setCorTexto(const SDL_Color& cor) {
    corTextoCustom = cor;
    usaCorTextoCustom = true;
}

/**
 * @brief Define manualmente as cores dos estados do botão.
 * 
 * @important Ao chamar esta função, a flag @a usaTemaPadrao é desativada. O botão 
 * manterá estas cores permanentemente, ignorando mudanças globais de tema.
 */
void Botao::setCor(const SDL_Color& padrao, const SDL_Color& hover, const SDL_Color& press) { 
    corPadrao = padrao; 
    corHover = hover; 
    corPressionado = press; 
    usaTemaPadrao = false; 
}

void Botao::desenhar(SDL_Renderer* renderer) {
    desenhar(renderer, 0, 0);
}

/**
 * @brief Método principal de renderização do Botão.
 * 
 * A lógica de desenho segue esta ordem de camadas:
 * 1. **Sincronização de Tema**: Se o tema estiver ativo, busca as cores do Singleton.
 * 2. **Fundo Geométrica**: Desenha um retângulo ou um círculo preenchido.
 * 3. **Imagem/Arte**: Se houver textura, aplica modulação de Alpha (transparência) 
 *    baseada no estado (hover, focado ou destaque).
 * 4. **Texto**: Renderiza o rótulo centralizado.
 * 5. **Feedback de Controle**: Se focado por gamepad, desenha uma borda externa vibrante.
 * 
 * @param renderer O renderizador SDL.
 * @param offsetX Deslocamento horizontal (scroll).
 * @param offsetY Deslocamento vertical (scroll).
 */
void Botao::desenhar(SDL_Renderer* renderer, int offsetX, int offsetY) {
    if (!renderer) return;

    // Lógica de Temas: Atualiza cores em tempo real se não houver override manual
    if (usaTemaPadrao) {
        auto& tema = GerenciadorTemas::getInstance();
        corPadrao = tema.getCorBotaoNormal();
        corHover = tema.getCorBotaoHover();
        corPressionado = tema.getCorBotaoPressionado();
    }

    // Aplica o deslocamento de scroll à área física
    SDL_Rect adjustedArea = {area.x + offsetX, area.y + offsetY, area.w, area.h};
    SDL_Color corAtual = determineCurrentColor();

    // 1. Renderização do Fundo (Background)
    if (corAtual.a > 0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, corAtual.r, corAtual.g, corAtual.b, corAtual.a);
        
        if (isRound) {
            // Cálculos para botões circulares (ex: ícones redondos)
            int centroX = adjustedArea.x + adjustedArea.w / 2;
            int centroY = adjustedArea.y + adjustedArea.h / 2;
            int raio = (adjustedArea.w < adjustedArea.h) ? (adjustedArea.w / 2) : (adjustedArea.h / 2);
            filledCircleRGBA(renderer, centroX, centroY, raio, corAtual.r, corAtual.g, corAtual.b, corAtual.a);
        } else {
            SDL_RenderFillRect(renderer, &adjustedArea);
        }
    }

    // 2. Renderização da Imagem (Arte do Jogo)
    if (texturaImagem) {
        /**
         * @section Modulação_Alpha
         * O Alpha é ajustado conforme o estado para dar feedback visual:
         * - Destaque (Banner): 128 (50% de transparência).
         * - Hover/Focado/Pressionado: 180 (Escurece levemente).
         * - Normal: 255 (Opaco).
         */
        Uint8 alphaValue = (isDestaque) ? 128 : ((hover || pressionado || focadoPorControle) ? 180 : 255);
        SDL_SetTextureAlphaMod(texturaImagem, alphaValue);
        SDL_RenderCopy(renderer, texturaImagem, nullptr, &adjustedArea);
        SDL_SetTextureAlphaMod(texturaImagem, 255); // Reseta para não afetar outros desenhos
    }
    // 3. Renderização do Texto (se não houver imagem)
    else if (!texto.empty()) {
        renderizarTextoCentralizado(renderer, adjustedArea);
    }
    
    // 4. Borda de Destaque do Controle (Gamepad Focus)
    if (focadoPorControle) {
        SDL_Color corDestaque = GerenciadorTemas::getInstance().getCorDestaque();
        if (isRound) {
            int centroX = adjustedArea.x + adjustedArea.w / 2;
            int centroY = adjustedArea.y + adjustedArea.h / 2;
            int raio = (adjustedArea.w < adjustedArea.h) ? (adjustedArea.w / 2) : (adjustedArea.h / 2);
            // Desenha 3 círculos com raios incrementais para simular uma borda espessa e suave (anti-aliasing)
            aacircleRGBA(renderer, centroX, centroY, raio, corDestaque.r, corDestaque.g, corDestaque.b, 255);
            aacircleRGBA(renderer, centroX, centroY, raio + 1, corDestaque.r, corDestaque.g, corDestaque.b, 255);
            aacircleRGBA(renderer, centroX, centroY, raio + 2, corDestaque.r, corDestaque.g, corDestaque.b, 255);
        } else {
            // Desenha dois retângulos arredondados para criar uma borda dupla de destaque
            roundedRectangleRGBA(renderer, adjustedArea.x - 3, adjustedArea.y - 3, adjustedArea.x + adjustedArea.w + 3, adjustedArea.y + adjustedArea.h + 3, 5, corDestaque.r, corDestaque.g, corDestaque.b, 255);
            roundedRectangleRGBA(renderer, adjustedArea.x - 2, adjustedArea.y - 2, adjustedArea.x + adjustedArea.w + 2, adjustedArea.y + adjustedArea.h + 2, 4, corDestaque.r, corDestaque.g, corDestaque.b, 255);
        }
    }
}

/**
 * @brief Máquina de estados para determinar a cor de renderização no frame atual.
 * @return SDL_Color priorizando o estado Pressionado > Hover/Foco > Normal.
 */
SDL_Color Botao::determineCurrentColor() {
    if (pressionado) return corPressionado;
    else if (hover || focadoPorControle) return corHover;
    return corPadrao;
}

/**
 * @brief Renderiza o texto exatamente no centro da área do botão.
 * 
 * Realiza cálculos de ancoragem: 
 * X = PosicaoBotaoX + (LarguraBotao - LarguraTexto) / 2
 */
void Botao::renderizarTextoCentralizado(SDL_Renderer* renderer, const SDL_Rect& area) {
    static GerenciadorFontes gerenciadorFontesBotoes;

    const std::string& fontePath = ConfigFontes::obterCaminho(tipoFonte);
    TTF_Font* font = gerenciadorFontesBotoes.carregar(fontePath, tamanhoFonte);
    if (!font) return;

    // Seleção de cor: Manual ou automática via tema
    SDL_Color corTexto = (usaCorTextoCustom) ? corTextoCustom : GerenciadorTemas::getInstance().getCorTextoNegrito();

    // Renderização Blended para alta qualidade (UTF8 suportado)
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, texto.c_str(), corTexto);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) { SDL_FreeSurface(surface); return; }

    int texW = surface->w, texH = surface->h;
    // Cálculo do retângulo de destino para centralização perfeita
    SDL_Rect rectTexto = { 
        area.x + (area.w - texW) / 2, 
        area.y + (area.h - texH) / 2, 
        texW, 
        texH 
    };

    SDL_RenderCopy(renderer, texture, nullptr, &rectTexto);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

/**
 * @brief Distribuidor de eventos (Event Dispatcher) para o botão.
 * 
 * @param evento Evento SDL capturado.
 * @param offsetX Deslocamento horizontal do scroll.
 * @param offsetY Deslocamento vertical do scroll.
 * @return true se o botão interagiu com o evento e o "consumiu".
 */
bool Botao::tratarEvento(SDL_Event& evento, int offsetX, int offsetY) {
    bool eventoConsumido = false;
    if (evento.type == SDL_MOUSEMOTION) 
        eventoConsumido = handleMouseMotion(evento, offsetX, offsetY);
    else if (evento.type == SDL_MOUSEBUTTONDOWN && evento.button.button == SDL_BUTTON_LEFT) 
        eventoConsumido = handleMouseButtonDown(evento, offsetX, offsetY);
    else if (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT) 
        eventoConsumido = handleMouseButtonUp(evento, offsetX, offsetY);
    else if (evento.type == SDL_CONTROLLERBUTTONDOWN || evento.type == SDL_CONTROLLERBUTTONUP) 
        eventoConsumido = handleGamepadButton(evento);
    return eventoConsumido;
}

/**
 * @brief Gerencia o movimento do mouse e a lógica de POPUP.
 * 
 * Se o mouse entrar em um botão de jogo (sem texto), solicita à janela pai
 * que abra um popup com os detalhes do jogo.
 */
bool Botao::handleMouseMotion(SDL_Event& evento, int offsetX, int offsetY) {
    // Normaliza a posição do mouse subtraindo o scroll
    int x = evento.motion.x - offsetX; 
    int y = evento.motion.y - offsetY;
    
    bool mouseDentro = contemPonto(x, y);
    bool abriuPopup = false;

    // Lógica de Entrada (Hover In)
    if (mouseDentro && !hover) { 
        if (parentJanela && !nomeJogo.empty() && texto.empty()) {
            // Dispara o popup informativo
            parentJanela->abrirPopup(nomeJogo, texturaImagem, descJogo, area.x, area.y, area.w, area.h);
            abriuPopup = true;
        }
    } 
    // Lógica de Saída (Hover Out)
    else if (!mouseDentro && hover) { 
        if (parentJanela) parentJanela->fecharPopup();
    }
    
    hover = mouseDentro; 
    return abriuPopup;
}

bool Botao::handleMouseButtonDown(SDL_Event& evento, int offsetX, int offsetY) {
    int x = evento.button.x - offsetX; 
    int y = evento.button.y - offsetY;
    if (contemPonto(x, y)) { pressionado = true; return true; }
    return false;
}

bool Botao::handleMouseButtonUp(SDL_Event& evento, int offsetX, int offsetY) {
    int x = evento.button.x - offsetX; 
    int y = evento.button.y - offsetY;
    if (contemPonto(x, y) && pressionado) {
        pressionado = false;
        // Dispara a ação de callback associada
        if (onClickCallback) onClickCallback();
        return true;
    }
    pressionado = false;
    return false;
}

/**
 * @brief Gerencia a interação via controle (Gamepad).
 * 
 * Mapeia o botão 'A' do controle para a mesma ação do clique esquerdo do mouse.
 */
bool Botao::handleGamepadButton(SDL_Event& evento) {
    if (!focadoPorControle) return false;
    
    if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
        if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_A) { pressionado = true; return true; }
    } else if (evento.type == SDL_CONTROLLERBUTTONUP) {
        if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
            if (pressionado) {
                pressionado = false;
                if (onClickCallback) onClickCallback();
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Define o estado de foco do controle e gerencia o popup.
 * 
 * Funciona de forma análoga ao mouse motion, mas disparado pela navegação
 * direcional do controle.
 */
void Botao::setFocado(bool focado) {
    bool estadoAnterior = focadoPorControle;
    focadoPorControle = focado;
    
    if (focado && !estadoAnterior) {
        if (parentJanela && !nomeJogo.empty() && texto.empty()) {
            parentJanela->abrirPopup(nomeJogo, texturaImagem, descJogo, area.x, area.y, area.w, area.h);
        }
    } else if (!focado && estadoAnterior) {
        if (parentJanela) parentJanela->fecharPopup();
        pressionado = false;
    }
}

void Botao::ativarPorControle() { if (focadoPorControle) pressionado = true; }
void Botao::desativarPorControle() { pressionado = false; }

/** @brief Executa a ação do botão imediatamente. Útil para atalhos de hardware. */
bool Botao::executarAcaoPorControle() {
    if (focadoPorControle && onClickCallback) { onClickCallback(); return true; }
    return false;
}

void Botao::setTexturaImagem(SDL_Texture* textura) { 
    texturaImagem = textura; 
    if(texturaImagem) SDL_SetTextureBlendMode(texturaImagem, SDL_BLENDMODE_BLEND); 
}

void Botao::setParentJanela(Janela* janela) { parentJanela = janela; }
void Botao::setIsRound(bool round) { isRound = round; }

/**
 * @brief Detecção de colisão (Hitbox).
 * 
 * Se o botão for redondo, utiliza a fórmula da distância Euclidiana:
 * (dx² + dy²) <= Raio²
 * Se for retangular, utiliza a comparação simples de limites.
 */
bool Botao::contemPonto(int x, int y) {
    if (isRound) {
        int cx = area.x + area.w/2, cy = area.y + area.h/2, r = area.w/2;
        int dx = x - cx, dy = y - cy;
        return (dx*dx + dy*dy) <= (r*r);
    }
    return x >= area.x && x <= area.x + area.w && y >= area.y && y <= area.y + area.h;
}