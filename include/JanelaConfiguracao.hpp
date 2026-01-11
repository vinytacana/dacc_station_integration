/**
 * @file JanelaConfiguracao.hpp
 * @brief Definição da classe JanelaConfiguracao para gestão de definições do sistema.
 * 
 * Este arquivo contém a interface de usuário para o menu de configurações. 
 * Diferente de outros componentes, esta classe gerencia sua própria janela 
 * e renderizador SDL, permitindo que as configurações apareçam de forma independente.
 */

#ifndef JANELA_CONFIGURACAO_HPP
#define JANELA_CONFIGURACAO_HPP

#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <memory>
#include "Botao.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @enum SubmenuConfig
 * @brief Identificadores para as diferentes seções dentro do menu de configurações.
 */
enum class SubmenuConfig {
    NENHUM = -1,            /**< Estado inicial ou nenhum submenu selecionado. */
    ALTERAR_CONTROLE = 0,   /**< Seção de remapeamento ou teste de joysticks. */
    RESTAURAR_PADROES = 1,  /**< Opção para resetar todas as configurações. */
    SUPORTE_AJUDA = 2,      /**< Informações de contato ou FAQ. */
    CONFIG_AVANCADAS = 3    /**< Ajustes técnicos profundos do launcher. */
};

/**
 * @class JanelaConfiguracao
 * @brief Gerencia uma janela secundária para ajustes e preferências.
 * 
 * Esta classe encapsula a criação de uma nova janela SDL (`SDL_Window`), 
 * seu próprio ciclo de renderização e a lógica de navegação entre botões 
 * de submenus. Ela suporta navegação tanto por mouse quanto por controle.
 */
class JanelaConfiguracao {
public:
    /**
     * @brief Construtor da classe JanelaConfiguracao.
     * Inicializa os ponteiros como nulos e define o estado inicial como fechada.
     */
    JanelaConfiguracao();

    /**
     * @brief Destrutor da classe JanelaConfiguracao.
     * Garante a destruição correta da janela e do renderizador secundário, 
     * além de limpar a memória dos botões alocados.
     */
    ~JanelaConfiguracao();

    /**
     * @brief Cria a janela SDL e inicializa o contexto de renderização para as configurações.
     */
    void abrir();

    /**
     * @brief Fecha e destrói os recursos da janela de configuração, retornando o foco à principal.
     */
    void fechar();

    /**
     * @brief Processa eventos de input (teclado, mouse, controle) específicos desta janela.
     * 
     * @param evento Referência ao evento SDL capturado no loop principal.
     * @return true Se o evento resultou no fechamento da janela ou ação confirmada.
     * @return false Se o evento não gerou uma mudança de estado crítica.
     */
    bool processarEvento(SDL_Event& evento);

    /**
     * @brief Renderiza os elementos do menu de configuração (botões, textos e fundos).
     */
    void desenhar();
    
    /**
     * @brief Verifica se a janela de configurações está ativa no momento.
     * @return true se visível, false caso contrário.
     */
    bool estaAberta() const { return aberta; }

    /**
     * @brief Obtém o identificador único da janela SDL associada.
     * 
     * Essencial para o filtro de eventos da SDL em aplicações multi-janela, 
     * permitindo saber se um clique pertence a esta janela ou à principal.
     * 
     * @return Uint32 O ID da janela SDL.
     */
    Uint32 getIDJanela() const;

private:
    SDL_Window* janela = nullptr;    /**< Ponteiro para a janela SDL dedicada. */
    SDL_Renderer* renderer = nullptr; /**< Renderizador exclusivo para esta janela. */
    bool aberta = false;             /**< Flag de controle de estado de exibição. */
    
    SubmenuConfig submenuAtivo = SubmenuConfig::NENHUM; /**< Rastreia qual seção está sendo visualizada. */
    
    std::vector<std::unique_ptr<Botao>> botoesMenu; /**< Coleção de ponteiros para os botões do menu principal de config. */
    std::unique_ptr<Botao> btnVoltar;     /**< Botão para retornar ao menu anterior. */
    std::unique_ptr<Botao> btnFechar;     /**< Botão para encerrar a janela de configurações. */
    
    int indiceFocado = -1; /**< Índice do botão atualmente em foco (para navegação via controle). */

    // Controle de Input de Periféricos
    Uint32 ultimoInputAnalogico = 0;      /**< Timestamp para evitar movimentos ultra-rápidos no analógico. */
    const Uint32 INTERVALO_ANALOGICO = 200; /**< Delay em ms entre movimentos de navegação. */
    const int DEADZONE = 16000;           /**< Limiar de sensibilidade para eixos do joystick. */
    
    /**
     * @brief Cria e configura as posições e labels dos botões da interface.
     */
    void inicializarBotoes();

    /**
     * @brief Dispara a lógica associada ao clique/seleção de um item do menu.
     * @param indice Posição do botão no vetor botoesMenu.
     */
    void executarAcaoMenu(int indice);
};

} // namespace MeuProjeto

#endif // JANELA_CONFIGURACAO_HPP