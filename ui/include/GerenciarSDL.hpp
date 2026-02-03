/**
 * @file GerenciarSDL.hpp
 * @brief Definição da classe responsável pelo gerenciamento do ciclo de vida da SDL2.
 * 
 * Este arquivo contém a classe GerenciarSDL, que centraliza as operações de 
 * inicialização dos subsistemas da SDL (vídeo, áudio, controle), criação de 
 * janelas, renderizadores e a limpeza final de memória.
 */

#ifndef GERENCIAR_SDL_HPP
#define GERENCIAR_SDL_HPP

#pragma once

#include <SDL2/SDL.h>
#include <iostream>
#include <string>

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class GerenciarSDL
 * @brief Classe utilitária para controle de hardware e renderização via SDL2.
 * 
 * A GerenciarSDL encapsula as chamadas de baixo nível da SDL2, garantindo que
 * a interface gráfica tenha um ambiente pronto para desenhar elementos e 
 * processar entradas de periféricos. Por ser composta apenas por métodos estáticos,
 * não há necessidade de instanciá-la.
 */
class GerenciarSDL {
public:
    /**
     * @brief Inicializa os subsistemas SDL, bibliotecas de extensão e cria a janela principal.
     * 
     * Este método realiza a chamada de `SDL_Init` e inicializa componentes como:
     * - SDL_Image (para suporte a imagens PNG/JPG de capas de jogos).
     * - SDL_TTF (para renderização de textos e fontes).
     * - Criação da janela e do renderizador com aceleração por hardware.
     * 
     * @param[out] janela Referência para o ponteiro SDL_Window que será alocado.
     * @param[out] renderer Referência para o ponteiro SDL_Renderer que será alocado.
     * @param largura Largura desejada para a janela do launcher em pixels.
     * @param altura Altura desejada para a janela do launcher em pixels.
     * 
     * @return true Se todos os subsistemas, a janela e o renderizador foram criados com sucesso.
     * @return false Se ocorrer falha em qualquer uma das etapas de inicialização (erro pode ser verificado com SDL_GetError()).
     */
    static bool inicializar(SDL_Window*& janela, SDL_Renderer*& renderer, int largura, int altura);

    /**
     * @brief Exibe uma tela de verificação inicial de periféricos.
     * 
     * Este método entra em um loop de espera (bloqueante) que solicita ao usuário a 
     * conexão de um controle (Gamepad) ou interação inicial. É útil para garantir que 
     * o usuário tenha um meio de navegação antes de entrar na biblioteca de jogos.
     * 
     * @param janela Ponteiro para a janela onde a mensagem de verificação será exibida.
     * @param renderer Ponteiro para o renderizador para desenhar o estado da verificação.
     * 
     * @return true Se um controle compatível foi detectado ou a verificação foi superada.
     * @return false Se o usuário fechar a janela durante o processo ou ocorrer um erro crítico de hardware.
     */
    static bool verificarControle(SDL_Window* janela, SDL_Renderer* renderer);

    static void tocarVideoIntro(const std::string& caminhoVideo);
    
    /**
     * @brief Realiza a limpeza completa de recursos e encerra a execução da SDL2.
     * 
     * Este método deve ser chamado obrigatoriamente antes do encerramento do programa.
     * Ele destrói o renderizador, fecha a janela fornecida e chama as funções de 
     * encerramento (Quit) da SDL e suas extensões (IMG, TTF), liberando a memória do sistema.
     * 
     * @param janela Ponteiro para a janela SDL a ser destruída.
     * @param renderer Ponteiro para o renderizador SDL a ser destruído.
     */
    static void limpar(SDL_Window* janela, SDL_Renderer* renderer);
};

} // namespace MeuProjeto

#endif // GERENCIAR_SDL_HPP