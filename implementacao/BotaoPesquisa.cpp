/**
 * @file BotaoPesquisa.cpp
 * @brief Implementação da barra de pesquisa com sugestões em tempo real.
 * 
 * Este arquivo gerencia a lógica de entrada de texto (via teclado físico ou virtual),
 * a filtragem dinâmica da biblioteca de jogos e a interação com a lista de resultados.
 */

#include "BotaoPesquisa.hpp"
#include "GerenciadorJogos.hpp"
#include "GerenciadorImagens.hpp" 
#include "Utils.hpp" 
#include "GerenciadorTemas.hpp" 
#include <SDL2/SDL2_gfxPrimitives.h> 
#include <iostream>

using namespace MeuProjeto;

/** @brief Referência ao gerenciador de dados para realizar as buscas. */
extern MeuProjeto::GerenciadorJogos gerenciadorJogos;
/** @brief Referência ao gerenciador de texturas para carregar as capas nos resultados. */
extern MeuProjeto::GerenciadorImagens gerImg; 

/**
 * @brief Construtor especializado.
 * 
 * Além de inicializar o botão base, prepara o subsistema da SDL para receber 
 * eventos de texto.
 */
BotaoPesquisa::BotaoPesquisa(SDL_Renderer* renderer, int x, int y, int largura, int altura, const std::string& texto)
    : Botao(x, y, largura, altura, texto), renderer(renderer), inputAtivo(false), indiceFocoResultado(-1), jogoClicado(nullptr) {
    if (renderer) SDL_StartTextInput(); // Habilita o buffer de texto da SDL
}

/**
 * @brief Destrutor.
 * Finaliza a captura de eventos de texto para economizar processamento.
 */
BotaoPesquisa::~BotaoPesquisa() { 
    SDL_StopTextInput(); 
}

/**
 * @brief Sincroniza a lista de resultados com o texto digitado.
 * 
 * Sempre que o texto muda, o `GerenciadorJogos` é consultado. Se o campo estiver
 * vazio, a lista de resultados é limpa imediatamente.
 */
void BotaoPesquisa::atualizarResultados() { 
    if (textoInput.empty()) resultadosAtuais.clear(); 
    else resultadosAtuais = gerenciadorJogos.buscarPorNome(textoInput); 
}

// Manipulação de String

void BotaoPesquisa::adicionarTexto(const std::string& txt) { 
    textoInput += txt; 
    inputAtivo = true; 
    atualizarResultados(); 
    resetarFocoResultados(); 
}

void BotaoPesquisa::apagarTexto() { 
    if (!textoInput.empty()) { 
        textoInput.pop_back(); 
        atualizarResultados(); 
    } 
    resetarFocoResultados(); 
}

void BotaoPesquisa::limparTexto() { 
    textoInput.clear(); 
    resultadosAtuais.clear(); 
    resetarFocoResultados(); 
    inputAtivo = true; 
    jogoClicado = nullptr; 
}

/**
 * @brief Lógica de navegação vertical para controles (Gamepads).
 * 
 * @details **Como funciona a ação de navegação:**
 * 1. O sistema verifica se há resultados.
 * 2. `direcao > 0` (D-Pad para baixo): Move o foco para o próximo item da lista.
 * 3. `direcao < 0` (D-Pad para cima): Move o foco para o item anterior.
 * 4. Se o usuário subir além do primeiro item, o foco volta para a barra de pesquisa (`indiceFocoResultado = -1`).
 * 
 * @param direcao Valor positivo para descer, negativo para subir.
 * @return true se o movimento foi válido dentro dos limites.
 */
bool BotaoPesquisa::navegarResultados(int direcao) {
    if (resultadosAtuais.empty()) return false;
    
    int maxIndex = std::min((int)resultadosAtuais.size(), MAX_RESULTADOS) - 1;
    
    // Entrar na lista vindo da barra
    if (indiceFocoResultado == -1 && direcao > 0) { 
        indiceFocoResultado = 0; 
        return true; 
    }
    
    // Sair da lista voltando para a barra
    if (indiceFocoResultado == 0 && direcao < 0) { 
        indiceFocoResultado = -1; 
        return false; 
    }
    
    int novoIndice = indiceFocoResultado + direcao;
    if (novoIndice >= 0 && novoIndice <= maxIndex) { 
        indiceFocoResultado = novoIndice; 
        return true; 
    }
    return false;
}

bool BotaoPesquisa::temResultadoFocado() const { return indiceFocoResultado != -1; }

Jogo* BotaoPesquisa::getJogoResultadoFocado() { 
    if (indiceFocoResultado >= 0 && indiceFocoResultado < (int)resultadosAtuais.size()) 
        return &resultadosAtuais[indiceFocoResultado]; 
    return nullptr; 
}

void BotaoPesquisa::resetarFocoResultados() { indiceFocoResultado = -1; }

/**
 * @brief Processamento de Eventos da Barra de Pesquisa.
 * 
 * @details **Tratamento de Ações:**
 * - **Movimento do Mouse:** Calcula se o cursor está sobre a barra ou sobre 
 *   cada um dos itens da lista de resultados suspensa.
 * - **Clique do Mouse (Barra):** Ativa o foco de escrita e solicita o teclado virtual.
 * - **Clique do Mouse (Resultado):** Define `jogoClicado`. O loop principal do 
 *   programa lerá este ponteiro para abrir a página do jogo.
 * - **Teclado:** Intercepta `SDL_TEXTINPUT` para caracteres e `SDL_KEYDOWN` para 
 *   teclas de comando (Backspace, Enter, Esc).
 */
bool BotaoPesquisa::tratarEvento(SDL_Event& evento, int offsetX, int offsetY) {
    bool consumido = false;

    // 1. Detecção de Mouse Hover
    if (evento.type == SDL_MOUSEMOTION) {
        int mx = evento.motion.x - offsetX, my = evento.motion.y - offsetY;
        hover = contemPonto(mx, my);
        resultadoHoverIndex = -1;

        if (!resultadosAtuais.empty()) {
            int yPos = area.y + area.h + 10;
            for (size_t i = 0; i < resultadosAtuais.size() && i < MAX_RESULTADOS; ++i) {
                SDL_Rect nr = {area.x, yPos, area.w, ITEM_HEIGHT};
                if (mx >= nr.x && mx <= nr.x + nr.w && my >= nr.y && my <= nr.y + nr.h) {
                    resultadoHoverIndex = (int)i;
                }
                yPos += ITEM_HEIGHT + ITEM_PADDING;
            }
        }
    }

    // 2. Detecção de Cliques (Execução de Ações)
    if (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT) {
        int mx = evento.button.x - offsetX, my = evento.button.y - offsetY;
        
        // Clicou na barra: Foca e pede teclado
        if (contemPonto(mx, my)) {
            inputAtivo = true; 
            SDL_StartTextInput(); 
            consumido = true; 
            solicitaTeclado = true; // Sinaliza para o main abrir o TecladoVirtual
        } 
        // Clicou em um dos resultados da lista
        else if (!resultadosAtuais.empty()) {
             int yPos = area.y + area.h + 10;
             for (size_t i = 0; i < resultadosAtuais.size() && i < MAX_RESULTADOS; ++i) {
                SDL_Rect nr = {area.x, yPos, area.w, ITEM_HEIGHT};
                if (mx >= nr.x && mx <= nr.x + nr.w && my >= nr.y && my <= nr.y + nr.h) {
                    // AÇÃO: Seleciona o jogo para visualização
                    jogoClicado = &resultadosAtuais[i]; 
                    inputAtivo = false; 
                    SDL_StopTextInput(); 
                    consumido = true; 
                    break;
                }
                yPos += ITEM_HEIGHT + ITEM_PADDING;
             }
        }
    }

    // 3. Entrada de Texto e Teclas Especiais
    if (inputAtivo) {
        if (evento.type == SDL_TEXTINPUT) { 
            adicionarTexto(evento.text.text); 
            consumido = true; 
        }
        else if (evento.type == SDL_KEYDOWN) {
            if (evento.key.keysym.sym == SDLK_BACKSPACE) { 
                apagarTexto(); 
                consumido = true; 
            }
            else if (evento.key.keysym.sym == SDLK_RETURN || evento.key.keysym.sym == SDLK_ESCAPE) { 
                inputAtivo = false; 
                SDL_StopTextInput(); 
                consumido = true; 
            }
        }
    }
    return consumido;
}

/**
 * @brief Renderização da Barra de Pesquisa.
 * 
 * @details **Lógica Visual:**
 * - Aplica cores dinâmicas baseadas no tema (Claro/Escuro).
 * - **Cursor Piscante:** Utiliza `SDL_GetTicks() % 1000 < 500` para alternar a 
 *   exibição do caractere `|` a cada meio segundo.
 * - **Placeholder:** Se não houver texto, exibe o rótulo padrão em cor suave.
 */
void BotaoPesquisa::desenhar(SDL_Renderer* renderer, int offsetX, int offsetY) {
    if (!renderer) return;

    SDL_Rect adjustedArea = {area.x + offsetX, area.y + offsetY, area.w, area.h};
    auto& tema = GerenciadorTemas::getInstance();
    
    // Definição da paleta reativa
    SDL_Color corBase = tema.getCorBotaoNormal(); 
    SDL_Color corHover = tema.getCorBotaoHover();
    
    SDL_Color corAtual;
    if (pressionado) corAtual = tema.getCorBotaoPressionado();
    else if (hover || (focadoPorControle && indiceFocoResultado == -1)) corAtual = corHover;
    else corAtual = corBase;
    
    // Desenha o corpo da barra (Arredondado via SDL2_gfx)
    SDL_SetRenderDrawColor(renderer, corAtual.r, corAtual.g, corAtual.b, corAtual.a);
    roundedBoxRGBA(renderer, adjustedArea.x, adjustedArea.y, adjustedArea.x + adjustedArea.w, adjustedArea.y + adjustedArea.h, 15, corAtual.r, corAtual.g, corAtual.b, corAtual.a);

    // Borda de foco para navegação por Gamepad
    if (focadoPorControle && indiceFocoResultado == -1) {
        SDL_Color corDest = tema.getCorDestaque();
        roundedRectangleRGBA(renderer, adjustedArea.x - 3, adjustedArea.y - 3, adjustedArea.x + adjustedArea.w + 3, adjustedArea.y + adjustedArea.h + 3, 18, corDest.r, corDest.g, corDest.b, 255);
    }

    // Processamento do texto exibido
    std::string textoParaDesenhar;
    SDL_Color corTexto = tema.getCorTextoNormal(); 

    if (inputAtivo) {
        textoParaDesenhar = textoInput;
        corTexto = tema.getCorTextoNegrito();
        // Efeito de Cursor Piscante
        if (SDL_GetTicks() % 1000 < 500) textoParaDesenhar += "|";
    } else if (!textoInput.empty()) {
        textoParaDesenhar = textoInput;
        corTexto = tema.getCorTextoNegrito();
    } else {
        textoParaDesenhar = texto; // Exibe o Placeholder (ex: "Pesquisar...")
    }

    if (!textoParaDesenhar.empty()) {
        int textY = adjustedArea.y + (adjustedArea.h / 2) - (tamanhoPlaceholder / 2);
        MeuProjeto::desenharTexto(renderer, textoParaDesenhar, adjustedArea.x + 20, textY, corTexto, tamanhoPlaceholder, fontePlaceholder);
    }

    // Renderiza a lista suspensa se houver jogos condizentes com a busca
    if (!resultadosAtuais.empty()) {
        desenharResultados(adjustedArea);
    }
}

/**
 * @brief Renderiza a lista de resultados (dropdown) abaixo da barra.
 * 
 * @details **Organização dos Itens:**
 * 1. Calcula a posição vertical `yPos` com um respiro de 10px abaixo da barra.
 * 2. Para cada jogo (até o limite `MAX_RESULTADOS`):
 *    - Desenha o fundo do item (destacado se focado por mouse ou controle).
 *    - Carrega e desenha a miniatura da capa do jogo.
 *    - Desenha o nome do jogo centralizado verticalmente em relação ao item.
 *    - Incrementa `yPos` para o próximo item considerando o `ITEM_PADDING`.
 */
void BotaoPesquisa::desenharResultados(const SDL_Rect& adjustedArea) {
    int yPos = adjustedArea.y + adjustedArea.h + 10;
    auto& tema = GerenciadorTemas::getInstance();

    for (size_t i = 0; i < resultadosAtuais.size() && i < MAX_RESULTADOS; ++i) {
        SDL_Rect nomeRect = {adjustedArea.x, yPos, adjustedArea.w, ITEM_HEIGHT};
        
        bool isFocado = ((int)i == indiceFocoResultado) || ((int)i == resultadoHoverIndex);
        
        // Estilização do fundo do resultado
        SDL_Color corFundo = isFocado ? tema.getCorBotaoHover() : tema.getCorRetangulos();
        
        SDL_SetRenderDrawColor(renderer, corFundo.r, corFundo.g, corFundo.b, corFundo.a);
        SDL_RenderFillRect(renderer, &nomeRect);
        
        // Borda interna de realce para navegação por controle
        if ((int)i == indiceFocoResultado && focadoPorControle) {
            SDL_Color corDest = tema.getCorDestaque();
            roundedRectangleRGBA(renderer, nomeRect.x + 2, nomeRect.y + 2, nomeRect.x + nomeRect.w - 2, nomeRect.y + nomeRect.h - 2, 5, corDest.r, corDest.g, corDest.b, 255);
        }

        // Desenho da Miniatura (Capa)
        const auto& jogo = resultadosAtuais[i];
        std::string caminho = jogo.getCapaTelaPrincipal();
        
        if (!caminho.empty()) {
            SDL_Texture* img = gerImg.carregar(renderer, caminho.c_str());
            if (img) {
                // Posicionamento da imagem com base nos offsets definidos no header
                SDL_Rect imgRect = {nomeRect.x + IMG_OFFSET_X, yPos + IMG_OFFSET_Y, IMG_SIZE, IMG_SIZE}; 
                SDL_RenderCopy(renderer, img, nullptr, &imgRect);
            }
        }

        // Desenho do Nome do Jogo
        int textY = yPos + (ITEM_HEIGHT / 2) - (tamanhoResultados / 2);
        MeuProjeto::desenharTexto(renderer, jogo.getNome(), nomeRect.x + TEXT_OFFSET_X, textY, tema.getCorTextoNegrito(), tamanhoResultados, fonteResultados);
        
        // Salto para a próxima linha de resultado
        yPos += ITEM_HEIGHT + ITEM_PADDING;
    }
}