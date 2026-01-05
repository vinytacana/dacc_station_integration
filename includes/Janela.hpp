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
 * @brief Gerencia popups e interfaces de janela na aplicação
 * 
 * Esta classe fornece funcionalidades para criar, exibir e gerenciar
 * popups com informações, imagens e descrições textuais
 */
class Janela 
{
public:
    /**
     * @brief Construtor da classe Janela
     * @param renderer Renderizador SDL usado para desenhar a janela
     */
    Janela(SDL_Renderer* renderer);
    
    /**
     * @brief Destrutor da classe Janela
     */
    ~Janela();

    /**
     * @brief Abre um popup com informações específicas
     * @param nome Nome/título do popup
     * @param textura Textura SDL para exibir no popup
     * @param desc Descrição textual do conteúdo
     * @param x Posição X de referência para posicionamento
     * @param y Posição Y de referência para posicionamento
     * @param w Largura de referência para posicionamento
     * @param h Altura de referência para posicionamento
     */
    void abrirPopup(const std::string& nome, SDL_Texture* textura, const std::string& desc, int x, int y, int w, int h);
    
    /**
     * @brief Fecha o popup atualmente aberto
     */
    void fecharPopup();
    
    /**
     * @brief Desenha o popup na tela
     * @param offsetX Deslocamento horizontal para aplicação de câmera
     * @param offsetY Deslocamento vertical para aplicação de câmera
     */
    void desenharPopup(int offsetX = 0, int offsetY = 0);
    
    /**
     * @brief Verifica se há um popup aberto atualmente
     * @return true se um popup está aberto, false caso contrário
     */
    bool isPopupOpen() const { return isPopupAberto; }

private:
    SDL_Renderer* renderer;         ///< Renderizador SDL para desenho
    bool isPopupAberto;             ///< Flag indicando se um popup está aberto
    SDL_Rect popupArea;             ///< Área retangular do popup na tela
    std::string popupNome;          ///< Nome/título do popup atual
    SDL_Texture* popupTextura;      ///< Textura exibida no popup
    std::string popupDesc;          ///< Descrição textual do popup
};

} // namespace MeuProjeto

#endif // JANELA_HPP