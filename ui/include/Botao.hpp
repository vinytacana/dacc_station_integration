/**
 * @file Botao.hpp
 * @brief Definição da classe base Botao para elementos interativos da interface.
 * 
 * Este arquivo contém a estrutura de um componente de interface genérico que pode
 * ser renderizado como um botão de texto ou um botão de imagem (capa de jogo),
 * suportando estados de hover, clique e foco via controle.
 */

#ifndef BOTAO_HPP
#define BOTAO_HPP

#include <SDL2/SDL.h>
#include <string>
#include <functional>
#include "Utils.hpp"
#include "Janela.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

class Janela; 

/**
 * @class Botao
 * @brief Classe base para botões interativos na interface gráfica.
 * 
 * A classe Botao gerencia o ciclo de vida de um elemento clicável, lidando com
 * a detecção de colisões, troca de estados visuais (normal, hover, focado) e
 * a execução de funções de retorno (callbacks). É projetada para ser estendida
 * por componentes mais específicos, como barras de pesquisa.
 */
class Botao {
public:
    /**
     * @brief Construtor para botões baseados apenas em texto.
     * 
     * Geralmente utilizado para menus de navegação ou botões de confirmação.
     * Por padrão, segue as cores definidas no GerenciadorTemas.
     * 
     * @param x Coordenada X na tela.
     * @param y Coordenada Y na tela.
     * @param largura Largura do botão.
     * @param altura Altura do botão.
     * @param texto String a ser exibida dentro do botão.
     */
    Botao(int x, int y, int largura, int altura, const std::string& texto);
    
    /**
     * @brief Construtor para botões baseados em imagens (Ícones ou Capas de Jogos).
     * 
     * Utilizado para representar os jogos na biblioteca ou banners de destaque.
     * 
     * @param renderer Ponteiro para o renderizador SDL para carregar texturas.
     * @param x Coordenada X.
     * @param y Coordenada Y.
     * @param largura Largura.
     * @param altura Altura.
     * @param imagemPath Caminho para o arquivo de imagem no disco.
     * @param nome Nome do jogo associado ao botão.
     * @param desc Descrição ou metadados do jogo.
     * @param isDestaque Define se o botão deve ser tratado como um banner principal.
     */
    Botao(SDL_Renderer* renderer, int x, int y, int largura, int altura,
          const std::string& imagemPath, const std::string& nome,
          const std::string& desc, bool isDestaque);
    
    /**
     * @brief Destrutor virtual para permitir polimorfismo seguro.
     */
    virtual ~Botao();

    /**
     * @brief Processa eventos de hardware direcionados ao botão.
     * 
     * Detecta movimentos do mouse para hover, cliques e interações de controle.
     * 
     * @param evento Referência ao evento capturado pela SDL.
     * @param offsetX Deslocamento horizontal do container do botão.
     * @param offsetY Deslocamento vertical do container do botão.
     * @return true se o botão capturou e processou o evento.
     */
    virtual bool tratarEvento(SDL_Event& evento, int offsetX, int offsetY);

    /**
     * @brief Renderiza o botão considerando deslocamentos (offsets).
     * 
     * @param renderer Renderizador SDL.
     * @param offsetX Deslocamento horizontal (scroll).
     * @param offsetY Deslocamento vertical (scroll).
     */
    virtual void desenhar(SDL_Renderer* renderer, int offsetX, int offsetY);

    /**
     * @brief Renderiza o botão em sua posição absoluta (sem scroll).
     * @param renderer Renderizador SDL.
     */
    virtual void desenhar(SDL_Renderer* renderer);

    /** @brief Define uma cor personalizada para o texto, ignorando o tema atual. */
    void setCorTexto(const SDL_Color& cor); 

    /** @brief Atualiza a textura de imagem do botão. */
    void setTexturaImagem(SDL_Texture* textura);
    
    /**
     * @brief Define cores manuais para os estados do botão.
     * @note Ao chamar este método, o botão deixará de seguir o tema global automaticamente.
     * @param padrao Cor em estado ocioso.
     * @param hover Cor ao passar o mouse.
     * @param press Cor ao clicar.
     */
    void setCor(const SDL_Color& padrao, const SDL_Color& hover, const SDL_Color& press);

    /**
    * @brief Define ou atualiza o texto exibido no botão.
    * @param novoTexto Nova string a ser exibida no centro do botão
    */
    void setTexto(const std::string& novoTexto);
    
    /** @brief Define o estilo da fonte (Normal/Negrito). */
    void setFonte(TipoFonte tipo) { tipoFonte = tipo; }

    /** @brief Define o tamanho da fonte em pixels. */
    void setTamanhoFonte(int tamanho) { tamanhoFonte = tamanho; }
    
    /** @brief Associa este botão a uma Janela pai (para controle de foco/popups). */
    void setParentJanela(Janela* janela);

    /** @brief Define se o botão deve ser desenhado com bordas arredondadas. */
    void setIsRound(bool round);

    void setRetanguloBordasArredondadas(int raio);

    /** @brief Define se o botão é um item de destaque (Banner). */
    void setDestaque(bool destaque) { isDestaque = destaque; }
    
    /**
     * @brief Define a ação a ser executada quando o botão é clicado.
     * @param callback Função lambda ou ponteiro de função sem retorno.
     */
    void setOnClick(std::function<void()> callback) { onClickCallback = callback; }
    
    // Métodos de Navegação por Controle

    /** @brief Define se o botão está selecionado pelo cursor do gamepad. */
    void setFocado(bool focado);

    /** @brief Ativa visualmente o estado de foco do controle. */
    void ativarPorControle();

    /** @brief Desativa o estado de foco do controle. */
    void desativarPorControle();

    /**
     * @brief Simula um clique no botão via comando do controle (ex: Botão A).
     * @return true se uma ação foi executada.
     */
    bool executarAcaoPorControle();
    
    // Getters

    /** @brief Verifica se o botão está sendo pressionado pelo mouse ou controle. */
    bool isPressionado() const { return pressionado; }

    /** @brief Verifica se o mouse está sobre o botão. */
    bool isHover() const { return hover; }

    /** @brief Verifica se o botão detém o foco atual da navegação por controle. */
    bool isFocado() const { return focadoPorControle; }

    /** @brief Retorna o retângulo de colisão/área do botão. */
    SDL_Rect getArea() const { return area; }
    
    /** @brief Retorna o nome do jogo caso seja um botão de capa. */
    std::string getNomeJogo() const { return nomeJogo; }

    /** @brief Retorna o path do arquivo de imagem associado. */
    std::string getImagemPath() const { return imagemPath; }

    /** @brief Retorna o ponteiro para a textura SDL carregada. */
    SDL_Texture* getTexturaImagem() const { return texturaImagem; }

    /**
     * @brief Verifica se um ponto (x,y) do mouse está dentro da área do botão.
     * @param x Coordenada X do ponto.
     * @param y Coordenada Y do ponto.
     * @return true se o ponto colidir com a área.
     */
    bool contemPonto(int x, int y);

    SDL_Rect area; /**< Geometria e posição do botão. */

    /** @brief Lida com movimento do mouse. */
    bool handleMouseMotion(SDL_Event& evento, int offsetX, int offsetY);

protected:
    SDL_Texture* texturaImagem; /**< Ativo gráfico do botão. */
    std::string texto;          /**< Rótulo textual do botão. */
    
    SDL_Color corPadrao;        /**< Cor de fundo normal. */
    SDL_Color corHover;         /**< Cor de fundo em hover. */
    SDL_Color corPressionado;   /**< Cor de fundo em clique. */
    
    // Variáveis de controle de tema e cor
    bool usaTemaPadrao;         /**< Se true, as cores vêm do GerenciadorTemas. */
    bool usaCorTextoCustom = false; /**< Indica se o usuário definiu uma cor de texto específica. */
    SDL_Color corTextoCustom = {255, 255, 255, 255}; /**< Valor da cor de texto customizada. */

    bool hover;                 /**< Estado interno: mouse sobre o componente. */
    bool pressionado;           /**< Estado interno: botão sendo clicado. */
    bool focadoPorControle;     /**< Estado interno: seleção ativa via gamepad. */
    
    Janela* parentJanela;       /**< Ponteiro para a janela que contém este botão. */
    std::string nomeJogo;       /**< Metadado: Nome do jogo vinculado. */
    std::string descJogo;       /**< Metadado: Descrição do jogo vinculado. */
    std::string imagemPath;     /**< Caminho original da imagem carregada. */
    
    bool isRound;               /**< Flag para estilo de bordas. */
    bool isDestaque;            /**< Flag para tratamento visual prioritário. */
    int raioBordasArredondadas; /**< Raio das bordas arredondadas. */
    
    TipoFonte tipoFonte;        /**< Estilo da fonte utilizada no texto. */
    int tamanhoFonte;           /**< Tamanho da fonte utilizada no texto. */
    
    /** @brief Objeto de função que armazena a lógica disparada no clique. */
    std::function<void()> onClickCallback;

    /**
     * @brief Auxiliar para calcular e desenhar o texto centralizado no botão.
     */
    void renderizarTextoCentralizado(SDL_Renderer* renderer, const SDL_Rect& area);

    /**
     * @brief Resolve qual cor deve ser usada no frame atual baseada nos estados.
     * @return SDL_Color resultante (Normal, Hover ou Pressionado).
     */
    SDL_Color determineCurrentColor();
    
    // Lógica interna de eventos segmentada

    /** @brief Lida com o pressionamento do botão do mouse. */
    bool handleMouseButtonDown(SDL_Event& evento, int offsetX, int offsetY);

    /** @brief Lida com a soltura do botão do mouse. */
    bool handleMouseButtonUp(SDL_Event& evento, int offsetX, int offsetY);

    /** @brief Lida com a confirmação via botão de gamepad. */
    bool handleGamepadButton(SDL_Event& evento);
    
};

} // namespace MeuProjeto

#endif // BOTAO_HPP