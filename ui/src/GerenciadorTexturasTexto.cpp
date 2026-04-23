/**
 * @file GerenciadorTexturasTexto.cpp
 * @brief Implementação do sistema de cache e gerenciamento de texturas de texto.
 *
 * Este arquivo contém a lógica para otimização da renderização de fontes, utilizando
 * um mecanismo de cache para evitar a re-renderização redundante de strings idênticas,
 * o que reduz o consumo de processamento de CPU e as transferências para a GPU.
 */

#include "GerenciadorTexturasTexto.hpp"
#include <iostream>

using namespace MeuProjeto;

/**
 * @brief Construtor padrão da classe GerenciadorTexturasTexto.
 * 
 * Inicializa a instância do gerenciador sem alocar recursos previamente, 
 * aguardando as demandas de renderização para popular o cache.
 */
GerenciadorTexturasTexto::GerenciadorTexturasTexto() = default;

/**
 * @brief Destrutor da classe GerenciadorTexturasTexto.
 * 
 * Garante que todas as texturas armazenadas no cache sejam devidamente 
 * liberadas da memória de vídeo através da chamada ao método liberarTudo().
 */
GerenciadorTexturasTexto::~GerenciadorTexturasTexto() {
    liberarTudo();
}

/**
 * @brief Obtém uma textura de texto, seja recuperando do cache ou gerando uma nova.
 * 
 * Este método implementa o padrão Flyweight para texturas de texto. Ele utiliza
 * um objeto ChaveTexto para verificar se uma combinação idêntica de texto, fonte,
 * tamanho e cor já foi processada. Caso positivo, retorna a textura existente.
 * Caso contrário, realiza a renderização via SDL_ttf e armazena o resultado.
 * 
 * @param renderer Ponteiro para o SDL_Renderer utilizado na criação da textura.
 * @param font Ponteiro para a estrutura TTF_Font utilizada para renderizar o texto.
 * @param texto String contendo o conteúdo textual a ser convertido em imagem.
 * @param caminhoFonte String com o identificador/caminho da fonte para composição da chave.
 * @param tamanho Valor inteiro indicando a dimensão da fonte.
 * @param cor Estrutura SDL_Color definindo os componentes RGBA do texto.
 * @return SDL_Texture* Ponteiro para a textura gerada ou recuperada. Retorna nullptr em caso de falha.
 */
SDL_Texture* GerenciadorTexturasTexto::obterTextura(SDL_Renderer* renderer, TTF_Font* font, const std::string& texto, 
                                                  const std::string& caminhoFonte, int tamanho, SDL_Color cor) {
    // 1. Criar a chave de busca
    // Consolida os metadados do texto em uma estrutura para busca única no container de cache.
    ChaveTexto chave = {renderer, texto, caminhoFonte, tamanho, cor};

    // 2. Verificar se já existe no cache
    // Realiza a busca no mapa interno para evitar operações de renderização desnecessárias.
    auto it = cache.find(chave);
    if (it != cache.end()) {
        return it->second.get();
    }

    // 3. Se não existir, renderizar agora (Operação custosa na CPU/GPU)
    // Cria uma superfície temporária a partir dos glifos da fonte em modo Blended (anti-aliasing de alta qualidade).
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, texto.c_str(), cor);
    if (!surface) return nullptr;

    // Converte a superfície (RAM) em textura (VRAM) para utilização pelo renderizador.
    SDL_Texture* textura = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface); // A superfície é liberada imediatamente após a criação da textura.

    if (!textura) return nullptr;

    // 4. Armazenar no cache com unique_ptr e deleter customizado
    // Insere a nova textura no cache utilizando um ponteiro inteligente com gerenciador de destruição específico para SDL_Texture.
    auto deleter = &destruirTextura;
    auto result = cache.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(chave),
        std::forward_as_tuple(textura, deleter)
    );

    return result.first->second.get();
}

/**
 * @brief Esvazia o cache de texturas e libera a memória associada.
 * 
 * Remove todos os elementos do container de cache. A liberação efetiva de cada 
 * SDL_Texture ocorre automaticamente via deleter associado aos ponteiros inteligentes.
 */
void GerenciadorTexturasTexto::liberarTudo() {
    cache.clear();
}
