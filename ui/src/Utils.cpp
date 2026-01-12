/**
 * @file Utils.cpp (Versão Otimizada com Cache de Texturas)
 */

#include "Utils.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorTemas.hpp"
#include "GerenciadorTexturasTexto.hpp"
#include <SDL2/SDL_ttf.h>
#include <iostream>

namespace MeuProjeto {


static GerenciadorFontes g_fontes;
static GerenciadorTexturasTexto g_texturasTexto;

std::string ConfigFontes::FONTE_NORMAL = "assets/fonts/Garet-Book.ttf";
std::string ConfigFontes::FONTE_NEGRITO = "assets/fonts/Garet-Heavy.ttf";
const std::string& CAMINHO_FONTE = ConfigFontes::FONTE_NORMAL;

const std::string& ConfigFontes::obterCaminho(TipoFonte tipo) {
    return (tipo == TipoFonte::NEGRITO) ? FONTE_NEGRITO : FONTE_NORMAL;
}

void ConfigFontes::configurarFontes(const std::string& principal, const std::string& secundaria) {
    FONTE_NORMAL = principal;
    FONTE_NEGRITO = secundaria;
}

/**
 * @brief Renderiza um texto na tela utilizando Cache de Texturas para performance.
 * 
 * @details **Otimização implementada:**
 * Em vez de gerar uma textura nova a cada frame (o que causaria uso excessivo de CPU/GPU),
 * a função agora consulta o @a GerenciadorTexturasTexto. Se o texto já foi desenhado
 * anteriormente com a mesma fonte, cor e tamanho, a textura existente é reutilizada.
 */
void desenharTexto(SDL_Renderer* renderer, const std::string& texto, int x, int y, SDL_Color cor, int tamanhoFonte, TipoFonte tipoFonte) {
    if (texto.empty()) return;

    std::string path = ConfigFontes::obterCaminho(tipoFonte);
    TTF_Font* font = g_fontes.carregar(path, tamanhoFonte); // Usa a instância global do arquivo
    if (!font) return;

    SDL_Texture* texture = g_texturasTexto.obterTextura(renderer, font, texto, path, tamanhoFonte, cor); // Usa a instância global
    
    if (texture) {
        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
        SDL_Rect dst = {x, y, w, h};
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
    }
}

void desenharTexto(SDL_Renderer* renderer, const std::string& texto, int x, int y, SDL_Color cor, int tamanhoFonte) {
    desenharTexto(renderer, texto, x, y, cor, tamanhoFonte, TipoFonte::NORMAL);
}

void desenharTexto(SDL_Renderer* renderer, const std::string& texto, int x, int y, int tamanhoFonte, TipoFonte tipoFonte) {
    SDL_Color corAutomatica = GerenciadorTemas::getInstance().getCorTextoNegrito();
    desenharTexto(renderer, texto, x, y, corAutomatica, tamanhoFonte, tipoFonte);
}

void limparCacheTexto() {
    g_texturasTexto.liberarTudo();
    g_fontes.liberarTudo();
}

} // namespace MeuProjeto