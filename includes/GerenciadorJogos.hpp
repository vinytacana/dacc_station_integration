#ifndef GERENCIADOR_JOGOS_HPP
#define GERENCIADOR_JOGOS_HPP

#pragma once

#include <vector>
#include <string>
#include "Jogo.hpp"

namespace MeuProjeto {

/**
 * @class GerenciadorJogos
 * @brief Responsável por gerenciar a coleção principal de objetos @c Jogo.
 *
 * Oferece métodos para adicionar, listar e buscar jogos por diferentes critérios
 * (código, nome e categoria).
 */
class GerenciadorJogos
{
public:
    /**
     * @brief Adiciona uma cópia de um objeto Jogo à coleção.
     *
     * @param jogo O objeto @c Jogo a ser adicionado.
     */
    void adicionarJogo(const Jogo& jogo);

    /**
     * @brief Retorna uma cópia da lista completa de jogos.
     *
     * @return Um vetor contendo cópias de todos os objetos @c Jogo.
     */
    std::vector<Jogo> listarJogos() const;

    /**
     * @brief Busca um jogo pelo seu código único.
     *
     * @param codigo O código identificador do jogo.
     * @return Um ponteiro para o objeto @c Jogo se encontrado, ou @c nullptr caso contrário.
     */
    Jogo* buscarPorCodigo(const std::string& codigo);

    /**
     * @brief Busca jogos cujo nome contenha o texto buscado (busca case-insensitive).
     *
     * @param buscado O termo de busca (substring).
     * @return Um vetor contendo cópias dos objetos @c Jogo que correspondem à busca.
     */
    std::vector<Jogo> buscarPorNome(const std::string& buscado);
    
    /**
     * @brief Busca jogos cujos códigos contenham a categoria especificada (busca case-insensitive).
     *
     * @param categoria O nome da categoria a ser buscada.
     * @return Um vetor de ponteiros para os objetos @c Jogo que correspondem à categoria.
     */
    std::vector<Jogo*> buscarPorCategoria(const std::string& categoria);
    
    /**
     * @brief Obtém um jogo pelo seu índice na coleção interna.
     *
     * @param indice O índice do jogo (baseado em zero).
     * @return Um ponteiro para o objeto @c Jogo se o índice for válido, ou @c nullptr caso contrário.
     */
    Jogo* obterJogoPorIndice(size_t indice);

private:
    /**
     * @brief Coleção interna de objetos Jogo.
     */
    std::vector<Jogo> jogos;

    /**
     * @brief Converte uma string para minúsculas (útil para comparações case-insensitive).
     *
     * @param str A string de entrada.
     * @return A string convertida para minúsculas.
     */
    std::string converterMinusculo(std::string str) const;

    /**
     * @brief Verifica se a string de busca é uma substring do texto fornecido.
     *
     * @param texto A string completa (e.g., nome do jogo em minúsculas).
     * @param busca A substring a ser encontrada.
     * @return Retorna @c true se a busca for encontrada no texto.
     */
    bool encontrarSubstrings(const std::string& texto, const std::string& busca) const;
}; 

} // namespace MeuProjeto

/**
 * @brief Declaração externa do gerenciador de jogos global.
 */
extern MeuProjeto::GerenciadorJogos gerenciadorJogos;

#endif // GERENCIADOR_JOGOS_HPP