/**
 * @file Jogo.cpp
 * @brief Implementação dos métodos da classe Jogo.
 * 
 * Este arquivo contém a implementação completa da classe Jogo, incluindo
 * construtor, getters e setters para todos os atributos relacionados a
 * um jogo no sistema. A classe encapsula informações como nome, descrições,
 * caminhos de recursos visuais (capas, fundos, capturas de tela) e o
 * executável do jogo.
 */

#include "Jogo.hpp"

using namespace MeuProjeto;

/**
 * @brief Construtor parametrizado da classe Jogo.
 * 
 * Inicializa todos os atributos de um objeto Jogo com os valores fornecidos.
 * Este construtor permite criar uma instância completa do jogo com todas as
 * informações necessárias para exibição na interface e execução.
 * 
 * @param nome Título do jogo que será exibido na interface.
 * @param descricaoLonga Descrição detalhada do jogo, usada na tela de detalhes.
 * @param descricaoCurta Sinopse breve do jogo, exibida em previews e listas.
 * @param codigo Identificador único do jogo no sistema (ID ou código de catálogo).
 * @param jogoExecutavel Caminho completo ou relativo para o arquivo executável do jogo.
 * @param capaTelaPrincipal Caminho para a imagem de capa exibida na tela principal/grade.
 * @param fundoDestaque Caminho para a imagem de fundo usada quando o jogo está em destaque.
 * @param capaJanelaJogo Caminho para a imagem de capa exibida na janela de detalhes do jogo.
 * @param capturas Vetor contendo os caminhos de todas as capturas de tela (screenshots) do jogo.
 * 
 * @note Todos os caminhos de arquivo devem ser válidos e acessíveis para evitar
 *       erros durante o carregamento de recursos visuais.
 */
Jogo::Jogo(const std::string& nome, 
           const std::string& descricaoLonga, 
           const std::string& descricaoCurta,
           const std::string& codigo,
           const std::string& jogoExecutavel, 
           const std::string& capaTelaPrincipal,
           const std::string& fundoDestaque,
           const std::string& capaJanelaJogo,
           const std::vector<std::string>& capturas)
    : nome(nome), 
      descricaoLonga(descricaoLonga),
      descricaoCurta(descricaoCurta),
      codigo(codigo), 
      jogoExecutavel(jogoExecutavel),
      capaTelaPrincipal(capaTelaPrincipal), 
      fundoDestaque(fundoDestaque), 
      capaJanelaJogo(capaJanelaJogo),
      capturas(capturas)
{
}

// GETTERS - Métodos de Acesso aos Atributos

/**
 * @brief Retorna o nome/título do jogo.
 * 
 * @return std::string Nome completo do jogo.
 */
std::string Jogo::getNome() const { return nome; }

/**
 * @brief Retorna a descrição longa e detalhada do jogo.
 * 
 * Esta descrição geralmente contém informações extensas sobre a história,
 * gameplay, características principais e outros detalhes relevantes.
 * Utilizada principalmente na tela de detalhes do jogo.
 * 
 * @return std::string Descrição completa do jogo.
 */
std::string Jogo::getDescricaoLonga() const { return descricaoLonga; } 

/**
 * @brief Retorna a descrição curta/sinopse do jogo.
 * 
 * Versão resumida da descrição, ideal para exibição em cards, tooltips
 * ou listagens onde o espaço é limitado.
 * 
 * @return std::string Sinopse breve do jogo.
 */
std::string Jogo::getDescricaoCurta() const { return descricaoCurta; }

/**
 * @brief Retorna o código identificador único do jogo.
 * 
 * Este código pode ser usado para identificação interna, busca em banco
 * de dados ou referência em sistemas de gerenciamento de biblioteca.
 * 
 * @return std::string Código/ID único do jogo.
 */
std::string Jogo::getCodigo() const { return codigo; }

/**
 * @brief Retorna o caminho do arquivo executável do jogo.
 * 
 * Este caminho aponta para o arquivo que deve ser executado para iniciar
 * o jogo. Pode ser relativo ao diretório da aplicação ou absoluto.
 * 
 * @return std::string Caminho completo para o executável (.exe, .sh, etc).
 */
std::string Jogo::getJogoExecutavel() const { return jogoExecutavel; }

/**
 * @brief Retorna o caminho da imagem de capa para a tela principal.
 * 
 * Esta imagem é tipicamente exibida na grade/carrossel da tela inicial,
 * representando visualmente o jogo na biblioteca.
 * 
 * @return std::string Caminho para a imagem de capa principal.
 */
std::string Jogo::getCapaTelaPrincipal() const { return capaTelaPrincipal; }

/**
 * @brief Retorna o caminho da imagem de fundo para modo destaque.
 * 
 * Utilizada como plano de fundo quando o jogo está em foco ou selecionado,
 * proporcionando contexto visual imersivo na interface.
 * 
 * @return std::string Caminho para a imagem de fundo em destaque.
 */
std::string Jogo::getFundoDestaque() const { return fundoDestaque; }

/**
 * @brief Retorna o caminho da imagem de capa para a janela de detalhes.
 * 
 * Esta capa é exibida na tela de informações detalhadas do jogo,
 * geralmente em tamanho maior ou formato diferente da capa principal.
 * 
 * @return std::string Caminho para a capa da janela de detalhes.
 */
std::string Jogo::getCapaJanelaJogo() const { return capaJanelaJogo; }

/**
 * @brief Retorna o vetor de caminhos das capturas de tela do jogo.
 * 
 * Cada elemento do vetor representa o caminho para uma screenshot do jogo,
 * usadas para criar galerias de imagens na interface de detalhes.
 * 
 * @return const std::vector<std::string>& Referência constante ao vetor de capturas.
 * 
 * @note A referência constante evita cópia desnecessária de dados ao acessar
 *       o vetor, melhorando a performance.
 */
const std::vector<std::string>& Jogo::getCapturas() const { return capturas; }


// SETTERS - Métodos de Modificação dos Atributos

/**
 * @brief Define/atualiza o nome do jogo.
 * 
 * @param nome Novo nome/título a ser atribuído ao jogo.
 */
void Jogo::setNome(const std::string& nome) { this->nome = nome; }

/**
 * @brief Define/atualiza a descrição longa do jogo.
 * 
 * Permite modificar a descrição detalhada do jogo após a criação do objeto.
 * Útil para atualizações de conteúdo ou correções de texto.
 * 
 * @param descricao Nova descrição completa do jogo.
 */
void Jogo::setDescricaoLonga(const std::string& descricao) { this->descricaoLonga = descricao; } 

/**
 * @brief Define/atualiza a descrição curta do jogo.
 * 
 * @param desc Nova sinopse breve do jogo.
 */
void Jogo::setDescricaoCurta(const std::string& desc) { this->descricaoCurta = desc; }

/**
 * @brief Define/atualiza o código identificador do jogo.
 * 
 * @param codigo Novo código/ID único a ser atribuído.
 */
void Jogo::setCodigo(const std::string& codigo) { this->codigo = codigo; }

/**
 * @brief Define/atualiza o caminho do executável do jogo.
 * 
 * Permite reconfigurar qual arquivo deve ser executado ao iniciar o jogo.
 * Útil para atualizações de versão ou mudanças na estrutura de diretórios.
 * 
 * @param jogoExecutavel Novo caminho para o arquivo executável.
 */
void Jogo::setJogoExecutavel(const std::string& jogoExecutavel) { this->jogoExecutavel = jogoExecutavel; }

/**
 * @brief Define/atualiza o caminho da capa da tela principal.
 * 
 * @param caminho Novo caminho para a imagem de capa principal.
 */
void Jogo::setCapaTelaPrincipal(const std::string& caminho) { this->capaTelaPrincipal = caminho; }

/**
 * @brief Define/atualiza o caminho da imagem de fundo em destaque.
 * 
 * @param caminho Novo caminho para a imagem de fundo.
 */
void Jogo::setFundoDestaque(const std::string& caminho) { this->fundoDestaque = caminho; }

/**
 * @brief Define/atualiza o caminho da capa da janela de detalhes.
 * 
 * @param caminho Novo caminho para a capa da janela de jogo.
 */
void Jogo::setCapaJanelaJogo(const std::string& caminho) { this->capaJanelaJogo = caminho; }

/**
 * @brief Substitui completamente o vetor de capturas de tela.
 * 
 * Remove todas as capturas anteriores e define um novo conjunto completo.
 * 
 * @param capturas Novo vetor contendo os caminhos das capturas de tela.
 */
void Jogo::setCapturas(const std::vector<std::string>& capturas) { this->capturas = capturas; }

/**
 * @brief Define imagens do jogo a partir de um vetor unificado.
 * 
 * Esta função processa um vetor de caminhos de imagem seguindo a convenção:
 * - O primeiro elemento (índice 0) é atribuído como a capa da tela principal.
 * - Todos os elementos subsequentes (índice 1 em diante) são adicionados
 *   ao vetor de capturas de tela.
 * 
 * Útil para inicialização rápida quando todas as imagens estão organizadas
 * em uma única estrutura de dados.
 * 
 * @param imagens Vetor contendo capa (primeiro elemento) seguida de capturas.
 * 
 * @note Se o vetor estiver vazio, nenhuma operação é realizada.
 * @note As capturas existentes NÃO são limpas; novas capturas são adicionadas.
 * 
 * @warning Esta função adiciona capturas sem limpar as anteriores. Para substituir
 *          completamente, use setCapturas() antes de chamar setImagens().
 */
void Jogo::setImagens(const std::vector<std::string>& imagens)
{
    /**
     * Verifica se há pelo menos uma imagem disponível.
     * A primeira imagem é sempre designada como capa principal.
     */
    if (!imagens.empty()) {
        this->capaTelaPrincipal = imagens[0];
    }
    
    /**
     * Itera sobre os elementos restantes (a partir do índice 1)
     * e adiciona cada um ao vetor de capturas de tela.
     * 
     * O loop usa size_t para compatibilidade com o tipo de retorno de size().
     */
    for (size_t i = 1; i < imagens.size(); ++i) {
        this->capturas.push_back(imagens[i]);
    }
}