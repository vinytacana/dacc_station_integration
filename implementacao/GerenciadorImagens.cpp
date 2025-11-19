#include "GerenciadorImagens.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h> 
#include <string>

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Construtor padrão.
 */
GerenciadorImagens::GerenciadorImagens() = default;

/**
 * @brief Destrutor. Chama liberarTudo() para destruir todas as texturas.
 */
GerenciadorImagens::~GerenciadorImagens() {
    liberarTudo();
}

/**
 * @brief Carrega uma textura do disco ou a recupera do cache.
 *
 * @param renderer O renderizador SDL.
 * @param caminho O caminho completo para o arquivo de imagem.
 * @return Um ponteiro para a SDL_Texture, ou @c nullptr em caso de falha.
 */
SDL_Texture* GerenciadorImagens::carregar(SDL_Renderer* renderer, const string& caminho) {
    if (!renderer) {
        SDL_Log("Erro: Renderer nulo ao tentar carregar '%s'", caminho.c_str());
        return nullptr;
    }

    // Passo 1: Verificar cache
    auto it = cache.find(caminho);
    if (it != cache.end()) {
        return it->second;
    }

    // Passo 2: Carregar do disco
    SDL_Texture* textura = carregarDoDisco(renderer, caminho);
    if (textura) {
        // Passo 3: Armazenar no cache se o carregamento foi bem-sucedido
        cache[caminho] = textura;
    }

    return textura;
}

/**
 * @brief Função auxiliar para carregar a imagem diretamente do disco usando IMG_LoadTexture.
 *
 * @param renderer O renderizador SDL.
 * @param caminho O caminho para o arquivo de imagem.
 * @return Um ponteiro para a SDL_Texture, ou @c nullptr em caso de falha.
 */
SDL_Texture* GerenciadorImagens::carregarDoDisco(SDL_Renderer* renderer, const string& caminho) {
    SDL_Texture* textura = IMG_LoadTexture(renderer, caminho.c_str());
    if (!textura) {
        SDL_Log("Falha ao carregar imagem '%s': %s", caminho.c_str(), IMG_GetError());
    }
    return textura;
}

/**
 * @brief Libera (destrói) todas as texturas armazenadas no cache.
 */
void GerenciadorImagens::liberarTudo() {
    for (auto& par : cache) {
        if (par.second) {
            SDL_DestroyTexture(par.second);
            // Definir o ponteiro para nullptr é opcional aqui, mas pode ajudar a evitar
            // uso indevido caso o ponteiro fosse acessado após o clear().
            par.second = nullptr; 
        }
    }
    cache.clear();
}