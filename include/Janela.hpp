/**
 * @file Janela.hpp
 * @brief Definição da classe Janela.
 *
 * Este arquivo estabelece a interface para o gerenciamento de janelas modais (popups)
 * dentro da interface gráfica. A classe é projetada para exibir informações detalhadas 
 * de um jogo, como título, imagem de capa e descrição, utilizando o sistema de 
 * renderização da SDL2.
 */

#ifndef JANELA_HPP
#define JANELA_HPP

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <sstream>      
#include "Utils.hpp"  

namespace MeuProjeto {

/**
 * @class Janela
 * @brief Responsável pela criação e exibição de componentes modais informativos.
 *
 * A classe Janela atua como um container visual sobreposto à interface principal.
 * Ela gerencia o estado de visibilidade, armazena os metadados do conteúdo a ser
 * exibido e calcula as proporções visuais baseando-se em variáveis de layout 
 * responsivo para garantir a integridade estética em diferentes resoluções.
 */
class Janela {
public:
    /**
     * @brief Construtor da classe Janela.
     * 
     * Inicializa a instância da janela vinculando-a a um renderizador e configurando
     * os estados iniciais de visibilidade e dimensões.
     * 
     * @param renderer Ponteiro para o SDL_Renderer que será utilizado para a renderização do popup.
     */
    Janela(SDL_Renderer* renderer);

    /**
     * @brief Destrutor da classe Janela.
     * 
     * Realiza a limpeza de referências e garante que nenhum recurso pendente comprometa
     * a integridade da memória ao destruir a instância da janela.
     */
    ~Janela();

    /**
     * @brief Configura e ativa a exibição de uma janela popup.
     * 
     * Este método define as informações que serão renderizadas, incluindo metadados 
     * textuais e visuais, além de determinar a área de ocupação inicial.
     * 
     * @param nome String contendo o título ou nome do jogo a ser exibido.
     * @param textura Ponteiro para a textura SDL_Texture contendo a imagem de capa.
     * @param desc String contendo a descrição detalhada do conteúdo.
     * @param x Coordenada horizontal para o posicionamento da janela.
     * @param y Coordenada vertical para o posicionamento da janela.
     * @param w Largura inicial da janela.
     * @param h Altura inicial da janela.
     */
    void abrirPopup(const std::string& nome, SDL_Texture* textura, const std::string& desc, int x, int y, int w, int h);

    /**
     * @brief Desativa a visibilidade da janela popup.
     * 
     * Altera o estado interno para fechado, interrompendo o ciclo de renderização
     * do componente modal.
     */
    void fecharPopup();

    /**
     * @brief Realiza o desenho da janela e seus subcomponentes no renderizador.
     * 
     * Renderiza o fundo da janela, a imagem, o título e a descrição textual, 
     * aplicando os deslocamentos fornecidos.
     * 
     * @param offsetX Valor de deslocamento horizontal para ajuste de posição.
     * @param offsetY Valor de deslocamento vertical para ajuste de posição.
     */
    void desenharPopup(int offsetX = 0, int offsetY = 0);

    /**
     * @brief Consulta o estado de visibilidade da janela.
     * 
     * @return true se a janela estiver aberta e visível, false caso contrário.
     */
    bool isPopupOpen() const { return isPopupAberto; }

private:
    SDL_Renderer* renderer; /**< Ponteiro para o renderizador SDL associado à janela. */
    bool isPopupAberto;     /**< Estado booleano que indica se o popup está ativo. */
    SDL_Rect popupArea;     /**< Estrutura que define a geometria (posição e tamanho) da janela. */
    std::string popupNome;  /**< Armazena o texto do título exibido na janela. */
    SDL_Texture* popupTextura; /**< Referência para a textura da imagem principal do popup. */
    std::string popupDesc;  /**< Armazena o corpo de texto descritivo do popup. */

    int larguraPadrao;   /**< Dimensão horizontal base definida para o layout da janela. */
    int alturaPadrao;    /**< Dimensão vertical base definida para o layout da janela. */
    int paddingExterno;  /**< Espaçamento de segurança entre a borda da janela e o limite da tela. */
    int paddingInterno;  /**< Espaçamento interno entre a borda da janela e seus elementos filhos. */
    int alturaImagem;    /**< Dimensão vertical reservada exclusivamente para a exibição da imagem de capa. */
    int espacoTitulo;    /**< Margem vertical entre o término da imagem e o início do título. */
    int espacoDesc;      /**< Margem vertical entre o título e o bloco de texto de descrição. */
};

} // namespace MeuProjeto

#endif // JANELA_HPP