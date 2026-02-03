/**
 * @file GerenciadorFontes.hpp
 * @brief Definição da classe GerenciadorFontes para gerenciamento de fontes TrueType (.ttf).
 * 
 * Este arquivo contém o sistema de cache de fontes, permitindo que diferentes 
 * partes da interface utilizem a mesma fonte e tamanho sem a necessidade de 
 * recarregá-las do disco múltiplas vezes.
 */

#pragma once
#include <SDL2/SDL_ttf.h>
#include <map>
#include <memory>
#include <string>
#include <tuple>

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class GerenciadorFontes
 * @brief Gerencia o carregamento, cache e ciclo de vida de objetos TTF_Font.
 * 
 * A classe utiliza um mapeamento interno para garantir que cada combinação de 
 * arquivo de fonte e tamanho seja carregada apenas uma vez. Ela gerencia 
 * automaticamente a memória utilizando smart pointers com deleters customizados 
 * para a API SDL_ttf.
 */
class GerenciadorFontes {
private:
    /**
     * @struct ChaveCache
     * @brief Estrutura interna para identificar unicamente uma fonte no cache.
     * 
     * Como o SDL_ttf cria objetos diferentes para a mesma fonte em tamanhos diferentes,
     * a chave do cache deve considerar tanto o caminho do arquivo quanto o tamanho (pontos).
     */
    struct ChaveCache {
        std::string caminho; /**< Caminho para o arquivo .ttf no disco. */
        int tamanho;         /**< Tamanho da fonte em pixels/pontos. */

        /**
         * @brief Operador de comparação menor que para permitir o uso em std::map.
         * 
         * Utiliza std::tie para uma comparação lexicográfica eficiente entre o 
         * caminho e o tamanho.
         * 
         * @param outra Referência para a outra chave a ser comparada.
         * @return true Se esta chave for considerada menor que a outra.
         */
        bool operator<(const ChaveCache& outra) const {
            return std::tie(caminho, tamanho) < std::tie(outra.caminho, outra.tamanho);
        }
    };

    /**
     * @brief Mapa que armazena as fontes carregadas.
     * 
     * Utiliza std::unique_ptr com um ponteiro de função para TTF_CloseFont como 
     * destruidor personalizado, garantindo que a memória da SDL seja liberada 
     * automaticamente quando o cache é limpo ou o gerenciador é destruído.
     */
    std::map<ChaveCache, std::unique_ptr<TTF_Font, void(*)(TTF_Font*)>> cache;

    /**
     * @brief Função auxiliar estática para destruir objetos TTF_Font.
     * 
     * Atua como o deleter para o std::unique_ptr, chamando internamente TTF_CloseFont.
     * 
     * @param font Ponteiro para a fonte a ser destruída.
     */
    static void destruirFonte(TTF_Font* font);

public:
    /**
     * @brief Construtor padrão.
     * Inicializa um gerenciador de fontes vazio.
     */
    GerenciadorFontes();

    /**
     * @brief Destrutor explícito.
     * Limpa o cache e libera todos os recursos de fontes associados.
     */
    ~GerenciadorFontes();

    /**
     * @brief Carrega uma fonte do disco ou a recupera do cache se já existir.
     * 
     * Verifica se a combinação de caminho e tamanho já está presente na memória. 
     * Se não estiver, realiza o carregamento via TTF_OpenFont.
     * 
     * @param caminho String contendo o caminho para o arquivo de fonte .ttf.
     * @param tamanho Inteiro representando o tamanho da fonte desejado.
     * @return TTF_Font* Ponteiro para a fonte carregada ou nullptr em caso de falha no carregamento.
     * 
     * @note O ponteiro retornado pertence ao gerenciador e não deve ser liberado 
     * manualmente pelo chamador.
     */
    TTF_Font* carregar(const std::string& caminho, int tamanho);

    /**
     * @brief Limpa completamente o cache de fontes.
     * 
     * Fecha todas as fontes abertas e remove todas as entradas do mapa, 
     * liberando a memória RAM ocupada pelos objetos TTF_Font.
     */
    void liberarTudo();
};

} // namespace MeuProjeto