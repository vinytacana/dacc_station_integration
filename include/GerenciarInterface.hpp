/**
 * @file GerenciarInterface.hpp
 * @brief Definição da classe GerenciarInterface responsável por organizar e renderizar a UI.
 * 
 * Este arquivo contém o gerenciador central da interface do usuário (UI), 
 * coordenando a criação, o layout e o desenho de todos os elementos visuais 
 * da biblioteca de jogos.
 */

#ifndef GERENCIAR_INTERFACE_HPP
#define GERENCIAR_INTERFACE_HPP

#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include "Botao.hpp"
#include "BotaoPesquisa.hpp"
#include "Forma.hpp"
#include "Janela.hpp"
#include "GerenciarScroll.hpp"
#include "TecladoVirtual.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class GerenciarInterface
 * @brief Centraliza a gestão de componentes visuais e o layout da aplicação.
 * 
 * A classe GerenciarInterface atua como um container e gerenciador de layout.
 * Ela armazena as listas de botões de jogos, categorias e destaques, além de 
 * lidar com a troca dinâmica de texturas para suporte a temas (Claro/Escuro).
 */
class GerenciarInterface {
public:
    /**
     * @brief Construtor da classe GerenciarInterface.
     */
    GerenciarInterface();

    /**
     * @brief Destrutor da classe GerenciarInterface.
     * Realiza a limpeza de ponteiros, texturas e subcomponentes alocados.
     */
    ~GerenciarInterface();

    /**
     * @brief Inicializa todos os componentes da interface.
     * 
     * Carrega texturas iniciais, cria os botões principais, define áreas de layout
     * e prepara o estado inicial da biblioteca para exibição.
     * 
     * @param renderer Ponteiro para o renderizador SDL principal.
     */
    void inicializar(SDL_Renderer* renderer);

    /**
     * @brief Renderiza todos os elementos visíveis na tela.
     * 
     * Orquestra a ordem de desenho: fundos, destaques, grid de jogos, categorias,
     * botões de navegação e, por fim, elementos de overlay (popups e teclado).
     * 
     * @param renderer Ponteiro para o renderizador SDL.
     * @param estado Referência ao objeto GerenciarScroll para aplicar deslocamentos.
     */
    void desenhar(SDL_Renderer* renderer, GerenciarScroll& estado);

    /**
     * @brief Filtra e reconstrói a lista de jogos exibida com base em uma categoria.
     * 
     * @param categoria Nome da categoria para filtrar (ex: "Ação", "Todas").
     * @param renderer Renderizador necessário para recriar as texturas dos botões de jogos.
     */
    void atualizarJogosPorCategoria(const std::string& categoria, SDL_Renderer* renderer);
    
    /**
     * @brief Atualiza as coordenadas (X, Y) de todos os botões com base no scroll atual.
     * 
     * Garante que a colisão de mouse e a renderização acompanhem o movimento da "câmera".
     * 
     * @param estado Referência ao estado global de navegação.
     */
    void atualizarPosicoes(GerenciarScroll& estado);
    
    /**
     * @brief Retorna uma lista de ponteiros para todos os botões que aceitam foco.
     * 
     * Essencial para o sistema de navegação por controle (gamepad), permitindo 
     * que o GerenciarInputs saiba quais elementos podem ser selecionados.
     * 
     * @return std::vector<Botao*>& Referência para o vetor de botões navegáveis.
     */
    std::vector<Botao*>& obterBotoesNavegaveis();

    // Getters de Componentes Específicos

    /** @brief Retorna o botão de abrir configurações. */
    Botao* getBotaoConfig() { return btnConfig; }

    /** @brief Retorna o botão de alternar tema (Claro/Escuro). */
    Botao* getBotaoMudarTema() { return btnMudarTema; }

    /** @brief Retorna o botão de seleção aleatória de jogo. */
    Botao* getBotaoAleatorio() { return btnAleatorio; }

    /** @brief Retorna o componente de barra de pesquisa. */
    BotaoPesquisa* getBotaoPesquisa() { return btnPesquisa; }
    
    /** @brief Retorna o botão de navegação para a esquerda (carrossel). */
    Botao* getSetaEsquerda() { return setaEsquerda; }

    /** @brief Retorna o botão de navegação para a direita (carrossel). */
    Botao* getSetaDireita() { return setaDireita; }
    
    /** 
     * @brief Busca um botão de categoria pelo seu índice.
     * @param indice Posição no vetor de categorias.
     * @return Botao* Ponteiro para o botão ou nullptr se inválido.
     */
    Botao* getBotaoCategoria(int indice);

    // Getters de Coleções

    /** @brief Retorna o vetor de botões da seção de destaques (Hero banners). */
    const std::vector<Botao>& getDestaques() const { return destaques; }

    /** @brief Retorna o vetor de botões da barra de categorias. */
    const std::vector<Botao>& getCategorias() const { return categorias; }

    /** @brief Retorna o vetor de botões que representam os jogos na grade principal. */
    const std::vector<Botao>& getJogos() const { return jogos; }

    // Getters de Contagem
    int getNumDestaques() const { return destaques.size(); }
    int getNumCategorias() const { return categorias.size(); }
    int getNumJogos() const { return jogos.size(); }

    /** @brief Retorna o componente do teclado virtual. */
    TecladoVirtual* getTecladoVirtual() { return tecladoVirtual; }

private:
    // Botões Fixos da UI Superior/Lateral
    Botao* btnConfig = nullptr;
    Botao* btnMudarTema = nullptr;
    Botao* btnAleatorio = nullptr;
    BotaoPesquisa* btnPesquisa = nullptr;
    
    // Controles do Carrossel de Categorias
    Botao* setaEsquerda = nullptr;
    Botao* setaDireita = nullptr;
    
    // Listas de Componentes Dinâmicos
    std::vector<Botao> destaques;   /**< Banners grandes de destaque. */
    std::vector<Botao> categorias;  /**< Botões de filtro de categoria. */
    std::vector<Botao> jogos;       /**< Grid de jogos da biblioteca. */
    
    Forma* fundoCategorias = nullptr; /**< Elemento visual de fundo da barra de categorias. */
    
    // Texturas Dinâmicas (Alternadas conforme o tema)
    SDL_Texture* iconeLupaClaro = nullptr;
    SDL_Texture* iconeLupaEscuro = nullptr;
    
    SDL_Texture* iconeDadoClaro = nullptr;
    SDL_Texture* iconeDadoEscuro = nullptr;

    SDL_Texture* iconeConfigClaro = nullptr;
    SDL_Texture* iconeConfigEscuro = nullptr;
    
    SDL_Texture* explicacaoClaro = nullptr;  /**< Infográfico de ajuda (Modo Claro). */
    SDL_Texture* explicacaoEscuro = nullptr; /**< Infográfico de ajuda (Modo Escuro). */
    
    Janela* popupJanela = nullptr;         /**< Sistema de popups informativos. */
    std::vector<Botao*> botoesNavegaveis;  /**< Cache de botões para navegação por controle. */

    // Constantes de Layout (Design System)
    
    // Categorias
    const int CATEGORIA_POS_Y = 930;
    const int CATEGORIA_LARGURA = 140;
    const int CATEGORIA_ALTURA = 80;
    const int CATEGORIA_ESPACAMENTO = 146;
    const int CATEGORIA_AREA_VISIVEL_INICIO = 275; /**< Limite esquerdo da área de categorias. */
    const int CATEGORIA_AREA_VISIVEL_FIM = 915;    /**< Limite direito da área de categorias. */
    const int POSICAO_BASE_X = 320;
    
    // Jogos
    const int JOGO_Y = 1080;
    const int JOGO_LARGURA = 384;
    const int JOGO_ALTURA = 328;
    const int JOGO_GAP = 76;
    
    const int QTD_JOGOS_LISTA = 8; /**< Número de jogos exibidos por vez no grid. */

    TecladoVirtual* tecladoVirtual = nullptr; /**< Instância do teclado para input via controle/mouse. */
};

} // namespace MeuProjeto

#endif // GERENCIAR_INTERFACE_HPP