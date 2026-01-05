/**
 * @file BotaoPesquisa.hpp
 * @brief Definição da classe BotaoPesquisa, uma barra de busca interativa com resultados dinâmicos.
 * 
 * Este arquivo define um componente de UI especializado que funciona como um campo de texto
 * para busca de jogos, exibindo uma lista de resultados (pop-up) conforme o usuário digita.
 */

#ifndef BOTAOPESQUISA_HPP
#define BOTAOPESQUISA_HPP

#pragma once

#include "Botao.hpp"
#include "Utils.hpp"
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "Jogo.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class BotaoPesquisa
 * @brief Especialização da classe Botao para busca e filtragem de jogos.
 * 
 * A classe BotaoPesquisa gerencia o estado de digitação, a consulta ao banco de dados 
 * (GerenciadorJogos) e a exibição de uma lista de sugestões. Ela suporta navegação
 * por mouse (hover e clique nos resultados) e por controle (navegação direcional 
 * na lista de resultados).
 */
class BotaoPesquisa : public Botao {
public:
    /**
     * @brief Construtor da barra de pesquisa.
     * 
     * @param renderer Renderizador SDL para criação de texturas e desenho.
     * @param x Posição X inicial na tela.
     * @param y Posição Y inicial na tela.
     * @param largura Largura do campo de busca.
     * @param altura Altura do campo de busca.
     * @param texto Texto inicial opcional (geralmente vazio).
     */
    BotaoPesquisa(SDL_Renderer* renderer, int x, int y, int largura, int altura, const std::string& texto = "");
    
    /**
     * @brief Destrutor da classe BotaoPesquisa.
     */
    ~BotaoPesquisa();

    /**
     * @brief Trata eventos de mouse e teclado para a barra de pesquisa.
     * 
     * Gerencia o foco do campo de texto, a captura de caracteres digitados e
     * a seleção de itens na lista de resultados.
     * 
     * @param evento Referência ao evento SDL.
     * @param offsetX Deslocamento horizontal da tela.
     * @param offsetY Deslocamento vertical da tela.
     * @return true se o evento foi consumido pelo componente.
     */
    bool tratarEvento(SDL_Event& evento, int offsetX, int offsetY) override;

    /**
     * @brief Renderiza a barra de pesquisa e sua lista de resultados suspensa.
     * 
     * @param renderer Renderizador SDL.
     * @param offsetX Deslocamento X para desenho relativo.
     * @param offsetY Deslocamento Y para desenho relativo.
     */
    void desenhar(SDL_Renderer* renderer, int offsetX, int offsetY) override;

    // Manipulação de Texto

    /**
     * @brief Adiciona um caractere ou string ao campo de busca atual.
     * @param txt Texto a ser concatenado.
     */
    void adicionarTexto(const std::string& txt);

    /**
     * @brief Remove o último caractere do campo de busca (Backspace).
     */
    void apagarTexto();

    /**
     * @brief Limpa completamente o campo de busca.
     */
    void limparTexto();

    // Lógica de Mouse

    /**
     * @brief Recupera o jogo que foi selecionado via clique de mouse na lista.
     * @return Jogo* Ponteiro para o objeto Jogo selecionado ou nullptr.
     */
    Jogo* getJogoClicado() const { return jogoClicado; }

    /**
     * @brief Reseta o ponteiro de seleção de clique, limpando a ação pendente.
     */
    void resetarJogoClicado() { jogoClicado = nullptr; }

    // Navegação de Resultados (Controle/Gamepad)

    /**
     * @brief Navega entre os resultados da lista suspensa usando o controle.
     * @param direcao Valor positivo (baixo) ou negativo (cima) para mudar o foco.
     * @return true se a navegação ocorreu com sucesso.
     */
    bool navegarResultados(int direcao); 

    /**
     * @brief Verifica se há algum item da lista de resultados com foco (seleção via controle).
     * @return true se algum resultado estiver destacado.
     */
    bool temResultadoFocado() const;

    /**
     * @brief Retorna o jogo selecionado atualmente através das setas do controle.
     * @return Jogo* Ponteiro para o jogo focado.
     */
    Jogo* getJogoResultadoFocado();

    /**
     * @brief Remove o destaque de seleção dos resultados.
     */
    void resetarFocoResultados();

    /**
     * @brief Verifica se o componente está solicitando a abertura do teclado virtual.
     * @return true se o teclado virtual deve ser exibido.
     */
    bool getSolicitaTeclado() { return solicitaTeclado; }

    /**
     * @brief Reseta a flag de solicitação do teclado virtual após o processamento.
     */
    void resetarSolicitacaoTeclado() { solicitaTeclado = false; }

    // Configurações de Fontes

    /** @brief Define o estilo da fonte do placeholder (ex: "Pesquisar..."). */
    void setFontePlaceholder(TipoFonte tipo) { fontePlaceholder = tipo; }
    
    /** @brief Define o tamanho da fonte do placeholder. */
    void setTamanhoFontePlaceholder(int tamanho) { tamanhoPlaceholder = tamanho; }
    
    /** @brief Define o estilo da fonte para os nomes dos jogos nos resultados. */
    void setFonteResultados(TipoFonte tipo) { fonteResultados = tipo; }
    
    /** @brief Define o tamanho da fonte dos nomes dos jogos nos resultados. */
    void setTamanhoFonteResultados(int tamanho) { tamanhoResultados = tamanho; }

private:
    SDL_Renderer* renderer;       /**< Ponteiro para o renderizador associado. */
    std::string textoInput;       /**< Buffer contendo o texto digitado pelo usuário. */
    bool inputAtivo;              /**< Indica se o campo está em modo de edição (focado). */
    
    std::vector<Jogo> resultadosAtuais; /**< Lista de jogos que condizem com a busca atual. */
    
    // Variáveis de Estado de Interação
    int indiceFocoResultado = -1; /**< Índice do item selecionado via controle (-1 = nenhum). */
    int resultadoHoverIndex = -1; /**< Índice do item sob o cursor do mouse. */
    Jogo* jogoClicado = nullptr;  /**< Armazena o jogo clicado para processamento posterior. */

    // Customização Visual
    TipoFonte fontePlaceholder = TipoFonte::NORMAL;
    int tamanhoPlaceholder = 28;
    TipoFonte fonteResultados = TipoFonte::NEGRITO;
    int tamanhoResultados = 32;

    // Constantes de Layout da Lista Suspensa
    const int ITEM_HEIGHT = 120;     /**< Altura de cada linha de resultado. */
    const int ITEM_PADDING = 6;      /**< Espaçamento interno entre itens. */
    const int IMG_SIZE = 90;         /**< Tamanho da miniatura da capa no resultado. */
    const int IMG_OFFSET_Y = (ITEM_HEIGHT - IMG_SIZE) / 2; /**< Centralização vertical da imagem. */
    const int IMG_OFFSET_X = 15;     /**< Margem esquerda da imagem. */
    const int TEXT_OFFSET_X = IMG_OFFSET_X + IMG_SIZE + 20; /**< Posição X onde o texto começa. */
    const int MAX_RESULTADOS = 3;    /**< Limite máximo de jogos exibidos na lista suspensa. */
    bool solicitaTeclado = false;    /**< Flag para ativação do teclado virtual (UI). */

    /**
     * @brief Renderiza a caixa contendo os resultados da busca.
     * @param adjustedArea Retângulo da área da barra de pesquisa para cálculo de posição.
     */
    void desenharResultados(const SDL_Rect& adjustedArea);

    /**
     * @brief Atualiza a lista resultadosAtuais consultando o GerenciadorJogos.
     * Chamada sempre que o textoInput é alterado.
     */
    void atualizarResultados();
};

} // namespace MeuProjeto

#endif // BOTAOPESQUISA_HPP