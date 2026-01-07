#include "Botao.hpp"
#include "Janela.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorImagens.hpp"
#include "GerenciadorAudio.hpp"
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
extern GerenciadorFontes gerFontes;

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
    : area{x, y, largura, altura}, 
      corPadrao({59, 59, 61, 255}), corHover({96, 97, 99, 255}), corPressionado({131, 131, 133, 255}),
      texto(texto),
      texturaImagem(nullptr), hover(false), pressionado(false),
      tipoFonte(TipoFonte::NEGRITO), tamanhoFonte(32),
      imagemPath(""), parentJanela(nullptr), nomeJogo(""), descJogo(""), 
      isRound(false), isDestaque(false), cornerRadius(10), iconText("")
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
    : area{x, y, largura, altura}, 
      corPadrao({0,0,0,0}), corHover({96, 97, 99, 255}), corPressionado({131, 131, 133, 255}),
      texto(""),
      texturaImagem(nullptr), hover(false), pressionado(false),
      tipoFonte(TipoFonte::NEGRITO), tamanhoFonte(32),
      imagemPath(imagemPath), parentJanela(nullptr),
      nomeJogo(nome), descJogo(desc), 
      isRound(false), isDestaque(isDestaque), cornerRadius(10), iconText("")
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
        gerAudio.tocarSom("nav.wav");
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
    // *** USA A FONTE E TAMANHO CONFIGURADOS NO BOTÃO ***
    const std::string& fontePath = ConfigFontes::obterCaminho(tipoFonte);
    TTF_Font* font = gerFontes.carregar(fontePath, tamanhoFonte);
    
    if (!font) {
        std::cerr << "[ERRO] Falha ao carregar fonte de botão: " << fontePath << std::endl;
        return;
    }

    // Preparar texto do ícone (se houver)
    int iconWidth = 0;
    int iconGap = 10;
    
    if (!iconText.empty()) {
        iconWidth = 32; // Tamanho fixo do círculo do ícone
    }

    // Renderizar Texto Principal
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, texto.c_str(), {255, 255, 255, 255});
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    int texW = surface->w, texH = surface->h;
    
    // Calcular largura total (Ícone + Gap + Texto)
    int totalContentWidth = texW;
    if (!iconText.empty()) totalContentWidth += iconWidth + iconGap;

    // Posição de início (centralizado)
    int startX = area.x + (area.w - totalContentWidth) / 2;
    int centerY = area.y + area.h / 2;

    // --- DESENHAR ÍCONE ---
    if (!iconText.empty()) {
        int iconX = startX;
        int iconY = centerY - (iconWidth / 2);
        int radius = iconWidth / 2;

        // Círculo Branco
        filledCircleRGBA(renderer, iconX + radius, iconY + radius, radius, 255, 255, 255, 255);
        
        // Letra do Ícone (Preta)
        SDL_Color black = {20, 20, 20, 255};
        // Usa uma fonte um pouco menor para a letra dentro do círculo
        TTF_Font* fontIcon = gerFontes.carregar(fontePath, 20); 
        if (fontIcon) {
            SDL_Surface* sIcon = TTF_RenderUTF8_Blended(fontIcon, iconText.c_str(), black);
            if (sIcon) {
                SDL_Texture* tIcon = SDL_CreateTextureFromSurface(renderer, sIcon);
                SDL_Rect rIcon = {
                    iconX + radius - sIcon->w/2, 
                    iconY + radius - sIcon->h/2, 
                    sIcon->w, sIcon->h
                };
                SDL_RenderCopy(renderer, tIcon, nullptr, &rIcon);
                SDL_FreeSurface(sIcon);
                SDL_DestroyTexture(tIcon);
            }
        }
        
        // Avançar X para o texto
        startX += iconWidth + iconGap;
    }

    // --- DESENHAR TEXTO PRINCIPAL ---
    SDL_Rect rectTexto = {
        startX, 
        centerY - texH / 2, 
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

void Botao::setIcon(const std::string& iconChar) {
    iconText = iconChar;
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