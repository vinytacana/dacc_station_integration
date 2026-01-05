#include "Utils.hpp"
#include "GerenciadorFontes.hpp"
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <string>

namespace MeuProjeto {

// DEFINIÇÃO DAS FONTES (CONFIGURE AQUI OS CAMINHOS)

/**
 * @brief Caminho para o arquivo da fonte Normal/Principal.
 */
std::string ConfigFontes::FONTE_NORMAL = 
    "Fontes/Garet-Book.ttf";

/**
 * @brief Caminho para o arquivo da fonte Negrito/Secundária.
 */
std::string ConfigFontes::FONTE_NEGRITO = 
    "Fontes/Garet-Heavy.ttf";


/**
 * @brief Referência para compatibilidade com código legado.
 */
const std::string& CAMINHO_FONTE = ConfigFontes::FONTE_NORMAL;

// OBTER CAMINHO DA FONTE

/**
 * @brief Obtém o caminho do arquivo da fonte baseado no seu tipo.
 *
 * @param tipo O enumerador @c TipoFonte desejado.
 * @return Uma referência constante para o caminho da fonte.
 */
const std::string& ConfigFontes::obterCaminho(TipoFonte tipo) {
    switch (tipo) {
        case TipoFonte::NORMAL:
            return FONTE_NORMAL;
        case TipoFonte::NEGRITO:
            return FONTE_NEGRITO;
        default:
            return FONTE_NORMAL;
    }
}

// CONFIGURAR FONTES 

/**
 * @brief Configura (muda em runtime) os caminhos dos arquivos de fontes.
 *
 * @param principal O novo caminho para a FONTE_NORMAL.
 * @param secundaria O novo caminho para a FONTE_NEGRITO.
 */
void ConfigFontes::configurarFontes(const std::string& principal,
                                   const std::string& secundaria) {
    FONTE_NORMAL = principal;
    FONTE_NEGRITO = secundaria;
    
    std::cout << "[CONFIG] Fontes atualizadas:" << std::endl;
    std::cout << "  Principal: " << principal << std::endl;
    std::cout << "  Secundária: " << secundaria << std::endl;
}

// DESENHAR TEXTO - Versão Original 

/**
 * @brief Desenha um texto na tela usando a fonte principal por padrão.
 *
 * @param renderer O renderizador SDL.
 * @param texto A string UTF-8 a ser desenhada.
 * @param x Coordenada X.
 * @param y Coordenada Y.
 * @param cor A cor do texto.
 * @param tamanhoFonte O tamanho da fonte.
 */
void desenharTexto(SDL_Renderer* renderer, 
                   const std::string& texto, 
                   int x, int y, 
                   SDL_Color cor, 
                   int tamanhoFonte) {
    // Usa fonte principal por padrão
    desenharTexto(renderer, texto, x, y, cor, tamanhoFonte, TipoFonte::NORMAL);
}

// DESENHAR TEXTO - Versão com Tipo de Fonte

/**
 * @brief Desenha um texto na tela usando um tipo de fonte específico.
 *
 * @param renderer O renderizador SDL.
 * @param texto A string UTF-8 a ser desenhada.
 * @param x Coordenada X.
 * @param y Coordenada Y.
 * @param cor A cor do texto.
 * @param tamanhoFonte O tamanho da fonte.
 * @param tipoFonte O tipo de fonte a ser usado.
 */
void desenharTexto(SDL_Renderer* renderer, 
                   const std::string& texto, 
                   int x, int y, 
                   SDL_Color cor, 
                   int tamanhoFonte,
                   TipoFonte tipoFonte) {
    static GerenciadorFontes gerenciador;

    const std::string& caminhoFonte = ConfigFontes::obterCaminho(tipoFonte);
    TTF_Font* font = gerenciador.carregar(caminhoFonte, tamanhoFonte);
    
    if (!font) {
        std::cerr << "[ERRO] Falha ao carregar fonte: " << caminhoFonte << std::endl;
        return;
    }

    // Cria a superfície com o texto
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, texto.c_str(), cor);
    if (!surface) return;

    // Cria a textura a partir da superfície
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) { 
        SDL_FreeSurface(surface); 
        return; 
    }

    // Área de destino na tela
    SDL_Rect dst = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &dst);

    // Limpa os recursos temporários
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

} // namespace MeuProjeto