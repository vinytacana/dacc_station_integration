/**
 * @file GerenciarScroll.hpp
 * @brief Definição da estrutura GerenciarScroll para controle de estado e navegação.
 * 
 * Este arquivo contém a estrutura que centraliza o estado da interface gráfica,
 * incluindo coordenadas de rolagem, índices de seleção de categorias, controle
 * de janelas de jogos e gerenciamento de tempo para entradas (input debounce).
 */

#ifndef GERENCIAR_SCROLL_HPP
#define GERENCIAR_SCROLL_HPP

#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <memory>
#include "JanelaJogo.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @struct GerenciarScroll
 * @brief Centraliza o estado de navegação, rolagem e visualização da biblioteca.
 * 
 * GerenciarScroll funciona como um "State Manager". Ela armazena onde o usuário
 * está na interface, qual jogo ou categoria está focado e controla a lógica
 * de janelas sobrepostas (popups de detalhes de jogos).
 */


   /** 
     * @brief Gerenciamento inteligente da janela de detalhes.
     * O uso de unique_ptr garante que ao abrir uma nova janela ou fechar o app,
     * a memória da janela anterior seja limpa automaticamente.
     */
    extern std::unique_ptr<JanelaJogo> janelaJogoAtual; 


struct GerenciarScroll {
    // Estado Geral do Sistema
    
    /** @brief Define se a aplicação deve continuar em execução. */
    bool rodando = true;
    
    /** @brief Indica se o menu de configurações está sobreposto à tela principal. */
    bool configAberta = false;
    
    // Controle de Scroll e Câmera
    
    /** @brief Deslocamento horizontal global da visualização (em pixels). */
    int scrollX = 0;
    
    /** @brief Deslocamento vertical global da visualização (em pixels). */
    int scrollY = 0;
    
    // Estado das Categorias
    
    /** @brief Índice da primeira categoria visível na barra de navegação horizontal. */
    int categoriaIndex = 0;        
    
    /** @brief Índice da categoria que está filtrando os jogos no momento (-1 para nenhuma). */
    int categoriaAtivaIndex = -1;  
    
    /** @brief Índice da categoria atualmente destacada pelo cursor do controle (gamepad). */
    int categoriaFocadaPorGatilho = -1; 
    
    /** @brief Nome da categoria selecionada (Ex: "Ação", "Aventura", "Todas"). */
    std::string categoriaAtual = "Todas";
    
    /** @brief Limite máximo de categorias renderizadas simultaneamente na tela. */
    const int maxCategoriasVisiveis = 4;
    
    // Estado dos Destaques (Banners Principais)
    
    /** @brief Índice da imagem de destaque (banner) exibida atualmente. */
    int currentDestaqueIndex = 0;
    
    /** @brief Índice do banner que está em foco pelo controle. */
    int destaqueFocadoPorGatilho = -1;
    
    /** @brief Lista de caminhos de arquivos para as imagens da seção de destaques. */
    std::vector<std::string> imagensDestaques; 

    /** @brief Caminho para a imagem de fundo atual da interface. */
    std::string backgroundImagePath;
    
    // Janelas e Popups
    
    /** 
     * @brief Ponteiro para a janela de detalhes do jogo selecionado.
     * Se for nullptr, nenhuma janela de jogo está aberta.
     */
    JanelaJogo* janelaJogoAtual = nullptr;
    
    // Controle de Input e Tempo
    
    /** @brief Armazena o timestamp (SDL_GetTicks) da última entrada processada. */
    Uint32 ultimoTempoInput = 0;
    
    /**
     * @brief Reseta as coordenadas de rolagem para a origem (0,0).
     */
    void resetarScroll() {
        scrollX = 0;
        scrollY = 0;
    }

    /**
     * @brief Verifica se um novo input pode ser processado com base em um intervalo.
     * 
     * Implementa uma lógica de "debounce" ou "cooldown". Essencial para evitar que
     * um toque no D-Pad do controle mova a seleção por várias categorias de uma vez.
     * 
     * @param intervalo Tempo de espera necessário em milissegundos.
     * @return true se o tempo decorrido desde o último input for maior ou igual ao intervalo.
     * @return false se o input deve ser ignorado para evitar repetição indesejada.
     */
    bool podeProcessarInput(Uint32 intervalo) {
        Uint32 tempoAtual = SDL_GetTicks();
        if (tempoAtual - ultimoTempoInput >= intervalo) {
            ultimoTempoInput = tempoAtual;
            return true;
        }
        return false;
    }
};

} // namespace MeuProjeto

#endif // GERENCIAR_SCROLL_HPP