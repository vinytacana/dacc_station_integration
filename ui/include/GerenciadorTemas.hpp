/**
 * @file GerenciadorTemas.hpp
 * @brief Definição da classe GerenciadorTemas para controle de identidade visual.
 * 
 * Este arquivo define o sistema de temas da aplicação, permitindo a gestão 
 * centralizada de paletas de cores (clara e escura) para todos os elementos 
 * da interface gráfica.
 */

#ifndef GERENCIADORTEMAS_HPP
#define GERENCIADORTEMAS_HPP

#pragma once

#include <SDL2/SDL.h>

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @enum TipoTema
 * @brief Define os tipos de temas visuais disponíveis no sistema.
 */
enum class TipoTema {
    ESCURO, /**< Tema com tons profundos e escuros (padrão). */
    CLARO   /**< Tema com tons vibrantes e claros. */
};

/**
 * @class GerenciadorTemas
 * @brief Gerenciador global de cores e estilos da interface (Singleton).
 * 
 * A classe GerenciadorTemas centraliza a definição de cores de todos os componentes
 * (botões, textos, painéis). Ao utilizar o padrão Singleton, ela garante que
 * qualquer alteração de tema seja refletida instantaneamente em toda a aplicação,
 * pois todos os componentes consultam esta instância única para obter suas cores.
 */
class GerenciadorTemas {
public:
    /**
     * @brief Acessa a instância única do Gerenciador de Temas.
     * 
     * Implementação do padrão Singleton para garantir que não existam paletas 
     * de cores conflitantes em diferentes partes do programa.
     * 
     * @return Referência para a instância única do GerenciadorTemas.
     */
    static GerenciadorTemas& getInstance();

    /**
     * @brief Alterna o tema atual entre Claro e Escuro.
     * 
     * Se o tema atual for ESCURO, muda para CLARO, e vice-versa. 
     * Útil para mapear a um botão de "trocar modo" na interface.
     */
    void alternarTema();
    
    /**
     * @brief Define um tema específico para a aplicação.
     * @param tema O estilo desejado (TipoTema::CLARO ou TipoTema::ESCURO).
     */
    void setTema(TipoTema tema);
    
    /**
     * @brief Retorna o identificador do tema que está sendo usado no momento.
     * @return O tipo de tema atual.
     */
    TipoTema getTemaAtual() const;

    // Getters de Cores (Retornam a cor baseada no tema atual)
    
    /** 
     * @brief Retorna a cor de preenchimento do fundo da janela principal. 
     * @return Estrutura SDL_Color com os valores RGBA correspondentes ao tema.
     */
    SDL_Color getCorFundo() const;
    
    /** 
     * @brief Retorna a cor usada em painéis, janelas flutuantes e containers.
     * @return Estrutura SDL_Color para fundos de seções.
     */
    SDL_Color getCorRetangulos() const;
    
    /** 
     * @brief Retorna a cor para textos de destaque ou títulos (Negrito).
     * @return Estrutura SDL_Color (geralmente alto contraste).
     */
    SDL_Color getCorTextoNegrito() const;
    
    /** 
     * @brief Retorna a cor para textos comuns, descrições ou placeholders.
     * @return Estrutura SDL_Color (geralmente um tom mais suave que o negrito).
     */
    SDL_Color getCorTextoNormal() const;
    
    /** 
     * @brief Retorna a cor base de um botão em estado normal.
     * @return Estrutura SDL_Color.
     */
    SDL_Color getCorBotaoNormal() const;

    /** 
     * @brief Retorna a cor de um botão quando o cursor está sobre ele.
     * @return Estrutura SDL_Color para efeito de hover.
     */
    SDL_Color getCorBotaoHover() const;

    /** 
     * @brief Retorna a cor de um botão no momento em que é clicado/pressionado.
     * @return Estrutura SDL_Color para feedback tátil visual.
     */
    SDL_Color getCorBotaoPressionado() const;
    
    /** 
     * @brief Retorna a cor usada para bordas de foco e destaques de seleção.
     * 
     * Essencial para navegação via controle (joystick), indicando visualmente
     * qual elemento está selecionado.
     * 
     * @return Estrutura SDL_Color vibrante.
     */
    SDL_Color getCorDestaque() const;

private:
    /**
     * @brief Construtor privado para impedir instanciação externa (Singleton).
     */
    GerenciadorTemas(); 
    
    TipoTema temaAtual; /**< Armazena o estado atual do tema do sistema. */

    // Paleta Modo Escuro
    const SDL_Color escuroFundo = {21, 3, 25, 255};       /**< Fundo roxo escuro profundo. */
    const SDL_Color escuroRetangulos = {39, 6, 46, 255};   /**< Tom para painéis internos. */

    const SDL_Color escuroTextoNegrito = {255, 255, 255, 255}; /**< Branco puro para visibilidade. */
    const SDL_Color escuroTextoNormal = {255, 255, 255, 255};   /**< Branco puro para visibilidade. */

    const SDL_Color escuroBtnNormal = {56, 9, 66, 255};        /**< Botão padrão escuro. */
    const SDL_Color escuroBtnHover = {73, 12, 87, 255};         /**< Realce ao passar o mouse. */
    const SDL_Color escuroBtnPressionado = {90, 15, 107, 255};  /**< Feedback de clique. */

    // Paleta Modo Claro
    const SDL_Color claroFundo = {129, 82, 152, 255};          /**< Roxo médio vibrante. */
    const SDL_Color claroRetangulos = {150, 104, 173, 255};    /**< Painéis mais claros. */

    const SDL_Color claroTextoNegrito = {255, 255, 255, 255};  /**< Branco puro para visibilidade. */
    const SDL_Color claroTextoNormal = {255, 255, 255, 255};   /**< Branco puro para visibilidade. */

    const SDL_Color claroBtnNormal = {172, 127, 193, 255};     /**< Botão base no modo claro. */
    const SDL_Color claroBtnHover = {193, 149, 214, 255};      /**< Realce claro. */
    const SDL_Color claroBtnPressionado = {215, 171, 235, 255}; /**< Feedback de clique claro. */
};

} // namespace MeuProjeto

#endif // GERENCIADORTEMAS_HPP