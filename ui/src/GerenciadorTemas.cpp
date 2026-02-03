/**
 * @file GerenciadorTemas.cpp
 * @brief Implementação da classe GerenciadorTemas para controle de paletas de cores.
 * 
 * Este arquivo contém a lógica que define qual cor deve ser entregue aos componentes
 * da interface com base no estado atual do tema (Claro ou Escuro).
 */

#include "GerenciadorTemas.hpp"

using namespace MeuProjeto;

/**
 * @brief Retorna a instância única do Gerenciador de Temas.
 * 
 * Utiliza o padrão "Meyers' Singleton", onde a instância é criada de forma estática
 * na primeira vez que a função é chamada, garantindo que o ciclo de vida da paleta
 * de cores dure por toda a execução do programa.
 * 
 * @return GerenciadorTemas& Referência para a instância global.
 */
GerenciadorTemas& GerenciadorTemas::getInstance() {
    static GerenciadorTemas instance;
    return instance;
}

/**
 * @brief Construtor privado da classe.
 * 
 * Define o tema inicial da aplicação. Por padrão, a interface inicia no modo ESCURO.
 */
GerenciadorTemas::GerenciadorTemas() : temaAtual(TipoTema::ESCURO) {
    // Começa com tema escuro por padrão
}

/**
 * @brief Alterna o estado do tema entre as opções disponíveis.
 * 
 * Realiza uma troca binária: se estiver no modo Escuro, muda para Claro;
 * se estiver no Claro, muda para Escuro.
 */
void GerenciadorTemas::alternarTema() {
    if (temaAtual == TipoTema::ESCURO) {
        temaAtual = TipoTema::CLARO;
    } else {
        temaAtual = TipoTema::ESCURO;
    }
}

/**
 * @brief Define manualmente um tema específico.
 * @param tema O tipo de tema desejado (TipoTema::CLARO ou TipoTema::ESCURO).
 */
void GerenciadorTemas::setTema(TipoTema tema) {
    temaAtual = tema;
}

/**
 * @brief Recupera o identificador do tema ativo.
 * @return O tema atual (Enum TipoTema).
 */
TipoTema GerenciadorTemas::getTemaAtual() const {
    return temaAtual;
}

/**
 * @brief Retorna a cor de fundo configurada para o tema atual.
 * @return SDL_Color correspondente ao fundo da janela.
 */
SDL_Color GerenciadorTemas::getCorFundo() const {
    return (temaAtual == TipoTema::ESCURO) ? escuroFundo : claroFundo;
}

/**
 * @brief Retorna a cor para containers e painéis.
 * @return SDL_Color para elementos de profundidade da interface.
 */
SDL_Color GerenciadorTemas::getCorRetangulos() const {
    return (temaAtual == TipoTema::ESCURO) ? escuroRetangulos : claroRetangulos;
}

/**
 * @brief Retorna a cor para textos de alto contraste (Títulos/Negritos).
 * @return SDL_Color para visibilidade máxima de texto.
 */
SDL_Color GerenciadorTemas::getCorTextoNegrito() const {
    return (temaAtual == TipoTema::ESCURO) ? escuroTextoNegrito : claroTextoNegrito;
}

/**
 * @brief Retorna a cor para textos informativos de baixo contraste.
 * @return SDL_Color para descrições e labels secundárias.
 */
SDL_Color GerenciadorTemas::getCorTextoNormal() const {
    return (temaAtual == TipoTema::ESCURO) ? escuroTextoNormal : claroTextoNormal;
}

/**
 * @brief Retorna a cor base dos botões.
 * @return SDL_Color para o estado ocioso dos botões no tema atual.
 */
SDL_Color GerenciadorTemas::getCorBotaoNormal() const {
    return (temaAtual == TipoTema::ESCURO) ? escuroBtnNormal : claroBtnNormal;
}

/**
 * @brief Retorna a cor de destaque para botões sob o cursor.
 * @return SDL_Color para o estado de Hover.
 */
SDL_Color GerenciadorTemas::getCorBotaoHover() const {
    return (temaAtual == TipoTema::ESCURO) ? escuroBtnHover : claroBtnHover;
}

/**
 * @brief Retorna a cor de feedback para botões clicados.
 * @return SDL_Color para o estado de Pressionado.
 */
SDL_Color GerenciadorTemas::getCorBotaoPressionado() const {
    return (temaAtual == TipoTema::ESCURO) ? escuroBtnPressionado : claroBtnPressionado;
}

/**
 * @brief Retorna a cor utilizada para bordas de foco de navegação.
 * 
 * Implementa uma lógica de visibilidade específica:
 * - No Tema Escuro: Utiliza uma cor próxima ao branco (alto brilho) para destacar contra o fundo roxo.
 * - No Tema Claro: Utiliza um tom azulado para garantir contraste contra as cores mais vibrantes do tema.
 * 
 * @return SDL_Color configurada para o feedback visual do controle (Gamepad).
 */
SDL_Color GerenciadorTemas::getCorDestaque() const {
    if (temaAtual == TipoTema::ESCURO) return {90, 15, 107, 255}; 
    else return {215, 171, 235, 255}; 
}