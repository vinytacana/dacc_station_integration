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

#include <fstream>

// ... (existing includes)

/**
 * @brief Helper simples para verificar existência de arquivo
 */
static bool arquivoExiste(const string& caminho) {
    ifstream f(caminho.c_str());
    return f.good();
}

SDL_Texture* GerenciadorImagens::getWallpaper(SDL_Renderer* renderer, const std::string& jogoId) {
    // 1. Tentar estrutura oficial: /dacc/games/[ID]/assets/hero_bg.jpg
    // Nota: Como estamos em dev, vamos assumir caminhos relativos ou absolutos mapeados
    // Se o jogoId for "jogo1", tentamos um caminho simulado ou mapeado se a estrutura completa nao existir
    
    // Caminho ideal (Produção/Target)
    // string path1 = "/dacc/games/" + jogoId + "/assets/hero_bg.jpg"; 
    
    // Caminho adaptado para o ambiente de desenvolvimento atual (assets/textures/jogoX...)
    // Mas o TODO 06 pede a estrutura nova. Vamos tentar suportar ambas ou priorizar a nova.
    // Vamos tentar construir o caminho baseando-se no ID.
    
    string paths[] = {
        "/dacc/games/" + jogoId + "/assets/hero_bg.jpg",
        "assets/textures/" + jogoId + "_hero.jpg", // Alternativa dev
        "assets/textures/" + jogoId + ".jpeg"      // Fallback para capa (existente)
    };

    for (const auto& path : paths) {
        if (arquivoExiste(path)) {
            return carregar(renderer, path);
        }
    }

    // Fallback final: Placeholder ou vazio
    // Se tivermos um placeholder global
    // return carregar(renderer, "assets/textures/placeholder.png");
    
    return nullptr;
}

SDL_Texture* GerenciadorImagens::getCapa(SDL_Renderer* renderer, const std::string& jogoId) {
    // 1. Tentar estrutura oficial
    string paths[] = {
        "/dacc/games/" + jogoId + "/assets/cover_portrait.jpg",
        "assets/textures/" + jogoId + ".jpeg", // Estrutura atual
        "assets/textures/" + jogoId + ".png"
    };

    for (const auto& path : paths) {
        if (arquivoExiste(path)) {
            return carregar(renderer, path);
        }
    }

    return nullptr;
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