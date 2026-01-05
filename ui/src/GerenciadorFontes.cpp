#include "GerenciadorFontes.hpp"
#include <SDL2/SDL.h>
#include <utility> // Para std::forward_as_tuple e std::piecewise_construct

using namespace MeuProjeto;

/**
 * @brief Implementação do deleter customizado.
 *
 * @param font O ponteiro TTF_Font a ser fechado.
 */
void GerenciadorFontes::destruirFonte(TTF_Font* font) {
    if (font) TTF_CloseFont(font);
}

/**
 * @brief Construtor padrão.
 */
GerenciadorFontes::GerenciadorFontes() = default;

/**
 * @brief Destrutor. Chama liberarTudo() para fechar todas as fontes.
 */
GerenciadorFontes::~GerenciadorFontes() {
    liberarTudo();
}

/**
 * @brief Carrega uma fonte ou a recupera do cache.
 *
 * @param caminho O caminho completo para o arquivo TTF.
 * @param tamanho O tamanho da fonte em pontos (pt).
 * @return Retorna um ponteiro para a estrutura TTF_Font*.
 */
TTF_Font* GerenciadorFontes::carregar(const std::string& caminho, int tamanho) {
    ChaveCache chave = {caminho, tamanho};
    auto it = cache.find(chave);

    // 1. Verifica se a fonte já está em cache
    if (it != cache.end()) {
        return it->second.get();
    }

    // 2. Tenta carregar a fonte usando SDL_ttf
    TTF_Font* font = TTF_OpenFont(caminho.c_str(), tamanho);
    if (!font) {
        SDL_Log("Falha ao carregar fonte '%s' (tamanho %d): %s", caminho.c_str(), tamanho, TTF_GetError());
        return nullptr;
    }

    // 3. Insere a nova fonte no cache usando std::unique_ptr com custom deleter
    auto deleter = &destruirFonte;
    auto result = cache.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(chave),
        std::forward_as_tuple(font, deleter)
    );

    return result.first->second.get();
}

/**
 * @brief Libera (fecha) todas as fontes atualmente armazenadas no cache.
 */
void GerenciadorFontes::liberarTudo() {
    // std::map::clear() destrói os elementos, o que aciona o custom deleter
    // TTF_CloseFont para cada TTF_Font*.
    cache.clear();
}