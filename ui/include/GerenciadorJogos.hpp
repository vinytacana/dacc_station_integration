/**
 * @file GerenciadorJogos.hpp
 * @brief Definição da classe GerenciadorJogos para administração do catálogo de jogos.
 * 
 * Este arquivo contém a lógica de gerenciamento da coleção de jogos, permitindo
 * operações de adição, busca por diferentes critérios (nome, código, categoria)
 * e listagem geral para exibição na interface.
 */

#ifndef GERENCIADOR_JOGOS_HPP
#define GERENCIADOR_JOGOS_HPP

#pragma once

#include <vector>
#include <string>
#include "Jogo.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class GerenciadorJogos
 * @brief Responsável por gerenciar a coleção de objetos Jogo.
 * 
 * A classe GerenciadorJogos centraliza todos os títulos disponíveis na biblioteca.
 * Ela oferece métodos de busca otimizados (case-insensitive) e auxilia na 
 * organização dos jogos por categorias, facilitando a filtragem na interface "Steam".
 */
class GerenciadorJogos
{
public:
    /**
     * @brief Adiciona um novo jogo ao catálogo gerenciado.
     * @param jogo Referência constante para o objeto Jogo a ser inserido.
     */
    void adicionarJogo(const Jogo& jogo);

    /**
     * @brief Retorna uma cópia de todos os jogos cadastrados.
     * @return std::vector<Jogo> Vetor contendo todos os objetos Jogo.
     */
    std::vector<Jogo> listarJogos() const;

    /**
     * @brief Busca um jogo específico através do seu código identificador único.
     * 
     * @param codigo String contendo o código/ID do jogo.
     * @return Jogo* Ponteiro para o jogo encontrado, ou nullptr caso não exista.
     */
    Jogo* buscarPorCodigo(const std::string& codigo);

    /**
     * @brief Busca jogos cujo nome contenha o termo pesquisado.
     * 
     * A busca é parcial e ignora diferenças entre maiúsculas e minúsculas.
     * 
     * @param buscado Termo ou parte do nome a ser pesquisado.
     * @return std::vector<Jogo> Lista de jogos que atendem ao critério de busca.
     */
    std::vector<Jogo> buscarPorNome(const std::string& buscado);
    
    /**
     * @brief Filtra a biblioteca para retornar apenas jogos de uma determinada categoria.
     * 
     * @param categoria Nome da categoria (ex: "Ação", "RPG").
     * @return std::vector<Jogo*> Vetor de ponteiros para os jogos que pertencem à categoria.
     */
    std::vector<Jogo*> buscarPorCategoria(const std::string& categoria);
    
    /**
     * @brief Recupera um jogo baseado em sua posição no vetor interno.
     * 
     * Muito útil para mapear cliques em grades ou listas da interface gráfica 
     * diretamente para o objeto de dados correspondente.
     * 
     * @param indice Posição numérica no vetor.
     * @return Jogo* Ponteiro para o jogo na posição informada, ou nullptr se o índice for inválido.
     */
    Jogo* obterJogoPorIndice(size_t indice);

private:
    /** @brief Recipiente interno que armazena todos os objetos Jogo da biblioteca. */
    std::vector<Jogo> jogos;

    /**
     * @brief Função auxiliar para converter strings para minúsculo.
     * Utilizada para realizar buscas que ignoram a capitalização (case-insensitive).
     * 
     * @param str String original.
     * @return std::string Versão em minúsculo da string original.
     */
    std::string converterMinusculo(std::string str) const;

    /**
     * @brief Verifica se uma string de busca está contida em um texto alvo.
     * 
     * @param texto O texto completo onde a busca será feita.
     * @param busca O termo a ser procurado.
     * @return true se o termo de busca for encontrado dentro do texto.
     */
    bool encontrarSubstrings(const std::string& texto, const std::string& busca) const;
}; 

} // namespace MeuProjeto

/**
 * @brief Instância global do gerenciador de jogos.
 * 
 * Permite que diferentes partes do sistema (como a tela principal ou o sistema de arquivos)
 * acessem a mesma base de dados de jogos de forma consistente.
 */
extern MeuProjeto::GerenciadorJogos gerenciadorJogos;

#endif // GERENCIADOR_JOGOS_HPP