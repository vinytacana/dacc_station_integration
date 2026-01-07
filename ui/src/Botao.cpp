#include "Botao.hpp"
#include "Janela.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorImagens.hpp"
#include "Utils.hpp" 
#include "Theme.hpp"
#include "GraphicsUtils.hpp"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h> // Para filledCircleRGBA
#include <iostream>
#include <ostream>  
#include "GerenciadorJogos.hpp"

using namespace MeuProjeto;

extern GerenciadorImagens gerImg;
extern GerenciadorJogos gerenciadorJogos;
static GerenciadorFontes gerenciadorFontesBotoes; 

// --- Construtores e Destrutor ---

/**
 * @brief Construtor padrão para um botão baseado em texto ou cor.
 *
 * @param x Coordenada X superior esquerda.
 * @param y Coordenada Y superior esquerda.
 * @param largura Largura do botão.
 * @param altura Altura do botão.
 * @param texto O texto a ser exibido no botão (opcional).
 */
Botao::Botao(int x, int y, int largura, int altura, const std::string& texto) 
    : area{x, y, largura, altura}, texturaImagem(nullptr), texto(texto), 
      corPadrao({59, 59, 61, 255}), corHover({96, 97, 99, 255}), corPressionado({131, 131, 133, 255}),
      hover(false), pressionado(false), parentJanela(nullptr), nomeJogo(""), descJogo(""), 
      isRound(false), isDestaque(false),
      tipoFonte(TipoFonte::NEGRITO), tamanhoFonte(32) 
{
 
}

/**
 * @brief Construtor para um botão baseado em imagem.
 *
 * @param renderer O renderizador SDL.
 * @param x Coordenada X superior esquerda.
 * @param y Coordenada Y superior esquerda.
 * @param largura Largura do botão.
 * @param altura Altura do botão.
 * @param imagemPath O caminho para o arquivo de imagem.
 * @param nome O nome associado ao jogo.
 * @param desc A descrição associada ao jogo.
 * @param isDestaque Indica se o botão é um destaque.
 */
Botao::Botao(SDL_Renderer* renderer, int x, int y, int largura, int altura,
             const std::string& imagemPath, const std::string& nome,
             const std::string& desc, bool isDestaque)
    : area{x, y, largura, altura}, texturaImagem(nullptr), texto(""),
      corPadrao({0,0,0,0}), corHover({96, 97, 99, 255}), corPressionado({131, 131, 133, 255}),
      hover(false), pressionado(false), parentJanela(nullptr),
      nomeJogo(nome), descJogo(desc), imagemPath(imagemPath), 
      isRound(false), isDestaque(isDestaque),
      tipoFonte(TipoFonte::NEGRITO), tamanhoFonte(32) 
{
    if (renderer && !imagemPath.empty()) {
        texturaImagem = gerImg.carregar(renderer, imagemPath);
        if (texturaImagem) {
            SDL_SetTextureBlendMode(texturaImagem, SDL_BLENDMODE_BLEND);
        }
    }
}

/**
 * @brief Destrutor virtual.
 */
Botao::~Botao() {
}

// --- Tratamento de Eventos ---

/**
 * @brief Trata eventos de mouse (movimento, clique) para atualizar o estado do botão.
 *
 * @param evento A estrutura SDL_Event a ser tratada.
 * @param offsetX Deslocamento X da área de desenho.
 * @param offsetY Deslocamento Y da área de desenho.
 * @return Retorna @c true se o evento foi consumido pelo botão.
 */
bool Botao::tratarEvento(SDL_Event& evento, int offsetX, int offsetY) {
    bool eventoConsumido = false;

    if (evento.type == SDL_MOUSEMOTION) {
        eventoConsumido = handleMouseMotion(evento, offsetX, offsetY);
    } else if (evento.type == SDL_MOUSEBUTTONDOWN && evento.button.button == SDL_BUTTON_LEFT) {
        eventoConsumido = handleMouseButtonDown(evento, offsetX, offsetY);
    } else if (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT) {
        eventoConsumido = handleMouseButtonUp(evento, offsetX, offsetY);
    }

    return eventoConsumido;
}

/**
 * @brief Trata o evento de movimento do mouse (atualiza hover e popups).
 *
 * @param evento O evento de movimento do mouse.
 * @param offsetX Deslocamento X.
 * @param offsetY Deslocamento Y.
 * @return Retorna @c true se um popup foi aberto.
 */
bool Botao::handleMouseMotion(SDL_Event& evento, int offsetX, int offsetY) {
    int x = evento.motion.x - offsetX; 
    int y = evento.motion.y - offsetY;

    bool mouseDentro = contemPonto(x, y);
    bool abriuPopup = false;

    if (mouseDentro && !hover) { 
        if (parentJanela && !nomeJogo.empty() && texto.empty()) {
            parentJanela->abrirPopup(nomeJogo, texturaImagem, descJogo, area.x, area.y, area.w, area.h);
            abriuPopup = true;
        }
    } else if (!mouseDentro && hover) { 
        if (parentJanela) {
            parentJanela->fecharPopup();
        }
    }
    
    hover = mouseDentro; 
    return abriuPopup;
}

/**
 * @brief Trata o evento de botão do mouse para baixo (atualiza pressionado).
 *
 * @param evento O evento de clique do mouse.
 * @param offsetX Deslocamento X.
 * @param offsetY Deslocamento Y.
 * @return Retorna @c true se o botão foi pressionado.
 */
bool Botao::handleMouseButtonDown(SDL_Event& evento, int offsetX, int offsetY) {
    int x = evento.button.x - offsetX;
    int y = evento.button.y - offsetY;

    if (contemPonto(x, y)) {
        pressionado = true;
        return true;
    }
    return false;
}

/**
 * @brief Trata o evento de botão do mouse para cima (finaliza o clique).
 *
 * @param evento O evento de soltura do mouse.
 * @param offsetX Deslocamento X.
 * @param offsetY Deslocamento Y.
 * @return Retorna @c true se o botão foi solto após estar pressionado.
 */
bool Botao::handleMouseButtonUp(SDL_Event& evento, int offsetX, int offsetY) {
    int x = evento.button.x - offsetX;
    int y = evento.button.y - offsetY;

    if (contemPonto(x, y) && pressionado) {
        pressionado = false;
        return true;
    }
    pressionado = false;
    return false;
}

// --- Funções de Desenho ---

/**
 * @brief Desenha o botão na posição absoluta (sem offset).
 *
 * @param renderer O renderizador SDL.
 */
void Botao::desenhar(SDL_Renderer* renderer) {
    desenhar(renderer, 0, 0);
}

/**
 * @brief Desenha o botão com um deslocamento (offset) aplicado.
 *
 * @param renderer O renderizador SDL.
 * @param offsetX Deslocamento X.
 * @param offsetY Deslocamento Y.
 */
void Botao::desenhar(SDL_Renderer* renderer, int offsetX, int offsetY) {
    if (!renderer) return;

    SDL_Rect adjustedArea = {area.x + offsetX, area.y + offsetY, area.w, area.h};
    SDL_Color corAtual = determineCurrentColor();

    // Desenha o overlay de cor PRIMEIRO (antes da imagem)
    if (corAtual.a > 0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        if (isRound) {
            // Desenha um círculo preenchido
            int centroX = adjustedArea.x + adjustedArea.w / 2;
            int centroY = adjustedArea.y + adjustedArea.h / 2;
            int raio = (adjustedArea.w < adjustedArea.h) ? (adjustedArea.w / 2) : (adjustedArea.h / 2);
            filledCircleRGBA(renderer, centroX, centroY, raio, corAtual.r, corAtual.g, corAtual.b, corAtual.a);
            
            // Highlight circular se hover
            if (hover) {
                aacircleRGBA(renderer, centroX, centroY, raio, 255, 255, 255, 255);
            }
        } else {
            // Desenha um retângulo ARREDONDADO
            GraphicsUtils::drawRoundedRect(renderer, adjustedArea, cornerRadius, corAtual);
            
            // Efeito de Borda (Highlight) quando selecionado
            if (hover) {
                GraphicsUtils::drawRoundedRectOutline(renderer, adjustedArea, cornerRadius, {255, 255, 255, 100});
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    // Desenha a textura da imagem POR CIMA do overlay
    if (texturaImagem) {
        SDL_SetTextureBlendMode(texturaImagem, SDL_BLENDMODE_BLEND);
        
        // Aplica transparência baseada no estado (hover/pressionado)
        Uint8 alphaValue;
        if (isDestaque) {
            alphaValue = 128; // Destaques sempre 50% transparentes
        } else if (hover || pressionado) {
            alphaValue = 180; // ~70% opaco quando hover/pressionado para ver o overlay
        } else {
            alphaValue = 255; // Totalmente opaco no estado normal
        }
        
        SDL_SetTextureAlphaMod(texturaImagem, alphaValue);
        SDL_RenderCopy(renderer, texturaImagem, nullptr, &adjustedArea);
        SDL_SetTextureAlphaMod(texturaImagem, 255); // Restaura
    }
    else if (!texto.empty()) {
        // Se não tem imagem, mas tem texto, renderiza o texto
        renderizarTextoCentralizado(renderer, adjustedArea);
    }
}

/**
 * @brief Determina a cor de fundo atual com base nos estados.
 *
 * @return A estrutura @c SDL_Color.
 */
SDL_Color Botao::determineCurrentColor() {
    if (pressionado) {
        return corPressionado;
    } else if (hover) {
        return corHover;
    }
    return corPadrao;
}

/**
 * @brief Renderiza o texto centralizado na área do botão.
 *
 * @param renderer O renderizador SDL.
 * @param area A área retangular ajustada do botão.
 */
void Botao::renderizarTextoCentralizado(SDL_Renderer* renderer, const SDL_Rect& area) {
    static GerenciadorFontes gerenciadorFontesBotoes;

    // *** USA A FONTE E TAMANHO CONFIGURADOS NO BOTÃO ***
    const std::string& fontePath = ConfigFontes::obterCaminho(tipoFonte);
    TTF_Font* font = gerenciadorFontesBotoes.carregar(fontePath, tamanhoFonte);
    
    if (!font) {
        std::cerr << "[ERRO] Falha ao carregar fonte de botão: " << fontePath << std::endl;
        return;
    }

    // Cor do texto branca
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, texto.c_str(), {255, 255, 255, 255});
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    int texW = surface->w, texH = surface->h;
    
    SDL_Rect rectTexto = {
        area.x + (area.w - texW) / 2, // Centraliza horizontalmente
        area.y + (area.h - texH) / 2, // Centraliza verticalmente
        texW,
        texH
    };

    SDL_RenderCopy(renderer, texture, nullptr, &rectTexto);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// --- Funções Set/Get ---

/**
 * @brief Define a textura da imagem do botão.
 *
 * @param textura O ponteiro para a textura SDL.
 */
void Botao::setTexturaImagem(SDL_Texture* textura) {
    texturaImagem = textura;
    if (texturaImagem) {
        SDL_SetTextureBlendMode(texturaImagem, SDL_BLENDMODE_BLEND);
    }
}

/**
 * @brief Define as cores dos estados do botão.
 *
 * @param corPadrao Cor normal.
 * @param corHover Cor quando o mouse está sobre o botão.
 * @param corPressionado Cor quando o botão está pressionado.
 */
void Botao::setCor(const SDL_Color& corPadrao, const SDL_Color& corHover, const SDL_Color& corPressionado) {
    this->corPadrao = corPadrao;
    this->corHover = corHover;
    this->corPressionado = corPressionado;
}

/**
 * @brief Define a janela pai para manipulação de popups.
 *
 * @param janela O ponteiro para a janela pai.
 */
void MeuProjeto::Botao::setParentJanela(Janela* janela) {  
    parentJanela = janela;
}

/**
 * @brief Define se o botão deve ser desenhado como um círculo (arredondado).
 *
 * @param round Se @c true, o botão é desenhado como um círculo.
 */
void Botao::setIsRound(bool round) {
    isRound = round;
}

void Botao::setCornerRadius(int radius) {
    cornerRadius = radius;
}

/**
 * @brief Verifica se um ponto (x, y) está contido na área do botão.
 *
 * @param x Coordenada X do ponto.
 * @param y Coordenada Y do ponto.
 * @return Retorna @c true se o ponto estiver dentro da área, @c false caso contrário.
 */
bool Botao::contemPonto(int x, int y) {
    bool contem = x >= area.x && x <= area.x + area.w &&
                  y >= area.y && y <= area.y + area.h;
    
    return contem;
}

void Botao::setSelecionado(bool selecionado) {
    this->hover = selecionado;
}

bool Botao::estaSelecionado() const {
    return this->hover;
}