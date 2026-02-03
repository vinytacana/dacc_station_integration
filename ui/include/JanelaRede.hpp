/**
 * @file JanelaRede.hpp
 * @brief Definição da classe JanelaRede para gerenciamento de configurações de rede.
 * 
 * Este arquivo contém a interface para o submenu de configurações de rede,
 * incluindo controle de Wi-Fi, listagem de redes disponíveis e gerenciamento
 * de conexões.
 */

#ifndef JANELA_REDE_HPP
#define JANELA_REDE_HPP

#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <memory>
#include <string>
#include "Botao.hpp"
#include "TecladoVirtual.hpp"
#include "ConfigLayout.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @struct RedeInfo
 * @brief Estrutura para armazenar informações sobre uma rede Wi-Fi.
 */
struct RedeInfo {
    std::string nome;      /**< Nome (SSID) da rede Wi-Fi. */
    bool salva;            /**< Indica se a rede está salva no sistema. */
    int intensidade;       /**< Intensidade do sinal (0-100). */
    
    /**
     * @brief Construtor da estrutura RedeInfo.
     * @param n Nome da rede.
     * @param s Se a rede está salva.
     * @param i Intensidade do sinal (padrão: 100).
     */
    RedeInfo(const std::string& n, bool s, int i = 100) 
        : nome(n), salva(s), intensidade(i) {}
};

/**
 * @class JanelaRede
 * @brief Gerencia a interface de configurações de rede.
 * 
 * Esta classe renderiza e controla a interação com o menu de configurações
 * de rede, incluindo:
 * - Toggle de Wi-Fi ON/OFF
 * - Lista de redes disponíveis
 * - Indicação de rede conectada
 * - Marcação de redes salvas
 * - Navegação via controle ou mouse
 * - Teclado virtual para entrada de senha
 */
class JanelaRede {
public:
    /**
     * @brief Construtor da classe JanelaRede.
     */
    JanelaRede();

    /**
     * @brief Destrutor da classe JanelaRede.
     */
    ~JanelaRede();

    /**
     * @brief Renderiza a interface de configurações de rede.
     * @param renderer Ponteiro para o renderizador SDL onde desenhar.
     */
    void desenhar(SDL_Renderer* renderer);

    /**
     * @brief Processa eventos de input específicos para esta tela.
     * @param evento Referência ao evento SDL capturado.
     * @return true se o evento foi processado e causou mudança de estado.
     */
    bool processarEvento(SDL_Event& evento);

    /**
     * @brief Reseta o estado da janela para valores padrão.
     */
    void resetar();
    
    /**
     * @brief Verifica se o teclado virtual está visível.
     * @return true se o teclado virtual está sendo exibido.
     */
    bool isTecladoVisivel() const { return tecladoVisivel; }

private:
    // Estado do Wi-Fi
    bool wifiAtivo = true;                /**< Estado atual do Wi-Fi (ON/OFF). */
    std::string redeConectada = "Casa_Wifi"; /**< Nome da rede atualmente conectada. */
    
    // Lista de redes disponíveis
    std::vector<RedeInfo> redesDisponiveis; /**< Vetor com todas as redes detectadas. */
    
    // Botões interativos
    std::unique_ptr<Botao> btnToggleWifi;   /**< Botão para ligar/desligar Wi-Fi. */
    std::vector<std::unique_ptr<Botao>> botoesRedes; /**< Botões para cada rede disponível. */
    
    // Controle de navegação
    int indiceFocado = -1;                  /**< Índice do elemento atualmente focado. */
    int scrollOffset = 0;                   /**< Offset de rolagem da lista de redes. */
    int maxRedesVisiveis = 4;               /**< Quantidade máxima de redes visíveis simultaneamente. */
    
    // Controle de Input de Periféricos
    Uint32 ultimoInputAnalogico = 0;        /**< Timestamp do último input analógico. */
    const Uint32 INTERVALO_ANALOGICO = 200; /**< Intervalo mínimo entre inputs (ms). */
    const int DEADZONE = 16000;             /**< Limiar de sensibilidade do analógico. */
    
    // Controle de teclado virtual para senha
    bool tecladoVisivel = false;            /**< Flag indicando se o teclado virtual está ativo. */
    int indiceRedeSelecionada = -1;         /**< Índice da rede que está recebendo senha. */
    std::string senhaAtual = "";            /**< Senha sendo digitada no momento. */
    // Centraliza o teclado para largura da janela (1525px)
    // Cálculo: (1525 - 1000) / 2 = 262px
    TecladoVirtual tecladoVirtual{ConfigLayout::X(262), ConfigLayout::Y(600)};  /**< Instância do teclado virtual para entrada de senha. */
    
    // Textura para imagem explicativa dos botões
    SDL_Texture* texturaExplicacao = nullptr; /**< Textura da imagem explicativa rodapé. */
    
    // Botões da tela de senha
    std::unique_ptr<Botao> btnConectarSenha;  /**< Botão para confirmar senha. */
    std::unique_ptr<Botao> btnCancelarSenha;  /**< Botão para cancelar entrada de senha. */
    
    /**
     * @brief Inicializa os botões e elementos interativos.
     */
    void inicializarBotoes();

    /**
     * @brief Inicializa a lista de redes disponíveis (dados de exemplo).
     */
    void inicializarRedes();

    /**
     * @brief Renderiza o cabeçalho com informações de conexão.
     * @param renderer Renderizador SDL.
     */
    void desenharCabecalho(SDL_Renderer* renderer);

    /**
     * @brief Renderiza o toggle de Wi-Fi.
     * @param renderer Renderizador SDL.
     */
    void desenharToggleWifi(SDL_Renderer* renderer);

    /**
     * @brief Renderiza a lista de redes disponíveis.
     * @param renderer Renderizador SDL.
     */
    void desenharListaRedes(SDL_Renderer* renderer);
    
    /**
     * @brief Renderiza a imagem explicativa dos botões no rodapé.
     * @param renderer Renderizador SDL.
     */
    void desenharImagemExplicativa(SDL_Renderer* renderer);
    
    /**
     * @brief Renderiza a tela de entrada de senha.
     * @param renderer Renderizador SDL.
     */
    void desenharTelasenha(SDL_Renderer* renderer);

    /**
     * @brief Alterna o estado do Wi-Fi (ON/OFF).
     */
    void toggleWifi();

    /**
     * @brief Seleciona uma rede da lista.
     * @param indice Índice da rede na lista de redes disponíveis.
     */
    void selecionarRede(int indice);

    /**
     * @brief Move o foco para o elemento anterior (navegação por controle).
     */
    void navegarParaCima();

    /**
     * @brief Move o foco para o próximo elemento (navegação por controle).
     */
    void navegarParaBaixo();

    /**
     * @brief Confirma a seleção do elemento focado.
     */
    void confirmarSelecao();

    /**
     * @brief Atualiza o scroll da lista de redes baseado no foco.
     */
    void atualizarScroll();
    
    /**
     * @brief Carrega a textura da imagem explicativa de acordo com o tema.
     * @param renderer Renderizador SDL usado para carregar a textura.
     */
    void carregarImagemExplicativa(SDL_Renderer* renderer);
    
    /**
     * @brief Libera a textura da imagem explicativa.
     */
    void liberarImagemExplicativa();
    
    /**
     * @brief Abre o teclado virtual para entrada de senha.
     * @param indiceRede Índice da rede que precisa de senha.
     */
    void abrirTecladoSenha(int indiceRede);
    
    /**
     * @brief Fecha o teclado virtual e limpa o estado de entrada de senha.
     */
    void fecharTecladoSenha();
    
    /**
     * @brief Confirma a senha digitada e conecta à rede.
     */
    void confirmarSenha();
};

} // namespace MeuProjeto

#endif // JANELA_REDE_HPP