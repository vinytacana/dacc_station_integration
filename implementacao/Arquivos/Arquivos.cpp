/**
 * @file Arquivos.cpp
 * @brief Implementação dos métodos da classe Arquivos.
 * 
 * Este arquivo contém a lógica detalhada para persistência de dados em disco,
 * utilizando um formato de arquivo de texto proprietário baseado em prefixos
 * de caracteres para identificar os atributos dos objetos Jogo.
 */

#include "Arquivos/Arquivos.hpp"
#include <fstream>   
#include <sstream>   

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Verifica a existência de um arquivo no disco.
 * 
 * A implementação utiliza a tentativa de abertura de um fluxo de entrada (ifstream).
 * Se o fluxo for considerado 'bom' (good), significa que o arquivo existe e o
 * sistema possui permissões de leitura.
 * 
 * @param caminho String contendo o caminho do arquivo.
 * @return true se o arquivo puder ser acessado, false caso contrário.
 */
bool Arquivos::arquivoExiste(const string& caminho)
{
    ifstream arquivo(caminho);
    return arquivo.good();
}

/**
 * @brief Serializa a lista de jogos para um arquivo de texto.
 * 
 * A implementação segue o seguinte protocolo de gravação:
 * - @ : Nome do Jogo
 * - # : Descrição
 * - $ : Código Identificador
 * - * : Caminho da Capa Principal
 * - & : Caminho do Fundo de Destaque
 * - + : Caminho da Capa da Janela Interna
 * - % : Lista de capturas de tela separadas por ponto e vírgula (;)
 * 
 * @param caminho Destino do arquivo de salvamento.
 * @param jogos Vetor de objetos Jogo a serem persistidos.
 * @return true se a gravação ocorrer sem erros, false se o arquivo não puder ser aberto.
 */
bool Arquivos::salvarJogos(const string& caminho, const vector<Jogo>& jogos)
{
    ofstream arquivo(caminho);
    if (!arquivo.is_open()) return false;

    for (const auto& jogo : jogos)
    {
        arquivo << "@" << jogo.getNome() << "\n";
        arquivo << "#" << jogo.getDescricao() << "\n";
        arquivo << "$" << jogo.getCodigo() << "\n";
        
        // Imagens de interface
        arquivo << "*" << jogo.getCapaTelaPrincipal() << "\n";
        arquivo << "&" << jogo.getFundoDestaque() << "\n";
        arquivo << "+" << jogo.getCapaJanelaJogo() << "\n";
        
        // Serialização do vetor de capturas
        arquivo << "%";
        const auto& capturas = jogo.getCapturas();
        for (size_t i = 0; i < capturas.size(); ++i)
        {
            arquivo << capturas[i];
            if (i < capturas.size() - 1) arquivo << ";";
        }
        arquivo << "\n\n";
    }

    arquivo.close();
    return true;
}

/**
 * @brief Carrega jogos de um arquivo e os insere no Gerenciador de Jogos.
 * 
 * A implementação utiliza um analisador (parser) baseado em linhas. Cada linha é
 * lida, o primeiro caractere é extraído como delimitador e o restante da string
 * é processado como o valor do atributo. Quando o delimitador de capturas (%)
 * é encontrado, o objeto Jogo é instanciado com os dados acumulados e adicionado
 * ao gerenciador.
 * 
 * @param caminho Origem do arquivo de dados.
 * @param gerenciador Referência para o objeto que armazenará os jogos carregados.
 * @return true se a leitura for concluída, false se houver falha ao abrir o arquivo.
 */
bool Arquivos::carregarJogos(const string& caminho, GerenciadorJogos& gerenciador)
{
    ifstream arquivo(caminho);
    if (!arquivo.is_open()) return false;

    string linha;
    // Variáveis temporárias para acumular dados durante o parsing do bloco
    string nome, descricao, codigo;
    string capaPrincipal, fundoDestaque, capaJanela;
    vector<string> capturas;

    while (getline(arquivo, linha))
    {
        if (linha.empty()) continue;

        char delimitador = linha[0];
        string conteudo = linha.substr(1);

        switch (delimitador)
        {
            case '@': nome = conteudo; break;
            case '#': descricao = conteudo; break;
            case '$': codigo = conteudo; break;
            
            case '*': capaPrincipal = conteudo; break;
            case '&': fundoDestaque = conteudo; break; 
            case '+': capaJanela = conteudo; break;   
            
            case '%':
                capturas.clear();
                {
                    // Quebra a string de capturas usando o delimitador ';'
                    stringstream ss(conteudo);
                    string captura_path;
                    while (getline(ss, captura_path, ';'))
                    {
                        capturas.push_back(captura_path);
                    }
                }
                
                // Finaliza a leitura do bloco do jogo e o registra no sistema
                gerenciador.adicionarJogo(Jogo(nome, descricao, codigo, 
                                             capaPrincipal, fundoDestaque, capaJanela, 
                                             capturas));
                
                // Reinicialização para garantir integridade no próximo bloco de dados
                nome = ""; descricao = ""; codigo = "";
                capaPrincipal = ""; fundoDestaque = ""; capaJanela = "";
                break;
        }
    }

    arquivo.close();
    return true;
}