/**
 * @file Janela.hpp
 * @brief Definição da classe Janela (Popup simplificado estilo Overlay).
 */

#ifndef JANELA_HPP
#define JANELA_HPP

#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include "Utils.hpp"  

namespace MeuProjeto {

/**
 * @class Janela
 * @brief Classe responsável por gerenciar popups estilo overlay na interface
 * 
 * Esta classe implementa um sistema de popup simplificado que aparece como uma
 * camada sobreposta (overlay) sobre outros elementos da interface. O popup exibe
 * informações contextuais como nome e descrição de itens, tipicamente aparecendo
 * sobre botões ou elementos interativos quando selecionados.
 * 
 * O popup possui fundo translúcido e é posicionado dinamicamente de acordo com
 * o elemento que o disparou.
 */
class Janela {
public:
    /**
     * @brief Construtor da classe Janela
     * 
     * Inicializa o sistema de popup com as configurações padrão e prepara
     * o renderer para desenhar os elementos de overlay.
     * 
     * @param renderer Ponteiro para o SDL_Renderer usado para renderizar o popup
     */
    Janela(SDL_Renderer* renderer);
    
    /**
     * @brief Destrutor da classe Janela
     * 
     * Libera recursos alocados e fecha qualquer popup que ainda esteja aberto.
     */
    ~Janela();

    /**
     * @brief Abre o popup estilo overlay com informações específicas
     * 
     * Exibe um popup translúcido sobre um elemento da interface, mostrando
     * nome e descrição do item. O popup é posicionado nas coordenadas especificadas
     * e dimensionado conforme os parâmetros fornecidos. Cria um fundo semitransparente
     * que sobrepõe o elemento subjacente, mantendo-o visível mas destacando o popup.
     * 
     * @param nome Título ou nome do item a ser exibido no topo do popup
     * @param desc Descrição detalhada do item, exibida abaixo do nome
     * @param x Coordenada X (horizontal) da posição do popup na tela
     * @param y Coordenada Y (vertical) da posição do popup na tela
     * @param w Largura do popup em pixels
     * @param h Altura do popup em pixels
     */
    void abrirPopup(const std::string& nome, const std::string& desc, 
                    int x, int y, int w, int h);
    
    /**
     * @brief Fecha o popup atualmente aberto
     * 
     * Remove o popup da tela e marca o estado como fechado. Após esta chamada,
     * o popup não será mais renderizado até que abrirPopup() seja chamado novamente.
     */
    void fecharPopup();
    
    /**
     * @brief Renderiza o popup na tela se estiver aberto
     * 
     * Desenha o popup com todas as suas informações (fundo translúcido, nome e descrição)
     * na posição definida. Este método deve ser chamado a cada frame no loop de renderização
     * principal. Os parâmetros de scroll permitem que o popup se mova junto com a câmera
     * ou área de visualização em interfaces com rolagem.
     * 
     * @param scrollX Deslocamento horizontal da câmera/viewport em pixels (para compensar scroll)
     * @param scrollY Deslocamento vertical da câmera/viewport em pixels (para compensar scroll)
     */
    void desenharPopup(int scrollX, int scrollY);

    /**
     * @brief Verifica se o popup está atualmente aberto
     * 
     * Método inline para consulta rápida do estado do popup. Útil para lógica
     * condicional que precisa saber se um popup está sendo exibido antes de
     * processar entrada do usuário ou renderizar outros elementos.
     * 
     * @return true se o popup está aberto e sendo exibido, false caso contrário
     */
    bool isPopupOpen() const { return isPopupAberto; }

private:
    /**
     * @brief Ponteiro para o renderer SDL usado para desenhar o popup
     */
    SDL_Renderer* renderer;
    
    /**
     * @brief Flag indicando se o popup está atualmente aberto/visível
     */
    bool isPopupAberto;
    
    /**
     * @brief Retângulo definindo a área do popup (posição e dimensões)
     * 
     * Contém as coordenadas x, y e dimensões w, h do popup na tela.
     */
    SDL_Rect popupArea;     
    
    /**
     * @brief Nome ou título exibido no popup
     * 
     * Armazena o texto do título que aparece na parte superior do popup.
     */
    std::string popupNome;
    
    /**
     * @brief Descrição detalhada exibida no popup
     * 
     * Armazena o texto da descrição que aparece abaixo do título no popup.
     */
    std::string popupDesc;
    
    /**
     * @brief Espaçamento interno (padding) das bordas do popup
     * 
     * Define a distância em pixels entre o conteúdo do popup e suas bordas,
     * criando margem interna para melhor legibilidade.
     */
    int paddingInterno;
    
    /**
     * @brief Espaçamento vertical entre o nome e a descrição
     * 
     * Define quantos pixels de distância devem separar o texto do nome
     * do texto da descrição dentro do popup.
     */
    int espacoDesc;
};

} // namespace MeuProjeto

#endif // JANELA_HPP