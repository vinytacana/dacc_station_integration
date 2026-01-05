#include "Arquivos.hpp"
#include <fstream>   
#include <sstream>   

using namespace MeuProjeto;
using namespace std;

/**
 * @brief Verifica se um arquivo existe no caminho especificado.
 *
 * @param caminho O caminho completo para o arquivo.
 * @return Retorna @c true se o arquivo existir e for bom (good), @c false caso contrário.
 */
bool Arquivos::arquivoExiste(const string& caminho)
{
    ifstream arquivo(caminho);
    return arquivo.good();
}

/**
 * @brief Salva uma coleção de jogos em um arquivo de texto.
 *
 * @param caminho O caminho completo onde o arquivo de jogos será salvo.
 * @param jogos Uma referência constante para o vetor de objetos @c Jogo.
 * @return Retorna @c true em caso de sucesso, @c false se a abertura do arquivo falhar.
 */
bool Arquivos::salvarJogos(const string& caminho, const vector<Jogo>& jogos)
{
    ofstream arquivo(caminho);
    if (!arquivo.is_open())
    {
        return false;
    }

    for (const auto& jogo : jogos)
    {
        // Campos obrigatórios: Nome (@), Descrição (#), Código ($)
        arquivo << "@" << jogo.getNome() << "\n";
        arquivo << "#" << jogo.getDescricao() << "\n";
        arquivo << "$" << jogo.getCodigo() << "\n";
        
        // Executável (campo &)
        if (!jogo.getExecutavel().empty()) {
            arquivo << "&" << jogo.getExecutavel() << "\n";
        }
        
        // Capa (campo *)
        arquivo << "*" << jogo.getCapa() << "\n";
        
        // Capturas (campo %)
        arquivo << "%";
        const auto& capturas = jogo.getCapturas();
        for (size_t i = 0; i < capturas.size(); ++i)
        {
            arquivo << capturas[i];
            if (i < capturas.size() - 1)
            {
                arquivo << ";"; // Separa as capturas por ponto e vírgula
            }
        }
        arquivo << "\n\n"; // Duas quebras de linha para separar os registros
    }

    arquivo.close();
    return true;
}

/**
 * @brief Carrega dados de jogos de um arquivo e os adiciona ao gerenciador.
 *
 * @param caminho O caminho completo do arquivo de jogos.
 * @param gerenciador Uma referência para o objeto @c GerenciadorJogos.
 * @return Retorna @c true em caso de sucesso, @c false se a abertura do arquivo falhar.
 */
bool Arquivos::carregarJogos(const string& caminho, GerenciadorJogos& gerenciador)
{
    ifstream arquivo(caminho);
    if (!arquivo.is_open())
    {
        return false;
    }

    string linha, nome, descricao, codigo, capa, executavel;
    vector<string> capturas;

    while (getline(arquivo, linha))
    {
        if (linha.empty())
        {
            continue;
        }

        // Pega o delimitador e o conteúdo
        char delimitador = linha[0];
        string conteudo = linha.substr(1);

        switch (delimitador)
        {
                // Nome do jogo
            case '@':
                nome = conteudo;
                break;
                // Descrição do jogo
            case '#':
                descricao = conteudo;
                break;
                // Categorias do jogo
            case '$':
                codigo = conteudo;
                break;
                
            case '&':
                // Caminho do executável
                executavel = conteudo;
                break;

            case '*':
                // Capa principal (apenas 1 imagem)
                capa = conteudo;
                break;
                
            case '%':
                // Capturas de tela (múltiplas, separadas por ;)
                capturas.clear();
                {
                    stringstream ss(conteudo);
                    string captura_path;
                    while (getline(ss, captura_path, ';'))
                    {
                        capturas.push_back(captura_path);
                    }
                }
                
                // Adicionar jogo ao gerenciador após coletar todos os campos
                gerenciador.adicionarJogo(Jogo(nome, descricao, codigo, capa, capturas, executavel));
                // Limpar variáveis para o próximo jogo (opcional, mas boa prática se houver bugs no arquivo)
                executavel = ""; 
                break;
        }
    }

    arquivo.close();
    return true;
}