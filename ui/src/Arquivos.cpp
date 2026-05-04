/**
 * @file Arquivos.cpp
 * @brief Implementação dos métodos da classe Arquivos.
 * 
 * Este arquivo contém a lógica detalhada para persistência de dados em disco,
 * utilizando um formato de arquivo de texto proprietário baseado em prefixos
 * de caracteres para identificar os atributos dos objetos Jogo.
 */

#include "Arquivos.hpp"
#include "Utils.hpp"
#include <fstream>   
#include <sstream>   

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Verifica a existência de um arquivo no disco
 * 
 * Tenta abrir o arquivo especificado para leitura e verifica se a operação
 * foi bem-sucedida. Método não-invasivo que não modifica o arquivo nem mantém
 * o stream aberto após a verificação.
 * 
 * @param caminho Caminho completo (absoluto ou relativo) do arquivo a ser verificado
 * @return true se o arquivo existe e pode ser aberto, false caso contrário
 */
bool Arquivos::arquivoExiste(const string& caminho)
{
    ifstream arquivo(caminho_absoluto_projeto(caminho));
    return arquivo.good();
}

/**
 * @brief Serializa a lista de jogos para um arquivo de texto proprietário
 * 
 * Persiste o vetor completo de objetos Jogo em disco usando um formato de texto
 * estruturado baseado em prefixos de linha. Cada jogo é gravado como um bloco
 * separado por linha vazia, e cada atributo é prefixado por um caractere especial
 * que identifica o tipo de dado.
 * 
 * Protocolo de gravação (prefixos de linha):
 * - @ : Nome do Jogo
 * - # : Descrição Longa (exibida na Janela de detalhes)
 * - > : Descrição Curta (exibida no Popup)
 * - $ : Código Identificador único do jogo
 * - ^ : Caminho do executável do jogo
 * - * : Caminho da Capa Principal (usada no grid/tela principal)
 * - & : Caminho do Fundo de Destaque (usado quando o jogo está em foco)
 * - + : Caminho da Capa da Janela Interna (usada em modais/popups)
 * - % : Lista de capturas de tela separadas por ponto e vírgula (;)
 * 
 * Formato de exemplo:
 * @Nome do Jogo
 * #Descrição longa aqui...
 * >Descrição curta
 * $GAME001
 * ^/path/to/game.exe
 * "*"/path/to/cover.png
 * &/path/to/background.png
 * +/path/to/window_cover.png
 * %screenshot1.png;screenshot2.png;screenshot3.png
 * 
 * @param caminho Caminho de destino onde o arquivo será criado/sobrescrito
 * @param jogos Vetor contendo todos os objetos Jogo a serem persistidos
 * @return true se a gravação ocorreu sem erros, false se o arquivo não pôde ser aberto
 */
bool Arquivos::salvarJogos(const string& caminho, const vector<Jogo>& jogos)
{
    ofstream arquivo(caminho_absoluto_projeto(caminho));
    if (!arquivo.is_open()) return false;

    for (const auto& jogo : jogos)
    {
        arquivo << "@" << jogo.getNome() << "\n";
        arquivo << "#" << jogo.getDescricaoLonga() << "\n";
        arquivo << ">" << jogo.getDescricaoCurta() << "\n";
        arquivo << "$" << jogo.getCodigo() << "\n"; 
        arquivo << "^" << jogo.getJogoExecutavel() << "\n";
        
        // Imagens de interface
        arquivo << "*" << jogo.getCapaTelaPrincipal() << "\n";
        arquivo << "&" << jogo.getFundoDestaque() << "\n";
        arquivo << "+" << jogo.getCapaJanelaJogo() << "\n";
        
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
 * @brief Desserializa jogos de um arquivo de texto para o gerenciador
 * 
 * Lê o arquivo de dados no formato proprietário e reconstrói os objetos Jogo,
 * adicionando-os ao GerenciadorJogos fornecido. O processo de parsing identifica
 * cada atributo através do caractere prefixo e acumula os dados até encontrar
 * o delimitador final (%), momento em que cria e adiciona o jogo completo.
 * 
 * Processo de leitura:
 * 1. Lê o arquivo linha por linha
 * 2. Identifica o tipo de dado pelo caractere prefixo
 * 3. Extrai o conteúdo removendo o prefixo
 * 4. Acumula os atributos em variáveis temporárias
 * 5. Ao encontrar '%' (capturas), constrói o objeto Jogo completo
 * 6. Adiciona o jogo ao gerenciador
 * 7. Reinicializa as variáveis para o próximo jogo
 * 
 * Tratamento especial:
 * - Remove caracteres de retorno de carro (\r) para compatibilidade multiplataforma
 * - Ignora linhas vazias
 * - Faz parsing de capturas separadas por ';' usando stringstream
 * - Garante inicialização segura de todas as variáveis temporárias
 * 
 * @param caminho Caminho do arquivo de dados a ser lido
 * @param gerenciador Referência ao GerenciadorJogos onde os jogos serão adicionados
 * @return true se a leitura ocorreu sem erros, false se o arquivo não pôde ser aberto
 */
bool Arquivos::carregarJogos(const string& caminho, GerenciadorJogos& gerenciador)
{
    ifstream arquivo(caminho_absoluto_projeto(caminho));
    if (!arquivo.is_open()) return false;

    string linha;

    // Variáveis temporárias para acumular dados de cada jogo
    string nome, descricaoLonga, descricaoCurta, codigo, jogoExecutavel; 
    string capaPrincipal, fundoDestaque, capaJanela;
    vector<string> capturas;

    // Inicialização segura de todas as strings
    nome = ""; descricaoLonga = ""; descricaoCurta = ""; codigo = ""; jogoExecutavel = ""; 

    while (getline(arquivo, linha))
    {
        // Pula linhas vazias
        if (linha.empty()) continue;
        
        // Remove carriage return (\r) para compatibilidade Windows/Unix
        if (linha.back() == '\r') linha.pop_back(); 
        if (linha.empty()) continue;

        // Extrai o caractere delimitador (prefixo) e o conteúdo
        char delimitador = linha[0];
        string conteudo = (linha.length() > 1) ? linha.substr(1) : "";

        // Switch para processar cada tipo de atributo baseado no prefixo
        switch (delimitador)
        {
            case '@': nome = conteudo; break;
            case '#': descricaoLonga = conteudo; break;
            case '>': descricaoCurta = conteudo; break;
            case '$': codigo = conteudo; break; 
            case '^': jogoExecutavel = conteudo; break; 
            
            case '*': capaPrincipal = conteudo; break;
            case '&': fundoDestaque = conteudo; break; 
            case '+': capaJanela = conteudo; break;   
            
            // Caso '%': último atributo, indica fim do bloco do jogo
            case '%':
                // Limpa vetor de capturas para o novo jogo
                capturas.clear();
                {
                    // Faz parsing da lista de capturas separadas por ';'
                    stringstream ss(conteudo);
                    string captura_path;
                    while (getline(ss, captura_path, ';'))
                    {
                        capturas.push_back(caminho_absoluto_projeto(captura_path));
                    }
                }

                capaPrincipal = caminho_absoluto_projeto(capaPrincipal);
                fundoDestaque = caminho_absoluto_projeto(fundoDestaque);
                capaJanela = caminho_absoluto_projeto(capaJanela);
                
                // Constrói e adiciona o jogo completo ao gerenciador
                gerenciador.adicionarJogo(Jogo(nome, 
                                             descricaoLonga, 
                                             descricaoCurta, 
                                             codigo,          // Código/Categoria
                                             jogoExecutavel,  // Caminho do executável
                                             capaPrincipal, 
                                             fundoDestaque, 
                                             capaJanela, 
                                             capturas));
                
                // Reinicialização das variáveis temporárias para o próximo jogo
                nome = ""; descricaoLonga = ""; descricaoCurta = ""; codigo = ""; jogoExecutavel = "";
                capaPrincipal = ""; fundoDestaque = ""; capaJanela = "";
                break;
        }
    }

    arquivo.close();
    return true;
}
