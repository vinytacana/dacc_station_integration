/**
 * @file Jogo.hpp
 * @brief Definição da classe Jogo que representa a entidade de um game no sistema.
 * 
 * Este arquivo contém a estrutura de dados necessária para armazenar todas as 
 * informações de um jogo, incluindo metadados textuais e referências para 
 * recursos visuais (capas, fundos e capturas de tela).
 */

#ifndef JOGO_HPP
#define JOGO_HPP

#pragma once

#include <string>
#include <vector>

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class Jogo
 * @brief Representa um jogo com suas informações básicas e imagens específicas.
 * 
 * A classe Jogo funciona como um modelo de dados (Data Object) que centraliza
 * as informações de um título. Ela diferencia três tipos de imagens principais
 * para permitir uma interface rica (estilo Steam), onde um jogo tem uma miniatura
 * para a lista, um fundo para destaque e uma imagem interna para sua própria página.
 */
class Jogo
{
public:
    /**
     * @brief Construtor parametrizado para inicializar um jogo completo.
     * 
     * @param nome Nome oficial do jogo.
     * @param descricao Breve texto descritivo ou sinopse do jogo.
     * @param codigo Identificador único (ID) ou código de referência do jogo.
     * @param capaTelaPrincipal Caminho do arquivo de imagem para a listagem/grid principal.
     * @param fundoDestaque Caminho do arquivo de imagem usado como wallpaper/fundo de tela.
     * @param capaJanelaJogo Caminho do arquivo de imagem para exibição interna na janela do jogo.
     * @param capturas Vetor de strings contendo caminhos para screenshots (capturas de tela).
     */
    Jogo(const std::string& nome, 
         const std::string& descricao, 
         const std::string& codigo, 
         const std::string& capaTelaPrincipal, 
         const std::string& fundoDestaque,    
         const std::string& capaJanelaJogo,    
         const std::vector<std::string>& capturas);

    // --- Getters (Métodos de Acesso) ---

    /** @brief Retorna o nome do jogo. */
    std::string getNome() const;

    /** @brief Retorna a descrição ou sinopse do jogo. */
    std::string getDescricao() const;

    /** @brief Retorna o código identificador do jogo. */
    std::string getCodigo() const;
    
    /** 
     * @brief Retorna o caminho da imagem usada na lista/categoria (thumbnail).
     * @return String com o caminho do arquivo.
     */
    std::string getCapaTelaPrincipal() const;

    /** 
     * @brief Retorna o caminho da imagem de fundo (Hero Image/Background).
     * @return String com o caminho do arquivo.
     */
    std::string getFundoDestaque() const;

    /** 
     * @brief Retorna o caminho da imagem interna da janela do jogo.
     * @return String com o caminho do arquivo.
     */
    std::string getCapaJanelaJogo() const;
    
    /** 
     * @brief Retorna a capa principal. Mantido para compatibilidade com versões anteriores.
     * @note Internamente mapeia para getCapaTelaPrincipal().
     * @return String com o caminho do arquivo.
     */
    std::string getCapa() const; 
    
    /** @brief Retorna o vetor contendo os caminhos de todas as capturas de tela. */
    std::vector<std::string> getCapturas() const;

    /** 
     * @brief Retorna o vetor de imagens. Mantido para compatibilidade.
     * @note Geralmente retorna a lista de capturas de tela.
     */
    std::vector<std::string> getImagens() const;

    // --- Setters (Métodos de Modificação) ---

    /** @brief Define ou altera o nome do jogo. */
    void setNome(const std::string& nome);

    /** @brief Define ou altera a descrição do jogo. */
    void setDescricao(const std::string& descricao);

    /** @brief Define ou altera o código identificador. */
    void setCodigo(const std::string& codigo);
    
    /** @brief Define o caminho da imagem para a grade/lista principal. */
    void setCapaTelaPrincipal(const std::string& caminho);

    /** @brief Define o caminho da imagem de fundo/destaque. */
    void setFundoDestaque(const std::string& caminho);

    /** @brief Define o caminho da imagem interna da janela. */
    void setCapaJanelaJogo(const std::string& caminho);
    
    /** @brief Define a lista de caminhos para as capturas de tela. */
    void setCapturas(const std::vector<std::string>& capturas);

    /** 
     * @brief Define a lista de imagens. Mantido para compatibilidade.
     * @param imagens Vetor de strings com os caminhos.
     */
    void setImagens(const std::vector<std::string>& imagens);

private:
    std::string nome;               /**< Nome do título. */
    std::string descricao;          /**< Texto de descrição. */
    std::string codigo;             /**< Identificador (ex: ID na Steam ou SKU). */
    
    std::string capaTelaPrincipal;  /**< Caminho: Imagem vertical/horizontal da grade. */
    std::string fundoDestaque;     /**< Caminho: Imagem panorâmica de fundo. */
    std::string capaJanelaJogo;    /**< Caminho: Banner interno. */
    
    std::vector<std::string> capturas; /**< Coleção de paths para screenshots. */
};

} // namespace MeuProjeto
#endif // JOGO_HPP