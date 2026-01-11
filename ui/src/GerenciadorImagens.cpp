/**
 * @file GerenciadorImagens.cpp
 * @brief Implementação do sistema de cache de texturas para a interface.
 * 
 * Este arquivo contém a lógica para evitar carregamentos redundantes de arquivos
 * de imagem, gerenciando a memória de vídeo (VRAM) de forma eficiente através
 * de um mapeamento de caminhos para objetos de textura.
 */

#include "GerenciadorImagens.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h> 

using namespace MeuProjeto;

using namespace std;

/**
 * @brief Construtor padrão da classe GerenciadorImagens.
 * 
 * Utiliza a implementação padrão do compilador para inicializar o mapa de cache.
 */
GerenciadorImagens::GerenciadorImagens() = default;

/**
 * @brief Destrutor da classe GerenciadorImagens.
 * 
 * Invoca automaticamente o método liberarTudo() para garantir que, ao final do 
 * ciclo de vida do objeto, nenhuma textura permaneça alocada na memória de vídeo.
 */
GerenciadorImagens::~GerenciadorImagens() {
    liberarTudo();
}

/**
 * @brief Gerencia a obtenção de texturas de forma otimizada.
 * 
 * O método opera em duas fases:
 * 1. **Busca no Cache**: Verifica se a string do caminho já é uma chave no mapa. 
 *    Se for, retorna o ponteiro existente imediatamente, evitando IO de disco.
 * 2. **Carregamento e Registro**: Se não estiver no cache, carrega a imagem do disco,
 *    armazena o novo ponteiro no mapa para uso futuro e o retorna.
 * 
 * @param renderer O renderizador SDL onde a textura será vinculada.
 * @param caminho String contendo o caminho para o arquivo de imagem (ex: "Assets/capa.png").
 * @return SDL_Texture* Ponteiro para a textura pronta para renderização, ou nullptr em caso de erro.
 */
SDL_Texture* GerenciadorImagens::carregar(SDL_Renderer* renderer, const string& caminho) {
    if (!renderer) {
        SDL_Log("Erro: Renderer nulo ao tentar carregar '%s'", caminho.c_str());
        return nullptr;
    }

    // Passo 1: Verificar se a imagem já foi carregada anteriormente
    auto it = cache.find(caminho);
    if (it != cache.end()) {
        return it->second; // Retorna a textura existente no cache
    }

    // Passo 2: Se não estiver no cache, carregar do disco
    SDL_Texture* textura = carregarDoDisco(renderer, caminho);
    if (textura) {
        cache[caminho] = textura; // Registra no cache para otimizar chamadas futuras
    }

    return textura;
}

/**
 * @brief Encapsula a chamada à biblioteca SDL_image para leitura de arquivos.
 * 
 * Utiliza `IMG_LoadTexture`, que suporta nativamente diversos formatos (PNG, JPG, etc)
 * e já converte a superfície diretamente para uma textura acelerada por hardware.
 * 
 * @param renderer O renderizador SDL associado.
 * @param caminho Caminho físico do arquivo no sistema de arquivos.
 * @return SDL_Texture* Ponteiro para a nova textura ou nullptr se o arquivo for inválido/inexistente.
 */
SDL_Texture* GerenciadorImagens::carregarDoDisco(SDL_Renderer* renderer, const string& caminho) {
    SDL_Texture* textura = IMG_LoadTexture(renderer, caminho.c_str());
    if (!textura) {
        // Log de erro formatado para auxílio no debug de ativos faltando
        SDL_Log("Falha ao carregar imagem '%s': %s", caminho.c_str(), IMG_GetError());
    }
    return textura;
}

/**
 * @brief Realiza a limpeza completa do cache e liberação de memória.
 * 
 * Itera por todos os pares do mapa de cache, chamando a função da API SDL 
 * `SDL_DestroyTexture` para cada valor. Ao final, limpa o mapa para 
 * permitir que novos carregamentos iniciem do zero, se necessário.
 */
void GerenciadorImagens::liberarTudo() {
    for (auto& par : cache) {
        if (par.second) {
            SDL_DestroyTexture(par.second);
            par.second = nullptr;
        }
    }
    cache.clear();
}