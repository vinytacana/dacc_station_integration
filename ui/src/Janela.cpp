#include "Janela.hpp"
#include "GerenciadorFontes.hpp"
#include <iostream>
#include <sstream>

// Declaração externa (definida em main.cpp no escopo global)
extern MeuProjeto::GerenciadorFontes gerFontes;

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Construtor da Janela.
 *
 * @param renderer O renderizador SDL principal da aplicação.
 */
Janela::Janela(SDL_Renderer* renderer)
    : renderer(renderer), isPopupAberto(false), popupTextura(nullptr) {
    if (!renderer) {
        cerr << "Renderizador nulo na Janela" << endl;
    }
}

/**
 * @brief Destrutor.
 */
Janela::~Janela() {
    // Texturas são gerenciadas pelo GerenciadorImagens ou pelo Botao,
    // não destruímos popupTextura aqui.
}

/**
 * @brief Abre o popup com informações detalhadas de um item.
 *
 * @param nome O título do item.
 * @param textura A textura da imagem principal do item.
 * @param desc A descrição detalhada do item.
 * @param x Coordenada X do elemento que disparou o popup.
 * @param y Coordenada Y do elemento que disparou o popup.
 * @param w Largura do elemento que disparou o popup.
 * @param h Altura do elemento que disparou o popup.
 */
void Janela::abrirPopup(const string& nome, SDL_Texture* textura, const string& desc, int x, int y, int w, int h) {
    popupNome = nome;
    popupTextura = textura; 
    popupDesc = desc;

    // --- CORREÇÃO DE POSICIONAMENTO ---
    const int POPUP_LARGURA = 400;
    const int POPUP_ALTURA = 450;
    
    // Nova lógica (centralizado acima):
    int popupX = x + (w / 2) - (POPUP_LARGURA / 2); // Centraliza horizontalmente
    int popupY = y - POPUP_ALTURA - 10;             // Posiciona 10px acima do botão

    // Verificação de Limites (Impede de sair da tela)
    
    // Se sair pelo topo da tela (Y < 0), posiciona ABAIXO do botão
    if (popupY < 0) {
        popupY = y + h + 10; // 10px abaixo
    }

    // Se sair pela esquerda
    if (popupX < 10) {
        popupX = 10;
    }
    // Se sair pela direita (assumindo largura do mundo de 1920)
    if (popupX + POPUP_LARGURA > 1910) {
        popupX = 1910 - POPUP_LARGURA;
    }

    popupArea = {popupX, popupY, POPUP_LARGURA, POPUP_ALTURA};
    isPopupAberto = true;
}

/**
 * @brief Fecha o popup, desativando sua renderização.
 */
void Janela::fecharPopup() {
    isPopupAberto = false;
}

/**
 * @brief Desenha o popup, incluindo fundo, imagem, título e descrição.
 *
 * @param offsetX Deslocamento X da viewport.
 * @param offsetY Deslocamento Y da viewport.
 */
void Janela::desenharPopup(int offsetX, int offsetY) {
    if (!isPopupAberto || !renderer) return;

    SDL_Rect adjustedArea = {
        popupArea.x + offsetX,
        popupArea.y + offsetY,
        popupArea.w,
        popupArea.h
    };

    // Habilita transparência (blending)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // 1. Desenhar Fundo com transparência
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 198);
    SDL_RenderFillRect(renderer, &adjustedArea);

    // 2. Desenhar Imagem do Jogo (Grande)
    if (popupTextura) {
        SDL_Rect texRect = {
            adjustedArea.x + 10, 
            adjustedArea.y + 10, 
            adjustedArea.w - 20, // Largura (400 - 20)
            215                  // Altura fixa (aprox. 16:9)
        };

        // Ativa transparência na textura 
        SDL_SetTextureBlendMode(popupTextura, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(popupTextura, 198); // mesmo alpha do fundo

        SDL_RenderCopy(renderer, popupTextura, nullptr, &texRect);
        SDL_SetTextureAlphaMod(popupTextura, 255); // Restaura o alpha da textura original
    }

    // 3. Desenhar Título (Grande e Centralizado)
    
    int yPos = adjustedArea.y + 215 + 20; 
    int fontSizeTitulo = 30;
    

    const std::string& fontePath = ConfigFontes::obterCaminho(TipoFonte::NEGRITO);
    TTF_Font* font = gerFontes.carregar(fontePath, fontSizeTitulo);
    
    int textWidth = 0;
    int textHeight = 0;
    
    if (font) {
        // Obter tamanho real do texto renderizado
        TTF_SizeUTF8(font, popupNome.c_str(), &textWidth, &textHeight);
    } else {
        // Fallback para estimativa (se falhar carregar fonte)
        textWidth = popupNome.length() * (fontSizeTitulo / 2);
    }
    
    // Centraliza baseado na largura REAL
    int xPosTitulo = adjustedArea.x + (adjustedArea.w - textWidth) / 2;
    
    // Garante que não saia da área do popup (bounding box)
    int paddingLateral = 10;
    if (xPosTitulo < adjustedArea.x + paddingLateral) {
        xPosTitulo = adjustedArea.x + paddingLateral;
    }
    if (xPosTitulo + textWidth > adjustedArea.x + adjustedArea.w - paddingLateral) {
        xPosTitulo = adjustedArea.x + adjustedArea.w - textWidth - paddingLateral;
    }

    // Renderiza o título
    desenharTexto(renderer, popupNome, xPosTitulo, yPos, {255, 255, 255, 198}, fontSizeTitulo, MeuProjeto::TipoFonte::NEGRITO);

    // 4. Desenhar Descrição (Pequena, com quebra de linha)
    yPos += fontSizeTitulo + 10;
    int fontSizeDesc = 18;
    int linhaAltura = 22; // Altura aproximada da linha
    int paddingDesc = 15;
    int xPosDesc = adjustedArea.x + paddingDesc;
    int maxLargura = adjustedArea.w - (paddingDesc * 2);

    stringstream ss(popupDesc);
    string linhaAtual, palavra;
    
    // Processamento de quebra de linha
    while (ss >> palavra) {
        // Tenta estimar a largura
        int larguraLinhaAtual = (int)linhaAtual.length() * (fontSizeDesc / 2);
        int larguraPalavra = (int)palavra.length() * (fontSizeDesc / 2);

        // Se a palavra cabe na linha atual, adiciona
        if (larguraLinhaAtual + larguraPalavra < maxLargura) {
            linhaAtual += palavra + " ";
        } else {
            // Senão, desenha a linha atual e começa uma nova
            desenharTexto(renderer, linhaAtual, xPosDesc, yPos, {200, 200, 200, 198}, fontSizeDesc, MeuProjeto::TipoFonte::NORMAL);
            yPos += linhaAltura;
            linhaAtual = palavra + " ";
        }
        // Sai se o texto atingir o limite inferior
        if (yPos > adjustedArea.y + adjustedArea.h - linhaAltura) break;
    }

    // Desenha a última linha (se houver espaço e conteúdo)
    if (yPos <= adjustedArea.y + adjustedArea.h - linhaAltura && !linhaAtual.empty()) {
        desenharTexto(renderer, linhaAtual, xPosDesc, yPos, {200, 200, 200, 198}, fontSizeDesc, MeuProjeto::TipoFonte::NORMAL);
    }
}