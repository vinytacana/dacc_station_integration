/**
 * @file Janela.hpp
 * @brief Definição da classe Janela, responsável pelo gerenciamento de interfaces sobrepostas (popups).
 * 
 * Este arquivo define os mecanismos para exibir janelas modais ou informativas
 * sobre a interface principal, permitindo a visualização de detalhes de um jogo
 * sem sair da tela de navegação.
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

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class Janela
 * @brief Gerencia a exibição e o estado de janelas de popup na interface.
 * 
 * A classe Janela permite configurar, abrir e desenhar elementos de interface
 * que flutuam sobre a grade de jogos. Ela armazena o estado atual do popup,
 * como a textura da imagem do jogo, o nome e a descrição técnica.
 */
class Janela {
public:
    /**
     * @brief Construtor da classe Janela.
     * @param renderer Ponteiro para o renderizador SDL que será usado para desenhar o popup.
     */
    Janela(SDL_Renderer* renderer);

    /**
     * @brief Destrutor da classe Janela.
     * Limpa referências e garante que o estado do popup seja encerrado corretamente.
     */
    ~Janela();

    /**
     * @brief Configura e ativa a exibição de um popup com informações de um jogo.
     * 
     * @param nome Nome do jogo a ser exibido no título do popup.
     * @param textura Ponteiro para a SDL_Texture contendo a imagem (capa) do jogo.
     * @param desc Texto descritivo ou detalhes do jogo.
     * @param x Coordenada X da posição inicial do popup.
     * @param y Coordenada Y da posição inicial do popup.
     * @param w Largura (width) da janela de popup.
     * @param h Altura (height) da janela de popup.
     */
    void abrirPopup(const std::string& nome, SDL_Texture* textura, const std::string& desc, int x, int y, int w, int h);

    /**
     * @brief Desativa o popup, parando sua renderização e limpando o estado de exibição.
     */
    void fecharPopup();

    /**
     * @brief Renderiza o popup na tela utilizando o renderizador associado.
     * 
     * Este método deve ser chamado dentro do loop principal de renderização, 
     * preferencialmente após o desenho da biblioteca de fundo.
     * 
     * @param offsetX Deslocamento horizontal opcional para animações ou ajustes de layout.
     * @param offsetY Deslocamento vertical opcional para animações ou ajustes de layout.
     */
    void desenharPopup(int offsetX = 0, int offsetY = 0);

    /**
     * @brief Verifica se existe um popup ativo no momento.
     * @return true se o popup estiver visível, false caso contrário.
     */
    bool isPopupOpen() const { return isPopupAberto; }

private:
    SDL_Renderer* renderer;  /**< Referência ao renderizador principal da aplicação. */
    bool isPopupAberto;      /**< Flag de controle de estado: indica se o popup deve ser desenhado. */
    SDL_Rect popupArea;      /**< Estrutura que define a geometria (pos x, pos y, largura, altura) do popup. */
    std::string popupNome;   /**< Nome armazenado para exibição no popup atual. */
    SDL_Texture* popupTextura; /**< Textura da imagem do jogo sendo exibida no popup. */
    std::string popupDesc;   /**< Descrição armazenada para exibição no popup atual. */

};

} // namespace MeuProjeto

#endif // JANELA_HPP