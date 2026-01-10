/**
 * @file Utils.hpp
 * @brief Definição de utilitários gráficos e gerenciamento de fontes para a interface.
 * 
 * Este arquivo contém definições de tipos, estruturas e funções auxiliares para
 * facilitar a renderização de texto utilizando SDL_ttf e a configuração global
 * dos caminhos de fontes do sistema.
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#pragma once

#include <SDL2/SDL.h>
#include <string>

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @enum TipoFonte
 * @brief Enumeração que define os estilos de fontes suportados pela interface.
 */
enum class TipoFonte {
    NORMAL,  /**< Estilo de fonte padrão. */
    NEGRITO  /**< Estilo de fonte enfatizado. */
};

/**
 * @struct ConfigFontes
 * @brief Estrutura de configuração estática para gerenciar os arquivos de fonte (.ttf).
 * 
 * Permite que a aplicação defina e recupere globalmente os caminhos dos arquivos de fonte
 * que serão carregados para renderização de textos na interface.
 */
struct ConfigFontes {
    /** @brief Caminho absoluto ou relativo para o arquivo de fonte normal. */
    static std::string FONTE_NORMAL;

    /** @brief Caminho absoluto ou relativo para o arquivo de fonte em negrito. */
    static std::string FONTE_NEGRITO;

    /**
     * @brief Recupera o caminho do arquivo de fonte baseado no tipo solicitado.
     * @param tipo O estilo da fonte (NORMAL ou NEGRITO).
     * @return Uma referência constante para a string contendo o caminho do arquivo.
     */
    static const std::string& obterCaminho(TipoFonte tipo);

    /**
     * @brief Configura os caminhos globais das fontes utilizadas no projeto.
     * 
     * Este método deve ser chamado na inicialização do sistema para apontar para
     * os arquivos .ttf corretos.
     * 
     * @param principal Caminho para a fonte que será usada como padrão.
     * @param secundaria Caminho para a fonte que será usada em destaques (negrito).
     */
    static void configurarFontes(const std::string& principal, const std::string& secundaria);
};

/**
 * @brief Renderiza um texto na tela com cor customizada e tamanho opcional.
 * 
 * Esta sobrecarga utiliza a fonte padrão (NORMAL) por definição interna.
 * 
 * @param renderer O renderizador SDL onde o texto será desenhado.
 * @param texto A string contendo o texto a ser exibido.
 * @param x Coordenada X na tela (origem superior esquerda).
 * @param y Coordenada Y na tela (origem superior esquerda).
 * @param cor Estrutura SDL_Color definindo RGBA do texto.
 * @param tamanhoFonte Tamanho da fonte em pixels (Padrão: 28).
 */
void desenharTexto(SDL_Renderer* renderer, const std::string& texto, int x, int y, SDL_Color cor, int tamanhoFonte = 28);

/**
 * @brief Renderiza um texto na tela especificando explicitamente o tipo da fonte.
 * 
 * Permite alternar entre fontes normais e negritos ao desenhar, mantendo o controle de cor.
 * 
 * @param renderer O renderizador SDL onde o texto será desenhado.
 * @param texto A string contendo o texto a ser exibido.
 * @param x Coordenada X na tela.
 * @param y Coordenada Y na tela.
 * @param cor Estrutura SDL_Color definindo RGBA do texto.
 * @param tamanhoFonte Tamanho da fonte em pixels.
 * @param tipoFonte O estilo da fonte a ser utilizado (NORMAL ou NEGRITO).
 */
void desenharTexto(SDL_Renderer* renderer, const std::string& texto, int x, int y, SDL_Color cor, int tamanhoFonte, TipoFonte tipoFonte);

/**
 * @brief Renderiza um texto na tela utilizando uma cor padrão pré-definida.
 * 
 * Versão simplificada da função de desenho que não exige a passagem de um objeto SDL_Color,
 * facilitando a escrita de textos rápidos com cores padrão do sistema.
 * 
 * @param renderer O renderizador SDL onde o texto será desenhado.
 * @param texto A string contendo o texto a ser exibido.
 * @param x Coordenada X na tela.
 * @param y Coordenada Y na tela.
 * @param tamanhoFonte Tamanho da fonte em pixels.
 * @param tipoFonte O estilo da fonte (Padrão: NORMAL).
 */
void desenharTexto(SDL_Renderer* renderer, const std::string& texto, int x, int y, int tamanhoFonte, TipoFonte tipoFonte = TipoFonte::NORMAL);

/**
 * @brief Limpa o cache global de texturas de texto.
 * Deve ser chamado ao trocar de tela ou limpar buscas para liberar VRAM.
 */
void limparCacheTexto(); 

/**
 * @brief Referência externa para um caminho de fonte padrão.
 * 
 * Pode ser usada como fallback caso as configurações de ConfigFontes não sejam preenchidas.
 */
extern const std::string& CAMINHO_FONTE;

} // namespace MeuProjeto

#endif // UTILS_HPP