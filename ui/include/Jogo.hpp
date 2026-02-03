/**
 * @file Jogo.hpp
 * @brief Definição da classe Jogo.
 */

#ifndef JOGO_HPP
#define JOGO_HPP

#pragma once

#include <string>
#include <vector>

namespace MeuProjeto {

/**
 * @class Jogo
 * @brief Classe que representa um jogo na biblioteca/catálogo
 * 
 * Esta classe encapsula todas as informações e metadados relacionados a um jogo,
 * incluindo nome, descrições, imagens de capa, capturas de tela e o caminho para
 * o executável do jogo. Funciona como um modelo de dados (data model) para gerenciar
 * jogos individuais no sistema.
 * 
 * Cada jogo possui múltiplas representações visuais (capa principal, fundo de destaque,
 * capa de janela) e textual (descrição longa e curta), além de uma coleção de capturas
 * de tela para visualização prévia.
 */
class Jogo
{
public:
    /**
     * @brief Construtor completo da classe Jogo
     * 
     * Inicializa um objeto Jogo com todos os seus atributos. Este construtor permite
     * criar uma instância completa do jogo com todas as informações necessárias
     * para exibição e execução.
     * 
     * @param nome Nome/título do jogo
     * @param descricaoLonga Descrição detalhada do jogo (até aproximadamente 600 caracteres),
     *                       usada em telas de detalhes ou informações expandidas
     * @param descricaoCurta Descrição resumida do jogo (até aproximadamente 260 caracteres),
     *                       usada em listagens ou previews rápidas
     * @param codigo Código único identificador do jogo (ID, SKU ou código interno)
     * @param jogoExecutavel Caminho completo para o arquivo executável do jogo
     * @param capaTelaPrincipal Caminho para a imagem de capa exibida na tela principal/grid
     * @param fundoDestaque Caminho para a imagem de fundo usada quando o jogo está em destaque
     * @param capaJanelaJogo Caminho para a imagem de capa exibida na janela/modal do jogo
     * @param capturas Vetor contendo caminhos para todas as capturas de tela (screenshots) do jogo
     */
    Jogo(const std::string& nome, 
         const std::string& descricaoLonga, 
         const std::string& descricaoCurta, 
         const std::string& codigo, 
         const std::string& jogoExecutavel,
         const std::string& capaTelaPrincipal,
         const std::string& fundoDestaque,
         const std::string& capaJanelaJogo,
         const std::vector<std::string>& capturas);

    // Getters
    
    /**
     * @brief Obtém o nome do jogo
     * @return String contendo o nome/título do jogo
     */
    std::string getNome() const;
    
    /**
     * @brief Obtém a descrição longa do jogo
     * @return String contendo a descrição detalhada (aproximadamente 600 caracteres)
     */
    std::string getDescricaoLonga() const;
    
    /**
     * @brief Obtém a descrição curta do jogo
     * @return String contendo a descrição resumida (aproximadamente 260 caracteres)
     */
    std::string getDescricaoCurta() const; 
    
    /**
     * @brief Obtém o código identificador do jogo
     * @return String contendo o código único do jogo
     */
    std::string getCodigo() const;
    
    /**
     * @brief Obtém o caminho para o executável do jogo
     * @return String contendo o caminho completo para o arquivo executável
     */
    std::string getJogoExecutavel() const;
    
    /**
     * @brief Obtém o caminho da capa da tela principal
     * @return String contendo o caminho para a imagem de capa exibida na tela principal
     */
    std::string getCapaTelaPrincipal() const;
    
    /**
     * @brief Obtém o caminho da imagem de fundo para destaque
     * @return String contendo o caminho para a imagem de fundo usada quando em destaque
     */
    std::string getFundoDestaque() const;
    
    /**
     * @brief Obtém o caminho da capa da janela do jogo
     * @return String contendo o caminho para a imagem de capa da janela/modal do jogo
     */
    std::string getCapaJanelaJogo() const;
    
    /**
     * @brief Obtém todas as capturas de tela do jogo
     * @return Referência constante para o vetor contendo os caminhos de todas as screenshots
     */
    const std::vector<std::string>& getCapturas() const;

    // Setters
    
    /**
     * @brief Define o nome do jogo
     * @param nome Novo nome/título do jogo
     */
    void setNome(const std::string& nome);
    
    /**
     * @brief Define a descrição longa do jogo
     * @param descricao Nova descrição detalhada (recomendado até 600 caracteres)
     */
    void setDescricaoLonga(const std::string& descricao);
    
    /**
     * @brief Define a descrição curta do jogo
     * @param desc Nova descrição resumida (recomendado até 260 caracteres)
     */
    void setDescricaoCurta(const std::string& desc);
    
    /**
     * @brief Define o código identificador do jogo
     * @param codigo Novo código único do jogo
     */
    void setCodigo(const std::string& codigo);
    
    /**
     * @brief Define o caminho para o executável do jogo
     * @param executavel Novo caminho completo para o arquivo executável
     */
    void setJogoExecutavel(const std::string& executavel);
    
    /**
     * @brief Define o caminho da capa da tela principal
     * @param caminho Novo caminho para a imagem de capa da tela principal
     */
    void setCapaTelaPrincipal(const std::string& caminho);
    
    /**
     * @brief Define o caminho da imagem de fundo para destaque
     * @param caminho Novo caminho para a imagem de fundo de destaque
     */
    void setFundoDestaque(const std::string& caminho);
    
    /**
     * @brief Define o caminho da capa da janela do jogo
     * @param caminho Novo caminho para a imagem de capa da janela/modal
     */
    void setCapaJanelaJogo(const std::string& caminho);
    
    /**
     * @brief Define todas as capturas de tela do jogo
     * @param capturas Novo vetor contendo os caminhos de todas as screenshots
     */
    void setCapturas(const std::vector<std::string>& capturas);

    /**
     * @brief Define imagens do jogo (método auxiliar)
     * 
     * Método genérico para definir uma coleção de imagens relacionadas ao jogo.
     * O comportamento específico depende da implementação.
     * 
     * @param imagens Vetor contendo caminhos para imagens
     */
    void setImagens(const std::vector<std::string>& imagens);

private:
    /**
     * @brief Nome/título do jogo
     */
    std::string nome;
    
    /**
     * @brief Descrição detalhada do jogo (600 caracteres)
     * 
     * Usada em telas de informações completas, modais ou páginas de detalhes do jogo.
     */
    std::string descricaoLonga;
    
    /**
     * @brief Descrição resumida do jogo (260 caracteres)
     * 
     * Usada em listagens, cards ou previews onde o espaço é limitado.
     */
    std::string descricaoCurta;
    
    /**
     * @brief Código identificador único do jogo
     * 
     * Pode ser um ID interno, SKU ou qualquer código usado para identificar o jogo
     * de forma única no sistema.
     */
    std::string codigo;
    
    /**
     * @brief Caminho completo para o arquivo executável do jogo
     * 
     * Utilizado para lançar/iniciar o jogo quando selecionado pelo usuário.
     */
    std::string jogoExecutavel;
    
    /**
     * @brief Caminho para a imagem de capa exibida na tela principal
     * 
     * Imagem usada nos grids, listagens ou galeria principal de jogos.
     */
    std::string capaTelaPrincipal;
    
    /**
     * @brief Caminho para a imagem de fundo usada quando o jogo está em destaque
     * 
     * Imagem de background exibida quando o jogo é selecionado ou destacado na interface.
     */
    std::string fundoDestaque;
    
    /**
     * @brief Caminho para a imagem de capa da janela/modal do jogo
     * 
     * Imagem de capa específica para exibição em janelas popup ou modais de detalhes.
     */
    std::string capaJanelaJogo;
    
    /**
     * @brief Vetor contendo caminhos para todas as capturas de tela do jogo
     * 
     * Coleção de screenshots que podem ser exibidas em galerias, carrosséis ou
     * previews visuais do jogo.
     */
    std::vector<std::string> capturas;
};

} // namespace MeuProjeto

#endif // JOGO_HPP