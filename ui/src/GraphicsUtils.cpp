/**
 * @file GraphicsUtils.cpp
 * @brief Implementação de utilitários gráficos para elementos visuais da interface.
 * 
 * Este arquivo contém funções auxiliares para renderização de elementos gráficos
 * complexos, incluindo retângulos arredondados, overlays, barra de status do sistema
 * e barra de navegação inferior. Utiliza a biblioteca SDL2_gfx para primitivas
 * gráficas avançadas com anti-aliasing.
 */

#include "GraphicsUtils.hpp"
#include "SystemStatus.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace MeuProjeto {

/**
 * @brief Desenha um retângulo com cantos arredondados preenchido.
 * 
 * Utiliza a função roundedBoxRGBA da SDL2_gfx para criar retângulos com
 * bordas suaves e anti-aliasing, proporcionando visual moderno à interface.
 * 
 * @param renderer Renderizador SDL onde o retângulo será desenhado.
 * @param rect Estrutura SDL_Rect definindo posição (x, y) e dimensões (w, h).
 * @param radius Raio de curvatura dos cantos em pixels (valores maiores = cantos mais arredondados).
 * @param color Cor RGBA do preenchimento do retângulo.
 * 
 * @note A função roundedBoxRGBA utiliza coordenadas dos cantos opostos (x1,y1,x2,y2)
 *       em vez de dimensões, por isso é feita a conversão x+w e y+h.
 */
void GraphicsUtils::drawRoundedRect(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color) {
    roundedBoxRGBA(renderer, 
                   rect.x, rect.y, 
                   rect.x + rect.w, rect.y + rect.h, 
                   radius, 
                   color.r, color.g, color.b, color.a);
}

/**
 * @brief Desenha apenas o contorno de um retângulo com cantos arredondados.
 * 
 * Similar a drawRoundedRect(), mas renderiza apenas a borda sem preenchimento.
 * Útil para criar elementos de interface com efeito de moldura ou destaque.
 * 
 * @param renderer Renderizador SDL onde o contorno será desenhado.
 * @param rect Estrutura SDL_Rect definindo posição e dimensões.
 * @param radius Raio de curvatura dos cantos em pixels.
 * @param color Cor RGBA da linha de contorno.
 */
void GraphicsUtils::drawRoundedRectOutline(SDL_Renderer* renderer, const SDL_Rect& rect, int radius, SDL_Color color) {
    roundedRectangleRGBA(renderer, 
                         rect.x, rect.y, 
                         rect.x + rect.w, rect.y + rect.h, 
                         radius, 
                         color.r, color.g, color.b, color.a);
}

/**
 * @brief Renderiza uma textura com cobertura completa da área alvo (crop centralizado).
 * 
 * Implementa o comportamento "cover" do CSS: a textura preenche completamente
 * o retângulo de destino mantendo sua proporção original. Partes da textura
 * que excedem a área alvo são cortadas (crop), centralizando a imagem.
 * 
 * **Comportamento por aspect ratio:**
 * - Textura mais larga que alvo: corta laterais (crop horizontal)
 * - Textura mais alta que alvo: corta topo/fundo (crop vertical)
 * - Proporções iguais: renderiza completa sem crop
 * 
 * @param renderer Renderizador SDL onde a textura será desenhada.
 * @param texture Ponteiro para a textura SDL a ser renderizada.
 * @param targetRect Área de destino onde a textura será desenhada.
 * @param alpha Nível de opacidade (0=transparente, 255=opaco).
 * 
 * @note A função restaura o alpha da textura para 255 após renderização
 *       para não afetar outros desenhos subsequentes.
 */
void GraphicsUtils::drawTextureCover(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect& targetRect, int alpha) {
    if (!texture) return;

    /**
     * Obtém dimensões originais da textura usando SDL_QueryTexture.
     * Retorna 0 em sucesso, valor negativo em caso de erro.
     */
    int texW, texH;
    if (SDL_QueryTexture(texture, NULL, NULL, &texW, &texH) != 0) return;

    /// Calcula as proporções (aspect ratio) da textura e da área de destino
    float texAspect = static_cast<float>(texW) / static_cast<float>(texH);
    float targetAspect = static_cast<float>(targetRect.w) / static_cast<float>(targetRect.h);

    SDL_Rect srcRect;

    if (texAspect > targetAspect) {
        /**
         * Textura é mais larga que a área de destino (ex: 16:9 em 4:3).
         * Estratégia: Crop horizontal (corta laterais), mantém altura completa.
         * 
         * - Usa altura completa da textura (srcRect.h = texH)
         * - Calcula largura proporcional ao target
         * - Centraliza horizontalmente cortando igualmente dos dois lados
         */
        srcRect.h = texH;
        srcRect.w = static_cast<int>(texH * targetAspect);
        srcRect.x = (texW - srcRect.w) / 2;
        srcRect.y = 0;
    } else {
        /**
         * Textura é mais alta que a área de destino (ex: 4:3 em 16:9).
         * Estratégia: Crop vertical (corta topo/fundo), mantém largura completa.
         * 
         * - Usa largura completa da textura (srcRect.w = texW)
         * - Calcula altura proporcional ao target
         * - Centraliza verticalmente cortando igualmente de cima e baixo
         */
        srcRect.w = texW;
        srcRect.h = static_cast<int>(texW / targetAspect);
        srcRect.x = 0;
        srcRect.y = (texH - srcRect.h) / 2;
    }

    /// Aplica o modificador de transparência à textura
    SDL_SetTextureAlphaMod(texture, alpha);
    
    /// Renderiza a região srcRect da textura no targetRect da tela
    SDL_RenderCopy(renderer, texture, &srcRect, &targetRect);
    
    /// Restaura opacidade total para não afetar próximas renderizações
    SDL_SetTextureAlphaMod(texture, 255);
}

/**
 * @brief Desenha overlay semi-transparente escuro na metade inferior da tela.
 * 
 * Cria um gradiente suave de escurecimento que preserva a visibilidade da
 * arte principal no topo enquanto garante legibilidade de texto e elementos
 * UI na parte inferior. Implementa transição suave entre as áreas.
 * 
 * **Estrutura do overlay:**
 * 1. Faixa de transição (-50px acima do meio): Alpha = alpha/2 (suave)
 * 2. Metade inferior completa: Alpha = alpha (opaco)
 * 
 * @param renderer Renderizador SDL onde o overlay será aplicado.
 * @param width Largura total da tela em pixels.
 * @param height Altura total da tela em pixels.
 * @param alpha Nível de opacidade do overlay (0-255), tipicamente 120-180.
 * 
 * @note Blend mode é definido como SDL_BLENDMODE_BLEND para suportar transparência.
 */
void GraphicsUtils::drawOverlay(SDL_Renderer* renderer, int width, int height, int alpha) {
    /// Ativa modo de mistura para permitir transparência no overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    /**
     * Calcula ponto inicial do overlay na metade vertical da tela.
     * Garante que a área superior (arte de fundo) permaneça visível.
     */
    int startY = height / 2;
    
    /// Desenha retângulo escuro semi-transparente na metade inferior
    boxRGBA(renderer, 0, startY, width, height, 0, 0, 0, alpha);
    
    /**
     * Faixa de transição suave de 50 pixels acima do início do overlay.
     * Usa metade da opacidade (alpha/2) para criar gradiente visual.
     * Simula efeito de gradiente linear sem necessidade de shaders.
     */
    boxRGBA(renderer, 0, startY - 50, width, startY, 0, 0, 0, alpha / 2);
}

/**
 * @brief Renderiza a barra de status superior do sistema (relógio, WiFi, bateria, avatar).
 * 
 * Desenha uma barra de informações no topo da tela contendo:
 * - **Avatar do usuário** (canto direito): Círculo com inicial "U"
 * - **Relógio**: Horário atual no formato HH:MM
 * - **Bateria**: Indicador visual de carga (0-100%)
 * - **WiFi**: Ícone de conectividade quando disponível
 * 
 * Os dados são obtidos do singleton SystemStatus que atualiza cache periodicamente.
 * 
 * @param renderer Renderizador SDL onde a barra será desenhada.
 * @param screenWidth Largura total da tela para posicionamento à direita.
 * @param font Fonte TTF para renderização de texto (relógio e inicial).
 * 
 * @note Se a fonte for nula, a função retorna sem desenhar nada.
 * @note Todos os elementos usam anti-aliasing para melhor qualidade visual.
 */
void GraphicsUtils::drawSystemTopBar(SDL_Renderer* renderer, int screenWidth, TTF_Font* font) {
    /**
     * Obtém dados em cache do SystemStatus (atualizado a cada 1 segundo).
     * Contém: currentTime (string HH:MM), wifiConnected (bool), batteryLevel (int 0-100 ou BATERIA_INDISPONIVEL)
     */
    SystemData data = SystemStatus::getInstance().getCachedData();
    
    if (!font) return;

    /// Cor branca suave para texto e ícones
    SDL_Color white = {220, 220, 220, 255};
    int topMargin = 25; ///< Margem superior da tela

    // AVATAR DO USUÁRIO (Canto Superior Direito)
    
    int avatarRadius = 24; ///< Raio do círculo do avatar
    int avatarX = screenWidth - 50; ///< Posição X (50px da borda direita)
    int avatarY = topMargin + 10; ///< Posição Y
    
    /**
     * Desenha borda dupla branca ao redor do avatar para destaque.
     * Duas chamadas de aacircleRGBA com raios incrementais criam efeito de espessura.
     */
    aacircleRGBA(renderer, avatarX, avatarY, avatarRadius + 2, 255, 255, 255, 255);
    aacircleRGBA(renderer, avatarX, avatarY, avatarRadius + 1, 255, 255, 255, 255);
    
    /// Preenchimento do círculo com cor azul-acinzentado escuro
    filledCircleRGBA(renderer, avatarX, avatarY, avatarRadius, 40, 45, 60, 255);
    
    /**
     * Renderiza a inicial "U" (User) no centro do avatar.
     * Utiliza SDL_TTF para criar textura de texto centralizada.
     */
    if(font) {
         SDL_Color uCol = {200, 200, 200, 255};
         SDL_Surface* sU = TTF_RenderUTF8_Blended(font, "U", uCol);
         if(sU) {
             SDL_Texture* tU = SDL_CreateTextureFromSurface(renderer, sU);
             /// Calcula posição centralizada subtraindo metade das dimensões
             SDL_Rect rU = {avatarX - sU->w/2, avatarY - sU->h/2, sU->w, sU->h};
             SDL_RenderCopy(renderer, tU, NULL, &rU);
             SDL_FreeSurface(sU);
             SDL_DestroyTexture(tU);
         }
    }

    // RELÓGIO (À Esquerda do Avatar)
    
    /// Renderiza o horário atual usando SDL_TTF com suavização (blended)
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, data.currentTime.c_str(), white);
    if(surf) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
        
        /**
         * Posiciona à esquerda do avatar com espaçamento:
         * AvatarX - Radius - Padding(30px) - Largura do Texto
         */
        int clockX = avatarX - avatarRadius - 30 - surf->w;
        
        SDL_Rect dst = {clockX, topMargin, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &dst);
        
        SDL_FreeSurface(surf);
        SDL_DestroyTexture(tex);

        // INDICADOR DE BATERIA (À Esquerda do Relógio)
        
        int batW = 40; ///< Largura do corpo da bateria
        int batH = 20; ///< Altura do corpo da bateria
        int batX = clockX - 50; ///< 50px de espaçamento do relógio
        int batY = topMargin + 5; ///< Ajuste vertical para alinhamento

        if (data.batteryLevel == BATERIA_INDISPONIVEL) {
            SDL_Surface* acSurf = TTF_RenderUTF8_Blended(font, "AC", white);
            if (acSurf) {
                SDL_Texture* acTex = SDL_CreateTextureFromSurface(renderer, acSurf);
                if (acTex) {
                    SDL_Rect acDst = {
                        batX + (batW - acSurf->w) / 2,
                        batY + (batH - acSurf->h) / 2,
                        acSurf->w,
                        acSurf->h
                    };
                    SDL_RenderCopy(renderer, acTex, NULL, &acDst);
                    SDL_DestroyTexture(acTex);
                }
                SDL_FreeSurface(acSurf);
            }
        } else {
            /**
             * Desenha contorno da bateria com cantos arredondados.
             * Utiliza cinza claro (200,200,200) para visibilidade.
             */
            roundedRectangleRGBA(renderer, batX, batY, batX + batW, batY + batH, 4, 200, 200, 200, 255);

            /**
             * Desenha o terminal positivo da bateria (pequeno retângulo à direita).
             * Representa o "bico" típico de ícones de bateria.
             */
            boxRGBA(renderer, batX + batW, batY + 6, batX + batW + 4, batY + batH - 6, 200, 200, 200, 255);

            /**
             * Calcula largura do preenchimento baseado no nível de carga.
             * batteryLevel varia de 0-100, convertido proporcionalmente.
             * Limitadores garantem que não exceda dimensões da bateria.
             */
            int fillW = (int)(batW * (data.batteryLevel / 100.0f));
            if(fillW > batW - 6) fillW = batW - 6; ///< Margem interna de 3px
            if(fillW < 0) fillW = 0; ///< Nunca negativo

            /// Desenha preenchimento branco proporcional à carga
            boxRGBA(renderer, batX + 3, batY + 3, batX + 3 + fillW, batY + batH - 3, 255, 255, 255, 255);
        }

        // INDICADOR WiFi (À Esquerda da Bateria)
        
        /**
         * Desenha ícone WiFi estilizado usando arcos concêntricos.
         * Renderizado se houver Wi-Fi ou conexão cabeada.
         */
        if (data.wifiConnected || data.wiredConnected) {
            int wifiX = batX - 40; ///< Posição X do centro do ícone
            int wifiY = batY + 18; ///< Base dos arcos (parte inferior)

            if (data.wiredConnected) {
                roundedRectangleRGBA(renderer, wifiX - 11, wifiY - 14, wifiX + 11, wifiY + 3, 3, 255, 255, 255, 255);
                boxRGBA(renderer, wifiX - 4, wifiY + 3, wifiX + 4, wifiY + 9, 255, 255, 255, 255);
                lineRGBA(renderer, wifiX - 14, wifiY + 9, wifiX + 14, wifiY + 9, 255, 255, 255, 255);
            } else {
                for(int r = 4; r <= 16; r += 6) {
                     arcRGBA(renderer, wifiX, wifiY, r, 225, 315, 255, 255, 255, 255);
                     if(r > 4) arcRGBA(renderer, wifiX, wifiY, r-1, 225, 315, 255, 255, 255, 255);
                }
                filledCircleRGBA(renderer, wifiX, wifiY, 2, 255, 255, 255, 255);
            }
        }
    }
}

/**
 * @brief Renderiza a barra de navegação inferior com dicas de controles.
 * 
 * Desenha uma barra escura no rodapé da tela contendo:
 * - **Botões de ação** (direita): A=SELECIONAR, B=VOLTAR
 * - **Menu de opções** (esquerda): Ícone hamburger + texto "OPÇÕES"
 * 
 * Cada botão é representado por um círculo branco com a letra do controle,
 * seguido do texto descritivo da ação. Estilo inspirado em interfaces modernas
 * como Steam Big Picture e interfaces de console.
 * 
 * @param renderer Renderizador SDL onde a barra será desenhada.
 * @param screenWidth Largura total da tela para posicionamento.
 * @param screenHeight Altura total da tela para calcular posição inferior.
 * @param font Fonte TTF para renderização de texto e letras dos botões.
 * 
 * @note A barra possui altura fixa de 70 pixels.
 * @note Utiliza lambda function interna para desenhar cada prompt de forma reutilizável.
 */
void GraphicsUtils::drawBottomNavHints(SDL_Renderer* renderer, int screenWidth, int screenHeight, TTF_Font* font) {
    if (!font) return;

    int barHeight = 70; ///< Altura total da barra de navegação
    int yStart = screenHeight - barHeight; ///< Posição Y inicial (topo da barra)

    /**
     * Desenha fundo escuro sólido para a barra.
     * Cor azul-acinzentado escuro (20,23,26) com alta opacidade (240).
     * Inspirado na paleta Steam Dark.
     */
    boxRGBA(renderer, 0, yStart, screenWidth, screenHeight, 20, 23, 26, 240);

    /**
     * @brief Lambda auxiliar para desenhar cada prompt (botão + texto).
     * 
     * Renderiza da direita para esquerda, atualizando cursor (xRight).
     * 
     * Estrutura de cada prompt:
     * 1. Texto da ação (ex: "VOLTAR")
     * 2. Círculo branco com letra do botão (ex: "B")
     * 
     * @param xRight Referência para posição X atual (modificada pela função).
     * @param key Letra do botão do controle (A, B, X, Y, etc).
     * @param action Texto descritivo da ação.
     */
    auto drawPrompt = [&](int& xRight, const std::string& key, const std::string& action) {
        /// Renderiza texto da ação em cinza claro
        SDL_Color textCol = {200, 200, 200, 255};
        SDL_Surface* sText = TTF_RenderUTF8_Blended(font, action.c_str(), textCol);
        if(!sText) return;
        
        /// Move cursor para a esquerda (largura do texto + espaçamento)
        xRight -= (sText->w + 30);
        
        SDL_Texture* tText = SDL_CreateTextureFromSurface(renderer, sText);
        /// Centraliza verticalmente na barra
        SDL_Rect rText = {xRight, yStart + (barHeight - sText->h)/2, sText->w, sText->h};
        SDL_RenderCopy(renderer, tText, NULL, &rText);
        
        /**
         * Desenha círculo branco representando o botão do controle.
         * Posicionado à esquerda do texto com 10px de espaçamento.
         */
        int btnSize = 32; ///< Diâmetro do botão
        int btnX = xRight - btnSize - 10;
        int btnY = yStart + (barHeight - btnSize)/2;

        /// Círculo preenchido branco sólido
        filledCircleRGBA(renderer, btnX + btnSize/2, btnY + btnSize/2, btnSize/2, 255, 255, 255, 255);
        
        /**
         * Renderiza a letra do botão em preto no centro do círculo.
         * Ajuste vertical de -1px para melhor alinhamento visual.
         */
        SDL_Color keyCol = {20, 20, 20, 255};
        SDL_Surface* sKey = TTF_RenderUTF8_Blended(font, key.c_str(), keyCol);
        if(sKey) {
            SDL_Texture* tKey = SDL_CreateTextureFromSurface(renderer, sKey);
            SDL_Rect rKey = {
                btnX + (btnSize - sKey->w)/2,
                btnY + (btnSize - sKey->h)/2 - 1, ///< Ajuste visual
                sKey->w, sKey->h
            };
            SDL_RenderCopy(renderer, tKey, NULL, &rKey);
            SDL_FreeSurface(sKey);
            SDL_DestroyTexture(tKey);
        }

        SDL_FreeSurface(sText);
        SDL_DestroyTexture(tText);
        
        /// Atualiza cursor para próximo elemento (posição do início do botão)
        xRight = btnX; 
    };

    /**
     * Renderiza prompts alinhados à direita, começando da borda.
     * Ordem de renderização: VOLTAR (mais à direita) → SELECIONAR
     */
    int cursorX = screenWidth - 40; ///< Cursor inicial com margem de 40px
    drawPrompt(cursorX, "B", "VOLTAR");
    drawPrompt(cursorX, "A", "SELECIONAR");
    
    // MENU DE OPÇÕES (Lado Esquerdo)
    
    int leftX = 40; ///< Posição X inicial com margem esquerda
    
    /// Renderiza texto "OPÇÕES" em cinza médio
    SDL_Color menuCol = {150, 150, 150, 255};
    SDL_Surface* sMenu = TTF_RenderUTF8_Blended(font, "OPÇÕES", menuCol);
    if(sMenu) {
        /**
         * Desenha ícone hamburger (três linhas horizontais) em círculo escuro.
         * Representa menu de opções/configurações de forma universalmente reconhecida.
         */
        int cX = leftX + 16; ///< Centro X do círculo
        int cY = yStart + barHeight/2; ///< Centro Y (meio da barra)
        
        /// Círculo de fundo cinza escuro
        filledCircleRGBA(renderer, cX, cY, 16, 60, 60, 60, 255);
        
        /**
         * Três linhas horizontais paralelas formam o ícone hamburger.
         * Espaçamento vertical de 4px entre as linhas.
         */
        lineRGBA(renderer, cX-8, cY-4, cX+8, cY-4, 255, 255, 255, 255); ///< Linha superior
        lineRGBA(renderer, cX-8, cY, cX+8, cY, 255, 255, 255, 255);     ///< Linha central
        lineRGBA(renderer, cX-8, cY+4, cX+8, cY+4, 255, 255, 255, 255); ///< Linha inferior

        /// Renderiza texto "OPÇÕES" à direita do ícone
        SDL_Texture* tMenu = SDL_CreateTextureFromSurface(renderer, sMenu);
        SDL_Rect rMenu = {leftX + 45, yStart + (barHeight - sMenu->h)/2, sMenu->w, sMenu->h};
        SDL_RenderCopy(renderer, tMenu, NULL, &rMenu);
        
        SDL_FreeSurface(sMenu);
        SDL_DestroyTexture(tMenu);
    }
}

} // namespace MeuProjeto
