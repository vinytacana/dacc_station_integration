/**
 * @file TecladoVirtual.cpp
 * @brief Implementação da lógica de renderização e interação do teclado virtual.
 * 
 * Este arquivo contém os cálculos de posicionamento das teclas, a lógica de 
 * navegação por foco (para controles) e a detecção de colisão para cliques de mouse.
 */

#include "TecladoVirtual.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp" 
#include <SDL2/SDL2_gfxPrimitives.h>
#include <iostream>

using namespace MeuProjeto;
using namespace std;

TecladoVirtual::TecladoVirtual() {}
TecladoVirtual::~TecladoVirtual() {}

/**
 * @brief Renderiza o teclado na tela se estiver visível.
 * 
 * A renderização segue os seguintes passos:
 * 1. Desenha um painel de fundo semitransparente (overlay).
 * 2. Calcula a centralização horizontal de cada linha de teclas.
 * 3. Itera sobre as strings das linhas para desenhar teclas individuais.
 * 4. Chama o desenho das teclas especiais na base.
 * 
 * @param renderer Renderizador SDL principal.
 */
void TecladoVirtual::desenhar(SDL_Renderer* renderer) {
    if (!visivel) return;

    auto& tema = GerenciadorTemas::getInstance();

    // Geometria do Fundo
    // O teclado é fixado a partir de BASE_X e BASE_Y. 
    // O padding externo do painel é de 20px em relação às teclas.
    SDL_Rect fundo = {BASE_X - 20, BASE_Y - 20, 1000, 400};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    // Define a cor do painel com base no tema, com Alpha em 240 (quase opaco)
    if (tema.getTemaAtual() == TipoTema::CLARO) {
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 240); 
    } else {
        SDL_SetRenderDrawColor(renderer, 30, 30, 35, 240);
    }
    
    SDL_RenderFillRect(renderer, &fundo);

    int startY = BASE_Y;
    
    /**
     * @brief Lambda auxiliar para desenhar uma linha de teclas.
     * 
     * Calcula o startX para que a linha fique perfeitamente centralizada dentro
     * da largura de 1000px do painel, considerando o tamanho das teclas e o GAP.
     */
    auto desenharLinha = [&](const vector<string>& linha, int yIdx) {
        int larguraTotalLinha = (linha.size() * (TECLA_SIZE + GAP)) - GAP;
        int startX = BASE_X + (1000 - larguraTotalLinha) / 2;

        for (size_t i = 0; i < linha.size(); i++) {
            // Verifica se esta tecla é a que detém o foco atual do controle
            bool focado = (indiceY == yIdx && indiceX == (int)i);
            desenharTecla(renderer, startX + i * (TECLA_SIZE + GAP), startY, linha[i], focado);
        }
        // Avança o Y para a próxima linha
        startY += TECLA_SIZE + GAP;
    };

    // Desenha as 4 linhas alfanuméricas
    desenharLinha(linha1, 0);
    desenharLinha(linha2, 1);
    desenharLinha(linha3, 2);
    desenharLinha(linha4, 3);

    // Desenha a última linha (Especiais)
    desenharEspeciais(renderer);
}

/**
 * @brief Desenha uma tecla individual (letra ou número).
 * 
 * Utiliza a biblioteca SDL2_gfx para desenhar retângulos arredondados.
 * Se a tecla estiver focada, desenha uma borda extra de destaque.
 */
void TecladoVirtual::desenharTecla(SDL_Renderer* renderer, int x, int y, const string& letra, bool focado) {
    auto& tema = GerenciadorTemas::getInstance();
    
    SDL_Color corTecla = tema.getCorBotaoNormal();
    SDL_Color corTexto = tema.getCorTextoNegrito();
    SDL_Color corDestaque = tema.getCorDestaque();

    if (focado) {
        // Borda de foco: Padding de 2px para fora da tecla
        roundedBoxRGBA(renderer, x - 2, y - 2, x + TECLA_SIZE + 2, y + TECLA_SIZE + 2, 5, 
                       corDestaque.r, corDestaque.g, corDestaque.b, 255);
        // Fundo da tecla focada (usa cor de Hover para feedback)
        roundedBoxRGBA(renderer, x, y, x + TECLA_SIZE, y + TECLA_SIZE, 5, 
                       tema.getCorBotaoHover().r, tema.getCorBotaoHover().g, tema.getCorBotaoHover().b, 255);
    } else {
        // Fundo da tecla em estado normal
        roundedBoxRGBA(renderer, x, y, x + TECLA_SIZE, y + TECLA_SIZE, 5, 
                       corTecla.r, corTecla.g, corTecla.b, 255);
    }
    
    // Renderiza o caractere centralizado (Offset manual de 20x10px)
    desenharTexto(renderer, letra, x + 20, y + 10, corTexto, 32, TipoFonte::NEGRITO);
}

/**
 * @brief Desenha a linha de botões funcionais (Espaço, Limpar, Apagar, OK).
 * 
 * Estes botões possuem larguras diferentes das teclas normais e cores
 * semânticas (Vermelho para limpar, Verde para OK).
 */
void TecladoVirtual::desenharEspeciais(SDL_Renderer* renderer) {
    int y = BASE_Y + 4 * (TECLA_SIZE + GAP); // Posiciona na 5ª linha (índice 4)
    int espacoX = BASE_X + 50;
    int largPadrao = 180;
    int gapBotoes = 20;
    int yEsp = y;

    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color corTexto = tema.getCorTextoNegrito();
    SDL_Color corDestaque = tema.getCorDestaque();

    /**
     * @brief Lambda para desenhar botões especiais coloridos.
     */
    auto desenharEsp = [&](int x, int w, const string& txt, int idxX, int r, int g, int b) {
        bool focado = (indiceY == 4 && indiceX == idxX);
        if (focado) {
            roundedBoxRGBA(renderer, x-2, yEsp-2, x+w+2, yEsp+TECLA_SIZE+2, 5, 
                           corDestaque.r, corDestaque.g, corDestaque.b, 255);
        }
        roundedBoxRGBA(renderer, x, yEsp, x+w, yEsp+TECLA_SIZE, 5, r, g, b, 255);
        
        // Centralização do texto baseada na largura do botão
        int textX = x + (w/2) - (txt.length() * 8); 
        desenharTexto(renderer, txt, textX, yEsp + 15, {255,255,255,255}, 24, TipoFonte::NEGRITO); 
    };

    // 0: ESPAÇO (Segue a cor do tema)
    SDL_Color btnCor = tema.getCorBotaoNormal();
    bool focadoEspaco = (indiceY == 4 && indiceX == 0);
    if (focadoEspaco) {
        roundedBoxRGBA(renderer, espacoX-2, y-2, espacoX+largPadrao+2, y+TECLA_SIZE+2, 5, 
                       corDestaque.r, corDestaque.g, corDestaque.b, 255);
    }
    roundedBoxRGBA(renderer, espacoX, y, espacoX+largPadrao, y+TECLA_SIZE, 5, 
                   btnCor.r, btnCor.g, btnCor.b, 255);
    desenharTexto(renderer, "ESPAÇO", espacoX + 40, y + 15, corTexto, 24, TipoFonte::NEGRITO);

    // 1: LIMPAR (Vermelho suave)
    int limparX = espacoX + largPadrao + gapBotoes;
    desenharEsp(limparX, largPadrao, "LIMPAR", 1, 120, 60, 60);

    // 2: APAGAR (Laranja suave)
    int apagarX = limparX + largPadrao + gapBotoes;
    desenharEsp(apagarX, largPadrao, "<--", 2, 150, 100, 60);

    // 3: OK (Verde suave)
    int okX = apagarX + largPadrao + gapBotoes;
    int largOk = 100;
    desenharEsp(okX, largOk, "OK", 3, 60, 150, 60);
}

/**
 * @brief Gerencia o movimento do cursor na matriz do teclado.
 * 
 * Ajusta os índices X e Y garantindo que o cursor não saia dos limites das linhas,
 * que possuem tamanhos variados.
 */
void TecladoVirtual::moverCursor(int deltaX, int deltaY) {
    if (deltaY != 0) {
        indiceY += deltaY;
        if (indiceY < 0) indiceY = 0;
        if (indiceY > 4) indiceY = 4;
        
        // Ajusta o X se a nova linha for menor que a anterior
        int maxX = 0;
        if (indiceY == 0) maxX = linha1.size();
        else if (indiceY == 1) maxX = linha2.size();
        else if (indiceY == 2) maxX = linha3.size();
        else if (indiceY == 3) maxX = linha4.size();
        else if (indiceY == 4) maxX = 4; // Linha de especiais
        
        if (indiceX >= maxX) indiceX = maxX - 1;
    }
    
    if (deltaX != 0) {
        indiceX += deltaX;
        int maxX = 0;
        if (indiceY == 0) maxX = linha1.size();
        else if (indiceY == 1) maxX = linha2.size();
        else if (indiceY == 2) maxX = linha3.size();
        else if (indiceY == 3) maxX = linha4.size();
        else if (indiceY == 4) maxX = 4;
        
        if (indiceX < 0) indiceX = 0;
        if (indiceX >= maxX) indiceX = maxX - 1;
    }
}

/**
 * @brief Processa entradas de controles (Gamepad).
 * 
 * Lida com:
 * - Movimento Analógico (com Deadzone).
 * - Movimento D-Pad (Digital).
 * - Botão A para confirmar tecla.
 * - Botão B para fechar teclado.
 */
bool TecladoVirtual::processarControle(SDL_Event& evento, BotaoPesquisa* alvo) {
    if (!visivel) return false;
    Uint32 tempoAtual = SDL_GetTicks();

    // Processamento Analógico
    if (evento.type == SDL_CONTROLLERAXISMOTION) {
        if (tempoAtual - ultimoInputAnalogico > INTERVALO_ANALOGICO) {
            int valX = 0, valY = 0;
            if (evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                if (evento.caxis.value > DEADZONE) valX = 1; 
                else if (evento.caxis.value < -DEADZONE) valX = -1;
            }
            else if (evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                if (evento.caxis.value > DEADZONE) valY = 1; 
                else if (evento.caxis.value < -DEADZONE) valY = -1;
            }
            
            if (valX != 0 || valY != 0) {
                moverCursor(valX, valY);
                ultimoInputAnalogico = tempoAtual;
                return true;
            }
        }
        return true;
    }

    // Processamento de Botões
    if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
        if (tempoAtual - ultimoInput < INTERVALO_INPUT) return true;
        
        switch (evento.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:    moverCursor(0, -1); break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  moverCursor(0, 1);  break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  moverCursor(-1, 0); break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: moverCursor(1, 0);  break;
            
            case SDL_CONTROLLER_BUTTON_A:
                if (indiceY < 4) {
                    // Insere caractere alfanumérico
                    string car;
                    if (indiceY == 0) car = linha1[indiceX];
                    else if (indiceY == 1) car = linha2[indiceX];
                    else if (indiceY == 2) car = linha3[indiceX];
                    else if (indiceY == 3) car = linha4[indiceX];
                    if (alvo) alvo->adicionarTexto(car);
                } else {
                    // Ações especiais (Linha 4)
                    if (indiceX == 0) { if(alvo) alvo->adicionarTexto(" "); }
                    else if (indiceX == 1) { if(alvo) alvo->limparTexto(); }
                    else if (indiceX == 2) { if(alvo) alvo->apagarTexto(); }
                    else if (indiceX == 3) fechar(); // Botão OK
                }
                break;
            
            case SDL_CONTROLLER_BUTTON_B:
                fechar();
                break;
        }
        ultimoInput = tempoAtual;
        return true;
    }
    return true; 
}

/**
 * @brief Processa interações do mouse com as teclas.
 * 
 * Detecta o clique em coordenadas específicas e mapeia para a tecla correspondente.
 * Se o clique ocorrer fora da área do teclado, o componente é fechado.
 */
bool TecladoVirtual::processarMouse(SDL_Event& evento, BotaoPesquisa* alvo) {
    if (!visivel) return false;

    // Fecha o teclado se clicar fora da área total (com padding de 20px)
    if (evento.type == SDL_MOUSEBUTTONDOWN || evento.type == SDL_MOUSEBUTTONUP) {
        int mx = evento.button.x;
        int my = evento.button.y;
        SDL_Rect areaTotal = {BASE_X - 20, BASE_Y - 20, 1000, 400};
        if (mx < areaTotal.x || mx > areaTotal.x + areaTotal.w || 
            my < areaTotal.y || my > areaTotal.y + areaTotal.h) {
            if (evento.type == SDL_MOUSEBUTTONUP) fechar();
            return true; 
        }
    }

    if (evento.type != SDL_MOUSEBUTTONUP) return true; 

    int mx = evento.button.x;
    int my = evento.button.y;
    int startY = BASE_Y;

    /**
     * @brief Lambda para verificar clique em uma linha específica.
     */
    auto verificarCliqueLinha = [&](const vector<string>& linha, int yIdx) {
        int larguraTotalLinha = (linha.size() * (TECLA_SIZE + GAP)) - GAP;
        int startX = BASE_X + (1000 - larguraTotalLinha) / 2;

        for (size_t i = 0; i < linha.size(); i++) {
            if (checarCliqueTecla(mx, my, startX + i * (TECLA_SIZE + GAP), startY, TECLA_SIZE, TECLA_SIZE)) {
                if (alvo) alvo->adicionarTexto(linha[i]);
                indiceX = i; indiceY = yIdx; 
                return true;
            }
        }
        return false;
    };

    // Verifica cliques linha por linha
    if (verificarCliqueLinha(linha1, 0)) return true;
    startY += TECLA_SIZE + GAP;
    if (verificarCliqueLinha(linha2, 1)) return true;
    startY += TECLA_SIZE + GAP;
    if (verificarCliqueLinha(linha3, 2)) return true;
    startY += TECLA_SIZE + GAP;
    if (verificarCliqueLinha(linha4, 3)) return true;
    startY += TECLA_SIZE + GAP;

    // Verificação dos botões especiais na base
    int espacoX = BASE_X + 50;
    int largPadrao = 180;
    int gapBotoes = 20;
    int yEsp = startY;

    if (checarCliqueTecla(mx, my, espacoX, yEsp, largPadrao, TECLA_SIZE)) {
        if(alvo) alvo->adicionarTexto(" ");
        indiceX=0; indiceY=4; return true;
    }
    int limparX = espacoX + largPadrao + gapBotoes;
    if (checarCliqueTecla(mx, my, limparX, yEsp, largPadrao, TECLA_SIZE)) {
        if(alvo) alvo->limparTexto();
        indiceX=1; indiceY=4; return true;
    }
    int apagarX = limparX + largPadrao + gapBotoes;
    if (checarCliqueTecla(mx, my, apagarX, yEsp, largPadrao, TECLA_SIZE)) {
        if(alvo) alvo->apagarTexto();
        indiceX=2; indiceY=4; return true;
    }
    int okX = apagarX + largPadrao + gapBotoes;
    if (checarCliqueTecla(mx, my, okX, yEsp, 100, TECLA_SIZE)) {
        fechar();
        return true;
    }
    return true; 
}

/**
 * @brief Helper de colisão AABB simples.
 */
bool TecladoVirtual::checarCliqueTecla(int mouseX, int mouseY, int x, int y, int w, int h) {
    return (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h);
}