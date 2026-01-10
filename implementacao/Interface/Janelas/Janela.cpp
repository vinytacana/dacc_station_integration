/**
 * @file Janela.cpp
 * @brief Implementação Responsiva da Janela de Popup.
 *
 * Este arquivo contém a lógica operacional para a exibição de janelas informativas 
 * suspensas. A implementação foca no cálculo dinâmico de coordenadas para evitar 
 * que elementos fiquem fora da área visível da tela (clamping), além de gerenciar 
 * a renderização de componentes visuais e textuais adaptáveis.
 */

#include "Interface/Janelas/Janela.hpp"
#include "Gerenciadores/GerenciadorFontes.hpp"
#include "Gerenciadores/GerenciadorTemas.hpp"
#include "Principais/ConfigLayout.hpp"
#include "Principais/Utils.hpp" 
#include <iostream>
#include <sstream>

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Construtor da classe Janela.
 * Inicializa os parâmetros de layout baseando-se em um design de referência de 1920x1080, 
 * utilizando a classe ConfigLayout para escalar as dimensões para a resolução atual. 
 * Define áreas de respiro (padding), altura de imagem e espaçamentos entre blocos textuais.
 * @param renderer Ponteiro para o renderizador SDL_Renderer.
 */
Janela::Janela(SDL_Renderer* renderer)
    : renderer(renderer), isPopupAberto(false), popupTextura(nullptr) {
    if (!renderer) {
        cerr << "Renderizador nulo na Janela" << endl;
    }

    // Inicialização de Layout Responsivo 
    larguraPadrao = ConfigLayout::X(400);
    alturaPadrao = ConfigLayout::Y(450);
    paddingExterno = ConfigLayout::X(10); 
    
    paddingInterno = ConfigLayout::X(10);
    alturaImagem = ConfigLayout::Y(215);
    espacoTitulo = ConfigLayout::Y(20);
    espacoDesc = ConfigLayout::Y(10);
}

/**
 * @brief Destrutor da classe Janela.
 */
Janela::~Janela() {
}

/**
 * @brief Ativa o popup e calcula sua posição ótima na tela.
 * Implementa um algoritmo de posicionamento inteligente que:
 * 1. Centraliza horizontalmente o popup em relação ao objeto de origem.
 * 2. Tenta posicionar o popup acima do objeto.
 * 3. Realiza a inversão vertical (flip) caso o popup ultrapasse o limite superior da tela.
 * 4. Aplica "clamping" lateral para garantir que o componente permaneça dentro dos 
 * limites horizontais da janela do sistema.
 * @param nome String contendo o título do popup.
 * @param textura Ponteiro para a textura da imagem de capa.
 * @param desc String contendo a descrição do conteúdo.
 * @param x Coordenada X do objeto que disparou o popup.
 * @param y Coordenada Y do objeto que disparou o popup.
 * @param w Largura do objeto que disparou o popup.
 * @param h Altura do objeto que disparou o popup.
 */
void Janela::abrirPopup(const string& nome, SDL_Texture* textura, const string& desc, int x, int y, int w, int h) {
    popupNome = nome;
    popupTextura = textura; 
    popupDesc = desc;

    // Cálculo da centralização horizontal relativa ao pai
    int popupX = x + (w / 2) - (larguraPadrao / 2); 
    
    // Tentativa de posicionamento superior com margem de segurança
    int popupY = y - alturaPadrao - paddingExterno; 

    // Lógica de inversão vertical por colisão com o limite superior (y < 0)
    if (popupY < 0) {
        popupY = y + h + paddingExterno; 
    }

    // Algoritmo de clamping para limites laterais da tela
    if (popupX < paddingExterno) popupX = paddingExterno;
    
    int limiteDireita = ConfigLayout::larguraTela - larguraPadrao - paddingExterno;
    if (popupX > limiteDireita) popupX = limiteDireita;

    popupArea = {popupX, popupY, larguraPadrao, alturaPadrao};
    isPopupAberto = true;
}

/**
 * @brief Desativa o estado de exibição do popup.
 */
void Janela::fecharPopup() {
    isPopupAberto = false;
}

/**
 * @brief Renderiza a interface do popup e processa a diagramação de texto.
 * O método realiza as seguintes operações técnicas:
 * 1. Renderiza o fundo com transparência utilizando SDL_BLENDMODE_BLEND.
 * 2. Aplica modulação de alfa na textura da imagem para consistência visual.
 * 3. Calcula a centralização do título com base na largura real do texto renderizado.
 * 4. Implementa um algoritmo de "Word Wrap" (quebra de linha) para a descrição, 
 * utilizando estimativas de largura de glifos baseadas no tamanho da fonte escalada.
 * 5. Garante que o texto não ultrapasse o limite vertical inferior do popup (clipping lógico).
 * @param offsetX Deslocamento horizontal para ajuste de renderização.
 * @param offsetY Deslocamento vertical para ajuste de renderização.
 */
void Janela::desenharPopup(int offsetX, int offsetY) {
    if (!isPopupAberto || !renderer) return;

    auto& tema = GerenciadorTemas::getInstance(); 

    SDL_Rect adjustedArea = {
        popupArea.x + offsetX,
        popupArea.y + offsetY,
        popupArea.w,
        popupArea.h
    };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_Color corBg = tema.getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, corBg.r, corBg.g, corBg.b, 220);
    SDL_RenderFillRect(renderer, &adjustedArea);

    // Renderização da imagem principal com preservação de canal alfa
    if (popupTextura) {
        SDL_Rect texRect = {
            adjustedArea.x + paddingInterno, 
            adjustedArea.y + paddingInterno, 
            adjustedArea.w - (paddingInterno * 2), 
            alturaImagem 
        };

        SDL_SetTextureBlendMode(popupTextura, SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(popupTextura, 255); 

        SDL_RenderCopy(renderer, popupTextura, nullptr, &texRect);
    }

    static GerenciadorFontes gerenciadorFontes;
    
    // Cálculo e renderização do Título
    int yPos = adjustedArea.y + alturaImagem + espacoTitulo; 
    int fontSizeTitulo = ConfigLayout::F(30);
    
    const std::string& fontePath = ConfigFontes::obterCaminho(TipoFonte::NEGRITO);
    TTF_Font* font = gerenciadorFontes.carregar(fontePath, fontSizeTitulo);
    
    int textWidth = 0;
    int textHeight = 0;
    
    // [NOVO] Lógica de Truncamento do Título (Limite 20 caracteres)
    std::string tituloExibicao = popupNome;
    if (tituloExibicao.length() > 20) {
        tituloExibicao = tituloExibicao.substr(0, 20) + "...";
    }
    
    // Utiliza a string truncada para medir o tamanho
    if (font) {
        TTF_SizeUTF8(font, tituloExibicao.c_str(), &textWidth, &textHeight);
    } else {
        textWidth = tituloExibicao.length() * (fontSizeTitulo / 2);
    }
    
    // Centralização com tratamento de bordas (safety padding)
    int xPosTitulo = adjustedArea.x + (adjustedArea.w - textWidth) / 2;
    
    if (xPosTitulo < adjustedArea.x + paddingInterno) {
        xPosTitulo = adjustedArea.x + paddingInterno;
    }
    if (xPosTitulo + textWidth > adjustedArea.x + adjustedArea.w - paddingInterno) {
        xPosTitulo = adjustedArea.x + adjustedArea.w - textWidth - paddingInterno;
    }

    SDL_Color corTitulo = tema.getCorTextoNegrito();
    // Renderiza a versão truncada
    MeuProjeto::desenharTexto(renderer, tituloExibicao, xPosTitulo, yPos, corTitulo, fontSizeTitulo, MeuProjeto::TipoFonte::NEGRITO);

    // Lógica de Word Wrap Responsivo para a Descrição
    yPos += fontSizeTitulo + espacoDesc;
    int fontSizeDesc = ConfigLayout::F(18);
    int linhaAltura = ConfigLayout::Y(22);

    int paddingDesc = ConfigLayout::X(15);
    int xPosDesc = adjustedArea.x + paddingDesc;
    int maxLargura = adjustedArea.w - (paddingDesc * 2);

    stringstream ss(popupDesc);
    string linhaAtual, palavra;
    
    SDL_Color corDesc = tema.getCorTextoNormal();

    // Processamento de fluxo de texto palavra por palavra
    while (ss >> palavra) {
        // Cálculo de largura estimada para detecção de quebra de linha
        int larguraLinhaAtual = linhaAtual.length() * (fontSizeDesc / 2); 
        int larguraPalavra = palavra.length() * (fontSizeDesc / 2);

        if (larguraLinhaAtual + larguraPalavra < maxLargura) {
            linhaAtual += palavra + " ";
        } else {
            MeuProjeto::desenharTexto(renderer, linhaAtual, xPosDesc, yPos, corDesc, fontSizeDesc, MeuProjeto::TipoFonte::NORMAL);
            yPos += linhaAltura;
            linhaAtual = palavra + " ";
        }
        
        // Proteção contra overflow vertical do popup
        if (yPos > adjustedArea.y + adjustedArea.h - linhaAltura) break;
    }

    // Renderização do remanescente do buffer de texto
    if (yPos <= adjustedArea.y + adjustedArea.h - linhaAltura && !linhaAtual.empty()) {
        MeuProjeto::desenharTexto(renderer, linhaAtual, xPosDesc, yPos, corDesc, fontSizeDesc, MeuProjeto::TipoFonte::NORMAL);
    }
}