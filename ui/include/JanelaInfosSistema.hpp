/**
 * @file JanelaInfosSistema.hpp
 * @brief Definição da classe JanelaInfosSistema para exibição de informações do sistema.
 * 
 * Este arquivo contém a interface para o submenu de informações do sistema,
 * exibindo dados sobre hardware, sistema operacional, versão do projeto e
 * informações sobre colaboradores. Suporta scroll via mouse e gamepad com
 * barra visual de rolagem.
 * 
 */

#ifndef JANELA_INFOS_SISTEMA_HPP
#define JANELA_INFOS_SISTEMA_HPP

#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

class GerenciadorImagens;

/**
 * @struct InfoHardware
 * @brief Estrutura para armazenar informações de hardware.
 */
struct InfoHardware {
    std::string processador;     /**< Nome/modelo do processador. */
    int nucleos;                 /**< Número de núcleos da CPU. */
    int memoriaRAM;              /**< Quantidade de RAM em MB. */
    std::string placaVideo;      /**< Nome da placa de vídeo. */
    std::string sistemaOperacional; /**< Nome e versão do SO. */
    std::string arquitetura;     /**< Arquitetura do sistema (x86, x64, ARM, etc). */
};

/**
 * @struct InfoProjeto
 * @brief Estrutura para armazenar informações sobre o projeto.
 */
struct InfoProjeto {
    std::string nome;            /**< Nome do projeto. */
    std::string versao;          /**< Versão atual do projeto. */
    std::string dataCompilacao;  /**< Data de compilação. */
    std::string descricao;       /**< Descrição breve do projeto. */
};

/**
 * @struct Colaborador
 * @brief Estrutura para armazenar informações de um colaborador.
 */
struct Colaborador {
    std::string nome;            /**< Nome do colaborador. */
    std::string funcao;          /**< Função/papel no projeto. */
    
    /**
     * @brief Construtor do Colaborador.
     * @param n Nome do colaborador.
     * @param f Função no projeto.
     */
    Colaborador(const std::string& n, const std::string& f) 
        : nome(n), funcao(f) {}
};

/**
 * @class JanelaInfosSistema
 * @brief Exibe informações do sistema, projeto e colaboradores com scroll interativo.
 * 
 * Esta classe renderiza uma tela informativa com:
 * - Informações de hardware (CPU, RAM, GPU, SO)
 * - Informações do projeto (nome, versão, data)
 * - Lista de colaboradores e suas funções
 * - Ícone de bateria na barra de status
 * - Sistema de scroll via mouse e gamepad
 * - Barra de rolagem visual no lado direito
 * - Imagem explicativa dos controles na parte inferior
 */
class JanelaInfosSistema {
public:
    /**
     * @brief Construtor da classe JanelaInfosSistema.
     */
    JanelaInfosSistema(GerenciadorImagens* gerImgLocal);

    /**
     * @brief Destrutor da classe JanelaInfosSistema.
     * Libera texturas alocadas (bateria e explicação).
     */
    ~JanelaInfosSistema();

    /**
     * @brief Renderiza a interface de informações do sistema.
     * @param renderer Ponteiro para o renderizador SDL onde desenhar.
     */
    void desenhar(SDL_Renderer* renderer);

    /**
     * @brief Processa eventos de scroll (mouse wheel e D-Pad do controle).
     * @param evento Referência ao evento SDL capturado.
     * @return true se o evento foi processado, false caso contrário.
     */
    bool processarEvento(SDL_Event& evento);

    /**
     * @brief Reseta o estado da janela (posição de scroll).
     */
    void resetar();

    /**
     * @brief Carrega as texturas necessárias (bateria e explicação).
     * Deve ser chamado após ter um renderer válido.
     * @param renderer Ponteiro para o renderizador SDL.
     */
    void carregarTexturas(SDL_Renderer* renderer);

private:
    InfoHardware hardware;          /**< Informações de hardware do sistema. */
    InfoProjeto projeto;            /**< Informações sobre o projeto. */
    std::vector<Colaborador> colaboradores; /**< Lista de colaboradores. */
    
    int offsetScroll = 0;           /**< Offset de scroll para conteúdo longo. */
    int maxScroll = 0;              /**< Limite máximo de scroll baseado no conteúdo. */
    int alturaConteudo = 0;         /**< Altura total do conteúdo renderizado. */
    
    GerenciadorImagens* gerImgRef = nullptr;    /**< Referência ao gerenciador de imagens isolado. */
    SDL_Texture* texturaExplicacao = nullptr;   /**< Textura da imagem explicativa dos botões. */
    
    bool precisaRecalcularScroll = true;        /**< Flag para recalcular altura do conteúdo. */
    
    // Constantes de scroll
    const int VELOCIDADE_SCROLL_MOUSE = 20;    /**< Pixels por tick de scroll do mouse. */
    const int VELOCIDADE_SCROLL_GAMEPAD = 30;  /**< Pixels por pressão de botão do gamepad. */
    const int LARGURA_BARRA_SCROLL = 12;       /**< Largura da barra de rolagem em pixels. */
    
    /**
     * @brief Coleta informações de hardware do sistema.
     * Usa APIs do SDL e do sistema operacional para obter dados reais.
     */
    void coletarInfosHardware();

    /**
     * @brief Inicializa informações do projeto.
     * Define nome, versão, data de compilação e descrição.
     */
    void inicializarInfosProjeto();

    /**
     * @brief Inicializa lista de colaboradores.
     * Adiciona todos os contribuidores do projeto com suas funções.
     */
    void inicializarColaboradores();

    /**
     * @brief Renderiza o cabeçalho da tela com barra de status.
     * @param renderer Renderizador SDL.
     */
    void desenharCabecalho(SDL_Renderer* renderer);

    /**
     * @brief Renderiza a seção de informações de hardware.
     * @param renderer Renderizador SDL.
     * @param posY Posição Y inicial da seção.
     * @return Nova posição Y após renderizar a seção.
     */
    int desenharSecaoHardware(SDL_Renderer* renderer, int posY);

    /**
     * @brief Renderiza a seção de informações do projeto.
     * @param renderer Renderizador SDL.
     * @param posY Posição Y inicial da seção.
     * @return Nova posição Y após renderizar a seção.
     */
    int desenharSecaoProjeto(SDL_Renderer* renderer, int posY);

    /**
     * @brief Renderiza a seção de colaboradores.
     * @param renderer Renderizador SDL.
     * @param posY Posição Y inicial da seção.
     * @return Nova posição Y após renderizar a seção.
     */
    int desenharSecaoColaboradores(SDL_Renderer* renderer, int posY);

    /**
     * @brief Desenha um separador visual entre seções.
     * @param renderer Renderizador SDL.
     * @param posY Posição Y do separador.
     */
    void desenharSeparador(SDL_Renderer* renderer, int posY);

    /**
     * @brief Renderiza a barra de rolagem no lado direito da tela.
     * A barra só aparece se houver conteúdo rolável.
     * @param renderer Renderizador SDL.
     */
    void desenharBarraScroll(SDL_Renderer* renderer);

    /**
     * @brief Renderiza a imagem explicativa dos controles na parte inferior.
     * @param renderer Renderizador SDL.
     */
    void desenharImagemExplicativa(SDL_Renderer* renderer);

    /**
     * @brief Calcula a altura total do conteúdo para determinar maxScroll.
     * Deve ser chamado após renderizar todo o conteúdo uma vez.
     * @param alturaFinal Altura Y final do último elemento renderizado.
     */
    void calcularAlturaConteudo(int alturaFinal);

    /**
     * @brief Obtém informações da CPU (nome e núcleos).
     * @return String formatada com informações da CPU.
     */
    std::string obterInfoCPU();

    /**
     * @brief Obtém quantidade de RAM do sistema.
     * @return Quantidade de RAM em MB.
     */
    int obterRAM();

    /**
     * @brief Obtém informações da GPU.
     * @return String com nome/modelo da placa de vídeo.
     */
    std::string obterInfoGPU();

    /**
     * @brief Obtém informações do sistema operacional.
     * @return String com nome e versão do SO.
     */
    std::string obterInfoSO();

    /**
     * @brief Obtém a arquitetura do sistema.
     * @return String com arquitetura (x86, x64, ARM, etc).
     */
    std::string obterArquitetura();

    /**
     * @brief Carrega a imagem explicativa apropriada baseada no tema atual.
     * @param renderer Renderizador SDL.
     */
    void carregarImagemExplicativa(SDL_Renderer* renderer);

    /**
     * @brief Libera texturas antigas antes de recarregar.
     * Evita memory leaks ao trocar de tema.
     */
    void liberarTexturas();
};

} // namespace MeuProjeto

#endif // JANELA_INFOS_SISTEMA_HPP