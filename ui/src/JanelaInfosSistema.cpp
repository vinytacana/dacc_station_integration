/**
 * @file JanelaInfosSistema.cpp
 * 
 * @details Este arquivo implementa a classe JanelaInfosSistema, responsável por
 * coletar, armazenar e exibir informações detalhadas sobre o hardware do sistema,
 * informações do projeto e lista de colaboradores. A interface inclui funcionalidades
 * de scroll interativo, suporte a temas claro/escuro e carregamento dinâmico de
 * recursos visuais.
 *
 */

#include "JanelaInfosSistema.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "GerenciadorImagens.hpp"
#include "ConfigLayout.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <sys/utsname.h>
#include <fstream>

using namespace MeuProjeto;

/// @brief Instância global do gerenciador de imagens
extern GerenciadorImagens gerImg;

/**
 * @brief Construtor padrão da classe JanelaInfosSistema
 * 
 * @details Inicializa todos os componentes necessários para a janela de informações
 * do sistema, incluindo:
 * - Coleta de informações de hardware
 * - Inicialização dos dados do projeto
 * - Carregamento da lista de colaboradores
 * 
 * @note Este construtor não recebe parâmetros e inicializa o objeto em um estado
 * totalmente funcional
 * 
 * @see coletarInfosHardware()
 * @see inicializarInfosProjeto()
 * @see inicializarColaboradores()
 */
JanelaInfosSistema::JanelaInfosSistema() {
    coletarInfosHardware();
    inicializarInfosProjeto();
    inicializarColaboradores();
}

/**
 * @brief Destrutor da classe JanelaInfosSistema
 * 
 * @details Realiza a limpeza de recursos alocados durante o ciclo de vida do objeto:
 * - Limpa o vetor de colaboradores
 * - Libera texturas carregadas na memória de vídeo
 * 
 * @note A liberação das texturas é delegada ao GerenciadorImagens para evitar
 * vazamentos de memória
 * 
 * @see liberarTexturas()
 */
JanelaInfosSistema::~JanelaInfosSistema() {
    colaboradores.clear();
    liberarTexturas();
}

/**
 * @brief Libera todas as texturas carregadas pela janela
 * 
 * @details Este método reseta o ponteiro da textura explicativa para nullptr.
 * A destruição efetiva da textura é gerenciada pelo GerenciadorImagens, que
 * mantém controle centralizado sobre todos os recursos gráficos.
 * 
 * @warning Não chamar SDL_DestroyTexture diretamente, pois o GerenciadorImagens
 * cuida deste processo
 * 
 * @post texturaExplicacao == nullptr
 */
void JanelaInfosSistema::liberarTexturas() {
    // GerenciadorImagens cuida da destruição
    texturaExplicacao = nullptr;
}

/**
 * @brief Coleta informações detalhadas sobre o hardware do sistema
 * 
 * @details Este método preenche a estrutura de hardware com informações obtidas
 * através de chamadas ao sistema operacional e à SDL2:
 * - Modelo e especificações do processador
 * - Número de núcleos físicos/lógicos
 * - Quantidade total de memória RAM
 * - Informações sobre a placa de vídeo/driver gráfico
 * - Sistema operacional e versão
 * - Arquitetura do processador (x86_64, ARM, etc.)
 * 
 * @pre Nenhuma pré-condição específica
 * @post Todos os campos da estrutura hardware são preenchidos com dados válidos
 * 
 * @see obterInfoCPU()
 * @see obterRAM()
 * @see obterInfoGPU()
 * @see obterInfoSO()
 * @see obterArquitetura()
 */
void JanelaInfosSistema::coletarInfosHardware() {
    hardware.processador = obterInfoCPU();
    hardware.nucleos = SDL_GetCPUCount();
    hardware.memoriaRAM = obterRAM();
    hardware.placaVideo = obterInfoGPU();
    hardware.sistemaOperacional = obterInfoSO();
    hardware.arquitetura = obterArquitetura();
}

/**
 * @brief Inicializa as informações sobre o projeto
 * 
 * @details Preenche a estrutura de projeto com:
 * - Nome do projeto
 * - Versão atual (formato semântico: MAJOR.MINOR.PATCH)
 * - Data e hora de compilação (usando macros __DATE__ e __TIME__)
 * - Descrição resumida do propósito do projeto
 * 
 * @post Todos os campos da estrutura projeto contêm valores válidos
 * 
 * @note A data de compilação é obtida em tempo de compilação através das
 * macros predefinidas do preprocessador C++
 */
void JanelaInfosSistema::inicializarInfosProjeto() {
    projeto.nome = "DACC Station";
    projeto.versao = "PJ053-2025";
    
    std::stringstream ss;
    ss << __DATE__ << " " << __TIME__;
    projeto.dataCompilacao = ss.str();
    
    projeto.descricao = "Console academico para preservar, executar e configurar jogos do DACC";
}

/**
 * @brief Inicializa a lista de colaboradores do projeto
 * 
 * @details Popula o vetor de colaboradores com informações sobre os membros
 * da equipe de desenvolvimento. Cada colaborador possui:
 * - Nome/cargo
 * - Função/responsabilidade no projeto
 * 
 * @pre Nenhuma pré-condição
 * @post O vetor colaboradores contém todos os membros da equipe
 * 
 * @note Este método limpa o vetor antes de adicionar novos colaboradores,
 * permitindo reinicialização segura
 * 
 */
void JanelaInfosSistema::inicializarColaboradores() {
    colaboradores.clear();
    
    colaboradores.push_back(Colaborador(
        "Feliph de Matos Macêdo Lima",
        "Executor de jogos, monitoramento de processos e telemetria"
    ));
    colaboradores.push_back(Colaborador(
        "João Eduardo Coelho Neves",
        "Interface gráfica, experiência do usuário e integração com o executor"
    ));
    colaboradores.push_back(Colaborador(
        "João Henrique Vieira do Carmo",
        "Sistema de arquivos, benchmarks e compatibilidade dos jogos"
    ));
    colaboradores.push_back(Colaborador(
        "Vinícius dos Santos Tacaná",
        "Configurações do sistema, periféricos e documentação técnica"
    ));
    colaboradores.push_back(Colaborador(
        "Valmir Batista Prestes de Souza",
        "Orientação do projeto"
    ));
}

/**
 * @brief Obtém informações detalhadas sobre o processador do sistema
 * 
 * @details Lê o arquivo /proc/cpuinfo para extrair o nome do modelo do processador.
 * O método procura pela linha que contém "model name" e retorna o valor após
 * os dois pontos.
 * 
 * @return String contendo o nome completo do modelo do processador, ou
 *         "CPU Genérica" caso não seja possível obter a informação
 * 
 * @note Este método é específico para sistemas Linux que expõem /proc/cpuinfo
 * 
 * @warning Em sistemas não-Linux ou onde /proc/cpuinfo não está disponível,
 * retorna um valor genérico
 */
std::string JanelaInfosSistema::obterInfoCPU() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                std::string cpu = line.substr(pos + 2);
                return cpu;
            }
        }
    }
    return "CPU Genérica";
}

/**
 * @brief Obtém a quantidade total de memória RAM do sistema
 * 
 * @details Utiliza a função SDL_GetSystemRAM() para obter a quantidade de
 * memória RAM disponível no sistema.
 * 
 * @return Quantidade de RAM em megabytes (MB)
 * 
 * @note SDL2 fornece esta informação de forma multiplataforma, funcionando
 * em Windows, Linux e macOS
 */
int JanelaInfosSistema::obterRAM() {
    int ram = SDL_GetSystemRAM();
    return ram;
}

/**
 * @brief Obtém informações sobre a placa de vídeo/driver gráfico
 * 
 * @details Utiliza SDL_GetCurrentVideoDriver() para obter o nome do driver
 * de vídeo atualmente em uso pelo SDL.
 * 
 * @return String no formato "Driver: [nome_do_driver]", ou "GPU Genérica"
 *         caso não seja possível obter a informação
 * 
 * @note O driver retornado pode ser "x11", "wayland", "windows", etc.,
 * dependendo da plataforma e configuração
 * 
 * @warning Esta função retorna o driver SDL, não necessariamente informações
 * detalhadas sobre o hardware da GPU
 */
std::string JanelaInfosSistema::obterInfoGPU() {
    const char* renderer = SDL_GetCurrentVideoDriver();
    
    if (renderer) {
        std::stringstream ss;
        ss << "Driver: " << renderer;
        return ss.str();
    }
    
    return "GPU Genérica";
}

/**
 * @brief Obtém informações sobre o sistema operacional
 * 
 * @details Utiliza a chamada de sistema uname() para obter informações sobre
 * o sistema operacional, incluindo nome e versão do kernel.
 * 
 * @return String no formato "[nome_do_so] [versão]" (ex: "Linux 5.15.0"), ou
 *         "Linux" como valor padrão em caso de falha
 * 
 * @note Esta função é específica para sistemas POSIX (Linux, Unix, macOS)
 * 
 * @warning Em sistemas não-POSIX, a função pode falhar e retornar o valor padrão
 */
std::string JanelaInfosSistema::obterInfoSO() {
    struct utsname info;
    if (uname(&info) == 0) {
        std::stringstream ss;
        ss << info.sysname << " " << info.release;
        return ss.str();
    }
    return "Linux";
}

/**
 * @brief Obtém a arquitetura do processador
 * 
 * @details Utiliza a chamada de sistema uname() para obter informações sobre
 * a arquitetura da máquina (campo machine).
 * 
 * @return String com a arquitetura (ex: "x86_64", "aarch64", "armv7l"), ou
 *         "x86_64" como valor padrão em caso de falha
 * 
 * @note Esta informação indica se o sistema é 32-bit ou 64-bit, e qual
 * família de processadores está sendo usada
 */
std::string JanelaInfosSistema::obterArquitetura() {
    struct utsname info;
    if (uname(&info) == 0) {
        return std::string(info.machine);
    }
    return "x86_64";
}

/**
 * @brief Carrega a imagem explicativa baseada no tema atual
 * 
 * @details Esta função seleciona e carrega a imagem explicativa
 * apropriada (clara ou escura) baseada no tema atualmente ativo. O processo inclui:
 * - Verificação de validade do renderer
 * - Consulta ao tema atual (claro ou escuro)
 * - Seleção do caminho correto da imagem
 * - Carregamento através do GerenciadorImagens
 * - Logging detalhado para debug
 * 
 * @param[in] renderer Ponteiro para o SDL_Renderer válido
 * 
 * @pre renderer != nullptr
 * @post texturaExplicacao aponta para textura válida ou nullptr em caso de falha
 * 
 * 
 * @warning Se a imagem não for encontrada, a aplicação continua funcionando
 * mas sem exibir a imagem explicativa
 * 
 * @see GerenciadorImagens::carregar()
 * @see GerenciadorTemas::getTemaAtual()
 */
void JanelaInfosSistema::carregarImagemExplicativa(SDL_Renderer* renderer) {
    if (!renderer) {
        SDL_Log("[ERRO] Renderer nulo em carregarImagemExplicativa!");
        return;
    }
    
    auto& tema = GerenciadorTemas::getInstance();
    std::string caminhoImagem;
    
    // Seleciona imagem baseada no tema
    if (tema.getTemaAtual() == TipoTema::CLARO) {
        caminhoImagem = "assets/images/light/explicacaoBotoesJanelaInfosSistemaClaro.jpg";
    } else {
        caminhoImagem = "assets/images/dark/explicacaoBotoesJanelaInfosSistemaEscuro.jpg";
    }
    
    // Carrega através do gerenciador
    texturaExplicacao = gerImg.carregar(renderer, caminhoImagem);
    
    if (!texturaExplicacao) {
        SDL_Log("[AVISO] Imagem explicativa não encontrada: %s", caminhoImagem.c_str());
    } 
}

/**
 * @brief Carrega todas as texturas necessárias para a janela
 * 
 * @details Método principal para carregamento de recursos gráficos. Atualmente
 * carrega apenas a imagem explicativa, mas pode ser estendido para carregar
 * outros recursos visuais no futuro.
 * 
 * @param[in] renderer Ponteiro para o SDL_Renderer válido
 * 
 * @pre renderer != nullptr
 * @post Todas as texturas necessárias estão carregadas
 * 
 * @note Este método inclui logging para facilitar debug do processo de carregamento
 * 
 * @see carregarImagemExplicativa()
 */
void JanelaInfosSistema::carregarTexturas(SDL_Renderer* renderer) {
    carregarImagemExplicativa(renderer);
}

/**
 * @brief Desenha a imagem explicativa no rodapé da tela
 * 
 * @details CORREÇÃO: Desenha a imagem explicativa com verificações adicionais
 * de segurança. A imagem é posicionada no rodapé da tela com dimensões fixas:
 * - Largura: Total da tela (1525px escalonados)
 * - Altura: 30px escalonados
 * 
 * @param[in] renderer Ponteiro para o SDL_Renderer válido
 * 
 * @pre renderer != nullptr
 * @pre texturaExplicacao != nullptr (caso contrário, retorna sem desenhar)
 * 
 * @post Imagem explicativa renderizada no rodapé da tela
 * 
 * @note As dimensões são escalonadas através de ConfigLayout para suportar
 * diferentes resoluções de tela
 * 
 * @see ConfigLayout::X()
 * @see ConfigLayout::Y()
 */
void JanelaInfosSistema::desenharImagemExplicativa(SDL_Renderer* renderer) {
    if (!texturaExplicacao) {
        return;
    }
    
    // 1. Define a altura fixa da imagem e largura total da tela
    int larguraTela = ConfigLayout::X(1525); 
    int alturaImagem = ConfigLayout::Y(30);
    
    // 2. Define a posição Y como (Altura da Tela - Altura da Imagem)
    int posY = ConfigLayout::Y(1080) - alturaImagem;
    
    // 3. Monta o retângulo de destino
    SDL_Rect destExplicacao = {
        0,              // X inicial no canto esquerdo
        posY,           // Y calculado para o rodapé
        larguraTela,    // Largura total
        alturaImagem    // Altura de 30px
    };
    
    // 4. Renderiza
    SDL_RenderCopy(renderer, texturaExplicacao, nullptr, &destExplicacao);
}

/**
 * @brief Método principal de renderização da janela
 * 
 * @details Coordena o desenho de todos os elementos da interface de informações
 * do sistema. A renderização ocorre em camadas:
 * 
 * 1. Cabeçalho da janela
 * 2. Área de conteúdo com scroll (com clipping region):
 *    - Seção de Hardware
 *    - Separador visual
 *    - Seção do Projeto
 *    - Separador visual
 *    - Seção de Colaboradores
 * 3. Barra de rolagem (scroll bar)
 * 4. Imagem explicativa no rodapé
 * 
 * @param[in] renderer Ponteiro para o SDL_Renderer válido
 * 
 * @pre renderer != nullptr
 * @post Toda a interface está renderizada na tela
 * 
 * @note O método utiliza clipping region (SDL_RenderSetClipRect) para garantir
 * que o conteúdo rolável não ultrapasse os limites da área visível
 * 
 * @see desenharCabecalho()
 * @see desenharSecaoHardware()
 * @see desenharSecaoProjeto()
 * @see desenharSecaoColaboradores()
 * @see desenharBarraScroll()
 * @see desenharImagemExplicativa()
 */
void JanelaInfosSistema::desenhar(SDL_Renderer* renderer) {
    if (!renderer) return;

    // Desenha cabeçalho
    desenharCabecalho(renderer);
    
    // Define área de clipping para o conteúdo rolável
    SDL_Rect clipRect = {
        0,
        ConfigLayout::Y(220),
        ConfigLayout::X(1525),
        ConfigLayout::Y(1086) - ConfigLayout::Y(220)
    };
    SDL_RenderSetClipRect(renderer, &clipRect);
    
    // Desenha seções com offset de scroll
    int posY = ConfigLayout::Y(220) - offsetScroll;
    
    posY = desenharSecaoHardware(renderer, posY);
    desenharSeparador(renderer, posY);
    posY += ConfigLayout::Y(40);
    
    posY = desenharSecaoProjeto(renderer, posY);
    desenharSeparador(renderer, posY);
    posY += ConfigLayout::Y(40);
    
    posY = desenharSecaoColaboradores(renderer, posY);
    
    // Calcula altura total do conteúdo na primeira renderização
    if (precisaRecalcularScroll) {
        calcularAlturaConteudo(posY);
        precisaRecalcularScroll = false;
    }
    
    // Remove clipping
    SDL_RenderSetClipRect(renderer, nullptr);
    
    // Desenha barra de scroll
    desenharBarraScroll(renderer);
    
    // Desenha imagem explicativa no rodapé
    desenharImagemExplicativa(renderer);
}

/** 
 * @brief Desenha o cabeçalho da janela de informações do sistema
 * 
 * @details Centraliza e desenha o título "Informações do Sistema" no topo da janela.
 * O título é renderizado com tamanho de fonte grande para destaque.
 * 
 * @param[in] renderer Ponteiro para o SDL_Renderer válido
 * 
 * @pre renderer != nullptr
 * @post Título renderizado na posição correta
 * 
 * @note A posição X é calculada dinamicamente para centralizar o texto
 * baseado na largura estimada do título
 * 
 * @see desenharTexto()
 */

void JanelaInfosSistema::desenharCabecalho(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    std::string titulo = "Informações do Sistema";
    int tamanhoFonte = ConfigLayout::F(48);
    
    int larguraEstimada = (int)(titulo.length() * tamanhoFonte * 0.6f);
    int larguraJanela = ConfigLayout::X(1525);
    int posX = (larguraJanela - larguraEstimada) / 2;
    
    desenharTexto(renderer, titulo, 
                  posX, ConfigLayout::Y(150), 
                  tema.getCorTextoNegrito(), 
                  tamanhoFonte,
                  TipoFonte::NORMAL);
}

/**
 * @brief Desenha a seção de informações de hardware
 * 
 * @details Renderiza todas as informações coletadas sobre o hardware do sistema,
 * incluindo:
 * - Título da seção "HARDWARE" em negrito
 * - Modelo do processador
 * - Número de núcleos
 * - Quantidade de RAM (convertida para GB)
 * - Informações da placa de vídeo
 * - Sistema operacional
 * - Arquitetura do processador
 * 
 * @param[in] renderer Ponteiro para o SDL_Renderer válido
 * @param[in] posY Posição Y inicial para começar a desenhar a seção
 * 
 * @return Posição Y final após desenhar toda a seção (para encadeamento)
 * 
 * @pre renderer != nullptr
 * @pre hardware contém dados válidos
 * 
 * @post Seção de hardware renderizada na tela
 * 
 * @note A memória RAM é convertida de MB para GB com precisão de 2 casas decimais
 * @note As cores e fontes são obtidas do GerenciadorTemas atual
 * 
 * @see desenharTexto()
 * @see GerenciadorTemas::getCorTextoNormal()
 * @see GerenciadorTemas::getCorTextoNegrito()
 */
int JanelaInfosSistema::desenharSecaoHardware(SDL_Renderer* renderer, int posY) {
    auto& tema = GerenciadorTemas::getInstance();
    int baseX = ConfigLayout::X(250);
    int espacamentoLinha = ConfigLayout::Y(40);
    
    desenharTexto(renderer, "HARDWARE", 
                  baseX, posY, 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(36),
                  TipoFonte::NEGRITO);
    posY += ConfigLayout::Y(60);
    
    std::stringstream ssCPU;
    ssCPU << "Processador: " << hardware.processador;
    desenharTexto(renderer, ssCPU.str(), 
                  baseX + ConfigLayout::X(30), posY, 
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(24),
                  TipoFonte::NORMAL);
    posY += espacamentoLinha;
    
    std::stringstream ssNucleos;
    ssNucleos << "Núcleos: " << hardware.nucleos;
    desenharTexto(renderer, ssNucleos.str(), 
                  baseX + ConfigLayout::X(30), posY, 
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(24),
                  TipoFonte::NORMAL);
    posY += espacamentoLinha;
    
    std::stringstream ssRAM;
    float ramGB = hardware.memoriaRAM / 1024.0f;
    ssRAM << "Memória RAM: " << std::fixed << std::setprecision(2) << ramGB << " GB";
    desenharTexto(renderer, ssRAM.str(), 
                  baseX + ConfigLayout::X(30), posY, 
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(24),
                  TipoFonte::NORMAL);
    posY += espacamentoLinha;
    
    std::stringstream ssGPU;
    ssGPU << "Placa de Vídeo: " << hardware.placaVideo;
    desenharTexto(renderer, ssGPU.str(), 
                  baseX + ConfigLayout::X(30), posY, 
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(24),
                  TipoFonte::NORMAL);
    posY += espacamentoLinha;
    
    std::stringstream ssSO;
    ssSO << "Sistema Operacional: " << hardware.sistemaOperacional;
    desenharTexto(renderer, ssSO.str(), 
                  baseX + ConfigLayout::X(30), posY, 
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(24),
                  TipoFonte::NORMAL);
    posY += espacamentoLinha;
    
    std::stringstream ssArq;
    ssArq << "Arquitetura: " << hardware.arquitetura;
    desenharTexto(renderer, ssArq.str(), 
                  baseX + ConfigLayout::X(30), posY, 
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(24),
                  TipoFonte::NORMAL);
    posY += espacamentoLinha;
    
    return posY + ConfigLayout::Y(20);
}

/**
 * @brief Desenha a seção de informações do projeto
 * 
 * @details Renderiza informações sobre o projeto, incluindo:
 * - Título da seção "PROJETO" em negrito
 * - Nome do projeto
 * - Versão atual
 * - Data e hora de compilação
 * - Descrição do projeto
 * 
 * @param[in] renderer Ponteiro para o SDL_Renderer válido
 * @param[in] posY Posição Y inicial para começar a desenhar a seção
 * 
 * @return Posição Y final após desenhar toda a seção (para encadeamento)
 * 
 * @pre renderer != nullptr
 * @pre projeto contém dados válidos
 * 
 * @post Seção de projeto renderizada na tela
 * 
 * @note As cores e fontes são obtidas do GerenciadorTemas atual
 * 
 * @see desenharTexto()
 * @see GerenciadorTemas::getCorTextoNormal()
 * @see GerenciadorTemas::getCorTextoNegrito()
 */
int JanelaInfosSistema::desenharSecaoProjeto(SDL_Renderer* renderer, int posY) {
    auto& tema = GerenciadorTemas::getInstance();
    int baseX = ConfigLayout::X(250);
    int espacamentoLinha = ConfigLayout::Y(40);
    
    desenharTexto(renderer, "PROJETO", 
                  baseX, posY, 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(36),
                  TipoFonte::NEGRITO);
    posY += ConfigLayout::Y(60);
    
    std::stringstream ssNome;
    ssNome << "Nome: " << projeto.nome;
    desenharTexto(renderer, ssNome.str(), 
                  baseX + ConfigLayout::X(30), posY, 
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(24),
                  TipoFonte::NORMAL);
    posY += espacamentoLinha;
    
    std::stringstream ssVersao;
    ssVersao << "Versão: " << projeto.versao;
    desenharTexto(renderer, ssVersao.str(), 
                  baseX + ConfigLayout::X(30), posY, 
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(24),
                  TipoFonte::NORMAL);
    posY += espacamentoLinha;
    
    std::stringstream ssData;
    ssData << "Compilado em: " << projeto.dataCompilacao;
    desenharTexto(renderer, ssData.str(), 
                  baseX + ConfigLayout::X(30), posY, 
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(24),
                  TipoFonte::NORMAL);
    posY += espacamentoLinha;
    
    std::stringstream ssDesc;
    ssDesc << "Descrição: " << projeto.descricao;
    desenharTexto(renderer, ssDesc.str(), 
                  baseX + ConfigLayout::X(30), posY, 
                  tema.getCorTextoNormal(), 
                  ConfigLayout::F(24),
                  TipoFonte::NORMAL);
    posY += espacamentoLinha;
    
    return posY + ConfigLayout::Y(20);
}

/**
 * @brief Desenha a seção de colaboradores
 * 
 * @details Renderiza a lista completa de colaboradores do projeto. Cada
 * colaborador é exibido em uma linha com:
 * - Marcador bullet (•)
 * - Nome/cargo do colaborador
 * - Hífen separador (-)
 * - Função/responsabilidade
 * 
 * @param[in] renderer Ponteiro para o SDL_Renderer válido
 * @param[in] posY Posição Y inicial para começar a desenhar a seção
 * 
 * @return Posição Y final após desenhar toda a seção (para encadeamento)
 * 
 * @pre renderer != nullptr
 * @pre colaboradores não está vazio
 * 
 * @post Seção de colaboradores renderizada na tela
 * 
 * @note O número de linhas desenhadas depende da quantidade de colaboradores
 * @note As cores e fontes são obtidas do GerenciadorTemas atual
 * 
 * @see desenharTexto()
 * @see GerenciadorTemas::getCorTextoNormal()
 * @see GerenciadorTemas::getCorTextoNegrito()
 */
int JanelaInfosSistema::desenharSecaoColaboradores(SDL_Renderer* renderer, int posY) {
    auto& tema = GerenciadorTemas::getInstance();
    int baseX = ConfigLayout::X(250);
    int espacamentoColaborador = ConfigLayout::Y(72);
    
    desenharTexto(renderer, "COLABORADORES", 
                  baseX, posY, 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(36),
                  TipoFonte::NEGRITO);
    posY += ConfigLayout::Y(60);
    
    for (const auto& colaborador : colaboradores) {
        std::stringstream ssNome;
        ssNome << "• " << colaborador.nome;
        
        desenharTexto(renderer, ssNome.str(),
                      baseX + ConfigLayout::X(30), posY,
                      tema.getCorTextoNegrito(),
                      ConfigLayout::F(24),
                      TipoFonte::NEGRITO);
        posY += ConfigLayout::Y(32);

        desenharTexto(renderer, colaborador.funcao,
                      baseX + ConfigLayout::X(65), posY,
                      tema.getCorTextoNormal(),
                      ConfigLayout::F(21),
                      TipoFonte::NORMAL);
        posY += espacamentoColaborador - ConfigLayout::Y(32);
    }
    
    return posY + ConfigLayout::Y(20);
}

/**
 * @brief Desenha uma linha separadora horizontal
 * 
 * @details Renderiza uma linha horizontal semi-transparente usada para separar
 * visualmente as diferentes seções da interface. A linha tem:
 * - Largura: 1020px (escalonado)
 * - Altura: 2px (escalonado)
 * - Opacidade: 100/255 (~39%)
 * - Cor: Baseada na cor de texto normal do tema atual
 * 
 * @param[in] renderer Ponteiro para o SDL_Renderer válido
 * @param[in] posY Posição Y onde a linha deve ser desenhada
 * 
 * @pre renderer != nullptr
 * @post Linha separadora renderizada na posição especificada
 * 
 * @note A cor da linha é derivada da cor de texto do tema atual,
 * mas com opacidade reduzida para efeito sutil
 * 
 * @see GerenciadorTemas::getCorTextoNormal()
 */
void JanelaInfosSistema::desenharSeparador(SDL_Renderer* renderer, int posY) {
    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color corSeparador = tema.getCorTextoNormal();
    
    SDL_Rect linha = {
        ConfigLayout::X(250),
        posY,
        ConfigLayout::X(1020),
        ConfigLayout::Y(2)
    };
    
    SDL_SetRenderDrawColor(renderer, corSeparador.r, corSeparador.g, 
                          corSeparador.b, 100);
    SDL_RenderFillRect(renderer, &linha);
}

/**
 * @brief Desenha a barra de rolagem vertical
 * 
 * @details Renderiza uma barra de scroll interativa quando há conteúdo
 * rolável (maxScroll > 0). A barra consiste em:
 * 
 * - Trilho: Área total onde o handle pode se mover
 *   - Cor: Baseada em corRetangulos do tema
 *   - Opacidade: 150/255 (~59%)
 * 
 * - Handle: Indicador da posição atual do scroll
 *   - Cor: Baseada em corDestaque do tema
 *   - Opacidade: 255/255 (100%)
 *   - Altura proporcional à área visível vs total
 *   - Altura mínima: 30px
 *   - Posição: Proporcional ao offset de scroll
 * 
 * @param[in] renderer Ponteiro para o SDL_Renderer válido
 * 
 * @pre renderer != nullptr
 * @post Barra de scroll renderizada se maxScroll > 0
 * 
 * @note A barra não é desenhada se todo o conteúdo cabe na área visível
 * @note O tamanho do handle reflete a proporção entre área visível e conteúdo total
 * 
 * @see GerenciadorTemas::getCorRetangulos()
 * @see GerenciadorTemas::getCorDestaque()
 */
void JanelaInfosSistema::desenharBarraScroll(SDL_Renderer* renderer) {
    if (maxScroll <= 0) return;
    
    auto& tema = GerenciadorTemas::getInstance();
    
    int areaScrollY = ConfigLayout::Y(220);
    int areaScrollAltura = ConfigLayout::Y(1086) - areaScrollY;
    int barraX = ConfigLayout::X(1525) - ConfigLayout::X(LARGURA_BARRA_SCROLL + 10);
    
    SDL_Rect trilho = {
        barraX,
        areaScrollY,
        ConfigLayout::X(LARGURA_BARRA_SCROLL),
        areaScrollAltura
    };
    
    SDL_Color corTrilho = tema.getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, corTrilho.r, corTrilho.g, corTrilho.b, 150);
    SDL_RenderFillRect(renderer, &trilho);
    
    float proporcaoVisivel = (float)areaScrollAltura / (areaScrollAltura + maxScroll);
    int alturaHandle = std::max(30, (int)(areaScrollAltura * proporcaoVisivel));
    
    float proporcaoScroll = maxScroll > 0 ? (float)offsetScroll / maxScroll : 0;
    int posYHandle = areaScrollY + (int)((areaScrollAltura - alturaHandle) * proporcaoScroll);
    
    SDL_Rect handle = {
        barraX,
        posYHandle,
        ConfigLayout::X(LARGURA_BARRA_SCROLL),
        alturaHandle
    };
    
    SDL_Color corHandle = tema.getCorDestaque();
    SDL_SetRenderDrawColor(renderer, corHandle.r, corHandle.g, corHandle.b, 255);
    SDL_RenderFillRect(renderer, &handle);
}

/**
 * @brief Calcula a altura total do conteúdo rolável
 * 
 * @details Determina quanto espaço vertical o conteúdo ocupa e calcula o
 * scroll máximo permitido. O cálculo considera:
 * - Altura total do conteúdo (fornecida como parâmetro)
 * - Altura da área visível (1086 - 220 = 866px)
 * - Offset do cabeçalho (220px)
 * 
 * O scroll máximo é zero se o conteúdo cabe inteiramente na área visível.
 * 
 * @param[in] alturaFinal Altura Y final do último elemento renderizado
 * 
 * @post alturaConteudo e maxScroll são atualizados
 * @post maxScroll >= 0
 * 
 * @note Este método deve ser chamado após renderizar todo o conteúdo
 * pelo menos uma vez para obter medidas precisas
 * 
 * @see desenhar()
 */
void JanelaInfosSistema::calcularAlturaConteudo(int alturaFinal) {
    alturaConteudo = alturaFinal;
    int areaVisivel = ConfigLayout::Y(1086) - ConfigLayout::Y(220);
    maxScroll = std::max(0, alturaConteudo - ConfigLayout::Y(220) - areaVisivel);
}

/**
 * @brief Processa eventos de entrada do usuário
 * 
 * @details Gerencia a interação do usuário com o sistema de scroll através de:
 * 
 * **Mouse Wheel (Roda do mouse):**
 * - Rolar para cima: Diminui offsetScroll (move conteúdo para baixo)
 * - Rolar para baixo: Aumenta offsetScroll (move conteúdo para cima)
 * - Velocidade: VELOCIDADE_SCROLL_MOUSE pixels por unidade de scroll
 * 
 * **Gamepad D-Pad (Direcional):**
 * - D-Pad Up: Diminui offsetScroll
 * - D-Pad Down: Aumenta offsetScroll
 * - Velocidade: VELOCIDADE_SCROLL_GAMEPAD pixels por pressionamento
 * 
 * @param[in,out] evento Referência ao evento SDL a ser processado
 * 
 * @return true se o evento foi processado (scroll modificado), false caso contrário
 * 
 * @post offsetScroll atualizado e limitado ao intervalo [0, maxScroll]
 * 
 * @note O offset de scroll é sempre mantido dentro dos limites válidos usando std::clamp
 * @note Eventos não relacionados ao scroll são ignorados e retornam false
 * 
 * @see VELOCIDADE_SCROLL_MOUSE
 * @see VELOCIDADE_SCROLL_GAMEPAD
 */
bool JanelaInfosSistema::processarEvento(SDL_Event& evento) {
    if (evento.type == SDL_MOUSEWHEEL) {
        offsetScroll -= evento.wheel.y * VELOCIDADE_SCROLL_MOUSE;
        offsetScroll = std::clamp(offsetScroll, 0, maxScroll);
        return true;
    }
    
    if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
        if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
            offsetScroll -= VELOCIDADE_SCROLL_GAMEPAD;
            offsetScroll = std::clamp(offsetScroll, 0, maxScroll);
            return true;
        }
        else if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
            offsetScroll += VELOCIDADE_SCROLL_GAMEPAD;
            offsetScroll = std::clamp(offsetScroll, 0, maxScroll);
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Reseta o estado da janela para valores iniciais
 * 
 * @details Reinicia o estado de scroll da janela, útil quando:
 * - A janela é reaberta
 * - O conteúdo é recarregado
 * - É necessário voltar ao topo da visualização
 * 
 * As seguintes ações são executadas:
 * - offsetScroll volta para 0 (topo do conteúdo)
 * - precisaRecalcularScroll é marcado como true para forçar recálculo
 * 
 * @post offsetScroll == 0
 * @post precisaRecalcularScroll == true
 * 
 * @note Não altera o conteúdo carregado, apenas a posição de visualização
 */
void JanelaInfosSistema::resetar() {
    offsetScroll = 0;
    precisaRecalcularScroll = true;
}
