#ifndef UTILS_HPP
#define UTILS_HPP

#pragma once

#include <SDL2/SDL.h>
#include <string>

namespace MeuProjeto {

// Tipos de Fonte

/**
 * @enum TipoFonte
 * @brief Enumeração dos tipos de fontes disponíveis no sistema.
 */
enum class TipoFonte {
    NORMAL,      ///< Fonte principal (padrão do sistema).
    NEGRITO      ///< Fonte para destaque, títulos ou texto mais pesado.
};

// CONFIGURAÇÃO DE FONTES
/**
 * @struct ConfigFontes
 * @brief Estrutura estática para armazenar e gerenciar os caminhos dos arquivos de fontes.
 */
struct ConfigFontes {
    /**
     * @brief Caminho para o arquivo da fonte Normal/Principal.
     */
    static std::string FONTE_NORMAL;

    /**
     * @brief Caminho para o arquivo da fonte Negrito/Secundária.
     */
    static std::string FONTE_NEGRITO;
    
    /**
     * @brief Obtém o caminho do arquivo da fonte baseado no seu tipo.
     *
     * @param tipo O enumerador @c TipoFonte desejado.
     * @return Uma referência constante para o caminho da fonte.
     */
    static const std::string& obterCaminho(TipoFonte tipo);
    
    /**
     * @brief Configura (muda em runtime) os caminhos dos arquivos de fontes.
     *
     * @param principal O novo caminho para a FONTE_NORMAL.
     * @param secundaria O novo caminho para a FONTE_NEGRITO.
     */
    static void configurarFontes(const std::string& principal,
                                const std::string& secundaria);
};

// FUNÇÕES DE DESENHO (Sobrecarga)

/**
 * @brief Desenha um texto na tela usando a fonte principal.
 *
 * Esta é uma versão de compatibilidade que usa @c TipoFonte::NORMAL por padrão.
 *
 * @param renderer O renderizador SDL.
 * @param texto A string UTF-8 a ser desenhada.
 * @param x Coordenada X superior esquerda.
 * @param y Coordenada Y superior esquerda.
 * @param cor A cor do texto (SDL_Color).
 * @param tamanhoFonte O tamanho da fonte em pontos (pt). Padrão é 28.
 */
void desenharTexto(SDL_Renderer* renderer, 
                  const std::string& texto, 
                  int x, int y, 
                  SDL_Color cor, 
                  int tamanhoFonte = 28);

/**
 * @brief Desenha um texto na tela usando um tipo de fonte específico.
 *
 * @param renderer O renderizador SDL.
 * @param texto A string UTF-8 a ser desenhada.
 * @param x Coordenada X superior esquerda.
 * @param y Coordenada Y superior esquerda.
 * @param cor A cor do texto (SDL_Color).
 * @param tamanhoFonte O tamanho da fonte em pontos (pt).
 * @param tipoFonte O tipo de fonte a ser usado (@c TipoFonte).
 */
void desenharTexto(SDL_Renderer* renderer, 
                  const std::string& texto, 
                  int x, int y, 
                  SDL_Color cor, 
                  int tamanhoFonte,
                  TipoFonte tipoFonte);

/**
 * @brief Referência de compatibilidade para o caminho da fonte principal.
 *
 * Equivalente a @c ConfigFontes::FONTE_NORMAL.
 */
extern const std::string& CAMINHO_FONTE;

} // namespace MeuProjeto

#endif // UTILS_HPP