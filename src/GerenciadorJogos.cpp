#include "GerenciadorJogos.hpp"
#include <algorithm>
#include <cctype>

using namespace MeuProjeto;

/**
 * @brief Adiciona uma cópia de um objeto Jogo à coleção.
 *
 * @param jogo O objeto @c Jogo a ser adicionado.
 */
void GerenciadorJogos::adicionarJogo(const Jogo& jogo) {
    jogos.push_back(jogo);
}

/**
 * @brief Retorna uma cópia da lista completa de jogos.
 *
 * @return Um vetor contendo cópias de todos os objetos @c Jogo.
 */
std::vector<Jogo> GerenciadorJogos::listarJogos() const {
    return jogos;
}

/**
 * @brief Busca um jogo pelo seu código único.
 *
 * @param codigo O código identificador do jogo.
 * @return Um ponteiro para o objeto @c Jogo se encontrado, ou @c nullptr.
 */
Jogo* GerenciadorJogos::buscarPorCodigo(const std::string& codigo) {
    for (auto& jogo : jogos) {
        if (jogo.getCodigo() == codigo) {
            return &jogo;
        }
    }
    return nullptr;
}

/**
 * @brief Busca jogos cujo nome contenha o texto buscado (busca case-insensitive).
 *
 * @param buscado O termo de busca (substring).
 * @return Um vetor contendo cópias dos objetos @c Jogo que correspondem à busca.
 */
std::vector<Jogo> GerenciadorJogos::buscarPorNome(const std::string& buscado) {
    if (buscado.empty()) {
        return {};
    }

    std::string buscaMinuscula = converterMinusculo(buscado);
    std::vector<Jogo> resultados;

    for (const auto& jogo : jogos) {
        std::string nomeMinusculo = converterMinusculo(jogo.getNome());
        if (encontrarSubstrings(nomeMinusculo, buscaMinuscula)) {
            resultados.push_back(jogo);
        }
    }

    return resultados;
}

/**
 * @brief Busca jogos cujos códigos contenham a categoria especificada (busca case-insensitive).
 *
 * @param categoria O nome da categoria a ser buscada.
 * @return Um vetor de ponteiros para os objetos @c Jogo que correspondem à categoria.
 */
std::vector<Jogo*> GerenciadorJogos::buscarPorCategoria(const std::string& categoria) {
    std::vector<Jogo*> resultados;
    
    // Se for "Todas" ou vazio, retornar todos os jogos
    if (categoria.empty() || categoria == "Todas") {
        for (auto& jogo : jogos) {
            resultados.push_back(&jogo);
        }
        return resultados;
    }
    
    // Converter categoria para minúsculo para comparação 
    std::string catMinuscula = converterMinusculo(categoria);
    
    for (auto& jogo : jogos) {
        std::string codigoMinusculo = converterMinusculo(jogo.getCodigo());
        
        // Verificar se a categoria está presente no código do jogo
        if (encontrarSubstrings(codigoMinusculo, catMinuscula)) {
            resultados.push_back(&jogo);
        }
    }
    
    return resultados;
}

/**
 * @brief Obtém um jogo pelo seu índice na coleção interna.
 *
 * @param indice O índice do jogo (baseado em zero).
 * @return Um ponteiro para o objeto @c Jogo se o índice for válido, ou @c nullptr.
 */
Jogo* GerenciadorJogos::obterJogoPorIndice(size_t indice) {
    if (indice < jogos.size()) {
        return &jogos[indice];
    }
    return nullptr;
}

/**
 * @brief Converte uma string para minúsculas 
 *
 * @param str A string de entrada.
 * @return A string convertida para minúsculas.
 */
std::string GerenciadorJogos::converterMinusculo(std::string str) const {
    // std::transform usa a função ::tolower global, que funciona para ASCII.
    // Para Unicode completo, seria necessário usar <locale> ou bibliotecas externas.
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

/**
 * @brief Verifica se a string de busca é uma substring do texto fornecido.
 *
 * @param texto A string completa.
 * @param busca A substring a ser encontrada.
 * @return Retorna @c true se a busca for encontrada no texto.
 */
bool GerenciadorJogos::encontrarSubstrings(const std::string& texto, const std::string& busca) const {
    return texto.find(busca) != std::string::npos;
}