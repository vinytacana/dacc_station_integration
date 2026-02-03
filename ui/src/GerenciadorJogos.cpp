/**
 * @file GerenciadorJogos.cpp
 * @brief Implementação da lógica de gerenciamento e busca do catálogo de jogos.
 * 
 * Este arquivo contém os algoritmos para manipulação da lista de jogos em memória,
 * provendo funcionalidades de filtragem por nome, categoria e identificadores únicos,
 * sempre prezando pela facilidade de busca (case-insensitive).
 */

#include "GerenciadorJogos.hpp"
#include <algorithm>
#include <cctype>

using namespace MeuProjeto;

/**
 * @brief Insere um novo objeto Jogo na coleção interna.
 * @param jogo Referência para o objeto a ser copiado para o vetor.
 */
void GerenciadorJogos::adicionarJogo(const Jogo& jogo) {
    jogos.push_back(jogo);
}

/**
 * @brief Retorna a lista completa de jogos cadastrados.
 * @return std::vector<Jogo> Uma cópia do vetor de jogos.
 */
std::vector<Jogo> GerenciadorJogos::listarJogos() const {
    return jogos;
}

/**
 * @brief Localiza um jogo através de uma comparação exata de código.
 * 
 * @param codigo String contendo o identificador único.
 * @return Jogo* Ponteiro para o objeto encontrado ou nullptr caso não exista.
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
 * @brief Realiza uma busca por nome utilizando correspondência parcial de texto.
 * 
 * O algoritmo converte tanto o termo de busca quanto o nome do jogo para minúsculo
 * antes da comparação, garantindo que "Aventura" e "aventura" retornem o mesmo resultado.
 * 
 * @param buscado Termo de pesquisa fornecido pelo usuário.
 * @return std::vector<Jogo> Lista de jogos cujos nomes contêm a substring buscada.
 */
std::vector<Jogo> GerenciadorJogos::buscarPorNome(const std::string& buscado) {
    if (buscado.empty()) {
        return {};
    }

    std::string buscaMinuscula = converterMinusculo(buscado);
    std::vector<Jogo> resultados;

    for (const auto& jogo : jogos) {
        std::string nomeMinusculo = converterMinusculo(jogo.getNome());
        // Verifica se a string de busca está contida no nome do jogo
        if (encontrarSubstrings(nomeMinusculo, buscaMinuscula)) {
            resultados.push_back(jogo);
        }
    }

    return resultados;
}

/**
 * @brief Filtra a biblioteca baseando-se em categorias/tags.
 * 
 * Esta função trata o campo 'codigo' do objeto Jogo como um repositório de metadados.
 * Se a categoria for "Todas" ou estiver vazia, a função retorna ponteiros para 
 * toda a biblioteca. Caso contrário, verifica se a categoria solicitada existe 
 * dentro das tags do jogo.
 * 
 * @param categoria Nome da categoria para filtro (ex: "RPG", "Ação").
 * @return std::vector<Jogo*> Vetor de ponteiros para os jogos correspondentes.
 */
std::vector<Jogo*> GerenciadorJogos::buscarPorCategoria(const std::string& categoria) {
    std::vector<Jogo*> resultados;
    
    // Caso especial: "Todas" desativa o filtro de categoria
    if (categoria.empty() || categoria == "Todas") {
        for (auto& jogo : jogos) {
            resultados.push_back(&jogo);
        }
        return resultados;
    }
    
    // Normalização para busca case-insensitive
    std::string catMinuscula = converterMinusculo(categoria);
    
    for (auto& jogo : jogos) {
        // No sistema atual, o campo 'codigo' é usado para armazenar as categorias/tags
        std::string codigoMinusculo = converterMinusculo(jogo.getCodigo());
        
        /**
         * @note A busca por substring permite que um jogo tenha múltiplas categorias
         * separadas por vírgula ou espaço no campo de código.
         */
        if (encontrarSubstrings(codigoMinusculo, catMinuscula)) {
            resultados.push_back(&jogo);
        }
    }
    
    return resultados;
}

/**
 * @brief Acessa um jogo diretamente pelo seu índice no container.
 * 
 * Realiza uma verificação de limites (bounds check) antes de retornar o ponteiro.
 * 
 * @param indice Posição no vetor de jogos.
 * @return Jogo* Ponteiro para o jogo ou nullptr se o índice for inválido.
 */
Jogo* GerenciadorJogos::obterJogoPorIndice(size_t indice) {
    if (indice < jogos.size()) {
        return &jogos[indice];
    }
    return nullptr;
}

/**
 * @brief Converte todos os caracteres de uma string para minúsculo.
 * 
 * Utiliza o algoritmo `std::transform` junto com a função `::tolower` da 
 * biblioteca padrão para processar a string de forma eficiente.
 * 
 * @param str String original a ser convertida.
 * @return std::string Nova string totalmente em minúsculo.
 */
std::string GerenciadorJogos::converterMinusculo(std::string str) const {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

/**
 * @brief Helper para verificar a existência de uma substring dentro de outra.
 * 
 * @param texto String principal (alvo).
 * @param busca String a ser procurada (padrão).
 * @return true se 'busca' existir dentro de 'texto'.
 */
bool GerenciadorJogos::encontrarSubstrings(const std::string& texto, const std::string& busca) const {
    return texto.find(busca) != std::string::npos;
}