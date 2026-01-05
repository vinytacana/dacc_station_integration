#pragma once

#include <SDL2/SDL_ttf.h>
#include <map>
#include <memory>
#include <string>
#include <tuple> // Para std::tie

namespace MeuProjeto {

/**
 * @class GerenciadorFontes
 * @brief Implementa um cache de fontes TTF para evitar recarregar a mesma fonte e tamanho.
 *
 * Utiliza um std::map e std::unique_ptr com um deleter customizado
 * para gerenciar o ciclo de vida das fontes SDL_ttf (TTF_Font*).
 */
class GerenciadorFontes {
private:
    /**
     * @brief Estrutura usada como chave de cache, combinando o caminho e o tamanho da fonte.
     */
    struct ChaveCache {
        std::string caminho; ///< Caminho do arquivo da fonte (e.g., .ttf).
        int tamanho;         ///< Tamanho da fonte em pontos (pt).
        
        /**
         * @brief Operador de comparação necessário para usar a estrutura como chave em std::map.
         */
        bool operator<(const ChaveCache& outra) const {
            // Usa std::tie para uma comparação lexicográfica segura.
            return std::tie(caminho, tamanho) < std::tie(outra.caminho, outra.tamanho);
        }
    };

    /**
     * @brief O cache principal. Mapeia a chave de (caminho, tamanho) para um ponteiro único de TTF_Font.
     *
     * O std::unique_ptr utiliza um custom deleter (destruirFonte) para chamar TTF_CloseFont.
     */
    std::map<ChaveCache, std::unique_ptr<TTF_Font, void(*)(TTF_Font*)>> cache;

    /**
     * @brief Função estática de deleção customizada para TTF_CloseFont.
     * @param font O ponteiro TTF_Font a ser fechado.
     */
    static void destruirFonte(TTF_Font* font);

public:
    /**
     * @brief Construtor padrão.
     */
    GerenciadorFontes();

    /**
     * @brief Destrutor. Chama liberarTudo() para fechar todas as fontes abertas.
     */
    ~GerenciadorFontes();

    /**
     * @brief Carrega uma fonte ou a recupera do cache se já estiver carregada.
     *
     * @param caminho O caminho completo para o arquivo TTF.
     * @param tamanho O tamanho da fonte em pontos (pt).
     * @return Retorna um ponteiro para a estrutura TTF_Font* em caso de sucesso, ou @c nullptr em caso de falha.
     */
    TTF_Font* carregar(const std::string& caminho, int tamanho);

    /**
     * @brief Libera (fecha) todas as fontes atualmente armazenadas no cache.
     *
     * O std::map.clear() aciona os deleters customizados dos std::unique_ptr.
     */
    void liberarTudo();
};

}