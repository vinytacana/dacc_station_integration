/**
 * @file TecladoVirtual.cpp
 * @brief Implementação Responsiva do Teclado Virtual.
 *
 * Este arquivo contém a lógica de funcionamento e renderização do teclado virtual.
 * A implementação foca na responsividade, utilizando a classe ConfigLayout para 
 * escalar dimensões e posições, e integra-se ao GerenciadorTemas para garantir 
 * a consistência visual com o restante da interface.
 */

#include "TecladoVirtual.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include <SDL2/SDL2_gfxPrimitives.h>
#include <iostream>

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Construtor da classe TecladoVirtual.
 * 
 * Realiza a inicialização de todos os parâmetros de layout utilizando métodos de 
 * escala (ConfigLayout::X, Y e F). Define as dimensões das teclas, espaçamentos (gaps), 
 * preenchimentos (padding) e tamanhos de fonte de forma a manter a proporção visual 
 * em diferentes resoluções de tela.
 */
TecladoVirtual::TecladoVirtual() {
    // Define posição padrão
    baseX = ConfigLayout::X(460);
    baseY = ConfigLayout::Y(600);
    
    // Chama inicialização comum
    inicializarLayout();

}

/**
 * @brief Construtor customizado com posição específica.
 * @param customBaseX Posição X base personalizada.
 * @param customBaseY Posição Y base personalizada.
 */
TecladoVirtual::TecladoVirtual(int customBaseX, int customBaseY) {
    std::cout << "[TECLADO] Criando teclado customizado em X:" << customBaseX << " Y:" << customBaseY << std::endl;
    
    // Define posição customizada
    baseX = customBaseX;
    baseY = customBaseY;
    
    // Chama inicialização comum
    inicializarLayout();
}

/**
 * @brief Inicializa variáveis comuns de layout (usado por ambos construtores).
 *
 */
void TecladoVirtual::inicializarLayout() {
    // Configuração das dimensões das teclas alfanuméricas
    teclaLargura = ConfigLayout::F(60);
    teclaAltura = ConfigLayout::F(60);
    gapX = ConfigLayout::F(10);
    gapY = ConfigLayout::F(10);

    // Configuração do painel de fundo que abriga o teclado
    painelLargura = ConfigLayout::F(1000);
    painelAltura = ConfigLayout::F(400);
    painelPadding = ConfigLayout::F(20); 

    // Configuração dos elementos da linha de funções especiais
    offsetEspacoX = ConfigLayout::F(50);
    larguraBotaoEsp = ConfigLayout::F(180);
    larguraBotaoOk = ConfigLayout::F(100);
    gapBotoesEsp = ConfigLayout::F(20);
    offsetTextoY = ConfigLayout::F(15);

    // Configuração de fontes e ajustes de centralização de caracteres
    fonteTamanhoTecla = ConfigLayout::F(32);
    fonteTamanhoEsp = ConfigLayout::F(24);
    offsetCharX = ConfigLayout::F(20);
    offsetCharY = ConfigLayout::F(10);
    
    std::cout << "[TECLADO] Layout inicializado - baseX:" << baseX << " baseY:" << baseY << std::endl;
}

/**
 * @brief Destrutor da classe TecladoVirtual.
 */
TecladoVirtual::~TecladoVirtual() {}

/**
 * @brief Gerencia a renderização principal do teclado e seus componentes.
 * 
 * O método desenha o painel de fundo com transparência (blend mode), calcula a 
 * centralização dinâmica de cada linha de teclas baseando-se na largura do painel 
 * e delega o desenho individual das teclas e dos botões especiais.
 * 
 * @param renderer Ponteiro para o renderizador SDL_Renderer.
 */
void TecladoVirtual::desenhar(SDL_Renderer* renderer) {
    if (!visivel) return;

    auto& tema = GerenciadorTemas::getInstance();

    // Definição da área do fundo responsivo
    SDL_Rect fundo = {
        baseX - painelPadding, 
        baseY - painelPadding, 
        painelLargura, 
        painelAltura
    };
    
    // Configuração de transparência e cor de fundo baseada no tema atual
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_Color corFundo = tema.getCorBotaoPressionado();
    SDL_SetRenderDrawColor(renderer, corFundo.r, corFundo.g, corFundo.b, 240);
   
    SDL_RenderFillRect(renderer, &fundo);

    int startY = baseY;
    
    /**
     * @brief Lambda interna para renderizar linhas de teclas com centralização automática.
     * @param linha Vetor de strings com os caracteres da linha.
     * @param yIdx Índice vertical da linha para controle de foco.
     */
    auto desenharLinha = [&](const vector<string>& linha, int yIdx) {
        int larguraTotalLinha = (linha.size() * (teclaLargura + gapX)) - gapX;
        
        // Cálculo de início horizontal para centralizar a linha dentro do painel
        int startX = baseX + (painelLargura - (2 * painelPadding) - larguraTotalLinha) / 2;

        for (size_t i = 0; i < linha.size(); i++) {
            bool focado = (indiceY == yIdx && indiceX == (int)i);
            desenharTecla(renderer, startX + i * (teclaLargura + gapX), startY, linha[i], focado);
        }
        startY += teclaAltura + gapY;
    };

    desenharLinha(linha1, 0);
    desenharLinha(linha2, 1);
    desenharLinha(linha3, 2);
    desenharLinha(linha4, 3);

    desenharEspeciais(renderer);
}

/**
 * @brief Renderiza uma tecla individual utilizando primitivas gráficas.
 * 
 * Utiliza a biblioteca SDL2_gfx para desenhar caixas arredondadas (roundedBoxRGBA), 
 * aplicando cores diferenciadas para estados de repouso e foco, além de renderizar 
 * o texto centralizado na tecla.
 * 
 * @param renderer Ponteiro para o renderizador SDL_Renderer.
 * @param x Posição horizontal da tecla.
 * @param y Posição vertical da tecla.
 * @param letra Texto/caractere a ser exibido.
 * @param focado Booleano que indica se a tecla possui o foco atual de navegação.
 */
void TecladoVirtual::desenharTecla(SDL_Renderer* renderer, int x, int y, const string& letra, bool focado) {
    auto& tema = GerenciadorTemas::getInstance();
    
    SDL_Color corTecla = tema.getCorBotaoNormal();
    SDL_Color corTexto = tema.getCorTextoNegrito();
    SDL_Color corDestaque = tema.getCorDestaque();

    int raio = ConfigLayout::F(5); 

    if (focado) {
        int padFoco = ConfigLayout::F(2);
        // Desenha a borda de destaque externa
        roundedBoxRGBA(renderer, x - padFoco, y - padFoco, x + teclaLargura + padFoco, y + teclaAltura + padFoco, raio, 
                       corDestaque.r, corDestaque.g, corDestaque.b, 255);
        // Desenha o corpo da tecla em estado hover
        roundedBoxRGBA(renderer, x, y, x + teclaLargura, y + teclaAltura, raio, 
                       tema.getCorBotaoHover().r, tema.getCorBotaoHover().g, tema.getCorBotaoHover().b, 255);
    } else {
        roundedBoxRGBA(renderer, x, y, x + teclaLargura, y + teclaAltura, raio, 
                       corTecla.r, corTecla.g, corTecla.b, 255);
    }
    
    desenharTexto(renderer, letra, x + offsetCharX, y + offsetCharY, corTexto, fonteTamanhoTecla, TipoFonte::NEGRITO);
}

/**
 * @brief Renderiza a linha de botões de funções especiais (Espaço, Limpar, Apagar, OK).
 * 
 * Organiza horizontalmente os botões funcionais na base do teclado, aplicando 
 * lógica de foco individual e estimativa de largura de caractere para centralização 
 * manual dos rótulos textuais.
 * 
 * @param renderer Ponteiro para o renderizador SDL_Renderer.
 */
void TecladoVirtual::desenharEspeciais(SDL_Renderer* renderer) {
    int y = baseY + 4 * (teclaAltura + gapY);
    int espacoX = baseX + offsetEspacoX;
    int yEsp = y;

    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color corTexto = tema.getCorTextoNegrito();
    SDL_Color corDestaque = tema.getCorDestaque();
    int raio = ConfigLayout::F(5);
    int padFoco = ConfigLayout::F(2);

    /**
     * @brief Lambda para facilitar a renderização recorrente de botões especiais.
     */
    auto desenharEsp = [&](int x, int w, const string& txt, int idxX, int r, int g, int b) {
        bool focado = (indiceY == 4 && indiceX == idxX);
        if (focado) {
            roundedBoxRGBA(renderer, x-padFoco, yEsp-padFoco, x+w+padFoco, yEsp+teclaAltura+padFoco, raio, 
                           corDestaque.r, corDestaque.g, corDestaque.b, 255);
        }
        roundedBoxRGBA(renderer, x, yEsp, x+w, yEsp+teclaAltura, raio, r, g, b, 255);
        
        int charWEstimate = fonteTamanhoEsp / 2; 
        int textX = x + (w/2) - (txt.length() * charWEstimate / 2); 
        desenharTexto(renderer, txt, textX, yEsp + offsetTextoY, {255,255,255,255}, fonteTamanhoEsp, TipoFonte::NEGRITO); 
    };

    // Renderização do botão ESPAÇO
    SDL_Color btnCor = tema.getCorBotaoNormal();
    bool focadoEspaco = (indiceY == 4 && indiceX == 0);
    if (focadoEspaco) {
        roundedBoxRGBA(renderer, espacoX-padFoco, y-padFoco, espacoX+larguraBotaoEsp+padFoco, y+teclaAltura+padFoco, raio, 
                       corDestaque.r, corDestaque.g, corDestaque.b, 255);
    }
    roundedBoxRGBA(renderer, espacoX, y, espacoX+larguraBotaoEsp, y+teclaAltura, raio, 
                   btnCor.r, btnCor.g, btnCor.b, 255);
    
    desenharTexto(renderer, "ESPACO", espacoX + ConfigLayout::F(40), y + offsetTextoY, corTexto, fonteTamanhoEsp, TipoFonte::NEGRITO);

    // Renderização dos demais botões com cores específicas de ação
    int limparX = espacoX + larguraBotaoEsp + gapBotoesEsp;
    desenharEsp(limparX, larguraBotaoEsp, "LIMPAR", 1, 120, 60, 60);

    int apagarX = limparX + larguraBotaoEsp + gapBotoesEsp;
    desenharEsp(apagarX, larguraBotaoEsp, "<--", 2, 150, 100, 60);

    int okX = apagarX + larguraBotaoEsp + gapBotoesEsp;
    desenharEsp(okX, larguraBotaoOk, "OK", 3, 60, 150, 60);
}

/**
 * @brief Executa a lógica de movimentação do foco na grade de teclas.
 * 
 * Controla os limites dos índices X e Y, garantindo que ao mudar de linha, o 
 * índice horizontal seja ajustado para o tamanho máximo da nova linha selecionada.
 * 
 * @param deltaX Deslocamento horizontal (-1, 0, 1).
 * @param deltaY Deslocamento vertical (-1, 0, 1).
 */
void TecladoVirtual::moverCursor(int deltaX, int deltaY) {
    if (deltaY != 0) {
        indiceY += deltaY;
        if (indiceY < 0) indiceY = 0;
        if (indiceY > 4) indiceY = 4;
        
        int maxX = 0;
        if (indiceY == 0) maxX = linha1.size();
        else if (indiceY == 1) maxX = linha2.size();
        else if (indiceY == 2) maxX = linha3.size();
        else if (indiceY == 3) maxX = linha4.size();
        else if (indiceY == 4) maxX = 4;
        
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
 * @brief Processa entradas de controles físicos (gamepad) para interação com o teclado.
 * 
 * Trata tanto eixos analógicos (aplicando deadzone e intervalos de tempo para evitar 
 * disparos múltiplos) quanto botões de D-Pad. O botão A seleciona o caractere e o 
 * botão B fecha o teclado.
 * 
 * @param evento Referência para o evento SDL.
 * @param alvo Ponteiro para o BotaoPesquisa que receberá os caracteres.
 * @return true se o evento de controle foi capturado e processado.
 */
bool TecladoVirtual::processarControle(SDL_Event& evento, BotaoPesquisa* alvo) {
    if (!visivel) return false;
    Uint32 tempoAtual = SDL_GetTicks();

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

    if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
        if (tempoAtual - ultimoInput < INTERVALO_INPUT) return true;
        
        switch (evento.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:    moverCursor(0, -1); break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  moverCursor(0, 1);  break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  moverCursor(-1, 0); break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: moverCursor(1, 0);  break;
            
            case SDL_CONTROLLER_BUTTON_A:
                if (indiceY < 4) {
                    string car;
                    if (indiceY == 0) car = linha1[indiceX];
                    else if (indiceY == 1) car = linha2[indiceX];
                    else if (indiceY == 2) car = linha3[indiceX];
                    else if (indiceY == 3) car = linha4[indiceX];
                    if (alvo) alvo->adicionarTexto(car);
                } else {
                    if (indiceX == 0) { if(alvo) alvo->adicionarTexto(" "); }
                    else if (indiceX == 1) { if(alvo) alvo->limparTexto(); }
                    else if (indiceX == 2) { if(alvo) alvo->apagarTexto(); }
                    else if (indiceX == 3){
                        if (alvo) {
                            alvo->resetarSolicitacaoTeclado();
                            
                            // CORREÇÃO PROBLEMA 3 (PARTE 1: FOCO)
                            // Em vez de resetar o foco (-1), tentamos navegar para o primeiro (0)
                            // Se a lista tiver resultados, isso seleciona o primeiro jogo.
                            alvo->resetarFocoResultados(); 
                            if (alvo->temResultadosVisiveis()) {
                                alvo->navegarResultados(1); // Move de -1 para 0
                            }
                            //-
                        }
                        fechar();
                    } 
                }
                break;
            
            case SDL_CONTROLLER_BUTTON_B:
                if (alvo) alvo->cancelarBusca(); 
                //-
                fechar();
                break;
        }
        ultimoInput = tempoAtual;
        return true;
    }
    return false; 
}

/**
 * @brief Processa cliques e interações do mouse com as teclas virtuais.
 * 
 * Verifica cliques fora do painel para fechamento automático, identifica qual tecla 
 * foi pressionada através de checagem de colisão 2D e envia o comando correspondente 
 * para o objeto alvo de pesquisa.
 * 
 * @param evento Referência para o evento SDL.
 * @param alvo Ponteiro para o BotaoPesquisa.
 * @return true se o evento de mouse foi consumido.
 */
bool TecladoVirtual::processarMouse(SDL_Event& evento, BotaoPesquisa* alvo) {
    if (!visivel) return false;

    // Fechar ao clicar fora da área do painel
    if (evento.type == SDL_MOUSEBUTTONDOWN || evento.type == SDL_MOUSEBUTTONUP) {
        int mx = evento.button.x;
        int my = evento.button.y;
        SDL_Rect areaTotal = {baseX - painelPadding, baseY - painelPadding, painelLargura, painelAltura};
        if (mx < areaTotal.x || mx > areaTotal.x + areaTotal.w || 
            my < areaTotal.y || my > areaTotal.y + areaTotal.h) {
            if (evento.type == SDL_MOUSEBUTTONUP) fechar();
            return true; 
        }
    }

    if (evento.type != SDL_MOUSEBUTTONUP) return true; 

    int mx = evento.button.x;
    int my = evento.button.y;
    int startY = baseY;

    /**
     * @brief Lambda para verificar colisão do mouse em uma linha específica.
     */
    auto verificarCliqueLinha = [&](const vector<string>& linha, int yIdx) {
        int larguraTotalLinha = (linha.size() * (teclaLargura + gapX)) - gapX;
        int startX = baseX + (painelLargura - (2 * painelPadding) - larguraTotalLinha) / 2;

        for (size_t i = 0; i < linha.size(); i++) {
            if (checarCliqueTecla(mx, my, startX + i * (teclaLargura + gapX), startY, teclaLargura, teclaAltura)) {
                if (alvo) alvo->adicionarTexto(linha[i]);
                indiceX = i; indiceY = yIdx; 
                return true;
            }
        }
        return false;
    };

    if (verificarCliqueLinha(linha1, 0)) return true;
    startY += teclaAltura + gapY;
    if (verificarCliqueLinha(linha2, 1)) return true;
    startY += teclaAltura + gapY;
    if (verificarCliqueLinha(linha3, 2)) return true;
    startY += teclaAltura + gapY;
    if (verificarCliqueLinha(linha4, 3)) return true;
    startY += teclaAltura + gapY;

    // Processamento de cliques nos botões especiais
    int espacoX = baseX + offsetEspacoX;
    int yEsp = startY;

    if (checarCliqueTecla(mx, my, espacoX, yEsp, larguraBotaoEsp, teclaAltura)) {
        if(alvo) alvo->adicionarTexto(" ");
        indiceX=0; indiceY=4; return true;
    }
    int limparX = espacoX + larguraBotaoEsp + gapBotoesEsp;
    if (checarCliqueTecla(mx, my, limparX, yEsp, larguraBotaoEsp, teclaAltura)) {
        if(alvo) alvo->limparTexto();
        indiceX=1; indiceY=4; return true;
    }
    int apagarX = limparX + larguraBotaoEsp + gapBotoesEsp;
    if (checarCliqueTecla(mx, my, apagarX, yEsp, larguraBotaoEsp, teclaAltura)) {
        if(alvo) alvo->apagarTexto();
        indiceX=2; indiceY=4; return true;
    }
    int okX = apagarX + larguraBotaoEsp + gapBotoesEsp;
    if (checarCliqueTecla(mx, my, okX, yEsp, larguraBotaoOk, teclaAltura)) {
        if (alvo) {
        alvo->resetarSolicitacaoTeclado();
        alvo->resetarFocoResultados();
        }   
        fechar();
        return true;
    }
    return true; 
}

/**
 * @brief Utilitário para detecção de ponto dentro de retângulo (AABB).
 * 
 * @param mouseX Coordenada X do mouse.
 * @param mouseY Coordenada Y do mouse.
 * @param x Coordenada X da tecla.
 * @param y Coordenada Y da tecla.
 * @param w Largura da tecla.
 * @param h Altura da tecla.
 * @return true se o ponto (mouseX, mouseY) estiver contido na área delimitada.
 */
bool TecladoVirtual::checarCliqueTecla(int mouseX, int mouseY, int x, int y, int w, int h) {
    return (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h);
}