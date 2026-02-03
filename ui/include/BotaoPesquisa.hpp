/**
 * @file BotaoPesquisa.hpp
 * @brief Definição da classe BotaoPesquisa.
 *
 * Este arquivo contém a especificação da classe BotaoPesquisa, que integra
 * funcionalidades de entrada de texto e exibição dinâmica de resultados de busca
 * para a interface da biblioteca de jogos, utilizando SDL2.
 */

#ifndef BOTAOPESQUISA_HPP
#define BOTAOPESQUISA_HPP

#pragma once

#include "Botao.hpp"
#include "Utils.hpp"
#include "Jogo.hpp"
#include <SDL2/SDL.h>
#include <string>
#include <vector>

namespace MeuProjeto {

/**
 * @class BotaoPesquisa
 * @brief Componente de interface para pesquisa de jogos com suporte a layout responsivo.
 *
 * A classe BotaoPesquisa herda de Botao e implementa uma caixa de busca interativa.
 * Além da captura de texto, ela gerencia uma lista de resultados (objetos Jogo),
 * lida com estados de foco, interação via teclado e renderização de uma interface
 * de sugestões suspensa (dropdown).
 */
class BotaoPesquisa : public Botao {
public:
    /**
     * @brief Construtor da classe BotaoPesquisa.
     * 
     * @param renderer Ponteiro para o renderizador SDL_Renderer utilizado para desenhar o componente.
     * @param x Coordenada horizontal da posição inicial.
     * @param y Coordenada vertical da posição inicial.
     * @param largura Dimensão horizontal inicial do componente.
     * @param altura Dimensão vertical inicial do componente.
     * @param texto String inicial opcional a ser exibida na caixa de pesquisa.
     */
    BotaoPesquisa(SDL_Renderer* renderer, int x, int y, int largura, int altura, const std::string& texto = "");

    /**
     * @brief Destrutor da classe BotaoPesquisa.
     * 
     * Realiza a liberação de recursos alocados internamente pela instância.
     */
    ~BotaoPesquisa();

    /**
     * @brief Processa eventos de entrada do SDL para a caixa de pesquisa.
     * 
     * Trata interações como cliques do mouse, pressionamento de teclas para entrada de texto
     * e navegação pelos resultados.
     * 
     * @param evento Referência para a estrutura SDL_Event contendo o evento capturado.
     * @param offsetX Deslocamento horizontal aplicado à posição do componente.
     * @param offsetY Deslocamento vertical aplicado à posição do componente.
     * @return true se o evento foi consumido pelo componente, false caso contrário.
     */
    bool tratarEvento(SDL_Event& evento, int offsetX, int offsetY) override;

    /**
     * @brief Renderiza o componente na tela.
     * 
     * Desenha a caixa de entrada de texto e, se houver resultados ativos, a lista
     * de sugestões suspensa.
     * 
     * @param renderer Ponteiro para o renderizador SDL_Renderer.
     * @param offsetX Deslocamento horizontal para renderização.
     * @param offsetY Deslocamento vertical para renderização.
     */
    void desenhar(SDL_Renderer* renderer, int offsetX, int offsetY) override;

    /**
     * @brief Adiciona caracteres ao texto de entrada atual.
     * 
     * @param txt String contendo os caracteres a serem concatenados ao texto de pesquisa.
     */
    virtual void adicionarTexto(const std::string& txt);

    /**
     * @brief Remove o último caractere do texto de entrada atual (Backspace).
     */
    virtual void apagarTexto();

    /**
     * @brief Limpa integralmente o conteúdo do texto de entrada.
     */
    virtual void limparTexto();

    /**
     * @brief Obtém o ponteiro para o jogo que foi selecionado via clique.
     * 
     * @return Ponteiro para o objeto Jogo selecionado, ou nullptr se nenhum jogo foi clicado.
     */
    Jogo* getJogoClicado() const { return jogoClicado; }

    /**
     * @brief Reseta o estado do jogo clicado para nulo.
     */
    void resetarJogoClicado() { jogoClicado = nullptr; }

    /**
     * @brief Realiza a navegação entre os resultados da pesquisa via teclado.
     * 
     * @param direcao Valor inteiro indicando a direção da navegação (ex: 1 para baixo, -1 para cima).
     * @return true se a navegação foi realizada com sucesso, false caso contrário.
     */
    bool navegarResultados(int direcao); 

    /**
     * @brief Verifica se algum resultado da lista possui o foco atual.
     * 
     * @return true se houver um item focado, false caso contrário.
     */
    bool temResultadoFocado() const;

    /**
     * @brief Obtém o ponteiro para o jogo que está atualmente com foco de navegação.
     * 
     * @return Ponteiro para o objeto Jogo focado.
     */
    Jogo* getJogoResultadoFocado();

    /**
     * @brief Reseta o índice de foco dos resultados para o estado inicial (sem foco).
     */
    virtual void resetarFocoResultados();

    /**
     * @brief Verifica se o componente está solicitando a abertura do teclado (interação).
     * 
     * @return true se houver solicitação ativa, false caso contrário.
     */
    bool getSolicitaTeclado() { return solicitaTeclado; }

    /**
     * @brief Reseta o sinalizador de solicitação de teclado.
     */
    virtual void resetarSolicitacaoTeclado() { solicitaTeclado = false; }

    /**
     * @brief Define o tipo de fonte utilizado para o texto de placeholder.
     * 
     * @param tipo Valor da enumeração TipoFonte.
     */
    void setFontePlaceholder(TipoFonte tipo) { fontePlaceholder = tipo; }

    /**
     * @brief Define o tamanho da fonte utilizado para o texto de placeholder.
     * 
     * @param tamanho Valor inteiro representando o tamanho da fonte.
     */
    void setTamanhoFontePlaceholder(int tamanho) { tamanhoPlaceholder = tamanho; }

    /**
     * @brief Define o tipo de fonte utilizado na exibição dos resultados da busca.
     * 
     * @param tipo Valor da enumeração TipoFonte.
     */
    void setFonteResultados(TipoFonte tipo) { fonteResultados = tipo; }

    /**
     * @brief Fecha a pesquisa: para de receber texto e limpa a lista visual de resultados.
     * O texto digitado é mantido.
     */
    virtual void cancelarBusca();

    /**
     * @brief Define o tamanho da fonte utilizado na exibição dos resultados da busca.
     * 
     * @param tamanho Valor inteiro representando o tamanho da fonte.
     */
    void setTamanhoFonteResultados(int tamanho) { tamanhoResultados = tamanho; }

    /**
     * @brief Verifica se há resultados sendo exibidos (dropdown aberto).
     */
    bool temResultadosVisiveis() const { return !resultadosAtuais.empty(); }

private:
    SDL_Renderer* renderer; /**< Ponteiro para o renderizador SDL. */
    std::string textoInput; /**< Armazena a string de texto digitada pelo usuário. */
    bool inputAtivo;        /**< Indica se a caixa de pesquisa está em estado de edição ativa. */
    
    std::vector<Jogo> resultadosAtuais; /**< Vetor contendo os objetos Jogo filtrados pela pesquisa. */
    
    int indiceFocoResultado = -1; /**< Índice do item que possui foco de navegação via teclado. */
    int resultadoHoverIndex = -1; /**< Índice do item que possui foco de mouse (hover). */
    Jogo* jogoClicado = nullptr;  /**< Ponteiro para o último jogo que sofreu uma ação de clique. */

    TipoFonte fontePlaceholder = TipoFonte::NORMAL; /**< Estilo da fonte para o placeholder. */
    int tamanhoPlaceholder; /**< Dimensão da fonte para o placeholder. */
    TipoFonte fonteResultados = TipoFonte::NEGRITO; /**< Estilo da fonte para os itens de resultado. */
    int tamanhoResultados;  /**< Dimensão da fonte para os itens de resultado. */

    int itemHeight;    /**< Altura individual de cada item na lista de resultados. */
    int itemPadding;   /**< Espaçamento interno entre os elementos de um item de resultado. */
    int imgSize;       /**< Dimensão da imagem (ícone) exibida nos resultados. */
    int imgOffsetY;    /**< Deslocamento vertical da imagem dentro do item. */
    int imgOffsetX;    /**< Deslocamento horizontal da imagem dentro do item. */
    int textOffsetX;   /**< Deslocamento horizontal do texto em relação à imagem. */
    int raioBorda;     /**< Raio de arredondamento da borda da caixa de pesquisa. */
    int raioDestaque;  /**< Raio de arredondamento para o destaque de seleção/foco. */
    
    /**
     * @brief Quantidade máxima de resultados exibidos simultaneamente no dropdown.
     */
    static constexpr int MAX_RESULTADOS = 3;
    
    bool solicitaTeclado = false; /**< Sinalizador para indicar necessidade de entrada de texto externa. */

    /**
     * @brief Realiza o desenho técnico da lista de resultados suspensa.
     * 
     * @param adjustedArea Área retangular calculada e ajustada para a exibição dos resultados.
     */
    void desenharResultados(const SDL_Rect& adjustedArea);

    /**
     * @brief Atualiza a lista interna de resultados baseando-se no textoInput atual.
     * 
     * Este método filtra os jogos disponíveis e atualiza o vetor resultadosAtuais.
     */
    void atualizarResultados();
};

} // namespace MeuProjeto

#endif // BOTAOPESQUISA_HPP