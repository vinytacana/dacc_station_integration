#ifndef JOGO_HPP
#define JOGO_HPP

#pragma once

#include <string>
#include <vector>

namespace MeuProjeto {

/**
 * @class Jogo
 * @brief Representa um jogo com suas informações básicas e caminhos para imagens.
 *
 * A classe separa explicitamente a imagem de capa (principal) das demais
 * capturas de tela, facilitando o gerenciamento de recursos.
 */
class Jogo
{
public:
    /**
     * @brief Construtor completo da classe Jogo.
     *
     * @param nome O nome ou título do jogo.
     * @param descricao Uma breve descrição do jogo.
     * @param codigo O código de identificação ou categorias do jogo.
     * @param capa O caminho para o arquivo da imagem de capa (principal).
     * @param capturas Um vetor de caminhos para os arquivos das capturas de tela adicionais.
     * @param executavel O caminho para o script ou binário executável do jogo.
     */
    Jogo(const std::string& nome, 
         const std::string& descricao, 
         const std::string& codigo, 
         const std::string& capa,
         const std::vector<std::string>& capturas,
         const std::string& executavel = "");

    // --- Getters ---
    /**
     * @brief Obtém o nome do jogo.
     * @return O nome do jogo.
     */
    std::string getNome() const;

    /**
     * @brief Obtém a descrição do jogo.
     * @return A descrição do jogo.
     */
    std::string getDescricao() const;

    /**
     * @brief Obtém o código (ou categorias) do jogo.
     * @return O código do jogo.
     */
    std::string getCodigo() const;

    /**
     * @brief Obtém o caminho da imagem de capa (principal).
     * @return O caminho do arquivo de capa.
     */
    std::string getCapa() const;

    /**
     * @brief Obtém os caminhos das capturas de tela adicionais.
     * @return Um vetor de caminhos das capturas.
     */
    std::vector<std::string> getCapturas() const;

    /**
     * @brief Obtém o caminho do executável do jogo.
     * @return O caminho do script ou binário.
     */
    std::string getExecutavel() const;

    /**
     * @brief Obtém todos os caminhos de imagem (capa seguida pelas capturas) para compatibilidade.
     * @return Um vetor contendo a capa e todas as capturas.
     */
    std::vector<std::string> getImagens() const;

    // --- Setters ---
    /**
     * @brief Define o nome do jogo.
     * @param nome O novo nome.
     */
    void setNome(const std::string& nome);

    /**
     * @brief Define a descrição do jogo.
     * @param descricao A nova descrição.
     */
    void setDescricao(const std::string& descricao);

    /**
     * @brief Define o código (ou categorias) do jogo.
     * @param codigo O novo código.
     */
    void setCodigo(const std::string& codigo);

    /**
     * @brief Define o caminho da imagem de capa.
     * @param capa O novo caminho da capa.
     */
    void setCapa(const std::string& capa);

    /**
     * @brief Define os caminhos das capturas de tela adicionais.
     * @param capturas O novo vetor de caminhos de capturas.
     */
    void setCapturas(const std::vector<std::string>& capturas);

    /**
     * @brief Define o caminho do executável.
     * @param executavel O novo caminho.
     */
    void setExecutavel(const std::string& executavel);

    /**
     * @brief Define todas as imagens. A primeira imagem se torna a capa e o resto as capturas.
     * @param imagens O vetor contendo todos os caminhos de imagem.
     */
    void setImagens(const std::vector<std::string>& imagens);

private:
    /**
     * @brief Nome do jogo.
     */
    std::string nome;

    /**
     * @brief Descrição do jogo.
     */
    std::string descricao;

    /**
     * @brief Código de identificação ou categorias.
     */
    std::string codigo;

    /**
     * @brief Caminho para a imagem principal (capa).
     */
    std::string capa;

    /**
     * @brief Vetor de caminhos para imagens secundárias (capturas de tela).
     */
    std::vector<std::string> capturas;

    /**
     * @brief Caminho para o executável (script .sh ou binário).
     */
    std::string executavel;
};

} // namespace MeuProjeto
#endif // JOGO_HPP