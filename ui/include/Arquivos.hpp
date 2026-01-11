/**
 * @file Arquivos.hpp
 * @brief Declaração da classe Arquivos responsável pela persistência de dados.
 * 
 * Este arquivo contém as definições da classe Arquivos, que provê funcionalidades
 * auxiliares para verificação de existência de arquivos e operações de leitura
 * e escrita da biblioteca de jogos no disco.
 */

#ifndef ARQUIVOS_HPP
#define ARQUIVOS_HPP
#pragma once

#include <string>
#include <vector>
#include "GerenciadorJogos.hpp"

/**
 * @namespace MeuProjeto
 * @brief Namespace principal do projeto da interface de biblioteca de jogos.
 */
namespace MeuProjeto {

/**
 * @class Arquivos
 * @brief Agrupa funções estáticas para lidar com a persistência de dados de jogos.
 * 
 * A classe Arquivos atua como uma classe utilitária (helper class), o que significa
 * que não precisa ser instanciada. Seus métodos permitem gerenciar como os dados
 * da aplicação (como a lista de jogos instalados ou cadastrados) são salvos ou 
 * recuperados de arquivos externos.
 */
class Arquivos
{
public:
    /**
     * @brief Verifica se um determinado arquivo existe no sistema de arquivos.
     * 
     * Este método é útil para validar caminhos de executáveis de jogos ou
     * verificar se o arquivo de banco de dados/configuração já foi criado.
     * 
     * @param caminho Uma string contendo o caminho (relativo ou absoluto) do arquivo.
     * @return true Se o arquivo existir e o sistema tiver permissão de acesso.
     * @return false Caso o arquivo não seja encontrado ou não possa ser acessado.
     */
    static bool arquivoExiste(const std::string& caminho);

    /**
     * @brief Salva a lista de jogos atual em um arquivo de dados.
     * 
     * Recebe um vetor de objetos do tipo Jogo e serializa esses dados para
     * um arquivo no caminho especificado. Geralmente utilizado ao fechar a 
     * aplicação ou ao adicionar um novo jogo à biblioteca.
     * 
     * @param caminho Caminho do arquivo onde os dados serão persistidos.
     * @param jogos Um vetor (std::vector) contendo os objetos Jogo a serem salvos.
     * @return true Se a operação de escrita foi concluída com sucesso.
     * @return false Se houver falha ao abrir o arquivo ou erro durante a escrita.
     */
    static bool salvarJogos(const std::string& caminho, const std::vector<Jogo>& jogos);

    /**
     * @brief Carrega os dados dos jogos a partir de um arquivo para o Gerenciador de Jogos.
     * 
     * Lê as informações persistidas no disco e as converte de volta para objetos Jogo,
     * inserindo-os na instância do GerenciadorJogos fornecida. Este método é 
     * fundamental para restaurar a biblioteca do usuário ao iniciar o programa.
     * 
     * @param caminho Caminho do arquivo de onde os dados serão lidos.
     * @param gerenciador Referência para o objeto GerenciadorJogos que armazenará os jogos carregados.
     * @return true Se o carregamento e o parsing dos dados ocorrerem sem erros.
     * @return false Se o arquivo não existir, estiver corrompido ou não puder ser lido.
     */
    static bool carregarJogos(const std::string& caminho, GerenciadorJogos& gerenciador);
};

} // namespace MeuProjeto
#endif // Fim de ARQUIVOS_HPP