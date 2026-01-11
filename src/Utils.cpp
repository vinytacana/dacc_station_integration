/**
 * @file Utils.cpp
 * @brief Implementação das funções utilitárias de renderização e configuração de fontes.
 * 
 * Este arquivo contém a lógica para desenho de texto na tela utilizando SDL_ttf,
 * além de gerenciar os caminhos globais dos arquivos de fonte TrueType (.ttf).
 */

#include "Utils.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorTemas.hpp" 
#include <SDL2/SDL_ttf.h>
#include <iostream>

namespace MeuProjeto {

/** @brief Inicialização do caminho padrão para a fonte de peso normal. */
std::string ConfigFontes::FONTE_NORMAL = "assets/fonts/Garet-Book.ttf";

/** @brief Inicialização do caminho padrão para a fonte em negrito. */
std::string ConfigFontes::FONTE_NEGRITO = "assets/fonts/Garet-Heavy.ttf";

/** @brief Referência externa que aponta para o caminho da fonte normal por padrão. */
const std::string& CAMINHO_FONTE = ConfigFontes::FONTE_NORMAL;

/**
 * @brief Recupera o caminho do arquivo de fonte baseado no estilo solicitado.
 * 
 * @param tipo O estilo da fonte (NORMAL ou NEGRITO).
 * @return const std::string& Caminho do arquivo .ttf correspondente.
 */
const std::string& ConfigFontes::obterCaminho(TipoFonte tipo) {
    return (tipo == TipoFonte::NEGRITO) ? FONTE_NEGRITO : FONTE_NORMAL;
}

/**
 * @brief Sobrescreve os caminhos padrão das fontes do sistema.
 * 
 * Permite que a aplicação mude sua tipografia globalmente em tempo de execução.
 * 
 * @param principal Novo caminho para a fonte normal.
 * @param secundaria Novo caminho para a fonte negrito.
 */
void ConfigFontes::configurarFontes(const std::string& principal, const std::string& secundaria) {
    FONTE_NORMAL = principal;
    FONTE_NEGRITO = secundaria;
}

/**
 * @brief Função mestre para renderização de texto UTF-8.
 * 
 * Esta função realiza o ciclo completo de renderização de texto em SDL2:
 * 1. Obtém a fonte do cache (GerenciadorFontes).
 * 2. Renderiza o texto em uma SDL_Surface (Blended para alta qualidade).
 * 3. Converte a Surface em uma SDL_Texture vinculada ao renderer.
 * 4. Copia a textura para a tela e libera os recursos temporários.
 * 
 * @note Utiliza uma instância estática de GerenciadorFontes para garantir que as 
 *       fontes permaneçam em cache durante toda a vida do programa.
 * 
 * @param renderer O renderizador SDL.
 * @param texto A string de texto a ser desenhada.
 * @param x Posição horizontal inicial.
 * @param y Posição vertical inicial.
 * @param cor Cor RGBA do texto.
 * @param tamanhoFonte Tamanho em pontos/pixels.
 * @param tipoFonte Estilo da fonte (Normal ou Negrito).
 */
void desenharTexto(SDL_Renderer* renderer, const std::string& texto, int x, int y, SDL_Color cor, int tamanhoFonte, TipoFonte tipoFonte) {
    // Instância estática para persistência do cache entre chamadas
    static GerenciadorFontes gerenciador;
    
    if (texto.empty()) return;

    // Carrega a fonte do cache ou disco
    TTF_Font* font = gerenciador.carregar(ConfigFontes::obterCaminho(tipoFonte), tamanhoFonte);
    if (!font) return;

    // Renderiza em modo Blended (Suave/Anti-aliasing com Alpha)
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, texto.c_str(), cor);
    if (!surface) return;

    // Transfere da RAM (Surface) para a GPU (Texture)
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_Rect dst = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, nullptr, &dst);
        
        // Texturas de texto são voláteis e destruídas após o desenho nesta implementação
        SDL_DestroyTexture(texture);
    }
    
    // Libera a memória da superfície
    SDL_FreeSurface(surface);
}

/**
 * @brief Sobrecarga simplificada para desenhar texto com estilo NORMAL.
 */
void desenharTexto(SDL_Renderer* renderer, const std::string& texto, int x, int y, SDL_Color cor, int tamanhoFonte) {
    desenharTexto(renderer, texto, x, y, cor, tamanhoFonte, TipoFonte::NORMAL);
}

/**
 * @brief Sobrecarga avançada que obtém a cor automaticamente do Gerenciador de Temas.
 * 
 * Esta função integra o desenho de texto com a paleta de cores atual do sistema,
 * selecionando a cor de texto (Negrito/Destaque) baseada no tema Claro ou Escuro.
 * 
 * @param renderer O renderizador SDL.
 * @param texto Texto a ser desenhado.
 * @param x Posição X.
 * @param y Posição Y.
 * @param tamanhoFonte Tamanho da fonte.
 * @param tipoFonte Estilo da fonte.
 */
void desenharTexto(SDL_Renderer* renderer, const std::string& texto, int x, int y, int tamanhoFonte, TipoFonte tipoFonte) {
    // Consulta o Singleton do Gerenciador de Temas para obter a cor ideal de texto
    SDL_Color corAutomatica = GerenciadorTemas::getInstance().getCorTextoNegrito();
    desenharTexto(renderer, texto, x, y, corAutomatica, tamanhoFonte, tipoFonte);
}

} // namespace MeuProjeto