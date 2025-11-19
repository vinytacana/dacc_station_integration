#ifndef ARQUIVOS_HPP
#define ARQUIVOS_HPP

#pragma once

#include <string>
#include <vector>
#include "GerenciadorJogos.hpp" 

namespace MeuProjeto
{

/**
 * @class Arquivos
 * @brief Agrupa funções estáticas para lidar com a persistência de dados de jogos.
 *
 * Esta classe é responsável por ler e escrever dados de jogos
 * em arquivos de texto, garantindo que o progresso da aplicação
 * possa ser salvo e restaurado.
 */
class Arquivos
{
public:
    /**
     * @brief Verifica se um arquivo existe no caminho especificado.
     *
     * @param caminho O caminho completo para o arquivo a ser verificado.
     * @return Retorna @c true se o arquivo existir e puder ser aberto, @c false caso contrário.
     */
    static bool arquivoExiste(const std::string& caminho);

    /**
     * @brief Salva uma coleção de jogos em um arquivo de texto.
     *
     * Os dados de cada jogo são formatados com delimitadores específicos
     * (@, #, $, *, %) para facilitar a leitura.
     *
     * @param caminho O caminho completo onde o arquivo de jogos será salvo.
     * @param jogos Uma referência constante para o vetor de objetos @c Jogo a serem salvos.
     * @return Retorna @c true em caso de sucesso, @c false se não for possível abrir o arquivo para escrita.
     */
    static bool salvarJogos(const std::string& caminho, const std::vector<Jogo>& jogos);

    /**
     * @brief Carrega dados de jogos de um arquivo e os adiciona ao gerenciador.
     *
     * O arquivo é lido linha por linha, e os dados são separados
     * pelos delimitadores esperados (@, #, $, *, %).
     *
     * @param caminho O caminho completo do arquivo de jogos a ser lido.
     * @param gerenciador Uma referência para o objeto @c GerenciadorJogos onde os jogos serão adicionados.
     * @return Retorna @c true em caso de sucesso, @c false se não for possível abrir o arquivo para leitura.
     */
    static bool carregarJogos(const std::string& caminho, GerenciadorJogos& gerenciador);
};

} // namespace MeuProjeto

#endif // ARQUIVOS_HPP