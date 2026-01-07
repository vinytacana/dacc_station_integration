#include "Jogo.hpp"

using namespace MeuProjeto;

/**
 * @brief Construtor completo da classe Jogo.
 *
 * @param nome O nome do jogo.
 * @param descricao A descrição do jogo.
 * @param codigo O código.
 * @param capa O caminho da capa.
 * @param capturas O vetor de caminhos das capturas.
 * @param executavel O caminho do executável.
 */
Jogo::Jogo(const std::string& nome, 
           const std::string& descricao, 
           const std::string& codigo, 
           const std::string& capa,
           const std::vector<std::string>& capturas,
           const std::string& executavel,
           int ano,
           int players,
           const std::string& genero)
    : nome(nome), ano(ano), players(players), genero(genero), descricao(descricao), codigo(codigo), id(""), capa(capa), capturas(capturas), executavel(executavel)
{
}

// Implementação dos Getters

/**
 * @brief Obtém o nome do jogo.
 * @return O nome do jogo.
 */
std::string Jogo::getNome() const
{
    return nome;
}

int Jogo::getAno() const { return ano; }
int Jogo::getPlayers() const { return players; }
std::string Jogo::getGenero() const { return genero; }

/**
 * @brief Obtém a descrição do jogo.
 * @return A descrição do jogo.
 */
std::string Jogo::getDescricao() const
{
    return descricao;
}

std::string Jogo::getCodigo() const {
    return codigo;
}

std::string Jogo::getId() const {
    return id;
}

std::string Jogo::getCapa() const {
    return capa;
}

/**
 * @brief Obtém os caminhos das capturas de tela adicionais.
 * @return Um vetor de caminhos das capturas.
 */
std::vector<std::string> Jogo::getCapturas() const
{
    return capturas;
}

/**
 * @brief Obtém o caminho do executável do jogo.
 * @return O caminho do script ou binário.
 */
std::string Jogo::getExecutavel() const
{
    return executavel;
}

/**
 * @brief Obtém todos os caminhos de imagem (capa seguida pelas capturas) para compatibilidade.
 * @return Um vetor contendo a capa e todas as capturas.
 */
std::vector<std::string> Jogo::getImagens() const
{
    std::vector<std::string> todasImagens;
    
    // Adiciona a capa primeiro
    if (!capa.empty()) {
        todasImagens.push_back(capa);
    }
    
    // Adiciona todas as capturas
    for (const auto& captura : capturas) {
        todasImagens.push_back(captura);
    }
    
    return todasImagens;
}

// Implementação dos Setters

/**
 * @brief Define o nome do jogo.
 * @param nome O novo nome.
 */
void Jogo::setNome(const std::string& nome)
{
    this->nome = nome;
}

/**
 * @brief Define a descrição do jogo.
 * @param descricao A nova descrição.
 */
void Jogo::setDescricao(const std::string& descricao)
{
    this->descricao = descricao;
}

void Jogo::setCodigo(const std::string& codigo) {
    this->codigo = codigo;
}

void Jogo::setId(const std::string& id) {
    this->id = id;
}

void Jogo::setCapa(const std::string& capa) {
    this->capa = capa;
}

/**
 * @brief Define os caminhos das capturas de tela adicionais.
 * @param capturas O novo vetor de caminhos de capturas.
 */
void Jogo::setCapturas(const std::vector<std::string>& capturas)
{
    this->capturas = capturas;
}

/**
 * @brief Define o caminho do executável.
 * @param executavel O novo caminho.
 */
void Jogo::setExecutavel(const std::string& executavel)
{
    this->executavel = executavel;
}

/**
 * @brief Define todas as imagens. A primeira imagem se torna a capa e o resto as capturas.
 * @param imagens O vetor contendo todos os caminhos de imagem.
 */
void Jogo::setImagens(const std::vector<std::string>& imagens)
{
    // Para compatibilidade: primeira imagem vira capa, resto vira capturas
    if (!imagens.empty()) {
        this->capa = imagens[0];
        this->capturas.clear();
        for (size_t i = 1; i < imagens.size(); i++) {
            this->capturas.push_back(imagens[i]);
        }
    } else {
        this->capa.clear();
        this->capturas.clear();
    }
}

void Jogo::setAno(int ano) { this->ano = ano; }
void Jogo::setPlayers(int players) { this->players = players; }
void Jogo::setGenero(const std::string& genero) { this->genero = genero; }