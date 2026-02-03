/**
 * @file Janela.cpp
 * @brief Implementação do sistema de popup overlay para exibição de informações contextuais.
 * 
 * Este arquivo gerencia a criação e renderização de popups semi-transparentes
 * que exibem título e descrição sobre elementos da interface. O popup é desenhado
 * como overlay flutuante com fundo escurecido e texto centralizado/justificado.
 */

#include "Janela.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include "Utils.hpp" 
#include <iostream>
#include <sstream>
#include <SDL2/SDL2_gfxPrimitives.h> 

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Construtor da classe Janela (gerenciador de popups).
 * 
 * Inicializa o sistema de popups com configurações padrão de layout
 * responsivas baseadas na resolução da tela através do ConfigLayout.
 * 
 * @param renderer Renderizador SDL onde os popups serão desenhados.
 * 
 * @note paddingInterno e espacoDesc são calculados proporcionalmente
 *       à resolução usando ConfigLayout::X() e ConfigLayout::Y().
 */
Janela::Janela(SDL_Renderer* renderer)
    : renderer(renderer), isPopupAberto(false) {
    
    /// Padding interno horizontal proporcional (base: 20px)
    paddingInterno = ConfigLayout::X(20);
    
    /// Espaçamento vertical entre título e descrição (base: 8px)
    espacoDesc = ConfigLayout::Y(8);
}

/**
 * @brief Destrutor da classe Janela.
 * 
 * Não realiza operações especiais pois o renderer é gerenciado externamente
 * e as strings são liberadas automaticamente (RAII).
 */
Janela::~Janela() {
}

/**
 * @brief Ativa e configura um popup para exibição.
 * 
 * Define o conteúdo (título e descrição) e a área de renderização do popup.
 * Se ambos os textos estiverem vazios, nenhum popup é aberto.
 * 
 * @param nome Título do popup, renderizado em negrito e centralizado.
 * @param desc Descrição detalhada, renderizada em fonte normal com quebra automática.
 * @param x Coordenada X do canto superior esquerdo do popup.
 * @param y Coordenada Y do canto superior esquerdo do popup.
 * @param w Largura total do popup em pixels.
 * @param h Altura total do popup em pixels.
 * 
 * @note O popup permanece aberto até que fecharPopup() seja chamado explicitamente.
 */
void Janela::abrirPopup(const std::string& nome, const std::string& desc, 
                        int x, int y, int w, int h) {
    /// Valida se há conteúdo para exibir
    if (nome.empty() && desc.empty()) return;

    popupNome = nome;
    popupDesc = desc;
    
    /// Define retângulo de área do popup
    popupArea = {x, y, w, h}; 
    isPopupAberto = true;
}

/**
 * @brief Fecha e oculta o popup ativo.
 * 
 * Desativa a flag de exibição, impedindo que o popup seja renderizado
 * no próximo frame. O conteúdo textual não é limpo para permitir reabertura rápida.
 */
void Janela::fecharPopup() {
    isPopupAberto = false;
}

/**
 * @brief Renderiza o popup na tela com suporte a scroll/offset.
 * 
 * Desenha um overlay semi-transparente contendo:
 * 1. **Fundo**: Retângulo com cor do tema e 80% de opacidade (204/255)
 * 2. **Título**: Texto em negrito, centralizado horizontalmente
 * 3. **Descrição**: Texto normal, alinhado à esquerda, com quebra automática de linha
 * 
 * **Sistema de quebra de linha:**
 * - Analisa palavra por palavra usando stringstream
 * - Calcula largura acumulada com TTF_SizeUTF8
 * - Quebra linha quando excede largura disponível
 * - Interrompe se ultrapassar altura do popup
 * 
 * @param scrollX Deslocamento horizontal para suporte a scroll (padrão: 0).
 * @param scrollY Deslocamento vertical para suporte a scroll (padrão: 0).
 * 
 * @note Se isPopupAberto for false ou renderer for nulo, nenhuma renderização ocorre.
 * @note Utiliza blend mode para transparência correta do overlay.
 */
void Janela::desenharPopup(int scrollX, int scrollY) {
    /// Verifica se há popup para desenhar e se renderer é válido
    if (!isPopupAberto || !renderer) return;

    /**
     * Aplica offset de scroll à área do popup.
     * Permite que o popup se mova junto com conteúdo scrollável.
     */
    SDL_Rect areaDesenho = popupArea;
    areaDesenho.x += scrollX;
    areaDesenho.y += scrollY;

    /// Obtém instância do gerenciador de temas para cores
    auto& tema = GerenciadorTemas::getInstance();
    
    /// Gerenciador de fontes estático para cache eficiente
    static GerenciadorFontes gerFontes;
    
    /// Tamanhos de fonte proporcionais baseados em ConfigLayout
    int fontSizeTitulo = ConfigLayout::F(20); ///< Título maior (base 20pt)
    int fontSizeDesc = ConfigLayout::F(18);   ///< Descrição menor (base 18pt)
    
    /**
     * Obtém cor de fundo do tema (geralmente cor de hover de botão).
     * Será aplicada com 80% de opacidade para efeito overlay.
     */
    SDL_Color corBase = tema.getCorBotaoHover(); 
    
    /**
     * Ativa modo de blend para suportar canal alpha.
     * Essencial para renderizar o overlay semi-transparente corretamente.
     */
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, corBase.r, corBase.g, corBase.b, 204); 
    SDL_RenderFillRect(renderer, &areaDesenho);

    /// Margem interna proporcional para espaçamento do conteúdo
    int margem = ConfigLayout::X(15);
    
    /// Cursor Y para posicionamento vertical progressivo dos elementos
    int yAtual = areaDesenho.y + margem;
    
    /// Largura disponível para texto (descontando margens laterais)
    int larguraTexto = areaDesenho.w - (margem * 2);
    
    /// Cor do texto sempre branca para contraste máximo com overlay escuro
    SDL_Color corTextoOverlay = {255, 255, 255, 255}; 

    // RENDERIZAÇÃO DO TÍTULO
    
    if (!popupNome.empty()) {
        /**
         * Carrega fonte em negrito para o título.
         * Necessário para calcular dimensões reais do texto renderizado.
         */
        TTF_Font* fontTitulo = gerFontes.carregar(ConfigFontes::obterCaminho(TipoFonte::NEGRITO), fontSizeTitulo);
        
        int xTitulo = areaDesenho.x + margem; ///< Posição X inicial (esquerda)
        int hTitulo = fontSizeTitulo;         ///< Altura estimada do texto

        if (fontTitulo) {
            int wText, hText;
            /**
             * TTF_SizeUTF8 calcula dimensões exatas do texto renderizado.
             * Retorna largura e altura em pixels sem precisar renderizar.
             */
            TTF_SizeUTF8(fontTitulo, popupNome.c_str(), &wText, &hText);
            
            /**
             * Centraliza horizontalmente o título:
             * X = X_inicial_da_area + (Largura_da_area - Largura_do_texto) / 2
             */
            xTitulo = areaDesenho.x + (areaDesenho.w - wText) / 2;
            hTitulo = hText;
        }

        /// Renderiza o título centralizado em negrito
        MeuProjeto::desenharTexto(renderer, popupNome, xTitulo, yAtual, corTextoOverlay, fontSizeTitulo, TipoFonte::NEGRITO);
        
        /// Avança cursor Y (altura do título + espaçamento)
        yAtual += hTitulo + espacoDesc; 
    }

    // RENDERIZAÇÃO DA DESCRIÇÃO COM QUEBRA AUTOMÁTICA
    
    if (!popupDesc.empty()) {
        /// Descrição sempre alinhada à esquerda para melhor leitura
        int xDesc = areaDesenho.x + margem;
        
        /// Carrega fonte normal para descrição
        TTF_Font* font = gerFontes.carregar(ConfigFontes::obterCaminho(TipoFonte::NORMAL), fontSizeDesc);
        
        if (font) {
            /**
             * Utiliza stringstream para processar palavra por palavra.
             * Permite quebra inteligente de linha sem cortar palavras.
             */
            stringstream ss(popupDesc);
            string palavra, linha;
            
            /// Itera sobre cada palavra do texto
            while (ss >> palavra) {
                /**
                 * Testa se adicionar a palavra atual mantém a linha dentro da largura.
                 * Espaço adicional ao final para separação visual entre palavras.
                 */
                string testeLinha = linha + palavra + " ";
                int w, h;
                TTF_SizeUTF8(font, testeLinha.c_str(), &w, &h);
                
                if (w > larguraTexto) {
                    /**
                     * Linha excedeu largura disponível:
                     * 1. Renderiza linha atual (se não vazia)
                     * 2. Inicia nova linha com palavra atual
                     */
                    if (!linha.empty()) {
                        SDL_Surface* surf = TTF_RenderUTF8_Blended(font, linha.c_str(), corTextoOverlay);
                        if(surf) {
                            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
                            SDL_Rect dst = {xDesc, yAtual, surf->w, surf->h};
                            SDL_RenderCopy(renderer, tex, nullptr, &dst);
                            SDL_DestroyTexture(tex);
                            SDL_FreeSurface(surf);
                        }
                        yAtual += h; ///< Avança para próxima linha
                    }
                    linha = palavra + " "; ///< Nova linha começa com palavra atual
                } else {
                    /// Palavra cabe na linha atual, adiciona à linha de teste
                    linha = testeLinha;
                }
                
                /**
                 * Verifica se ultrapassou a altura disponível do popup.
                 * Interrompe renderização para evitar texto fora da área.
                 */
                if (yAtual > areaDesenho.y + areaDesenho.h - margem) break; 
            }
            
            /**
             * Renderiza última linha acumulada (se houver espaço).
             * Linha final frequentemente não atinge largura máxima.
             */
            if (!linha.empty() && yAtual <= areaDesenho.y + areaDesenho.h - margem) {
                SDL_Surface* surf = TTF_RenderUTF8_Blended(font, linha.c_str(), corTextoOverlay);
                if(surf) {
                    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
                    SDL_Rect dst = {xDesc, yAtual, surf->w, surf->h};
                    SDL_RenderCopy(renderer, tex, nullptr, &dst);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
            }
        }
    }
}