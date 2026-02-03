/**
 * @file BotaoPesquisa.cpp
 * @brief Implementação Responsiva do componente BotaoPesquisa.
 *
 * Este arquivo contém a lógica operacional do componente de pesquisa, incluindo
 * o tratamento de entrada de texto via SDL_TextInput, a filtragem dinâmica de 
 * títulos através do GerenciadorJogos e a renderização de uma interface 
 * de resultados suspensa (dropdown) com suporte a temas e layout escalável.
 */

#include "BotaoPesquisa.hpp"
#include "GerenciadorJogos.hpp"
#include "GerenciadorImagens.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include <SDL2/SDL2_gfxPrimitives.h> 
#include <iostream>

using namespace MeuProjeto;

/** * @brief Instâncias externas para acesso aos dados de jogos e recursos de imagem. 
 */
extern MeuProjeto::GerenciadorJogos gerenciadorJogos;
extern MeuProjeto::GerenciadorImagens gerImg; 

/**
 * @brief Construtor da classe BotaoPesquisa.
 * Inicializa o componente herdando de Botao e configura as variáveis de layout 
 * responsivo. Utiliza a classe ConfigLayout para converter valores base em 
 * coordenadas e dimensões proporcionais à resolução atual. Ativa o subsistema 
 * de entrada de texto do SDL para captura de caracteres.
 * @param renderer Ponteiro para o renderizador SDL.
 * @param x Coordenada horizontal base.
 * @param y Coordenada vertical base.
 * @param largura Largura base do componente.
 * @param altura Altura base do componente.
 * @param texto Texto de placeholder exibido quando o campo está vazio.
 */
BotaoPesquisa::BotaoPesquisa(SDL_Renderer* renderer, int x, int y, int largura, int altura, const std::string& texto)
    : Botao(x, y, largura, altura, texto), 
      renderer(renderer), 
      inputAtivo(false), 
      indiceFocoResultado(-1), 
      jogoClicado(nullptr) 
{
    if (renderer) SDL_StartTextInput();

    // Inicialização de Variáveis de Layout Responsivo
    
    // Configuração das fontes escaladas
    tamanhoPlaceholder = ConfigLayout::F(28);
    tamanhoResultados = ConfigLayout::F(32);

    // Definição das dimensões da lista de resultados suspensa
    itemHeight = ConfigLayout::Y(120);
    itemPadding = ConfigLayout::Y(6);
    imgSize = ConfigLayout::F(90); 
    imgOffsetX = ConfigLayout::X(15);
    
    // Cálculo de centralização vertical da imagem dentro do item de resultado
    imgOffsetY = (itemHeight - imgSize) / 2;
    
    // Posicionamento do texto lateralmente em relação à imagem
    textOffsetX = imgOffsetX + imgSize + ConfigLayout::X(20);

    // Parâmetros estéticos de borda
    raioBorda = ConfigLayout::F(15);
    raioDestaque = ConfigLayout::F(18);
}

/**
 * @brief Destrutor da classe BotaoPesquisa.
 * Finaliza o subsistema de entrada de texto do SDL para liberar o foco do sistema operacional.
 */
BotaoPesquisa::~BotaoPesquisa() { 
    SDL_StopTextInput(); 
}

/**
 * @brief Atualiza o vetor de resultados baseando-se no conteúdo atual do input.
 * Realiza uma consulta ao GerenciadorJogos para filtrar títulos que correspondam
 * à string de pesquisa. Se o campo estiver vazio, a lista de resultados é limpa.
 */
void BotaoPesquisa::atualizarResultados() { 
    if (textoInput.empty()) resultadosAtuais.clear(); 
    else resultadosAtuais = gerenciadorJogos.buscarPorNome(textoInput); 
}

/**
 * @brief Adiciona texto ao buffer de entrada e atualiza a busca.
 * @param txt String contendo os novos caracteres a serem inseridos.
 */
void BotaoPesquisa::adicionarTexto(const std::string& txt) { 
    textoInput += txt; 
    inputAtivo = true; 
    atualizarResultados(); 
    resetarFocoResultados(); 
}

/**
 * @brief Remove o último caractere do buffer de entrada.
 * Aciona a re-filtragem dos resultados após a remoção do caractere.
 */
void BotaoPesquisa::apagarTexto() { 
    if (!textoInput.empty()) { 
        textoInput.pop_back(); 
        atualizarResultados(); 
    } 
    resetarFocoResultados(); 
}

/**
 * @brief Limpa integralmente o estado de pesquisa.
 * Reseta o texto, os resultados, o foco e o cache de renderização de texto.
 */
void BotaoPesquisa::limparTexto() { 
    textoInput.clear(); 
    resultadosAtuais.clear(); 
    resetarFocoResultados(); 
    inputAtivo = true; 
    jogoClicado = nullptr; 
    limparCacheTexto();
}

void BotaoPesquisa::cancelarBusca() {
    this->inputAtivo = false;         
    this->solicitaTeclado = false;
    this->resultadosAtuais.clear();   
    this->indiceFocoResultado = -1;   
    this->jogoClicado = nullptr; 
    SDL_StopTextInput();
}

/**
 * @brief Gerencia a navegação por índices na lista de resultados.
 * @param direcao Valor positivo para avançar ou negativo para retroceder.
 * @return true se a navegação resultou em uma mudança de foco válida.
 */
bool BotaoPesquisa::navegarResultados(int direcao) {
    if (resultadosAtuais.empty()) return false;
    
    int maxIndex = std::min((int)resultadosAtuais.size(), MAX_RESULTADOS) - 1;
    
    if (indiceFocoResultado == -1 && direcao > 0) { 
        indiceFocoResultado = 0; 
        return true; 
    }
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

/**
 * @brief Verifica se há um item da lista de resultados com foco.
 * @return true se houver foco ativo em algum resultado.
 */
bool BotaoPesquisa::temResultadoFocado() const { return indiceFocoResultado != -1; }

/**
 * @brief Obtém o jogo que detém o foco atual na lista de resultados.
 * @return Ponteiro para o objeto Jogo focado, ou nullptr se inexistente.
 */
Jogo* BotaoPesquisa::getJogoResultadoFocado() { 
    if (indiceFocoResultado >= 0 && indiceFocoResultado < (int)resultadosAtuais.size()) 
        return &resultadosAtuais[indiceFocoResultado]; 
    return nullptr; 
}

/**
 * @brief Reseta o índice de navegação dos resultados.
 */
void BotaoPesquisa::resetarFocoResultados() { indiceFocoResultado = -1; }

/**
 * @brief Processa eventos de interação (mouse e teclado) específicos do componente.
 * Lida com o rastreamento do mouse para hover nos resultados, ativação do modo 
 * de inserção de texto e captura de eventos físicos de teclado para edição do buffer.
 * @param evento Referência ao evento SDL capturado.
 * @param offsetX Deslocamento horizontal global.
 * @param offsetY Deslocamento vertical global.
 * @return true se o evento foi consumido pelo componente.
 */
bool BotaoPesquisa::tratarEvento(SDL_Event& evento, int offsetX, int offsetY) {
    bool consumido = false;

    // Processamento de movimento do mouse e hover dinâmico na lista de resultados
    if (evento.type == SDL_MOUSEMOTION) {
        int mx = evento.motion.x - offsetX, my = evento.motion.y - offsetY;
        hover = contemPonto(mx, my);
        resultadoHoverIndex = -1;

        if (!resultadosAtuais.empty()) {
            int yPos = area.y + area.h + ConfigLayout::Y(10); 
            for (size_t i = 0; i < resultadosAtuais.size() && i < MAX_RESULTADOS; ++i) {
                SDL_Rect nr = {area.x, yPos, area.w, itemHeight};
                if (mx >= nr.x && mx <= nr.x + nr.w && my >= nr.y && my <= nr.y + nr.h) {
                    resultadoHoverIndex = (int)i;
                }
                yPos += itemHeight + itemPadding;
            }
        }
    }

    // Processamento de cliques para ativação de input ou seleção de jogo
    if (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT) {
        int mx = evento.button.x - offsetX, my = evento.button.y - offsetY;
        
        if (contemPonto(mx, my)) {
            inputAtivo = true; 
            SDL_StartTextInput(); 
            consumido = true; 
            solicitaTeclado = true; 
        } 
        else if (!resultadosAtuais.empty()) {
             int yPos = area.y + area.h + ConfigLayout::Y(10);
             for (size_t i = 0; i < resultadosAtuais.size() && i < MAX_RESULTADOS; ++i) {
                SDL_Rect nr = {area.x, yPos, area.w, itemHeight};
                if (mx >= nr.x && mx <= nr.x + nr.w && my >= nr.y && my <= nr.y + nr.h) {
                    jogoClicado = &resultadosAtuais[i]; 
                    inputAtivo = false; 
                    SDL_StopTextInput(); 
                    consumido = true; 
                    break;
                }
                yPos += itemHeight + itemPadding;
             }
        }
    }

    // Tratamento de entrada de texto e teclas de controle de edição
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
 * @brief Renderiza a barra de pesquisa e gerencia o estado visual (foco/hover).
 * Desenha a caixa de entrada arredondada, o retângulo de destaque de foco por 
 * controle e o cursor intermitente (caret) quando o input está ativo.
 * @param renderer Ponteiro para o renderizador SDL.
 * @param offsetX Deslocamento horizontal.
 * @param offsetY Deslocamento vertical.
 */
void BotaoPesquisa::desenhar(SDL_Renderer* renderer, int offsetX, int offsetY) {
    if (!renderer) return;

    SDL_Rect adjustedArea = {area.x + offsetX, area.y + offsetY, area.w, area.h};
    auto& tema = GerenciadorTemas::getInstance();
    
    SDL_Color corBase = tema.getCorBotaoNormal(); 
    SDL_Color corHover = tema.getCorBotaoHover();
    
    SDL_Color corAtual;
    if (pressionado) corAtual = tema.getCorBotaoPressionado();
    else if (hover || (focadoPorControle && indiceFocoResultado == -1)) corAtual = corHover;
    else corAtual = corBase;
    
    SDL_SetRenderDrawColor(renderer, corAtual.r, corAtual.g, corAtual.b, corAtual.a);
    roundedBoxRGBA(renderer, adjustedArea.x, adjustedArea.y, adjustedArea.x + adjustedArea.w, adjustedArea.y + adjustedArea.h, raioBorda, corAtual.r, corAtual.g, corAtual.b, corAtual.a);

    // Desenho do retângulo de destaque externo para foco via controle
    if (focadoPorControle && indiceFocoResultado == -1) {
        SDL_Color corDest = tema.getCorDestaque();
        int offsetFoco = ConfigLayout::X(3);
        roundedRectangleRGBA(renderer, adjustedArea.x - offsetFoco, adjustedArea.y - offsetFoco, adjustedArea.x + adjustedArea.w + offsetFoco, adjustedArea.y + adjustedArea.h + offsetFoco, raioDestaque, corDest.r, corDest.g, corDest.b, 255);
    }

    std::string textoParaDesenhar;
    SDL_Color corTexto = tema.getCorTextoNormal(); 

    // Lógica para exibição do texto ou placeholder e efeito de piscar do cursor
    if (inputAtivo) {
        textoParaDesenhar = textoInput;
        corTexto = tema.getCorTextoNegrito();
        if (SDL_GetTicks() % 1000 < 500) textoParaDesenhar += "|";
    } else if (!textoInput.empty()) {
        textoParaDesenhar = textoInput;
        corTexto = tema.getCorTextoNegrito();
    } else {
        textoParaDesenhar = texto;
    }

    if (!textoParaDesenhar.empty()) {
        int textY = adjustedArea.y + (adjustedArea.h / 2) - (tamanhoPlaceholder / 2);
        MeuProjeto::desenharTexto(renderer, textoParaDesenhar, adjustedArea.x + ConfigLayout::X(20), textY, corTexto, tamanhoPlaceholder, fontePlaceholder);
    }

    if (!resultadosAtuais.empty()) {
        desenharResultados(adjustedArea);
    }
}

/**
 * @brief Renderiza a lista suspensa com as sugestões de pesquisa encontrados.
 * Desenha cada item de resultado, incluindo a capa do jogo, o título e os 
 * indicadores visuais de foco ou hover. Aplica o limite MAX_RESULTADOS para 
 * exibição vertical.
 * @param adjustedArea Retângulo da área da barra de pesquisa para cálculo de origem.
 */
void BotaoPesquisa::desenharResultados(const SDL_Rect& adjustedArea) {
    int yPos = adjustedArea.y + adjustedArea.h + ConfigLayout::Y(10);
    auto& tema = GerenciadorTemas::getInstance();

    for (size_t i = 0; i < resultadosAtuais.size() && i < MAX_RESULTADOS; ++i) {
        SDL_Rect nomeRect = {adjustedArea.x, yPos, adjustedArea.w, itemHeight};
        
        bool isFocado = ((int)i == indiceFocoResultado) || ((int)i == resultadoHoverIndex);
        
        SDL_Color corFundo = isFocado ? tema.getCorBotaoHover() : tema.getCorRetangulos();
        
        SDL_SetRenderDrawColor(renderer, corFundo.r, corFundo.g, corFundo.b, corFundo.a);
        SDL_RenderFillRect(renderer, &nomeRect);
        
        // Desenho de borda de destaque interna para itens selecionados via controle
        if ((int)i == indiceFocoResultado && focadoPorControle) {
            SDL_Color corDest = tema.getCorDestaque();
            int offsetInterno = ConfigLayout::X(2);
            int raioItem = ConfigLayout::F(5);
            roundedRectangleRGBA(renderer, nomeRect.x + offsetInterno, nomeRect.y + offsetInterno, nomeRect.x + nomeRect.w - offsetInterno, nomeRect.y + nomeRect.h - offsetInterno, raioItem, corDest.r, corDest.g, corDest.b, 255);
        }

        const auto& jogo = resultadosAtuais[i];
        std::string caminho = jogo.getCapaTelaPrincipal();
        
        // Renderização da miniatura da capa do jogo
        if (!caminho.empty()) {
            SDL_Texture* img = gerImg.carregar(renderer, caminho.c_str());
            if (img) {
                SDL_Rect imgRect = {nomeRect.x + imgOffsetX, yPos + imgOffsetY, imgSize, imgSize}; 
                SDL_RenderCopy(renderer, img, nullptr, &imgRect);
            }
        }

        // Renderização do nome do jogo com verificação de tamanho
        int textY = yPos + (itemHeight / 2) - (tamanhoResultados / 2);
        
        std::string nomeExibicao = jogo.getNome();
        // Se o nome for maior que 40 caracteres, corta e adiciona "..."
        if (nomeExibicao.length() > 40) {
            nomeExibicao = nomeExibicao.substr(0, 40) + "...";
        }

        MeuProjeto::desenharTexto(renderer, nomeExibicao, nomeRect.x + textOffsetX, textY, tema.getCorTextoNegrito(), tamanhoResultados, fonteResultados);
        
        yPos += itemHeight + itemPadding;
    }
}