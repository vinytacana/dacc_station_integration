#include "BotaoPesquisa.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorJogos.hpp"
#include "GerenciadorImagens.hpp" 
#include "Utils.hpp" 
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <SDL2/SDL2_gfxPrimitives.h> 

using namespace MeuProjeto;

extern MeuProjeto::GerenciadorJogos gerenciadorJogos;
extern MeuProjeto::GerenciadorImagens gerImg; 

/**
 * @brief Construtor do BotaoPesquisa.
 *
 * @param renderer O renderizador SDL.
 * @param x Coordenada X.
 * @param y Coordenada Y.
 * @param largura Largura do campo.
 * @param altura Altura do campo.
 * @param texto Texto placeholder.
 */
BotaoPesquisa::BotaoPesquisa(SDL_Renderer* renderer, int x, int y, int largura, int altura, const std::string& texto)
    : Botao(x, y, largura, altura, texto), renderer(renderer), inputAtivo(false), resultadoSelecionado(-1) {
    if (renderer == nullptr) {
        return;
    }
    // Não chama SDL_StartTextInput() aqui para evitar conflito na inicialização do programa
    // Ele é chamado no clique, mas mantido aqui por enquanto.
    // SDL_StartTextInput();
}

/**
 * @brief Destrutor. Para a entrada de texto do SDL.
 */
BotaoPesquisa::~BotaoPesquisa() {
    SDL_StopTextInput();
}

/**
 * @brief Trata eventos de mouse, teclado e entrada de texto.
 *
 * @param evento A estrutura SDL_Event.
 * @param offsetX Deslocamento X.
 * @param offsetY Deslocamento Y.
 * @return Retorna @c true se o evento foi consumido.
 */
bool BotaoPesquisa::tratarEvento(SDL_Event& evento, int offsetX, int offsetY) {
    bool consumido = false;

    // 1. Lidar com movimento do mouse para hover
    if (evento.type == SDL_MOUSEMOTION) {
        int mouseX = evento.motion.x - offsetX;
        int mouseY = evento.motion.y - offsetY;
        
        bool dentroBarraPrincipal = contemPonto(mouseX, mouseY);
        
        bool dentroResultados = false;
        int novoResultadoSelecionado = -1;
        
        if (inputAtivo && !textoInput.empty()) {
            std::vector<Jogo> resultados = gerenciadorJogos.buscarPorNome(textoInput);
            int yPos = area.y + area.h + 10;
            
            for (size_t i = 0; i < resultados.size() && i < MAX_RESULTADOS; ++i) {
                SDL_Rect nomeRect = {area.x, yPos, area.w, ITEM_HEIGHT};
                
                if (mouseX >= nomeRect.x && mouseX <= nomeRect.x + nomeRect.w &&
                    mouseY >= nomeRect.y && mouseY <= nomeRect.y + nomeRect.h)
                {
                    dentroResultados = true;
                    novoResultadoSelecionado = static_cast<int>(i);
                    break;
                }
                yPos += ITEM_HEIGHT + ITEM_PADDING;
            }
        }
        
        resultadoSelecionado = novoResultadoSelecionado;
        hover = dentroBarraPrincipal;
    }

    // 2. Lidar com mouse button down
    if (evento.type == SDL_MOUSEBUTTONDOWN && evento.button.button == SDL_BUTTON_LEFT) {
        int mouseX = evento.button.x - offsetX;
        int mouseY = evento.button.y - offsetY;
        
        if (contemPonto(mouseX, mouseY)) {
            pressionado = true;
            consumido = true;
        }
    }

    // 3. Lidar com cliques (mouseup)
    if (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT) {
        int mouseX = evento.button.x - offsetX;
        int mouseY = evento.button.y - offsetY;

        if (contemPonto(mouseX, mouseY) && pressionado) {
            // Clique no campo de input principal
            inputAtivo = true;
            SDL_StartTextInput();
            consumido = true;
        } 
        else if (inputAtivo) {
            // Clique fora ou clique em um resultado
            std::vector<Jogo> resultados = gerenciadorJogos.buscarPorNome(textoInput);
            int yPos = area.y + area.h + 10;

            for (size_t i = 0; i < resultados.size() && i < MAX_RESULTADOS; ++i) {
                SDL_Rect nomeRect = {area.x, yPos, area.w, ITEM_HEIGHT};
                
                if (mouseX >= nomeRect.x && mouseX <= nomeRect.x + nomeRect.w &&
                    mouseY >= nomeRect.y && mouseY <= nomeRect.y + nomeRect.h)
                {
                    // Clicou em um resultado
                    std::string codigo = resultados[i].getCodigo();
                    jogoClicado = gerenciadorJogos.buscarPorCodigo(codigo);
                    
                    inputAtivo = false;
                    SDL_StopTextInput();
                    textoInput = ""; // Limpa a busca
                    consumido = true;
                    break;
                }
                yPos += ITEM_HEIGHT + ITEM_PADDING;
            }
            
            if (!consumido) {
                // Clicou fora da barra e dos resultados
                inputAtivo = false;
                SDL_StopTextInput();
            }
        }
        
        pressionado = false;
    }

    // 4. Entrada de texto
    if (inputAtivo) {
        if (evento.type == SDL_TEXTINPUT) {
            textoInput += evento.text.text;
            consumido = true;
        } else if (evento.type == SDL_KEYDOWN) {
            if (evento.key.keysym.sym == SDLK_BACKSPACE && textoInput.length() > 0) {
                textoInput.pop_back();
                consumido = true;
            } else if (evento.key.keysym.sym == SDLK_RETURN) {
                // Fechar input ao pressionar Enter
                inputAtivo = false;
                SDL_StopTextInput();
                consumido = true;
            } else if (evento.key.keysym.sym == SDLK_ESCAPE) {
                // Fechar e limpar input ao pressionar ESC
                inputAtivo = false;
                SDL_StopTextInput();
                textoInput = "";
                consumido = true;
            }
        }
    }

    return consumido;
}

/**
 * @brief Desenha o campo de input e os resultados de pesquisa.
 *
 * @param renderer O renderizador SDL.
 * @param offsetX Deslocamento X.
 * @param offsetY Deslocamento Y.
 */
void BotaoPesquisa::desenhar(SDL_Renderer* renderer, int offsetX, int offsetY) {
    if (!renderer) return;

    SDL_Rect adjustedArea = {area.x + offsetX, area.y + offsetY, area.w, area.h};
    
    SDL_Color corAtual;
    if (pressionado) {
        corAtual = corPressionado;
    } else if (hover || inputAtivo) { // Hover ou Ativo
        corAtual = corHover;
    } else {
        corAtual = corPadrao;
    }
    
    // Desenha o campo de input arredondado
    SDL_SetRenderDrawColor(renderer, corAtual.r, corAtual.g, corAtual.b, corAtual.a);

    roundedBoxRGBA(renderer, adjustedArea.x, adjustedArea.y, 
                   adjustedArea.x + adjustedArea.w, adjustedArea.y + adjustedArea.h, 
                   15, corAtual.r, corAtual.g, corAtual.b, corAtual.a);

    std::string textoParaDesenhar;
    SDL_Color corTexto = {180, 180, 180, 255};

    if (inputAtivo) {
        // Exibe o texto digitado + cursor
        textoParaDesenhar = textoInput.empty() ? "" : textoInput;
        corTexto = {255, 255, 255, 255};
        if (SDL_GetTicks() % 1000 < 500) {
            textoParaDesenhar += "|"; // Cursor piscando
        }
    } else if (!textoInput.empty()) {
        // Exibe o último texto digitado (se não estiver ativo)
        textoParaDesenhar = textoInput;
        corTexto = {220, 220, 220, 255};
    } else {
        // Exibe o placeholder
        textoParaDesenhar = texto;
        corTexto = {150, 150, 150, 255};
    }

    // Renderiza o texto no campo de input
    if (!textoParaDesenhar.empty()) {
        int textY = adjustedArea.y + (adjustedArea.h / 2) - (tamanhoPlaceholder / 2);
        MeuProjeto::desenharTexto(renderer, textoParaDesenhar, adjustedArea.x + 20, textY, 
                                 corTexto, tamanhoPlaceholder, fontePlaceholder);
    }

    // Desenha os resultados se o input estiver ativo e houver texto
    if (inputAtivo && !textoInput.empty()) {
        desenharResultados(adjustedArea);
    }
}

/**
 * @brief Desenha a lista de resultados de pesquisa abaixo do campo de input.
 *
 * @param adjustedArea A área ajustada do campo de input principal.
 */
void BotaoPesquisa::desenharResultados(const SDL_Rect& adjustedArea) {
    std::vector<Jogo> resultados = gerenciadorJogos.buscarPorNome(textoInput);
    
    if (resultados.empty()) {
        int yPos = adjustedArea.y + adjustedArea.h + 10;
        SDL_Rect msgRect = {adjustedArea.x, yPos, adjustedArea.w, 80};
        
        // Fundo da mensagem de "Sem resultados"
        SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
        SDL_RenderFillRect(renderer, &msgRect);
        
        // Texto da mensagem
        int textY = yPos + (80 / 2) - (tamanhoResultados / 2);
        MeuProjeto::desenharTexto(renderer, "Sem resultados para a busca", 
                                 msgRect.x + 20, textY, 
                                 {180, 180, 180, 255}, tamanhoResultados, fonteResultados);
        return;
    }
    
    int yPos = adjustedArea.y + adjustedArea.h + 10;

    // Itera sobre o máximo de resultados
    for (size_t i = 0; i < resultados.size() && i < MAX_RESULTADOS; ++i) {
        const std::string& nome = resultados[i].getNome();
        
        std::string caminho;
        // Pega a primeira imagem da lista de imagens do jogo (capa)
        if (!resultados[i].getImagens().empty()) {
            caminho = resultados[i].getImagens()[0]; 
        }

        SDL_Rect nomeRect = {adjustedArea.x, yPos, adjustedArea.w, ITEM_HEIGHT};
        
        // Determina a cor de fundo do item (hover/selecionado)
        SDL_Color corFundoRes = (static_cast<int>(i) == resultadoSelecionado) ? 
                                SDL_Color{70, 70, 80, 255} : 
                                SDL_Color{40, 40, 50, 255};
        SDL_SetRenderDrawColor(renderer, corFundoRes.r, corFundoRes.g, corFundoRes.b, corFundoRes.a);
        SDL_RenderFillRect(renderer, &nomeRect);
        
        // Desenha a imagem de capa
        if (!caminho.empty()) {
            SDL_Texture* img = gerImg.carregar(renderer, caminho.c_str());
            if (img) {
                SDL_Rect imgRect = {nomeRect.x + IMG_OFFSET_X, yPos + IMG_OFFSET_Y, IMG_SIZE, IMG_SIZE}; 
                SDL_RenderCopy(renderer, img, nullptr, &imgRect);
            }
        }

        // Desenha o nome do jogo
        int textY = yPos + (ITEM_HEIGHT / 2) - (tamanhoResultados / 2);
        MeuProjeto::desenharTexto(renderer, nome, nomeRect.x + TEXT_OFFSET_X, textY, 
                                 {255, 255, 255, 255}, tamanhoResultados, fonteResultados);
        
        yPos += ITEM_HEIGHT + ITEM_PADDING;
    }
}

/**
 * @brief Conta quantos resultados de jogo correspondem ao input atual.
 *
 * @param input O texto de busca.
 * @return O número de resultados.
 */
int BotaoPesquisa::contarResultados(const std::string& input) {
    return static_cast<int>(gerenciadorJogos.buscarPorNome(input).size());
}

/**
 * @brief Função de tratamento de clique não utilizada (lógica movida para tratarEvento).
 */
bool BotaoPesquisa::handleMouseClick(SDL_Event& evento, int offsetX, int offsetY) { return false; }

/**
 * @brief Função de tratamento de input de texto não utilizada (lógica movida para tratarEvento).
 */
void BotaoPesquisa::handleTextInput(SDL_Event& evento) {}

/**
 * @brief Função de tratamento de tecla pressionada não utilizada (lógica movida para tratarEvento).
 */
bool BotaoPesquisa::handleKeyDown(SDL_Event& evento) { return false; }