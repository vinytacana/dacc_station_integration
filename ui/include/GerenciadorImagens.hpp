/**
 * @file Gerenciador_Imagens.hpp
 * @brief Definição da classe GerenciadorImagens para controle de cache de texturas.
 * 
 * Este arquivo contém a lógica de gerenciamento de ativos visuais. Ele garante que
 * cada imagem seja carregada do disco apenas uma vez, mantendo uma cópia em memória
 * para acesso rápido através de um mapeamento por caminho de arquivo.
 */

#ifndef GERENCIADOR_IMAGENS_HPP
#define GERENCIADOR_IMAGENS_HPP

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <map>
#include <string>
#include <tuple>

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class GerenciadorImagens
 * @brief Responsável pelo carregamento otimizado e cache de texturas SDL.
 * 
 * A classe GerenciadorImagens atua como um repositório central de texturas.
 * Ao solicitar uma imagem, a classe verifica se ela já existe no cache (std::map).
 * Se existir, retorna o ponteiro existente; caso contrário, realiza o carregamento
 * via disco, armazena no cache e retorna a nova textura.
 */
class GerenciadorImagens {
public:
    /**
     * @brief Construtor padrão da classe GerenciadorImagens.
     * Inicializa o cache de texturas vazio.
     */
    GerenciadorImagens();

    /**
     * @brief Destrutor da classe GerenciadorImagens.
     * Garante que todas as texturas armazenadas no cache sejam destruídas 
     * corretamente para evitar vazamentos de memória (memory leaks).
     */
    ~GerenciadorImagens();

    /**
     * @brief Carrega uma textura do disco ou recupera uma versão já carregada no cache.
     * 
     * Este é o método principal para obter imagens no projeto. Ele automatiza a 
     * verificação de duplicatas.
     * 
     * @param renderer O renderizador SDL onde a textura será vinculada.
     * @param caminho O caminho (path) relativo ou absoluto para o arquivo de imagem (PNG, JPG, etc).
     * @return SDL_Texture* Ponteiro para a textura carregada pronta para uso.
     * @return nullptr Caso o arquivo não seja encontrado ou ocorra erro no carregamento.
     */
    SDL_Texture* carregar(SDL_Renderer* renderer, const std::string& caminho);

    /**
     * @brief Libera todas as texturas armazenadas no cache e limpa o mapa.
     * 
     * Percorre todo o container de cache, chama `SDL_DestroyTexture` para cada
     * item e remove todas as entradas. Útil para limpeza total ou troca de contexto.
     */
    void liberarTudo();

    void liberarRenderer(SDL_Renderer* renderer);

private:
    /**
     * @brief Função auxiliar interna para realizar o carregamento físico do disco.
     * 
     * Este método encapsula a chamada a `IMG_Load` e `SDL_CreateTextureFromSurface`.
     * É um método privado usado apenas quando o cache não possui a imagem solicitada.
     * 
     * @param renderer O renderizador SDL.
     * @param caminho O caminho do arquivo no sistema.
     * @return SDL_Texture* Ponteiro da nova textura criada ou nullptr em falha.
     */
    SDL_Texture* carregarDoDisco(SDL_Renderer* renderer, const std::string& caminho);

    struct ChaveImagem {
        const SDL_Renderer* renderer;
        std::string caminho;

        bool operator<(const ChaveImagem& outra) const {
            return std::tie(renderer, caminho) < std::tie(outra.renderer, outra.caminho);
        }
    };

    std::map<ChaveImagem, SDL_Texture*> cache;
};

} // namespace MeuProjeto

/**
 * @brief Declaração externa da instância global do gerenciador de imagens.
 * 
 * Como as imagens são recursos compartilhados por toda a interface (janelas, botões, grid),
 * uma instância global (gerImg) facilita o acesso ao cache em diferentes partes do código
 * sem a necessidade de passar o gerenciador por parâmetro constantemente.
 */
extern MeuProjeto::GerenciadorImagens gerImg;

#endif // GERENCIADOR_IMAGENS_HPP
