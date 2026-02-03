/**
 * @file GerenciarInterface.hpp
 * @brief Definição da classe GerenciarInterface.
 *
 * Este arquivo define a classe responsável pela coordenação, gerenciamento e 
 * renderização de todos os elementos da interface gráfica da biblioteca de jogos. 
 * Atua como o núcleo centralizador de componentes como botões, teclado virtual e 
 * janelas modais.
 */

#ifndef GERENCIAR_INTERFACE_HPP
#define GERENCIAR_INTERFACE_HPP

#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <memory>
#include "Botao.hpp"
#include "BotaoPesquisa.hpp"
#include "TecladoVirtual.hpp"
#include "Forma.hpp"
#include "Janela.hpp"
#include "GerenciarScroll.hpp"

namespace MeuProjeto {

/**
 * @class GerenciarInterface
 * @brief Gerencia os principais componentes de interface do projeto.
 *
 * A classe GerenciarInterface é responsável por instanciar, organizar e renderizar 
 * os elementos visuais. Ela gerencia o ciclo de vida dos componentes, a troca de 
 * temas (claro/escuro) através de texturas dinâmicas, e a organização dos elementos 
 * navegáveis para interação via controle ou teclado.
 */
class GerenciarInterface {
public:
    /**
     * @brief Construtor da classe GerenciarInterface.
     *
     * Inicializa os ponteiros internos como nulos e prepara as estruturas de 
     * dados para o carregamento dos componentes.
     */
    GerenciarInterface();

    /**
     * @brief Destrutor da classe GerenciarInterface.
     *
     * Garante a liberação adequada da memória e dos recursos de interface 
     * gerenciados pela classe.
     */
    ~GerenciarInterface();

    /**
     * @brief Inicializa todos os componentes e recursos de interface.
     *
     * Realiza o carregamento de texturas, fontes e a configuração inicial das 
     * posições dos botões e painéis.
     *
     * @param renderer Ponteiro para o renderizador SDL_Renderer utilizado para criar texturas.
     * @return true se a inicialização for bem-sucedida, false caso ocorra falha no carregamento de recursos.
     */
    bool inicializar(SDL_Renderer* renderer);

    /**
     * @brief Renderiza a interface completa na tela.
     *
     * Coordena o desenho de fundos, botões, ícones, jogos e componentes especiais 
     * (como o teclado virtual) respeitando o estado de rolagem atual.
     *
     * @param renderer Ponteiro para o renderizador SDL_Renderer.
     * @param estado Referência para a classe GerenciarScroll contendo os dados de deslocamento da interface.
     */
    void desenhar(SDL_Renderer* renderer, GerenciarScroll& estado);

    /**
     * @brief Filtra e atualiza a exibição da lista de jogos baseada em uma categoria.
     *
     * @param categoria String contendo o nome da categoria selecionada.
     * @param renderer Ponteiro para o renderizador SDL_Renderer para atualização de texturas, se necessário.
     */
    void atualizarJogosPorCategoria(const std::string& categoria, SDL_Renderer* renderer);
    
    /**
     * @brief Recalcula as coordenadas dos elementos visuais.
     *
     * Ajusta a posição de todos os botões e componentes de acordo com as 
     * mudanças no estado de scroll da interface.
     *
     * @param estado Referência para a classe GerenciarScroll.
     */
    void atualizarPosicoes(GerenciarScroll& estado);
    
    /**
     * @brief Obtém o vetor de botões habilitados para navegação.
     *
     * @return Referência para um vetor de ponteiros do tipo Botao.
     */
    std::vector<Botao*>& obterBotoesNavegaveis();

    /**
     * @brief Retorna o ponteiro do botão de configurações.
     * @return Ponteiro para o objeto Botao.
     */
    Botao* getBotaoConfig() { return btnConfig; }

    /**
     * @brief Retorna o ponteiro do botão de alternância de tema.
     * @return Ponteiro para o objeto Botao.
     */
    Botao* getBotaoMudarTema() { return btnMudarTema; }

    /**
     * @brief Retorna o ponteiro do botão de seleção aleatória.
     * @return Ponteiro para o objeto Botao.
     */
    Botao* getBotaoAleatorio() { return btnAleatorio; }

    /**
     * @brief Retorna o ponteiro do componente de pesquisa.
     * @return Ponteiro para o objeto BotaoPesquisa.
     */
    BotaoPesquisa* getBotaoPesquisa() { return btnPesquisa; }
    
    /**
     * @brief Retorna o ponteiro da seta de navegação esquerda.
     * @return Ponteiro para o objeto Botao.
     */
    Botao* getSetaEsquerda() { return setaEsquerda; }

    /**
     * @brief Retorna o ponteiro da seta de navegação direita.
     * @return Ponteiro para o objeto Botao.
     */
    Botao* getSetaDireita() { return setaDireita; }
    
    /**
     * @brief Obtém um botão de categoria específico através de seu índice.
     * @param indice Posição do botão no vetor de categorias.
     * @return Ponteiro para o objeto Botao correspondente.
     */
    Botao* getBotaoCategoria(int indice);

    /**
     * @brief Retorna a lista de botões da seção de destaques.
     * @return Referência constante para o vetor de objetos Botao.
     */
    const std::vector<Botao>& getDestaques() const { return destaques; }

    /**
     * @brief Retorna a lista de botões da seção de categorias.
     * @return Referência constante para o vetor de objetos Botao.
     */
    const std::vector<Botao>& getCategorias() const { return categorias; }

    /**
     * @brief Retorna a lista de botões da seção de jogos.
     * @return Referência constante para o vetor de objetos Botao.
     */
    const std::vector<Botao>& getJogos() const { return jogos; }

    /**
     * @brief Retorna a quantidade de elementos na seção de destaques.
     * @return Valor inteiro representando o tamanho do vetor.
     */
    int getNumDestaques() const { return destaques.size(); }

    /**
     * @brief Retorna a quantidade de elementos na seção de categorias.
     * @return Valor inteiro representando o tamanho do vetor.
     */
    int getNumCategorias() const { return categorias.size(); }

    /**
     * @brief Retorna a quantidade de elementos na seção de jogos.
     * @return Valor inteiro representando o tamanho do vetor.
     */
    int getNumJogos() const { return jogos.size(); }

    /**
     * @brief Retorna o ponteiro para o teclado virtual gerenciado pela interface.
     * @return Ponteiro para o objeto TecladoVirtual.
     */
    TecladoVirtual* getTecladoVirtual() { return tecladoVirtual; }

private:
    Botao* btnConfig = nullptr;     /**< Ponteiro para o botão de acesso às configurações. */
    Botao* btnMudarTema = nullptr;  /**< Ponteiro para o botão de troca de tema visual. */
    Botao* btnAleatorio = nullptr;  /**< Ponteiro para o botão de escolha de jogo aleatório. */
    BotaoPesquisa* btnPesquisa = nullptr; /**< Ponteiro para o componente de busca de títulos. */
    
    Botao* setaEsquerda = nullptr;  /**< Ponteiro para a seta de navegação lateral esquerda. */
    Botao* setaDireita = nullptr;   /**< Ponteiro para a seta de navegação lateral direita. */
    
    std::vector<Botao> destaques;   /**< Vetor contendo os botões dos jogos em destaque. */
    std::vector<Botao> categorias;  /**< Vetor contendo os botões de categorias de jogos. */
    std::vector<Botao> jogos;       /**< Vetor contendo os botões da listagem geral de jogos. */
    
    Forma* fundoCategorias = nullptr; /**< Ponteiro para o componente visual de fundo das categorias. */
    
    SDL_Texture* iconeLupaClaro = nullptr;   /**< Textura do ícone de pesquisa para o tema claro. */
    SDL_Texture* iconeLupaEscuro = nullptr;  /**< Textura do ícone de pesquisa para o tema escuro. */
    
    SDL_Texture* iconeDado = nullptr;   /**< Textura do ícone de aleatoriedade. */

    SDL_Texture* iconeConfigClaro = nullptr;  /**< Textura do ícone de engrenagem para o tema claro. */
    SDL_Texture* iconeConfigEscuro = nullptr; /**< Textura do ícone de engrenagem para o tema escuro. */
    
    SDL_Texture* explicacaoClaro = nullptr;  /**< Textura informativa/tutorial para o tema claro. */
    SDL_Texture* explicacaoEscuro = nullptr; /**< Textura informativa/tutorial para o tema escuro. */
    
    Janela* popupJanela = nullptr; /**< Ponteiro para uma instância de janela popup/modal. */
    std::vector<Botao*> botoesNavegaveis; /**< Lista consolidada de ponteiros para botões que suportam foco de navegação. */

    TecladoVirtual* tecladoVirtual = nullptr; /**< Ponteiro para a instância do teclado virtual. */
};

} // namespace MeuProjeto

#endif