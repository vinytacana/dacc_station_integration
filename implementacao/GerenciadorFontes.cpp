/**
 * @file GerenciadorFontes.cpp
 * @brief Implementação do sistema de cache de fontes TrueType.
 * 
 * Este arquivo contém a lógica para carregar e armazenar fontes em memória,
 * otimizando o desempenho da interface ao reutilizar objetos TTF_Font já 
 * carregados para combinações específicas de arquivo e tamanho.
 */

#include "GerenciadorFontes.hpp"
#include <SDL2/SDL.h>

using namespace MeuProjeto;

/**
 * @brief Função auxiliar estática que atua como o destruidor para as fontes.
 * 
 * Esta função é passada como um ponteiro de função para o `std::unique_ptr`.
 * Ela garante que a função específica da SDL (`TTF_CloseFont`) seja chamada
 * quando a fonte for removida do cache ou o gerenciador for destruído.
 * 
 * @param font Ponteiro para a fonte a ser liberada.
 */
void GerenciadorFontes::destruirFonte(TTF_Font* font) {
    if (font) TTF_CloseFont(font);
}

/**
 * @brief Construtor padrão da classe GerenciadorFontes.
 */
GerenciadorFontes::GerenciadorFontes() = default;

/**
 * @brief Destrutor da classe.
 * 
 * Chama liberarTudo() para limpar o cache de forma explícita, embora os 
 * smart pointers cuidariam disso automaticamente ao serem destruídos.
 */
GerenciadorFontes::~GerenciadorFontes() {
    liberarTudo();
}

/**
 * @brief Carrega uma fonte do disco ou recupera uma instância existente do cache.
 * 
 * O método segue o fluxo:
 * 1. Gera uma chave única combinando o caminho e o tamanho.
 * 2. Verifica se a chave já existe no `std::map`.
 * 3. Se não existir, utiliza `TTF_OpenFont` para carregar o arquivo.
 * 4. Insere a nova fonte no mapa utilizando `std::unique_ptr` com um deleitador customizado.
 * 
 * @param caminho String com o path do arquivo .ttf.
 * @param tamanho Inteiro com o tamanho da fonte em pontos.
 * @return TTF_Font* Ponteiro bruto para a fonte (gerenciado pela classe).
 */
TTF_Font* GerenciadorFontes::carregar(const std::string& caminho, int tamanho) {
    // Cria a chave de busca
    ChaveCache chave = {caminho, tamanho};
    
    // Tenta encontrar no cache
    auto it = cache.find(chave);
    if (it != cache.end()) {
        return it->second.get(); // Retorna o ponteiro bruto contido no unique_ptr
    }

    // Carregamento físico do disco caso não esteja em cache
    TTF_Font* font = TTF_OpenFont(caminho.c_str(), tamanho);
    if (!font) {
        // Log de erro caso o arquivo não exista ou o tamanho seja inválido
        SDL_Log("Falha ao carregar fonte '%s' (tamanho %d): %s", caminho.c_str(), tamanho, TTF_GetError());
        return nullptr;
    }

    // Configura o destruidor customizado
    auto deleter = &destruirFonte;
    
    /**
     * @note Uso de emplace com piecewise_construct para construir a chave e o 
     * unique_ptr diretamente no mapa, evitando cópias desnecessárias.
     */
    auto result = cache.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(chave),
        std::forward_as_tuple(font, deleter)
    );

    // Retorna o ponteiro recém-criado
    return result.first->second.get();
}

/**
 * @brief Limpa o cache de fontes.
 * 
 * Ao limpar o mapa, todos os `std::unique_ptr` são destruídos, o que por sua vez
 * aciona a função `destruirFonte` para cada fonte carregada, fechando todos os 
 * handles de fonte abertos via SDL_ttf.
 */
void GerenciadorFontes::liberarTudo() {
    cache.clear();
}