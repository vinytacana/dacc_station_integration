#ifndef GERENCIADOR_IMAGENS_HPP
#define GERENCIADOR_IMAGENS_HPP

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <map>

namespace MeuProjeto {

/**
 * @class GerenciadorImagens
 * @brief Gerencia e armazena em cache as texturas (SDL_Texture*) carregadas a partir de arquivos de imagem.
 *
 * Esta classe evita o carregamento redundante de texturas, melhorando a performance
 * e controlando o ciclo de vida (destruição) dos recursos SDL.
 */
class GerenciadorImagens {
public:
    /**
     * @brief Construtor padrão.
     */
    GerenciadorImagens();

    /**
     * @brief Destrutor. Chama liberarTudo() para destruir todas as texturas.
     */
    ~GerenciadorImagens();

    /**
     * @brief Carrega uma textura do disco ou a recupera do cache se já estiver carregada.
     *
     * @param renderer O renderizador SDL.
     * @param caminho O caminho completo para o arquivo de imagem (chave do cache).
     * @return Um ponteiro para a SDL_Texture, ou @c nullptr em caso de falha.
     */
    SDL_Texture* carregar(SDL_Renderer* renderer, const std::string& caminho);

    /**
     * @brief Recupera a textura de "Hero Background" (Wallpaper) para um jogo específico.
     * Implementa lógica de fallback:
     * 1. Tenta /dacc/games/[ID]/assets/hero_bg.jpg
     * 2. Tenta /dacc/games/[ID]/assets/cover_portrait.jpg
     * 3. Retorna textura de placeholder.
     */
    SDL_Texture* getWallpaper(SDL_Renderer* renderer, const std::string& jogoId);

    /**
     * @brief Recupera a textura de "Capa" (Portrait) para um jogo específico.
     * Implementa lógica de fallback similar.
     */
    SDL_Texture* getCapa(SDL_Renderer* renderer, const std::string& jogoId);

    /**
     * @brief Recupera o logotipo do jogo.
     */
    SDL_Texture* getLogo(SDL_Renderer* renderer, const std::string& jogoId);

    /**
     * @brief Libera (destrói) todas as texturas armazenadas no cache.
     */
    void liberarTudo();

private:
    /**
     * @brief Função auxiliar para carregar a imagem diretamente do disco usando IMG_LoadTexture.
     *
     * @param renderer O renderizador SDL.
     * @param caminho O caminho para o arquivo de imagem.
     * @return Um ponteiro para a SDL_Texture, ou @c nullptr em caso de falha.
     */
    SDL_Texture* carregarDoDisco(SDL_Renderer* renderer, const std::string& caminho);

    /**
     * @brief Cache para armazenar texturas já carregadas.
     *
     * A chave é o caminho do arquivo (std::string) e o valor é o ponteiro para a textura (SDL_Texture*).
     */
    std::map<std::string, SDL_Texture*> cache;
};

} // namespace MeuProjeto

/**
 * @brief Declaração externa do gerenciador de imagens global (usado em outros arquivos .cpp).
 */
extern MeuProjeto::GerenciadorImagens gerImg;

#endif // GERENCIADOR_IMAGENS_HPP