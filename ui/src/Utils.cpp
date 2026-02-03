/**
 * @file Utils.cpp
 * @brief Implementação de funções utilitárias para renderização de texto com cache de texturas.
 * 
 * Este arquivo fornece funções auxiliares otimizadas para desenho de texto na tela,
 * implementando um sistema de cache para evitar recriação desnecessária de texturas
 * e melhorar significativamente a performance da renderização.
 */

#include "Utils.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorTemas.hpp"
#include "GerenciadorTexturasTexto.hpp"
#include <SDL2/SDL_ttf.h>
#include <iostream>

namespace MeuProjeto {

/// Instância global do gerenciador de fontes (TTF_Font*)
static GerenciadorFontes g_fontes;

/// Instância global do gerenciador de cache de texturas de texto
static GerenciadorTexturasTexto g_texturasTexto;

/// Caminho padrão para fonte regular/normal
std::string ConfigFontes::FONTE_NORMAL = "assets/fonts/Garet-Book.ttf";

/// Caminho padrão para fonte em negrito/destaque
std::string ConfigFontes::FONTE_NEGRITO = "assets/fonts/Garet-Heavy.ttf";

/// Referência para caminho da fonte padrão (compatibilidade legada)
const std::string& CAMINHO_FONTE = ConfigFontes::FONTE_NORMAL;

/**
 * @brief Retorna o caminho da fonte apropriada baseado no tipo solicitado.
 * 
 * @param tipo Tipo de fonte desejada (NORMAL ou NEGRITO).
 * @return const std::string& Referência para o caminho do arquivo TTF.
 */
const std::string& ConfigFontes::obterCaminho(TipoFonte tipo) {
    return (tipo == TipoFonte::NEGRITO) ? FONTE_NEGRITO : FONTE_NORMAL;
}

/**
 * @brief Configura os caminhos das fontes principal e secundária do sistema.
 * 
 * Permite personalizar as fontes utilizadas em toda a aplicação.
 * 
 * @param principal Caminho para o arquivo TTF da fonte normal/regular.
 * @param secundaria Caminho para o arquivo TTF da fonte em negrito.
 */
void ConfigFontes::configurarFontes(const std::string& principal, const std::string& secundaria) {
    FONTE_NORMAL = principal;
    FONTE_NEGRITO = secundaria;
}

/**
 * @brief Renderiza texto na tela utilizando cache de texturas para otimização de performance.
 * 
 * **Otimização Implementada:**
 * Em vez de gerar uma nova textura SDL a cada frame (causando overhead de CPU/GPU),
 * a função consulta o GerenciadorTexturasTexto. Se o texto já foi renderizado
 * anteriormente com os mesmos parâmetros (fonte, tamanho, cor), a textura existente
 * é reutilizada diretamente, economizando processamento significativo.
 * 
 * @param renderer Renderizador SDL onde o texto será desenhado.
 * @param texto String contendo o conteúdo a ser renderizado.
 * @param x Coordenada X (horizontal) do canto superior esquerdo do texto.
 * @param y Coordenada Y (vertical) do canto superior esquerdo do texto.
 * @param cor Cor RGBA do texto (estrutura SDL_Color).
 * @param tamanhoFonte Tamanho da fonte em pontos (pt).
 * @param tipoFonte Tipo de fonte a utilizar (NORMAL ou NEGRITO).
 * 
 * @note Se a string estiver vazia, nenhuma operação é realizada.
 * @note A textura retornada do cache não deve ser destruída manualmente.
 */
void desenharTexto(SDL_Renderer* renderer, const std::string& texto, int x, int y, SDL_Color cor, int tamanhoFonte, TipoFonte tipoFonte) {
    /// Evita processamento desnecessário se não há texto para renderizar
    if (texto.empty()) return;

    /// Obtém o caminho apropriado baseado no tipo de fonte solicitado
    std::string path = ConfigFontes::obterCaminho(tipoFonte);
    
    /// Carrega (ou recupera do cache) a fonte no tamanho especificado
    TTF_Font* font = g_fontes.carregar(path, tamanhoFonte);
    if (!font) return;

    /**
     * Obtém textura do cache ou gera nova se não existir.
     * Parâmetros de cache: renderer, fonte, texto, caminho, tamanho e cor.
     */
    SDL_Texture* texture = g_texturasTexto.obterTextura(renderer, font, texto, path, tamanhoFonte, cor);
    
    if (texture) {
        /// Consulta dimensões da textura gerada
        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
        
        /// Define retângulo de destino e renderiza
        SDL_Rect dst = {x, y, w, h};
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
    }
}

/**
 * @brief Sobrecarga simplificada que usa fonte NORMAL por padrão.
 * 
 * @param renderer Renderizador SDL.
 * @param texto Conteúdo textual a renderizar.
 * @param x Posição horizontal.
 * @param y Posição vertical.
 * @param cor Cor RGBA do texto.
 * @param tamanhoFonte Tamanho em pontos.
 */
void desenharTexto(SDL_Renderer* renderer, const std::string& texto, int x, int y, SDL_Color cor, int tamanhoFonte) {
    desenharTexto(renderer, texto, x, y, cor, tamanhoFonte, TipoFonte::NORMAL);
}

/**
 * @brief Sobrecarga que utiliza cor automática do tema ativo.
 * 
 * A cor é obtida dinamicamente do GerenciadorTemas, permitindo que o texto
 * se adapte automaticamente ao tema visual selecionado.
 * 
 * @param renderer Renderizador SDL.
 * @param texto Conteúdo textual a renderizar.
 * @param x Posição horizontal.
 * @param y Posição vertical.
 * @param tamanhoFonte Tamanho em pontos.
 * @param tipoFonte Tipo de fonte (NORMAL ou NEGRITO).
 */
void desenharTexto(SDL_Renderer* renderer, const std::string& texto, int x, int y, int tamanhoFonte, TipoFonte tipoFonte) {
    /// Obtém cor do tema ativo para textos em negrito
    SDL_Color corAutomatica = GerenciadorTemas::getInstance().getCorTextoNegrito();
    desenharTexto(renderer, texto, x, y, corAutomatica, tamanhoFonte, tipoFonte);
}

/**
 * @brief Libera todos os recursos do cache de texturas e fontes.
 * 
 * Deve ser chamada ao encerrar a aplicação ou ao trocar de contexto
 * que invalide as texturas existentes (mudança de renderer, tema, etc).
 * 
 * Após esta chamada, todas as texturas e fontes serão recarregadas
 * na próxima utilização.
 */
void limparCacheTexto() {
    g_texturasTexto.liberarTudo();
    g_fontes.liberarTudo();
}

} // namespace MeuProjeto