/**
 * @file GerenciadorTexturasTexto.hpp
 * @brief Gerenciamento de cache para texturas de texto renderizadas.
 * 
 * Este arquivo define um sistema de cache que armazena texturas de strings 
 * já processadas, evitando a recriação custosa de superfícies e texturas 
 * a cada frame.
 */

#ifndef GERENCIADOR_TEXTURAS_TEXTO_HPP
#define GERENCIADOR_TEXTURAS_TEXTO_HPP

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <map>
#include <memory>
#include <tuple>

namespace MeuProjeto {

/**
 * @class GerenciadorTexturasTexto
 * @brief Responsável por cachear textos renderizados como SDL_Texture.
 * 
 * Renderizar texto no SDL_ttf envolve criar uma Surface na CPU e enviá-la para 
 * a GPU como Texture. Esta classe garante que, se o conteúdo do texto, a cor 
 * e o tamanho forem os mesmos, a textura da GPU seja reaproveitada.
 */
class GerenciadorTexturasTexto {
private:
    /**
     * @struct ChaveTexto
     * @brief Identificador único para uma textura de texto no cache.
     */
    struct ChaveTexto {
        const SDL_Renderer* renderer; /**< Renderer dono da textura em cache. */
        std::string texto;       /**< O conteúdo da string. */
        std::string caminhoFonte; /**< O arquivo de fonte utilizado. */
        int tamanho;             /**< O tamanho da fonte em pontos. */
        SDL_Color cor;           /**< A cor RGBA do texto. */

        /**
         * @brief Operador de comparação para permitir o uso em std::map.
         */
        bool operator<(const ChaveTexto& outra) const {
            return std::tie(renderer, texto, caminhoFonte, tamanho, cor.r, cor.g, cor.b, cor.a) <
                   std::tie(outra.renderer, outra.texto, outra.caminhoFonte, outra.tamanho, outra.cor.r, outra.cor.g, outra.cor.b, outra.cor.a);
        }
    };

    /** @brief Cache que mapeia as propriedades do texto para a textura pronta na GPU. */
    std::map<ChaveTexto, std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)>> cache;

    /** @brief Deleter customizado para SDL_Texture. */
    static void destruirTextura(SDL_Texture* tex) { if (tex) SDL_DestroyTexture(tex); }

public:
    GerenciadorTexturasTexto();
    ~GerenciadorTexturasTexto();

    /**
     * @brief Obtém uma textura de texto do cache ou cria uma nova se não existir.
     * 
     * @param renderer Renderizador SDL.
     * @param font Ponteiro para a fonte TTF carregada.
     * @param texto A string a ser renderizada.
     * @param caminhoFonte Caminho da fonte (para a chave do cache).
     * @param tamanho Tamanho da fonte (para a chave do cache).
     * @param cor Cor do texto.
     * @return SDL_Texture* Ponteiro para a textura pronta para desenho.
     */
    SDL_Texture* obterTextura(SDL_Renderer* renderer, TTF_Font* font, const std::string& texto, 
                               const std::string& caminhoFonte, int tamanho, SDL_Color cor);

    /** @brief Limpa todas as texturas de texto da memória de vídeo. */
    void liberarTudo();
};

} // namespace MeuProjeto

#endif // GERENCIADOR_TEXTURAS_TEXTO_HPP
