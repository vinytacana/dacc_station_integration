/**
 * @file JanelaRede.cpp
 * @brief Implementação da interface de configurações de rede.
 *
 * Este arquivo contém toda a lógica visual e de interação para o submenu
 * de configurações de rede, incluindo renderização de elementos e tratamento
 * de eventos de navegação.
 * 
 */

#include "JanelaRede.hpp"
#include "functions.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "GerenciadorAudio.hpp"
#include "GerenciadorImagens.hpp"
#include "TecladoVirtual.hpp"
#include <exception>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

using namespace MeuProjeto;

extern GerenciadorAudio gerAudio;
extern GerenciadorImagens gerImg;

/**
 * @class StringInputAdapter
 * @brief Classe auxiliar que adapta uma std::string para ser usada com TecladoVirtual.
 * 
 * Como o TecladoVirtual foi projetado para trabalhar com BotaoPesquisa, esta classe
 * fornece uma interface compatível que opera diretamente em uma string por referência.
 * 
 * Não podemos herdar de BotaoPesquisa pois ela tem dependências do SDL.
 * Usamos reinterpret_cast no TecladoVirtual, então precisamos manter a mesma assinatura.
 */
/**
 * @class StringInputAdapter
 * @brief Agora herdando de BotaoPesquisa para garantir compatibilidade de memória.
 */

namespace MeuProjeto { 

class StringInputAdapter : public BotaoPesquisa {
public:
    StringInputAdapter(std::string& str) 
        : BotaoPesquisa(nullptr, 0, 0, 0, 0), texto(str) {}
    
    void adicionarTexto(const std::string& car) override {
        texto += car;
    }
    
    void limparTexto() override { texto.clear(); }
    void apagarTexto() override { if (!texto.empty()) texto.pop_back(); }
    void resetarSolicitacaoTeclado() override {}
    void resetarFocoResultados() override {}
    void cancelarBusca() override {}
    
private:
    std::string& texto;
};

} // fim namespace Me

/**
 * @brief Construtor da classe JanelaRede.
 * Inicializa o estado e os componentes da tela.
 */
JanelaRede::JanelaRede() {
    inicializarBotoes();
    definirMensagemStatus("Carregando redes Wi-Fi...");
    agendarSincronizacaoComBackend();
}

/**
 * @brief Destrutor da classe JanelaRede.
 */
JanelaRede::~JanelaRede() {
    if (workerThread.joinable()) {
        workerThread.join();
    }
    botoesRedes.clear();
    btnToggleWifi.reset();
    btnConectarSenha.reset();
    btnCancelarSenha.reset();
    liberarImagemExplicativa();
}

/**
 * @brief Inicializa a lista de redes disponíveis com dados de exemplo.
 * 
 * Em uma implementação real, esta função consultaria o sistema operacional
 * para obter a lista de redes Wi-Fi detectadas.
 */
void JanelaRede::inicializarRedes() {
    redesDisponiveis.clear();
    auto redes = ::listar_wifi_parsed();
    for (const auto& rede : redes) {
        bool requerSenha = !rede.seguranca.empty() && rede.seguranca != "--";
        redesDisponiveis.push_back(RedeInfo(rede.ssid, rede.em_uso, requerSenha, rede.sinal));
        if (rede.em_uso) {
            redeConectada = rede.ssid;
        }
    }
}

void JanelaRede::sincronizarComBackend() {
    wifi_adapter_status status = ::obter_status_wifi();
    std::vector<RedeInfo> novasRedes;
    std::string novaRedeConectada;

    if (!status.conectado_cabeado) {
        auto redes = ::listar_wifi_parsed();
        for (const auto& rede : redes) {
            bool requerSenha = !rede.seguranca.empty() && rede.seguranca != "--";
            novasRedes.emplace_back(rede.ssid, rede.em_uso, requerSenha, rede.sinal);
            if (rede.em_uso) {
                novaRedeConectada = rede.ssid;
            }
        }
    }

    std::lock_guard<std::mutex> lock(mtxRede);
    wifiAtivo = status.enabled;
    redeCabeadaAtiva = status.conectado_cabeado;
    conexaoCabeada = status.conexao_cabeada;
    dispositivoCabeado = status.dispositivo_cabeado;
    redeConectada = std::move(novaRedeConectada);
    redesDisponiveis = std::move(novasRedes);
    if (redeCabeadaAtiva) {
        mensagemStatus = "Conexao cabeada ativa. Scan Wi-Fi pausado.";
        mensagemErro = false;
    } else if (redesDisponiveis.empty() && wifiAtivo) {
        mensagemStatus = "Nenhuma rede Wi-Fi detectada.";
        mensagemErro = false;
    } else if (!wifiAtivo) {
        mensagemStatus = "Wi-Fi desativado.";
        mensagemErro = false;
    } else if (mensagemStatus == "Carregando redes Wi-Fi..." || mensagemStatus == "Atualizando redes Wi-Fi...") {
        mensagemStatus.clear();
        mensagemErro = false;
    }
    precisaAtualizarInterface = true;
}

void JanelaRede::agendarSincronizacaoComBackend(const std::string& mensagem) {
    if (!mensagem.empty()) {
        definirMensagemStatus(mensagem, false);
    }
    (void)iniciarTarefaEmSegundoPlano([this]() {
        sincronizarComBackend();
    });
}

void JanelaRede::definirMensagemStatus(const std::string& mensagem, bool erro) {
    std::lock_guard<std::mutex> lock(mtxRede);
    mensagemStatus = mensagem;
    mensagemErro = erro;
}

bool JanelaRede::iniciarTarefaEmSegundoPlano(std::function<void()> tarefa) {
    if (operacaoEmAndamento.exchange(true)) {
        definirMensagemStatus("Ha uma operacao de rede em andamento.", true);
        return false;
    }
    if (workerThread.joinable()) {
        workerThread.join();
    }
    workerThread = std::thread([this, tarefa = std::move(tarefa)]() mutable {
        try {
            tarefa();
        } catch (const std::exception& e) {
            definirMensagemStatus(std::string("Operacao de rede falhou: ") + e.what(), true);
        } catch (...) {
            definirMensagemStatus("Operacao de rede falhou inesperadamente.", true);
        }
        operacaoEmAndamento = false;
    });
    return true;
}

/**
 * @brief Inicializa os botões e elementos interativos da interface.
 * 
 * Configura o botão de toggle do Wi-Fi e cria os botões para cada rede
 * disponível, aplicando as cores do tema atual.
 */
void JanelaRede::inicializarBotoes() {
    bool wifiAtivoLocal = true;
    bool redeCabeadaLocal = false;
    std::vector<std::string> nomesRedes;
    {
        std::lock_guard<std::mutex> lock(mtxRede);
        wifiAtivoLocal = wifiAtivo;
        redeCabeadaLocal = redeCabeadaAtiva;
        nomesRedes.reserve(redesDisponiveis.size());
        for (const auto& rede : redesDisponiveis) {
            nomesRedes.push_back(rede.nome);
        }
    }

    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color btnNormal = tema.getCorBotaoNormal();
    SDL_Color btnHover = tema.getCorBotaoHover();
    SDL_Color btnPress = tema.getCorBotaoPressionado();

    // Botão Toggle Wi-Fi
    btnToggleWifi = std::make_unique<Botao>(
        ConfigLayout::X(1100), ConfigLayout::Y(280),
        ConfigLayout::X(200), ConfigLayout::Y(60),
        wifiAtivoLocal ? "ON" : "OFF"
    );
    btnToggleWifi->setCor(btnNormal, btnHover, btnPress);
    btnToggleWifi->setRetanguloBordasArredondadas(15);

    // Limpa e recria botões das redes
    botoesRedes.clear();
    if (redeCabeadaLocal) {
        indiceFocado = -1;
        scrollOffset = 0;
        return;
    }
    
    int posYInicial = 450;
    int espacamento = 90;
    
    for (size_t i = 0; i < nomesRedes.size(); i++) {
        int posY = posYInicial + (i * espacamento);
        
        auto btnRede = std::make_unique<Botao>(
            ConfigLayout::X(250), ConfigLayout::Y(posY),
            ConfigLayout::X(1000), ConfigLayout::Y(70),
            nomesRedes[i]
        );
        btnRede->setCor(btnNormal, btnHover, btnPress);
        btnRede->setRetanguloBordasArredondadas(15);
        
        botoesRedes.push_back(std::move(btnRede));
    }

    // Inicializa foco se houver controle conectado
    if (SDL_NumJoysticks() > 0) {
        indiceFocado = 0; // Foca no toggle do Wi-Fi
    } else {
        indiceFocado = -1;
    }
}

/**
 * @brief Carrega a textura da imagem explicativa de acordo com o tema.
 * @param renderer Renderizador SDL usado para carregar a textura.
 */
void JanelaRede::carregarImagemExplicativa(SDL_Renderer* renderer) {
    if (!renderer) return;
    
    liberarImagemExplicativa();
    
    auto& tema = GerenciadorTemas::getInstance();
    std::string caminhoImagem;
    
    if (tema.getTemaAtual() == TipoTema::ESCURO) {
        caminhoImagem = "assets/images/dark/explicacaoBotoesJanelaRedeEscuro.jpg";
    } else {
        caminhoImagem = "assets/images/light/explicacaoBotoesJanelaRedeClaro.jpg";
    }
    
    texturaExplicacao = gerImg.carregar(renderer, caminhoImagem);
    
    if (!texturaExplicacao) {
        std::cerr << "[REDE] Erro ao carregar imagem explicativa: " << caminhoImagem << std::endl;
    }
}

/**
 * @brief Libera a textura da imagem explicativa.
 */
void JanelaRede::liberarImagemExplicativa() {
    texturaExplicacao = nullptr;
}

/**
 * @brief Renderiza toda a interface de configurações de rede.
 * @param renderer Ponteiro para o renderizador SDL.
 */
void JanelaRede::desenhar(SDL_Renderer* renderer) {
    if (!renderer) return;

    if (precisaAtualizarInterface.exchange(false)) {
        inicializarBotoes();
    }

    // Carrega a imagem explicativa se ainda não foi carregada
    if (!texturaExplicacao) {
        carregarImagemExplicativa(renderer);
    }

    bool redeCabeadaLocal = false;
    {
        std::lock_guard<std::mutex> lock(mtxRede);
        redeCabeadaLocal = redeCabeadaAtiva;
    }

    desenharCabecalho(renderer);
    if (redeCabeadaLocal) {
        desenharPainelRedeCabeada(renderer);
    } else {
        desenharToggleWifi(renderer);
        desenharListaRedes(renderer);
    }
    desenharImagemExplicativa(renderer);
    
    // Se o teclado virtual estiver visível, desenha a tela de senha POR CIMA
    if (tecladoVisivel) {
        desenharTelasenha(renderer);
    }
}

/**
 * @brief Renderiza o cabeçalho com informações de conexão atual.
 * @param renderer Renderizador SDL.
 */
void JanelaRede::desenharCabecalho(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    bool wifiAtivoLocal = false;
    bool redeCabeadaLocal = false;
    std::string redeConectadaLocal;
    std::string conexaoCabeadaLocal;
    std::string dispositivoCabeadoLocal;
    std::string mensagem;
    bool erro = false;
    {
        std::lock_guard<std::mutex> lock(mtxRede);
        wifiAtivoLocal = wifiAtivo;
        redeCabeadaLocal = redeCabeadaAtiva;
        redeConectadaLocal = redeConectada;
        conexaoCabeadaLocal = conexaoCabeada;
        dispositivoCabeadoLocal = dispositivoCabeado;
        mensagem = mensagemStatus;
        erro = mensagemErro;
    }
    
    // Título da seção - CENTRALIZADO
    std::string titulo = "Configurações de Rede";
    int larguraTela = ConfigLayout::X(1525);
    int larguraTexto = titulo.length() * ConfigLayout::F(48) * 0.6; // Aproximação
    int posXCentralizada = (larguraTela - larguraTexto) / 2;
    
    desenharTexto(renderer, titulo, 
                  posXCentralizada, ConfigLayout::Y(150), 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(48));
    
    // Status da conexão
    std::string statusTexto;
    if (redeCabeadaLocal) {
        std::string nome = conexaoCabeadaLocal.empty() ? dispositivoCabeadoLocal : conexaoCabeadaLocal;
        statusTexto = nome.empty() ? "Estado: Conectado via cabo" : "Estado: Cabeada (" + nome + ")";
    } else if (!wifiAtivoLocal) {
        statusTexto = "Estado: Wi-Fi desativado";
    } else if (!redeConectadaLocal.empty()) {
        statusTexto = "Estado: Conectado (" + redeConectadaLocal + ")";
    } else {
        statusTexto = "Estado: Wi-Fi ativo, sem conexão";
    }
    
    SDL_Color corStatus = (redeCabeadaLocal || (wifiAtivoLocal && !redeConectadaLocal.empty()))
        ? SDL_Color{100, 255, 100, 255}
        : SDL_Color{255, 180, 100, 255};
    
    desenharTexto(renderer, statusTexto, 
                  ConfigLayout::X(250), ConfigLayout::Y(220), 
                  corStatus, 
                  ConfigLayout::F(28));
    if (!mensagem.empty()) {
        SDL_Color cor = erro ? SDL_Color{255, 120, 120, 255} : SDL_Color{120, 255, 120, 255};
        desenharTexto(renderer, mensagem,
                      ConfigLayout::X(250), ConfigLayout::Y(255),
                      cor, ConfigLayout::F(20));
    }
}

/**
 * @brief Renderiza o toggle de Wi-Fi.
 * @param renderer Renderizador SDL.
 */
void JanelaRede::desenharToggleWifi(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Label do toggle
    desenharTexto(renderer, "Wi-Fi:", 
                  ConfigLayout::X(250), ConfigLayout::Y(290), 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(32));
    
    // Aplica foco se for o elemento focado (índice 0)
    if (indiceFocado == 0 && SDL_NumJoysticks() > 0) {
        btnToggleWifi->setFocado(true);
    } else {
        btnToggleWifi->setFocado(false);
    }
    
    // Desenha o botão
    btnToggleWifi->desenhar(renderer);
}

void JanelaRede::desenharPainelRedeCabeada(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    std::string conexao;
    std::string dispositivo;
    {
        std::lock_guard<std::mutex> lock(mtxRede);
        conexao = conexaoCabeada;
        dispositivo = dispositivoCabeado;
    }

    SDL_Rect painel = {
        ConfigLayout::X(250),
        ConfigLayout::Y(330),
        ConfigLayout::X(1025),
        ConfigLayout::Y(250)
    };

    SDL_Color fundo = tema.getCorRetangulos();
    SDL_Color borda = tema.getCorDestaque();
    roundedBoxRGBA(renderer, painel.x, painel.y, painel.x + painel.w, painel.y + painel.h,
                   ConfigLayout::F(18), fundo.r, fundo.g, fundo.b, 210);
    roundedRectangleRGBA(renderer, painel.x, painel.y, painel.x + painel.w, painel.y + painel.h,
                         ConfigLayout::F(18), borda.r, borda.g, borda.b, 220);

    desenharTexto(renderer, "Rede cabeada detectada",
                  painel.x + ConfigLayout::X(35), painel.y + ConfigLayout::Y(35),
                  tema.getCorTextoNegrito(), ConfigLayout::F(36));

    std::string detalhe = conexao.empty() ? "Conectado por Ethernet" : "Conexao: " + conexao;
    desenharTexto(renderer, detalhe,
                  painel.x + ConfigLayout::X(35), painel.y + ConfigLayout::Y(100),
                  tema.getCorTextoNormal(), ConfigLayout::F(26));

    if (!dispositivo.empty()) {
        desenharTexto(renderer, "Dispositivo: " + dispositivo,
                      painel.x + ConfigLayout::X(35), painel.y + ConfigLayout::Y(145),
                      tema.getCorTextoNormal(), ConfigLayout::F(22));
    }

    desenharTexto(renderer, "Lista e scan de Wi-Fi pausados enquanto o cabo estiver ativo.",
                  painel.x + ConfigLayout::X(35), painel.y + ConfigLayout::Y(190),
                  tema.getCorTextoNormal(), ConfigLayout::F(22));
}

/**
 * @brief Renderiza a lista de redes disponíveis com scroll.
 * @param renderer Renderizador SDL.
 */
void JanelaRede::desenharListaRedes(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    bool wifiAtivoLocal = false;
    std::string redeConectadaLocal;
    std::vector<RedeInfo> redesSnapshot;
    {
        std::lock_guard<std::mutex> lock(mtxRede);
        wifiAtivoLocal = wifiAtivo;
        redeConectadaLocal = redeConectada;
        redesSnapshot = redesDisponiveis;
    }
    
    // Título da lista
    desenharTexto(renderer, "Redes Disponíveis:", 
                  ConfigLayout::X(250), ConfigLayout::Y(380), 
                  tema.getCorTextoNegrito(), 
                  ConfigLayout::F(32));
    
    // Só mostra as redes se o Wi-Fi estiver ativo
    if (!wifiAtivoLocal) {
        desenharTexto(renderer, "Wi-Fi desativado", 
                      ConfigLayout::X(250), ConfigLayout::Y(450), 
                      tema.getCorTextoNormal(), 
                      ConfigLayout::F(24));
        return;
    }
    
    // Calcula quais redes são visíveis baseado no scroll
    int primeiraVisivel = scrollOffset;
    int ultimaVisivel = std::min(primeiraVisivel + maxRedesVisiveis, 
                                  (int)redesSnapshot.size());
    
    // Renderiza as redes visíveis
    for (int i = primeiraVisivel; i < ultimaVisivel; i++) {
        int indiceVisual = i - primeiraVisivel;
        
        // Aplica foco se for o elemento focado (índice 1+ nas redes)
        int indiceFocoRede = indiceFocado - 1; // -1 porque índice 0 é o toggle
        if (indiceFocoRede == i && SDL_NumJoysticks() > 0) {
            botoesRedes[i]->setFocado(true);
        } else {
            botoesRedes[i]->setFocado(false);
        }

        botoesRedes[i]->area.y = ConfigLayout::Y(450 + (indiceVisual * 90));
        
        // Desenha o botão da rede
        botoesRedes[i]->desenhar(renderer);
        
        // Adiciona indicador visual se for rede conectada
        if (redesSnapshot[i].nome == redeConectadaLocal && wifiAtivoLocal) {
            int posX = ConfigLayout::X(220);
            int posY = ConfigLayout::Y(450 + (indiceVisual * 90) + 20);
            desenharTexto(renderer, ">", posX, posY, 
                          tema.getCorDestaque(), ConfigLayout::F(32));
        }
        
        if (redesSnapshot[i].requerSenha) {
            int posX = ConfigLayout::X(1270);
            int posY = ConfigLayout::Y(450 + (indiceVisual * 90) + 25);
            desenharTexto(renderer, "Senha", posX, posY,
                          tema.getCorTextoNormal(), ConfigLayout::F(20));
        }
    }
    
    // Indicadores de scroll (se houver mais redes)
    if (scrollOffset > 0) {
        // Seta para cima
        desenharTexto(renderer, "▲ Mais redes acima", 
                      ConfigLayout::X(250), ConfigLayout::Y(420), 
                      tema.getCorDestaque(), ConfigLayout::F(18));
    }
    
    if (ultimaVisivel < (int)redesSnapshot.size()) {
        // Seta para baixo
        int posY = ConfigLayout::Y(450 + (maxRedesVisiveis * 90) + 10);
        desenharTexto(renderer, "▼ Mais redes abaixo", 
                      ConfigLayout::X(250), posY, 
                      tema.getCorDestaque(), ConfigLayout::F(18));
    }
}

/**
 * @brief Renderiza a imagem explicativa dos botões no rodapé.
 * @param renderer Renderizador SDL.
 */
void JanelaRede::desenharImagemExplicativa(SDL_Renderer* renderer) {
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
 * @brief Renderiza a tela de entrada de senha.
 * @param renderer Renderizador SDL.
 * 
 */
void JanelaRede::desenharTelasenha(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Desenha um overlay semi-transparente sobre toda a tela
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, nullptr);
    
    // Painel central para entrada de senha - CENTRALIZADO CORRETAMENTE
    int painelLargura = ConfigLayout::X(800);
    int painelAltura = ConfigLayout::Y(250);
    int larguraTela = ConfigLayout::X(1525); // Largura real da janela
    int painelX = (larguraTela - painelLargura) / 2; // Centraliza corretamente
    int painelY = ConfigLayout::Y(150);
    
    // Cor do painel baseada no tema
    SDL_Color corPainel = tema.getCorRetangulos();
    
    // Desenha painel com bordas arredondadas
    roundedBoxRGBA(renderer, painelX, painelY, 
                   painelX + painelLargura, painelY + painelAltura,
                   15, corPainel.r, corPainel.g, corPainel.b, 255);
    
    // Título do painel - CENTRALIZADO
    std::string titulo;
    {
        std::lock_guard<std::mutex> lock(mtxRede);
        if (indiceRedeSelecionada >= 0 && indiceRedeSelecionada < (int)redesDisponiveis.size()) {
            titulo = "Conectar a: " + redesDisponiveis[indiceRedeSelecionada].nome;
        }
    }
    if (!titulo.empty()) {
        // Centraliza o título no painel
        int larguraTitulo = titulo.length() * ConfigLayout::F(28) * 0.6;
        int posXTitulo = painelX + (painelLargura - larguraTitulo) / 2;
        
        desenharTexto(renderer, titulo,
                      posXTitulo, painelY + ConfigLayout::Y(30),
                      tema.getCorTextoNegrito(), ConfigLayout::F(28));
    }
    
    // Label da senha
    desenharTexto(renderer, "Digite a senha:",
                  painelX + ConfigLayout::X(50), painelY + ConfigLayout::Y(80),
                  tema.getCorTextoNormal(), ConfigLayout::F(22));
    
    // Campo de senha usa cor de botão pressionado do tema
    SDL_Color corCampo = tema.getCorBotaoPressionado();
    
    int campoX = painelX + ConfigLayout::X(50);
    int campoY = painelY + ConfigLayout::Y(115);
    int campoLargura = painelLargura - ConfigLayout::X(100);
    int campoAltura = ConfigLayout::Y(50);
    
    // Desenha o campo
    roundedBoxRGBA(renderer, campoX, campoY,
                   campoX + campoLargura, campoY + campoAltura,
                   8, corCampo.r, corCampo.g, corCampo.b, 255);
    
    // Borda do campo (mais visível)
    SDL_Color corBorda = tema.getCorTextoNormal();
    roundedRectangleRGBA(renderer, campoX, campoY,
                         campoX + campoLargura, campoY + campoAltura,
                         8, corBorda.r, corBorda.g, corBorda.b, 180);
    
    // Texto da senha com cursor piscante
    if (senhaAtual.empty()) {
        // Placeholder
        desenharTexto(renderer, "Clique nas teclas abaixo",
                      campoX + ConfigLayout::X(15), campoY + ConfigLayout::Y(12),
                      SDL_Color{120, 120, 120, 255}, ConfigLayout::F(20));
    } else {
        // Senha oculta (asteriscos)
        std::string senhaOculta(senhaAtual.length(), '*');
        
        // Adiciona cursor piscante (pisca a cada 500ms)
        if (SDL_GetTicks() % 1000 < 500) {
            senhaOculta += "|";
        }
        
        desenharTexto(renderer, senhaOculta,
                      campoX + ConfigLayout::X(15), campoY + ConfigLayout::Y(12),
                      tema.getCorTextoNegrito(), ConfigLayout::F(22));
    }
    
    // Desenha os botões interativos se existirem
    if (btnConectarSenha) {
        btnConectarSenha->desenhar(renderer);
    }
    
    if (btnCancelarSenha) {
        btnCancelarSenha->desenhar(renderer);
    }
    
    // Desenha o teclado virtual POR ÚLTIMO (sobre tudo)
    if (tecladoVirtual.estaVisivel()) {
        tecladoVirtual.desenhar(renderer);
    }
}

/**
 * @brief Alterna o estado do Wi-Fi entre ON e OFF.
 */
void JanelaRede::toggleWifi() {
    bool estadoDesejado = false;
    {
        std::lock_guard<std::mutex> lock(mtxRede);
        if (redeCabeadaAtiva) {
            mensagemStatus = "Conexao cabeada ativa. Wi-Fi nao precisa ser alterado.";
            mensagemErro = false;
            return;
        }
        estadoDesejado = !wifiAtivo;
    }
    definirMensagemStatus(estadoDesejado ? "Ativando Wi-Fi..." : "Desativando Wi-Fi...");
    gerAudio.tocarSom("select.wav");

    iniciarTarefaEmSegundoPlano([this, estadoDesejado]() {
        system_result resultado = ::definir_estado_wifi_result(estadoDesejado);
        sincronizarComBackend();
        definirMensagemStatus(
            resultado.ok
                ? (estadoDesejado ? "Wi-Fi ativado." : "Wi-Fi desativado.")
                : (resultado.mensagem.empty() ? "Falha ao alterar o Wi-Fi." : resultado.mensagem),
            !resultado.ok
        );
    });
}

/**
 * @brief Abre o teclado virtual para entrada de senha.
 * @param indiceRede Índice da rede que precisa de senha.
 * 
 */
void JanelaRede::abrirTecladoSenha(int indiceRede) {
    std::string nomeRede;
    {
        std::lock_guard<std::mutex> lock(mtxRede);
        if (indiceRede < 0 || indiceRede >= (int)redesDisponiveis.size()) return;
        nomeRede = redesDisponiveis[indiceRede].nome;
    }
    
    indiceRedeSelecionada = indiceRede;
    senhaAtual = "";
    tecladoVisivel = true;
    
    // Abre o teclado virtual
    tecladoVirtual.abrir();
    
    // Cria os botões de Conectar e Cancelar
    // Posições ajustadas para os botões ficarem acima do teclado
    int painelLargura = ConfigLayout::X(800);
    int larguraTela = ConfigLayout::X(1525); // Usa largura correta
    int painelX = (larguraTela - painelLargura) / 2; // Centraliza corretamente
    int painelY = ConfigLayout::Y(150);
    
    int btnLargura = ConfigLayout::X(160);
    int btnAltura = ConfigLayout::Y(40);
    int btnY = painelY + ConfigLayout::Y(190); // Posição abaixo do campo de senha
    
    // Botão Conectar (verde)
    int btnConectarX = painelX + (painelLargura / 2) - btnLargura - ConfigLayout::X(20);
    btnConectarSenha = std::make_unique<MeuProjeto::Botao>(
        btnConectarX, btnY, btnLargura, btnAltura, "Conectar");
    btnConectarSenha->setCor(
        SDL_Color{60, 150, 60, 255},   // Normal verde
        SDL_Color{80, 180, 80, 255},   // Hover verde claro
        SDL_Color{40, 120, 40, 255}    // Pressed verde escuro
    );
    btnConectarSenha->setRetanguloBordasArredondadas(8);
    
    // Botão Cancelar (vermelho)
    int btnCancelarX = painelX + (painelLargura / 2) + ConfigLayout::X(20);
    btnCancelarSenha = std::make_unique<MeuProjeto::Botao>(
        btnCancelarX, btnY, btnLargura, btnAltura, "Cancelar");
    btnCancelarSenha->setCor(
        SDL_Color{180, 60, 60, 255},   // Normal vermelho
        SDL_Color{210, 80, 80, 255},   // Hover vermelho claro
        SDL_Color{150, 40, 40, 255}    // Pressed vermelho escuro
    );
    btnCancelarSenha->setRetanguloBordasArredondadas(8);
    
    gerAudio.tocarSom("select.wav");
    
    std::cout << "[REDE] Abrindo teclado para rede: " 
              << nomeRede << std::endl;
}

/**
 * @brief Fecha o teclado virtual e limpa o estado de entrada de senha.
 */
void JanelaRede::fecharTecladoSenha() {
    tecladoVisivel = false;
    tecladoVirtual.fechar();
    indiceRedeSelecionada = -1;
    senhaAtual = "";
    btnConectarSenha.reset();
    btnCancelarSenha.reset();
    
    gerAudio.tocarSom("navegacao.wav");
    
    std::cout << "[REDE] Teclado fechado" << std::endl;
}

/**
 * @brief Confirma a senha digitada e conecta à rede.
 * 
 */
void JanelaRede::confirmarSenha() {
    std::string ssid;
    {
        std::lock_guard<std::mutex> lock(mtxRede);
        if (indiceRedeSelecionada < 0 || indiceRedeSelecionada >= (int)redesDisponiveis.size()) {
            return;
        }
        ssid = redesDisponiveis[indiceRedeSelecionada].nome;
    }
    
    gerAudio.tocarSom("select.wav");
    const std::string senha = senhaAtual;
    fecharTecladoSenha();
    definirMensagemStatus("Conectando a " + ssid + "...");

    iniciarTarefaEmSegundoPlano([this, ssid, senha]() {
        system_result resultado = ::conectar_wifi_result(ssid, senha);
        sincronizarComBackend();
        definirMensagemStatus(
            resultado.ok
                ? ("Conectado a " + ssid + ".")
                : (resultado.mensagem.empty() ? "Falha ao conectar na rede." : resultado.mensagem),
            !resultado.ok
        );
    });
}

/**
 * @brief Seleciona uma rede da lista.
 * @param indice Índice da rede na lista de redes disponíveis.
 */
void JanelaRede::selecionarRede(int indice) {
    RedeInfo redeSelecionada("", false, false, 0);
    bool wifiAtivoLocal = false;
    {
        std::lock_guard<std::mutex> lock(mtxRede);
        if (indice < 0 || indice >= (int)redesDisponiveis.size()) return;
        wifiAtivoLocal = wifiAtivo;
        redeSelecionada = redesDisponiveis[indice];
    }
    if (!wifiAtivoLocal) return;
    
    if (redeSelecionada.conectada) {
        definirMensagemStatus("Esta rede ja esta conectada.");
        gerAudio.tocarSom("select.wav");
    } else if (redeSelecionada.requerSenha) {
        abrirTecladoSenha(indice);
    } else {
        gerAudio.tocarSom("select.wav");
        const std::string ssid = redeSelecionada.nome;
        definirMensagemStatus("Conectando a " + ssid + "...");
        iniciarTarefaEmSegundoPlano([this, ssid]() {
            system_result resultado = ::conectar_wifi_result(ssid, "");
            sincronizarComBackend();
            definirMensagemStatus(
                resultado.ok
                    ? ("Conectado a " + ssid + ".")
                    : (resultado.mensagem.empty() ? "Falha ao conectar na rede." : resultado.mensagem),
                !resultado.ok
            );
        });
    }
}

/**
 * @brief Move o foco para o elemento anterior.
 */
void JanelaRede::navegarParaCima() {
    if (indiceFocado > 0) {
        indiceFocado--;
        atualizarScroll();
        gerAudio.tocarSom("navegacao.wav");
    }
}

/**
 * @brief Move o foco para o próximo elemento.
 */
void JanelaRede::navegarParaBaixo() {
    int maxIndice = 0;
    {
        std::lock_guard<std::mutex> lock(mtxRede);
        if (redeCabeadaAtiva) return;
        maxIndice = (int)redesDisponiveis.size(); // +1 para incluir o toggle
    }
    if (indiceFocado < maxIndice) {
        indiceFocado++;
        atualizarScroll();
        gerAudio.tocarSom("navegacao.wav");
    }
}

/**
 * @brief Atualiza o scroll da lista baseado no elemento focado.
 */
void JanelaRede::atualizarScroll() {
    if (indiceFocado <= 0) {
        // Toggle focado, scroll no topo
        scrollOffset = 0;
        return;
    }
    
    int indiceRede = indiceFocado - 1;
    
    // Scroll para baixo se necessário
    if (indiceRede >= scrollOffset + maxRedesVisiveis) {
        scrollOffset = indiceRede - maxRedesVisiveis + 1;
    }
    
    // Scroll para cima se necessário
    if (indiceRede < scrollOffset) {
        scrollOffset = indiceRede;
    }
}

/**
 * @brief Confirma a seleção do elemento focado.
 */
void JanelaRede::confirmarSelecao() {
    {
        std::lock_guard<std::mutex> lock(mtxRede);
        if (redeCabeadaAtiva) return;
    }

    if (indiceFocado == 0) {
        // Toggle do Wi-Fi focado
        toggleWifi();
    } else if (indiceFocado > 0) {
        // Uma rede está focada
        int indiceRede = indiceFocado - 1;
        selecionarRede(indiceRede);
    }
}

/**
 * @brief Processa eventos de input específicos para esta tela.
 * @param evento Referência ao evento SDL capturado.
 * @return true se o evento foi processado e causou mudança de estado.
 * 
 */
bool JanelaRede::processarEvento(SDL_Event& evento) {
    if (tecladoVisivel) {

        // Cria um adaptador para permitir que o TecladoVirtual modifique senhaAtual
        MeuProjeto::StringInputAdapter adapter(senhaAtual);
        
        // Processa hover nos botões
        if (evento.type == SDL_MOUSEMOTION) {
            if (btnConectarSenha) {
                btnConectarSenha->handleMouseMotion(evento, 0, 0);
            }
            if (btnCancelarSenha) {
                btnCancelarSenha->handleMouseMotion(evento, 0, 0);
            }
        }
        
        // Processa cliques nos botões (DOWN para feedback visual)
        if (evento.type == SDL_MOUSEBUTTONDOWN && evento.button.button == SDL_BUTTON_LEFT) {
            int mouseX = evento.button.x;
            int mouseY = evento.button.y;
            
            // Verifica clique no botão Conectar
            if (btnConectarSenha && btnConectarSenha->contemPonto(mouseX, mouseY)) {
                btnConectarSenha->handleMouseMotion(evento, 0, 0);
                return true;
            }
            
            // Verifica clique no botão Cancelar
            if (btnCancelarSenha && btnCancelarSenha->contemPonto(mouseX, mouseY)) {
                btnCancelarSenha->handleMouseMotion(evento, 0, 0);
                return true;
            }
        }
        
        // Processa soltar botão do mouse (executa ação)
        if (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT) {
            int mouseX = evento.button.x;
            int mouseY = evento.button.y;
            
            // Verifica clique no botão Conectar
            if (btnConectarSenha && btnConectarSenha->contemPonto(mouseX, mouseY)) {
                confirmarSenha();
                return true;
            }
            
            // Verifica clique no botão Cancelar
            if (btnCancelarSenha && btnCancelarSenha->contemPonto(mouseX, mouseY)) {
                fecharTecladoSenha();
                return true;
            }
        }

        
        // Botão B do controle para cancelar
        if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
            if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
                fecharTecladoSenha();
                return true;
            }
        }
       
        
        // Processa eventos de controle no teclado (D-Pad, Analógico, Botão A)
        if (evento.type == SDL_CONTROLLERBUTTONDOWN || 
            evento.type == SDL_CONTROLLERBUTTONUP ||
            evento.type == SDL_CONTROLLERAXISMOTION) {
            
            // IMPORTANTE: processarControle retorna true se processou
            bool processado = tecladoVirtual.processarControle(evento, static_cast<BotaoPesquisa*>(&adapter));
            
            if (processado) {
                return true;
            }
        }
        
       
        if (evento.type == SDL_MOUSEBUTTONDOWN || evento.type == SDL_MOUSEBUTTONUP) {
            
            int mx = evento.button.x;
            int my = evento.button.y;
            
            // Calcula área do teclado virtual (mesmos valores do TecladoVirtual.cpp)
            int baseX = ConfigLayout::X(262);
            int baseY = ConfigLayout::Y(600);
            int painelPadding = ConfigLayout::X(20);
            int painelLargura = ConfigLayout::X(1000);
            int painelAltura = ConfigLayout::Y(400);
            
            SDL_Rect areaTeclado = {
                baseX - painelPadding, 
                baseY - painelPadding, 
                painelLargura, 
                painelAltura
            };
            
            // Verifica se o clique está DENTRO da área do teclado
            bool dentroTeclado = (mx >= areaTeclado.x && mx <= areaTeclado.x + areaTeclado.w &&
                                  my >= areaTeclado.y && my <= areaTeclado.y + areaTeclado.h);
            
            // Calcula área do painel de senha (para evitar processar clicks lá)
            int painelLargura2 = ConfigLayout::X(800);
            int painelAltura2 = ConfigLayout::Y(250);
            int larguraTela = ConfigLayout::X(1525);
            int painelX = (larguraTela - painelLargura2) / 2;
            int painelY = ConfigLayout::Y(150);
            
            SDL_Rect areaPainelSenha = {
                painelX, painelY,
                painelLargura2, painelAltura2
            };
            
            bool dentroPainelSenha = (mx >= areaPainelSenha.x && mx <= areaPainelSenha.x + areaPainelSenha.w &&
                                      my >= areaPainelSenha.y && my <= areaPainelSenha.y + areaPainelSenha.h);
            
     
            if (dentroTeclado && !dentroPainelSenha && tecladoVirtual.estaVisivel()) {
                
                bool processado = tecladoVirtual.processarMouse(evento, static_cast<BotaoPesquisa*>(&adapter));
                
                if (processado) {
                    return true;
                }
            }
            
        }
        
        
        // Tecla física para adicionar caracteres
        if (evento.type == SDL_TEXTINPUT) {
            senhaAtual += evento.text.text;
            std::cout << "[REDE] Texto físico adicionado: " << evento.text.text << std::endl;
            return true;
        }
        
        if (evento.type == SDL_KEYDOWN) {
            SDL_Keycode key = evento.key.keysym.sym;
            
            // Backspace
            if (key == SDLK_BACKSPACE && !senhaAtual.empty()) {
                senhaAtual.pop_back();
                std::cout << "[REDE] Backspace físico" << std::endl;
                return true;
            }
            
            // Enter para confirmar
            if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                confirmarSenha();
                return true;
            }
            
            // ESC para cancelar
            if (key == SDLK_ESCAPE) {
                fecharTecladoSenha();
                return true;
            }
        }
        
        // Consome TODOS os eventos quando o teclado está visível
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(mtxRede);
        if (redeCabeadaAtiva) {
            return false;
        }
    }
        
    // Processa cliques do mouse
    if (evento.type == SDL_MOUSEBUTTONDOWN) {
        if (evento.button.button == SDL_BUTTON_LEFT) {
            int mouseX = evento.button.x;
            int mouseY = evento.button.y;
            
            // Verifica clique no toggle do Wi-Fi
            if (btnToggleWifi && btnToggleWifi->contemPonto(mouseX, mouseY)) {
                toggleWifi();
                return true;
            }
            
            // Verifica clique nas redes (apenas se Wi-Fi estiver ativo)
            if (wifiAtivo) {
                int primeiraVisivel = scrollOffset;
                int ultimaVisivel = std::min(primeiraVisivel + maxRedesVisiveis,
                                             (int)botoesRedes.size());
                for (int i = primeiraVisivel; i < ultimaVisivel; i++) {
                    if (botoesRedes[i]->contemPonto(mouseX, mouseY)) {
                        selecionarRede(i);
                        return true;
                    }
                }
            }
        }
    }
    
    // Processa movimento do mouse (hover)
    if (evento.type == SDL_MOUSEMOTION) {
        if (btnToggleWifi) {
            btnToggleWifi->handleMouseMotion(evento, 0, 0);
        }
        
        int primeiraVisivel = scrollOffset;
        int ultimaVisivel = std::min(primeiraVisivel + maxRedesVisiveis,
                                     (int)botoesRedes.size());
        for (int i = primeiraVisivel; i < ultimaVisivel; i++) {
            botoesRedes[i]->handleMouseMotion(evento, 0, 0);
        }
    }
    
    // Navegação por controle/teclado
    if (evento.type == SDL_KEYDOWN || evento.type == SDL_CONTROLLERBUTTONDOWN) {
        bool paraAcao = false;
        bool paraBaixo = false;
        bool confirmar = false;
        
        if (evento.type == SDL_KEYDOWN) {
            switch (evento.key.keysym.sym) {
                case SDLK_UP:
                case SDLK_w:
                    paraAcao = true;
                    break;
                case SDLK_DOWN:
                case SDLK_s:
                    paraBaixo = true;
                    break;
                case SDLK_RETURN:
                case SDLK_SPACE:
                    confirmar = true;
                    break;
            }
        } else if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
            switch (evento.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    paraAcao = true;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    paraBaixo = true;
                    break;
                case SDL_CONTROLLER_BUTTON_A:
                    confirmar = true;
                    break;
            }
        }
        
        if (paraAcao) {
            navegarParaCima();
            return true;
        }
        if (paraBaixo) {
            navegarParaBaixo();
            return true;
        }
        if (confirmar) {
            confirmarSelecao();
            return true;
        }
    }
    
    // Navegação por analógico
    if (evento.type == SDL_CONTROLLERAXISMOTION) {
        Uint32 agora = SDL_GetTicks();
        if (agora - ultimoInputAnalogico < INTERVALO_ANALOGICO) {
            return false;
        }
        
        if (evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
            if (evento.caxis.value < -DEADZONE) {
                navegarParaCima();
                ultimoInputAnalogico = agora;
                return true;
            } else if (evento.caxis.value > DEADZONE) {
                navegarParaBaixo();
                ultimoInputAnalogico = agora;
                return true;
            }
        }
    }
    
    return false;
}

/**
 * @brief Reseta o estado da janela para valores padrão.
 */
void JanelaRede::resetar() {
    indiceFocado = SDL_NumJoysticks() > 0 ? 0 : -1;
    scrollOffset = 0;
    tecladoVisivel = false;
    indiceRedeSelecionada = -1;
    senhaAtual = "";
    inicializarBotoes();
    
    // Recarrega a imagem explicativa (pode ter mudado o tema)
    liberarImagemExplicativa();
    agendarSincronizacaoComBackend("Atualizando redes Wi-Fi...");
}
