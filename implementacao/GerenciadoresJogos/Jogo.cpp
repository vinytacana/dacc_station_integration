/**
 * @file Jogo.cpp
 * @brief Implementação dos métodos da classe Jogo.
 * 
 * Este arquivo contém a lógica dos construtores, seletores (getters) e 
 * modificadores (setters) para a classe Jogo, gerenciando as informações 
 * textuais e os caminhos de ativos visuais de cada título.
 */

#include "GerenciadoresJogos/Jogo.hpp"

using namespace MeuProjeto;

/**
 * @brief Construtor da classe Jogo.
 * 
 * Inicializa todos os campos do jogo através de uma lista de inicialização
 * para maior eficiência.
 * 
 * @param nome Nome do jogo.
 * @param descricao Descrição detalhada.
 * @param codigo Código identificador único.
 * @param capaTelaPrincipal Caminho da imagem para a grade principal.
 * @param fundoDestaque Caminho da imagem de fundo (background).
 * @param capaJanelaJogo Caminho da imagem para a interface interna da janela.
 * @param capturas Vetor contendo os caminhos das capturas de tela (screenshots).
 */
Jogo::Jogo(const std::string& nome, 
           const std::string& descricao, 
           const std::string& codigo, 
           const std::string& capaTelaPrincipal,
           const std::string& fundoDestaque,
           const std::string& capaJanelaJogo,
           const std::vector<std::string>& capturas)
    : nome(nome), descricao(descricao), codigo(codigo), 
      capaTelaPrincipal(capaTelaPrincipal), 
      fundoDestaque(fundoDestaque), 
      capaJanelaJogo(capaJanelaJogo), 
      capturas(capturas)
{
}

// Getters

/** @brief Retorna o nome do jogo. */
std::string Jogo::getNome() const { return nome; }

/** @brief Retorna a descrição do jogo. */
std::string Jogo::getDescricao() const { return descricao; }

/** @brief Retorna o código único do jogo. */
std::string Jogo::getCodigo() const { return codigo; }

/** @brief Retorna o caminho da imagem da capa na tela principal. */
std::string Jogo::getCapaTelaPrincipal() const { return capaTelaPrincipal; }

/** @brief Retorna o caminho da imagem de fundo de destaque. */
std::string Jogo::getFundoDestaque() const { return fundoDestaque; }

/** @brief Retorna o caminho da imagem da capa dentro da janela do jogo. */
std::string Jogo::getCapaJanelaJogo() const { return capaJanelaJogo; }

/** 
 * @brief Método de compatibilidade para obter a capa.
 * @return O mesmo que getCapaTelaPrincipal().
 */
std::string Jogo::getCapa() const { return capaTelaPrincipal; }

/** @brief Retorna o vetor com os caminhos de todas as capturas de tela. */
std::vector<std::string> Jogo::getCapturas() const { return capturas; }

/** 
 * @brief Agrega todos os caminhos de imagens em um único vetor.
 * 
 * Este método é útil para processamento em lote de ativos ou para sistemas 
 * legados que esperam uma lista única de strings de imagem.
 * 
 * @return std::vector<std::string> Lista contendo capa, fundo, imagem interna e capturas.
 */
std::vector<std::string> Jogo::getImagens() const
{
    std::vector<std::string> todasImagens;
    if (!capaTelaPrincipal.empty()) todasImagens.push_back(capaTelaPrincipal);
    if (!fundoDestaque.empty()) todasImagens.push_back(fundoDestaque);
    if (!capaJanelaJogo.empty()) todasImagens.push_back(capaJanelaJogo);
    for (const auto& cap : capturas) todasImagens.push_back(cap);
    return todasImagens;
}

// Setters

/** @brief Define o nome do jogo. */
void Jogo::setNome(const std::string& nome) { this->nome = nome; }

/** @brief Define a descrição do jogo. */
void Jogo::setDescricao(const std::string& descricao) { this->descricao = descricao; }

/** @brief Define o código identificador. */
void Jogo::setCodigo(const std::string& codigo) { this->codigo = codigo; }

/** @brief Define o caminho da imagem da capa principal. */
void Jogo::setCapaTelaPrincipal(const std::string& caminho) { this->capaTelaPrincipal = caminho; }

/** @brief Define o caminho da imagem de fundo. */
void Jogo::setFundoDestaque(const std::string& caminho) { this->fundoDestaque = caminho; }

/** @brief Define o caminho da imagem da capa interna. */
void Jogo::setCapaJanelaJogo(const std::string& caminho) { this->capaJanelaJogo = caminho; }

/** @brief Define o vetor de caminhos para as capturas de tela. */
void Jogo::setCapturas(const std::vector<std::string>& capturas) { this->capturas = capturas; }

/** 
 * @brief Método de compatibilidade para definir imagens através de uma lista única.
 * 
 * Implementa a seguinte lógica de mapeamento:
 * - O índice 0 é atribuído à Capa Principal.
 * - Os índices de 1 em diante são atribuídos ao vetor de Capturas.
 * - Fundo e Capa Interna não são alterados por este método.
 * 
 * @param imagens Vetor de strings com os caminhos das imagens.
 */
void Jogo::setImagens(const std::vector<std::string>& imagens)
{
    if (!imagens.empty()) {
        this->capaTelaPrincipal = imagens[0];
        this->capturas.clear();
        for (size_t i = 1; i < imagens.size(); i++) {
            this->capturas.push_back(imagens[i]);
        }
    }
}