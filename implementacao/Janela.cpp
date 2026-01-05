/**
 * @file Janela.cpp
 * @brief Implementação da classe Janela para exibição de Popups informativos.
 * 
 * Este arquivo contém a lógica de renderização de janelas flutuantes que surgem
 * ao interagir com elementos da biblioteca. Inclui sistemas de quebra de linha 
 * automática para descrições e cálculos de posicionamento relativo.
 */

#include "Janela.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorTemas.hpp"
#include <iostream>
#include <sstream>

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Construtor da Janela.
 * @param renderer Ponteiro para o renderizador SDL principal.
 */
Janela::Janela(SDL_Renderer* renderer)
    : renderer(renderer), isPopupAberto(false), popupTextura(nullptr) {
    if (!renderer) {
        cerr << "Renderizador nulo na Janela" << endl;
    }
}

/**
 * @brief Destrutor da Janela.
 */
Janela::~Janela() {
}

/**
 * @brief Configura os dados e calcula a posição ideal para o popup.
 * 
 * O método utiliza uma lógica de posicionamento inteligente:
 * 1. Tenta posicionar o popup centralizado acima do elemento pai (x, y, w, h).
 * 2. Se o popup for sair pelo topo da tela (y < 0), ele é invertido para baixo do elemento.
 * 3. Aplica restrições laterais (clamping) para garantir que o popup não saia pelas bordas horizontais.
 * 
 * @param nome Nome do jogo.
 * @param textura Textura da imagem de capa.
 * @param desc Texto de descrição.
 * @param x Coordenada X do elemento pai.
 * @param y Coordenada Y do elemento pai.
 * @param w Largura do elemento pai.
 * @param h Altura do elemento pai.
 */
void Janela::abrirPopup(const string& nome, SDL_Texture* textura, const string& desc, int x, int y, int w, int h) {
    popupNome = nome;
    popupTextura = textura; 
    popupDesc = desc;

    const int POPUP_LARGURA = 400;
    const int POPUP_ALTURA = 450;
    
    // Centraliza horizontalmente em relação ao objeto pai
    int popupX = x + (w / 2) - (POPUP_LARGURA / 2); 
    // Posiciona acima do objeto com 10px de respiro
    int popupY = y - POPUP_ALTURA - 10; 

    // Ajuste de segurança: Se sair pelo topo, joga para baixo do objeto
    if (popupY < 0) {
        popupY = y + h + 10; 
    }

    // Ajuste de segurança: Clamping lateral (limites da tela 1920px)
    if (popupX < 10) popupX = 10;
    if (popupX + POPUP_LARGURA > 1910) popupX = 1910 - POPUP_LARGURA;

    popupArea = {popupX, popupY, POPUP_LARGURA, POPUP_ALTURA};
    isPopupAberto = true;
}

/**
 * @brief Fecha o popup e interrompe sua renderização.
 */
void Janela::fecharPopup() {
    isPopupAberto = false;
}

/**
 * @brief Renderiza o popup e todo o seu conteúdo interno.
 * 
 * A renderização segue a ordem de camadas:
 * 1. Fundo semitransparente (Retângulo preenchido).
 * 2. Imagem do jogo (Texture).
 * 3. Título centralizado (Fonte Negrito).
 * 4. Descrição com quebra automática (Fonte Normal).
 * 
 * @param offsetX Deslocamento horizontal global (scroll).
 * @param offsetY Deslocamento vertical global (scroll).
 */
void Janela::desenharPopup(int offsetX, int offsetY) {
    if (!isPopupAberto || !renderer) return;

    // Obtém a instância do tema para consistência visual
    auto& tema = GerenciadorTemas::getInstance(); 

    // Aplica o scroll à área original do popup
    SDL_Rect adjustedArea = {
        popupArea.x + offsetX,
        popupArea.y + offsetY,
        popupArea.w,
        popupArea.h
    };

    // Habilita transparência para o fundo do popup
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Desenha o container principal
    SDL_Color corBg = tema.getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, corBg.r, corBg.g, corBg.b, 220); // Alpha 220 para leve transparência
    SDL_RenderFillRect(renderer, &adjustedArea);

    /**
     * @section Padding_Imagem
     * Padding é o espaçamento interno entre o conteúdo (imagem ou texto) e a borda do container (o popup)
     * O padding aqui é de 10 pixels. 
     */
    if (popupTextura) {
        SDL_Rect texRect = {
            adjustedArea.x + 10, 
            adjustedArea.y + 10, 
            adjustedArea.w - 20, 
            215 
        };

        SDL_SetTextureBlendMode(popupTextura, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(popupTextura, 255); 

        SDL_RenderCopy(renderer, popupTextura, nullptr, &texRect);
    }

    static GerenciadorFontes gerenciadorFontes;
    
    // Início da área de texto abaixo da imagem (215px da imagem + 20px de respiro)
    int yPos = adjustedArea.y + 215 + 20; 
    int fontSizeTitulo = 30;
    
    const std::string& fontePath = ConfigFontes::obterCaminho(TipoFonte::NEGRITO);
    TTF_Font* font = gerenciadorFontes.carregar(fontePath, fontSizeTitulo);
    
    int textWidth = 0;
    int textHeight = 0;
    
    if (font) {
        TTF_SizeUTF8(font, popupNome.c_str(), &textWidth, &textHeight);
    } else {
        textWidth = popupNome.length() * (fontSizeTitulo / 2);
    }
    
    /**
     * @section Padding_Titulo
     * Cálculo para centralizar o texto garantindo um 'paddingLateral' mínimo de 10px.
     * Isso impede que títulos muito longos toquem na borda lateral do popup.
     */
    int xPosTitulo = adjustedArea.x + (adjustedArea.w - textWidth) / 2;
    int paddingLateral = 10;
    if (xPosTitulo < adjustedArea.x + paddingLateral) {
        xPosTitulo = adjustedArea.x + paddingLateral;
    }
    if (xPosTitulo + textWidth > adjustedArea.x + adjustedArea.w - paddingLateral) {
        xPosTitulo = adjustedArea.x + adjustedArea.w - textWidth - paddingLateral;
    }

    SDL_Color corTitulo = tema.getCorTextoNegrito();
    desenharTexto(renderer, popupNome, xPosTitulo, yPos, corTitulo, fontSizeTitulo, MeuProjeto::TipoFonte::NEGRITO);

    // Configurações para a descrição (corpo do texto)
    yPos += fontSizeTitulo + 10;
    int fontSizeDesc = 18;
    int linhaAltura = 22;

    /**
     * @section Padding_Descricao
     * Definimos um padding de 15px para o texto da descrição.
     * A largura máxima (maxLargura) é a largura total do popup menos o padding de ambos os lados.
     */
    int paddingDesc = 15;
    int xPosDesc = adjustedArea.x + paddingDesc;
    int maxLargura = adjustedArea.w - (paddingDesc * 2);

    // Algoritmo simples de Word Wrap (Quebra de Linha)
    stringstream ss(popupDesc);
    string linhaAtual, palavra;
    
    SDL_Color corDesc = tema.getCorTextoNormal();

    while (ss >> palavra) {
        // Estima a largura da linha atual (baseado na média de pixels por caractere)
        int larguraLinhaAtual = linhaAtual.length() * (fontSizeDesc / 2);
        int larguraPalavra = palavra.length() * (fontSizeDesc / 2);

        // Se a palavra cabe na linha, adiciona. Senão, desenha a linha e pula para a próxima.
        if (larguraLinhaAtual + larguraPalavra < maxLargura) {
            linhaAtual += palavra + " ";
        } else {
            desenharTexto(renderer, linhaAtual, xPosDesc, yPos, corDesc, fontSizeDesc, MeuProjeto::TipoFonte::NORMAL);
            yPos += linhaAltura;
            linhaAtual = palavra + " ";
        }
        
        // Proteção para não desenhar texto fora da área vertical do popup
        if (yPos > adjustedArea.y + adjustedArea.h - linhaAltura) break;
    }

    // Desenha a última linha restante
    if (yPos <= adjustedArea.y + adjustedArea.h - linhaAltura && !linhaAtual.empty()) {
        desenharTexto(renderer, linhaAtual, xPosDesc, yPos, corDesc, fontSizeDesc, MeuProjeto::TipoFonte::NORMAL);
    }
}