/**
 * @file JanelaBluetooth.cpp
 * @brief Implementação da classe JanelaBluetooth para gerenciamento de dispositivos Bluetooth.
 * 
 * @details Esta classe implementa a interface gráfica para gerenciamento de dispositivos Bluetooth,
 * permitindo ativar/desativar o Bluetooth, parear dispositivos, conectar/desconectar e escanear
 * dispositivos disponíveis. A interface é compatível com controle (gamepad) e mouse.
 * 
 */

#include "JanelaBluetooth.hpp"
#include "functions.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include "GerenciadorAudio.hpp"
#include "GerenciadorImagens.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <utility>

using namespace MeuProjeto;

extern GerenciadorAudio gerAudio;       ///< Instância global do gerenciador de áudio
extern GerenciadorImagens gerImg;       ///< Instância global do gerenciador de imagens

// CONSTRUTOR E DESTRUTOR

/**
 * @brief Construtor da classe JanelaBluetooth.
 * 
 * @details Inicializa todos os componentes da janela Bluetooth, incluindo botões,
 * listas de dispositivos pareados e disponíveis.
 */
JanelaBluetooth::JanelaBluetooth() {
    sincronizarComHardware();
    inicializarBotoes();
}

/**
 * @brief Converte string de ícone do Bluetooth para TipoDispositivoBT.
 */
static TipoDispositivoBT stringParaTipoBT(const std::string& icon) {
    if (icon == "input-gaming" || icon == "input-keyboard" || icon == "input-mouse") 
        return TipoDispositivoBT::CONTROLE;
    if (icon == "audio-headset" || icon == "audio-card" || icon == "audio-headphones") 
        return TipoDispositivoBT::HEADPHONE;
    if (icon == "phone" || icon == "smartphone") 
        return TipoDispositivoBT::SMARTPHONE;
    if (icon == "tv" || icon == "video-display") 
        return TipoDispositivoBT::TV;
    if (icon == "computer" || icon == "laptop") 
        return TipoDispositivoBT::COMPUTADOR;
    return TipoDispositivoBT::OUTRO;
}

/**
 * @brief Sincroniza o estado da UI com o hardware real.
 */
void JanelaBluetooth::sincronizarComHardware() {
    bluetooth_adapter_status status = ::obter_status_bluetooth();
    bluetoothAtivo = status.powered;
    adaptadorDisponivel = status.controller_disponivel;
    adaptadorBloqueado = status.soft_blocked || status.hard_blocked;

    std::lock_guard<std::mutex> lock(mtx_dispositivos);

    dispositivosPareados.clear();
    if (bluetoothAtivo && adaptadorDisponivel) {
        auto reais = ::listar_dispositivos_bluetooth_pareados();
        
        for (const auto& d : reais) {
            dispositivosPareados.push_back(DispositivoBluetooth(
                d.nome, stringParaTipoBT(d.icon), d.pareado, d.conectado, d.mac
            ));
        }
    }
}

/**
 * @brief Destrutor da classe JanelaBluetooth.
 * 
 * @details Libera recursos alocados, especialmente a textura explicativa.
 */
JanelaBluetooth::~JanelaBluetooth() {
    encerrando = true;
    if (scanThread.joinable()) {
        scanThread.join();
    }
    if (workerThread.joinable()) {
        workerThread.join();
    }

    if (texturaExplicacao) {
        texturaExplicacao = nullptr;
    }
}

// INICIALIZAÇÃO

/**
 * @brief Inicializa os botões da interface.
 * 
 * @details Cria e configura o botão toggle para ativar/desativar o Bluetooth.
 */
void JanelaBluetooth::inicializarBotoes() {
    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color btnNormal = tema.getCorBotaoNormal();
    SDL_Color btnHover = tema.getCorBotaoHover();
    SDL_Color btnPress = tema.getCorBotaoPressionado();

    // --- Layout da Barra de Controles ---
    int espacamento = ConfigLayout::F(30);       // Espaço entre os dois botões
    int btnToggleW = ConfigLayout::F(380);       // Largura do botão ON/OFF
    int btnScanW = ConfigLayout::F(320);         // Largura do botão Escanear
    int btnH = ConfigLayout::F(70);              // Altura padronizada para ambos
    int btnY = ConfigLayout::F(160);             // Posição Y (Altura na tela)

    // Calcula o X inicial para que o conjunto todo fique centralizado
    int larguraTotal = btnToggleW + espacamento + btnScanW;
    int startX = (ConfigLayout::F(1525) - larguraTotal) / 2;

    // --- Botão Toggle (Esquerda) ---
    btnToggleBluetooth = std::make_unique<Botao>(
        startX, btnY, btnToggleW, btnH,
        bluetoothAtivo ? "Bluetooth: ON" : "Bluetooth: OFF"
    );
    btnToggleBluetooth->setCor(btnNormal, btnHover, btnPress);
    btnToggleBluetooth->setRetanguloBordasArredondadas(15);

    // --- Botão Escanear (Direita) ---
    btnEscanear = std::make_unique<Botao>(
        startX + btnToggleW + espacamento, btnY, btnScanW, btnH,
        "Escanear (Y)"
    );
    // Usamos a mesma cor para manter a harmonia visual
    btnEscanear->setCor(btnNormal, btnHover, btnPress);
    btnEscanear->setRetanguloBordasArredondadas(15);
}

/**
 * @brief Inicializa a lista de dispositivos pareados.
 */
void JanelaBluetooth::inicializarDispositivosPareados() {
    // Agora gerido pelo sincronizarComHardware
}

/**
 * @brief Inicializa a lista de dispositivos disponíveis.
 */
void JanelaBluetooth::inicializarDispositivosDisponiveis() {
    // Agora gerido pelo iniciarEscaneamento
}

// CARREGAMENTO DE TEXTURAS

/**
 * @brief Carrega as texturas necessárias para a interface.
 * 
 * @details Carrega a imagem explicativa dos botões, que varia conforme o tema atual
 * (claro ou escuro). A textura é carregada através do GerenciadorImagens.
 * 
 * @param renderer Ponteiro para o renderizador SDL.
 */
void JanelaBluetooth::carregarTexturas(SDL_Renderer* renderer) {
    if (!renderer) return;
    
    auto& tema = GerenciadorTemas::getInstance();
    TipoTema temaAtual = tema.getTemaAtual();
    
    // Otimização: Só recarrega se o tema mudou ou se a textura ainda não existe
    static TipoTema ultimoTema = static_cast<TipoTema>(-1);
    if (temaAtual == ultimoTema && texturaExplicacao != nullptr) return;
    
    std::string caminhoExplicacao;
    if (temaAtual == TipoTema::CLARO) {
        caminhoExplicacao = "assets/images/light/explicacaoBotoesJanelaBluetoothClaro.jpg";
    } else {
        caminhoExplicacao = "assets/images/dark/explicacaoBotoesJanelaBluetoothEscuro.jpg";
    }
    
    texturaExplicacao = gerImg.carregar(renderer, caminhoExplicacao);
    ultimoTema = temaAtual;
}

// DESENHO

/**
 * @brief Desenha toda a interface da janela Bluetooth.
 * 
 * @details Desenha todos os componentes da interface, incluindo título, botão toggle,
 * listas de dispositivos e imagem explicativa. A renderização varia conforme o estado
 * do Bluetooth (ativo/inativo).
 * 
 * @param renderer Ponteiro para o renderizador SDL.
 */
void JanelaBluetooth::desenhar(SDL_Renderer* renderer) {
    if (!renderer) return;

    carregarTexturas(renderer);
    
    auto& tema = GerenciadorTemas::getInstance();
    
    desenharTexto(renderer, "Configuração Bluetooth",
                  ConfigLayout::X(462), ConfigLayout::Y(80),
                  tema.getCorTextoNegrito(), ConfigLayout::F(48));
    desenharStatusOperacional(renderer);
    
    // Desenha toggle
    desenharToggleBluetooth(renderer);
    
    if (!adaptadorDisponivel) {
        desenharTexto(renderer, "Nenhum adaptador Bluetooth detectado no sistema.",
                      ConfigLayout::X(250), ConfigLayout::Y(350),
                      tema.getCorTextoNormal(), ConfigLayout::F(28));
        desenharTexto(renderer, "Verifique o hardware ou o servico do adaptador.",
                      ConfigLayout::X(250), ConfigLayout::Y(400),
                      tema.getCorTextoNormal(), ConfigLayout::F(24));
    } else if (bluetoothAtivo) {
        desenharDispositivosPareados(renderer);
        desenharDispositivosDisponiveis(renderer);
        desenharBotaoEscanear(renderer);
    } else {
        // Mensagem centralizada quando Bluetooth está desligado
        desenharTexto(renderer, "Bluetooth desativado. Ative para ver dispositivos.",
                      ConfigLayout::X(270), ConfigLayout::Y(400),
                      tema.getCorTextoNormal(), ConfigLayout::F(28));
    }

    desenharImagemExplicativa(renderer);
}

/**
 * @brief Desenha o botão toggle do Bluetooth.
 * 
 * @details Desenha o botão que alterna o estado do Bluetooth (ON/OFF).
 * Atualiza o texto do botão conforme o estado atual.
 * 
 * @param renderer Ponteiro para o renderizador SDL.
 */
void JanelaBluetooth::desenharToggleBluetooth(SDL_Renderer* renderer) {
    if (!btnToggleBluetooth) return;
    
    btnToggleBluetooth->setTexto(bluetoothAtivo ? "Bluetooth: ON" : "Bluetooth: OFF");
    btnToggleBluetooth->setFocado(indiceFocado == -1);
    btnToggleBluetooth->desenhar(renderer);
}

/**
 * @brief Renderiza o botão de escanear.
 */
void JanelaBluetooth::desenharBotaoEscanear(SDL_Renderer* renderer) {
    if (!btnEscanear || !bluetoothAtivo) return;

    // Atualiza o texto dependendo do estado
    if (escaneando) {
        btnEscanear->setTexto("Escaneando...");
    } else {
        btnEscanear->setTexto("Escanear (Y)");
    }

    btnEscanear->desenhar(renderer);

    auto& tema = GerenciadorTemas::getInstance();
    std::string dica = escaneando ? "Aguarde o scan terminar..." : "Mouse: clique em um dispositivo para agir";
    desenharTexto(renderer, dica,
                  ConfigLayout::F(460), ConfigLayout::F(245),
                  tema.getCorTextoNormal(), ConfigLayout::F(20));
}

/**
 * @brief Desenha a seção de dispositivos pareados.
 */
void JanelaBluetooth::desenharDispositivosPareados(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    int baseY = ConfigLayout::F(280); // Sobe a lista um pouco mais para melhorar um pouco a ui da janela bluetooth
    int baseX = ConfigLayout::F(217);
    
    desenharTexto(renderer, "Dispositivos Pareados",
                  baseX, baseY,
                  tema.getCorTextoNegrito(), ConfigLayout::F(32));
    
    std::lock_guard<std::mutex> lock(mtx_dispositivos);
    if (dispositivosPareados.empty()) {
        desenharTexto(renderer, "Nenhum dispositivo pareado",
                      baseX + ConfigLayout::F(33), baseY + ConfigLayout::F(60),
                      tema.getCorTextoNormal(), ConfigLayout::F(24));
        return;
    }
    
    int offsetY = baseY + ConfigLayout::F(60);
    int espacamento = ConfigLayout::F(90);
    
    size_t maxIndex = static_cast<size_t>(scrollOffsetPareados + maxDispositivosVisiveis);
    for (size_t i = scrollOffsetPareados; 
         i < dispositivosPareados.size() && i < maxIndex;
         ++i) {
        
        int indiceLista = i - scrollOffsetPareados;
        int posY = offsetY + (indiceLista * espacamento);
        bool focado = (indiceFocado == static_cast<int>(i));
        
        // Garante que o dispositivo tenha um nome para não ficar invisível
        DispositivoBluetooth d = dispositivosPareados[i];
        if (d.nome.empty()) d.nome = d.endereco;

        desenharDispositivo(renderer, d, ConfigLayout::F(250), posY, focado);
    }
    
    if (scrollOffsetPareados > 0) {
        desenharTexto(renderer, "▲ Mais acima",
                      ConfigLayout::F(250), baseY + ConfigLayout::F(40),
                      tema.getCorTextoNormal(), ConfigLayout::F(18));
    }
    
    if (static_cast<size_t>(scrollOffsetPareados + maxDispositivosVisiveis) < dispositivosPareados.size()) {
        int posIndicador = offsetY + (maxDispositivosVisiveis * espacamento) - ConfigLayout::F(30);
        desenharTexto(renderer, "▼ Mais abaixo",
                      ConfigLayout::F(250), posIndicador,
                      tema.getCorTextoNormal(), ConfigLayout::F(18));
    }
}

/**
 * @brief Desenha a seção de dispositivos disponíveis.
 */
void JanelaBluetooth::desenharDispositivosDisponiveis(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    std::lock_guard<std::mutex> lock(mtx_dispositivos);
    int numPareadosVisiveis = std::min(
        static_cast<int>(dispositivosPareados.size()), 
        maxDispositivosVisiveis
    );
    
    int baseYPareados = ConfigLayout::F(280);
    int espacamentoPareados = ConfigLayout::F(90);
    int alturaSecaoPareados = ConfigLayout::F(60) + (numPareadosVisiveis * espacamentoPareados);
    int baseY = baseYPareados + alturaSecaoPareados + ConfigLayout::F(40);
    int baseX = ConfigLayout::F(217);
    
    std::string textoDisponiveis = escaneando ? 
        "Dispositivos Disponíveis (Escaneando...)" : 
        "Dispositivos Disponíveis";
    
    desenharTexto(renderer, textoDisponiveis,
                  baseX, baseY,
                  tema.getCorTextoNegrito(), ConfigLayout::F(32));
    
    if (dispositivosDisponiveis.empty()) {
        std::string mensagem = escaneando ? 
            "Buscando dispositivos..." : 
            "Nenhum dispositivo encontrado.";
            
        desenharTexto(renderer, mensagem,
                      baseX + ConfigLayout::F(33), baseY + ConfigLayout::F(60),
                      tema.getCorTextoNormal(), ConfigLayout::F(24));
        return;
    }
    
    int offsetY = baseY + ConfigLayout::F(60);
    int espacamento = ConfigLayout::F(90);
    
    size_t maxIndex = static_cast<size_t>(scrollOffsetDisponiveis + maxDispositivosVisiveis);
    for (size_t i = scrollOffsetDisponiveis; 
         i < dispositivosDisponiveis.size() && i < maxIndex;
         ++i) {
        
        int indiceLista = i - scrollOffsetDisponiveis;
        int posY = offsetY + (indiceLista * espacamento);
        int indiceGlobal = dispositivosPareados.size() + i;
        bool focado = (indiceFocado == indiceGlobal);
        
        // Fallback para nome vazio
        DispositivoBluetooth d = dispositivosDisponiveis[i];
        if (d.nome.empty()) d.nome = d.endereco;

        desenharDispositivo(renderer, d, ConfigLayout::F(250), posY, focado);
    }

    if (scrollOffsetDisponiveis > 0) {
        desenharTexto(renderer, "▲ Mais acima",
                      ConfigLayout::F(250), baseY + ConfigLayout::F(40),
                      tema.getCorTextoNormal(), ConfigLayout::F(18));
    }

    if (static_cast<size_t>(scrollOffsetDisponiveis + maxDispositivosVisiveis) < dispositivosDisponiveis.size()) {
        int posIndicador = offsetY + (maxDispositivosVisiveis * espacamento) - ConfigLayout::F(30);
        desenharTexto(renderer, "▼ Mais abaixo",
                      ConfigLayout::F(250), posIndicador,
                      tema.getCorTextoNormal(), ConfigLayout::F(18));
    }
}

/**
 * @brief Desenha um dispositivo individual na lista.
 */
void JanelaBluetooth::desenharDispositivo(SDL_Renderer* renderer, 
                                          const DispositivoBluetooth& dispositivo,
                                          int x, int y, bool focado) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Fundo do dispositivo (se focado)
    if (focado) {
        SDL_Rect fundoFoco = {
            x - ConfigLayout::F(10),
            y - ConfigLayout::F(5),
            ConfigLayout::F(1000),
            ConfigLayout::F(70)
        };
        SDL_Color corFoco = tema.getCorDestaque();
        SDL_SetRenderDrawColor(renderer, corFoco.r, corFoco.g, corFoco.b, corFoco.a);
        SDL_RenderFillRect(renderer, &fundoFoco);
    }
    
    // Nome do dispositivo
    SDL_Color corNome = focado ? tema.getCorTextoNegrito() : tema.getCorTextoNormal();
    std::string nomeExibicao = dispositivo.nome.empty() ? dispositivo.endereco : dispositivo.nome;
    
    desenharTexto(renderer, nomeExibicao, 
                  x + ConfigLayout::F(60), y,
                  corNome, ConfigLayout::F(28));
    
    // Status (Conectado/Pareado/Disponível)
    std::string status;
    SDL_Color corStatus;
    
    if (dispositivo.conectado) {
        status = "● Conectado";
        corStatus = {100, 255, 100, 255}; // Verde
    } else if (dispositivo.pareado) {
        status = "○ Pareado";
        corStatus = tema.getCorTextoNormal();
    } else {
        status = "○ Disponível";
        corStatus = tema.getCorTextoNormal();
    }
    
    desenharTexto(renderer, status,
                  x + ConfigLayout::F(60), y + ConfigLayout::F(35),
                  corStatus, ConfigLayout::F(20));

    std::string acao = dispositivo.pareado
        ? (dispositivo.conectado ? "Clique/A: desconectar   Direito/X: esquecer" : "Clique/A: conectar   Direito/X: esquecer")
        : "Clique/A: parear e conectar";
    desenharTexto(renderer, acao,
                  x + ConfigLayout::F(520), y + ConfigLayout::F(35),
                  tema.getCorTextoNormal(), ConfigLayout::F(18));
}

/**
 * @brief Desenha a mensagem de escaneamento em andamento.
 */
void JanelaBluetooth::desenharMensagemEscaneamento(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    Uint32 tempoAtual = SDL_GetTicks();
    
    int posX = ConfigLayout::F(500);
    int posY = ConfigLayout::F(900);
    
    // Animação de pontos
    int numPontos = (tempoAtual / 500) % 4;
    std::string pontos(numPontos, '.');
    std::string mensagem = "Escaneando" + pontos;
    
    SDL_Rect fundoMsg = {
        posX - ConfigLayout::F(50),
        posY - ConfigLayout::Y(20),
        ConfigLayout::F(400),
        ConfigLayout::F(60)
    };
    SDL_Color corFundo = tema.getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, corFundo.r, corFundo.g, corFundo.b, 230);
    SDL_RenderFillRect(renderer, &fundoMsg);
    
    desenharTexto(renderer, mensagem, posX, posY,
                  tema.getCorTextoNegrito(), ConfigLayout::F(28));
}

/**
 * @brief Desenha a imagem explicativa no rodapé da tela.
 */
void JanelaBluetooth::desenharImagemExplicativa(SDL_Renderer* renderer) {
    if (!texturaExplicacao) return;
    
    int larguraTela = ConfigLayout::F(1525); 
    int alturaImagem = ConfigLayout::F(30);
    int posY = ConfigLayout::F(1080) - alturaImagem;
    
    SDL_Rect destExplicacao = { 0, posY, larguraTela, alturaImagem };
    SDL_RenderCopy(renderer, texturaExplicacao, nullptr, &destExplicacao);
}

void JanelaBluetooth::desenharStatusOperacional(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    std::string mensagem;
    bool erro = false;

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        mensagem = mensagemStatus;
        erro = mensagemErro;
    }

    std::string resumo;
    if (!adaptadorDisponivel) {
        resumo = "Adaptador: indisponivel";
    } else if (adaptadorBloqueado && !bluetoothAtivo) {
        resumo = "Adaptador: bloqueado";
    } else {
        resumo = bluetoothAtivo ? "Adaptador: ativo" : "Adaptador: desligado";
    }

    desenharTexto(renderer, resumo,
                  ConfigLayout::X(475), ConfigLayout::Y(130),
                  tema.getCorTextoNormal(), ConfigLayout::F(24));

    if (!mensagem.empty()) {
        SDL_Color cor = erro ? SDL_Color{255, 120, 120, 255}
                             : SDL_Color{120, 255, 120, 255};
        desenharTexto(renderer, mensagem,
                      ConfigLayout::X(250), ConfigLayout::Y(200),
                      cor, ConfigLayout::F(22));
    }
}

bool JanelaBluetooth::localizarDispositivoPorPonto(int x, int y, bool& pareado, int& indiceLocal) {
    std::lock_guard<std::mutex> lock(mtx_dispositivos);

    const int cardX = ConfigLayout::F(240);
    const int cardW = ConfigLayout::F(1000);
    const int cardH = ConfigLayout::F(70);
    const int baseY = ConfigLayout::F(280) + ConfigLayout::F(60);
    const int espacamento = ConfigLayout::F(90);

    size_t maxPareados = static_cast<size_t>(scrollOffsetPareados + maxDispositivosVisiveis);
    for (size_t i = scrollOffsetPareados; i < dispositivosPareados.size() && i < maxPareados; ++i) {
        int indiceLista = static_cast<int>(i) - scrollOffsetPareados;
        int posY = baseY + (indiceLista * espacamento) - ConfigLayout::F(5);
        SDL_Rect area{cardX, posY, cardW, cardH};
        if (x >= area.x && x <= area.x + area.w && y >= area.y && y <= area.y + area.h) {
            pareado = true;
            indiceLocal = static_cast<int>(i);
            return true;
        }
    }

    int numPareadosVisiveis = std::min(static_cast<int>(dispositivosPareados.size()), maxDispositivosVisiveis);
    int alturaSecaoPareados = ConfigLayout::F(60) + (numPareadosVisiveis * espacamento);
    int baseYDisponiveis = ConfigLayout::F(280) + alturaSecaoPareados + ConfigLayout::F(40) + ConfigLayout::F(60);

    size_t maxDisponiveis = static_cast<size_t>(scrollOffsetDisponiveis + maxDispositivosVisiveis);
    for (size_t i = scrollOffsetDisponiveis; i < dispositivosDisponiveis.size() && i < maxDisponiveis; ++i) {
        int indiceLista = static_cast<int>(i) - scrollOffsetDisponiveis;
        int posY = baseYDisponiveis + (indiceLista * espacamento) - ConfigLayout::F(5);
        SDL_Rect area{cardX, posY, cardW, cardH};
        if (x >= area.x && x <= area.x + area.w && y >= area.y && y <= area.y + area.h) {
            pareado = false;
            indiceLocal = static_cast<int>(i);
            return true;
        }
    }

    return false;
}

void JanelaBluetooth::definirMensagemStatus(const std::string& mensagem, bool erro) {
    std::lock_guard<std::mutex> lock(mtx_dispositivos);
    mensagemStatus = mensagem;
    mensagemErro = erro;
}

bool JanelaBluetooth::iniciarTarefaEmSegundoPlano(std::function<void()> tarefa) {
    if (operacaoEmAndamento.exchange(true)) {
        definirMensagemStatus("Ha uma operacao Bluetooth em andamento.", true);
        return false;
    }

    if (workerThread.joinable()) {
        workerThread.join();
    }

    workerThread = std::thread([this, tarefa = std::move(tarefa)]() mutable {
        tarefa();
        operacaoEmAndamento = false;
    });
    return true;
}


// AÇÕES

/**
 * @brief Alterna o estado do Bluetooth (ON/OFF).
 */
void JanelaBluetooth::toggleBluetooth() {
    if (alternandoBluetooth.exchange(true)) return;

    bool estado_desejado = !bluetoothAtivo.load();
    definirMensagemStatus(estado_desejado ? "Ativando Bluetooth..." : "Desativando Bluetooth...");

    if (!iniciarTarefaEmSegundoPlano([this, estado_desejado]() {
        bluetooth_result resultado = ::definir_estado_bt_result(estado_desejado);
        sincronizarComHardware();

        if (resultado.ok) {
            definirMensagemStatus(estado_desejado ? "Bluetooth ativado." : "Bluetooth desativado.");
        } else {
            definirMensagemStatus(
                resultado.mensagem.empty() ? "Falha ao alterar o estado do Bluetooth." : resultado.mensagem,
                true
            );
        }

        if (!bluetoothAtivo) {
            std::lock_guard<std::mutex> lock(mtx_dispositivos);
            indiceFocado = -1;
            dispositivosDisponiveis.clear();
        }

        alternandoBluetooth = false;
    })) {
        alternandoBluetooth = false;
    }
}

/**
 * @brief Inicia o escaneamento de dispositivos Bluetooth.
 */
void JanelaBluetooth::iniciarEscaneamento() {
    if (!bluetoothAtivo || escaneando || operacaoEmAndamento) return;

    if (scanThread.joinable()) {
        scanThread.join();
    }

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        escaneando = true;
        mensagemStatus = "Escaneando dispositivos proximos...";
        mensagemErro = false;
        dispositivosDisponiveis.clear();
    }
    tempoInicioEscanear = SDL_GetTicks();

    scanThread = std::thread([this]() {
        auto detectados = ::scan_dispositivos_bluetooth(5); // Scan de 5 segundos

        if (encerrando) {
            escaneando = false;
            return;
        }

        // Sincroniza dispositivos pareados (status de conexão pode ter mudado)
        this->sincronizarComHardware();

        {
            std::lock_guard<std::mutex> lock(this->mtx_dispositivos);
            if (encerrando) {
                this->escaneando = false;
                return;
            }
            this->dispositivosDisponiveis.clear();
            for (const auto& d : detectados) {
                // Verifica se já está nos pareados (lista já atualizada acima)
                bool ja_pareado = false;
                for (const auto& p : this->dispositivosPareados) {
                    if (p.endereco == d.mac) {
                        ja_pareado = true;
                        break;
                    }
                }
                if (!ja_pareado) {
                    // Garante que o dispositivo tenha um nome exibível
                    std::string nomeFinal = d.nome.empty() ? d.mac : d.nome;
                    
                    this->dispositivosDisponiveis.push_back(DispositivoBluetooth(
                        nomeFinal, stringParaTipoBT(d.icon), false, false, d.mac
                    ));
                }
            }
            this->escaneando = false;
        }
        this->definirMensagemStatus(
            detectados.empty() ? "Scan concluido. Nenhum dispositivo novo encontrado."
                               : "Scan concluido. Dispositivos atualizados."
        );
    });
}

/**
 * @brief Alterna a conexão de um dispositivo pareado.
 */
void JanelaBluetooth::toggleConexaoDispositivo(int indice) {
    std::string endereco;
    bool conectado = false;

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        if (indice < 0 || indice >= static_cast<int>(dispositivosPareados.size())) return;
        endereco = dispositivosPareados[indice].endereco;
        conectado = dispositivosPareados[indice].conectado;
    }

    std::string acao = conectado ? "Desconectando..." : "Conectando...";
    definirMensagemStatus(acao);

    iniciarTarefaEmSegundoPlano([this, endereco, conectado]() {
        bluetooth_result resultado = conectado ? ::desconectar_bluetooth_result(endereco)
                                               : ::conectar_bluetooth_result(endereco);
        sincronizarComHardware();
        definirMensagemStatus(
            resultado.ok
                ? (conectado ? "Dispositivo desconectado." : "Dispositivo conectado.")
                : (resultado.mensagem.empty()
                    ? (conectado ? "Falha ao desconectar o dispositivo." : "Falha ao conectar o dispositivo.")
                    : resultado.mensagem),
            !resultado.ok
        );
    });
}

/**
 * @brief Pareia um dispositivo disponível.
 */
void JanelaBluetooth::parearDispositivo(int indice) {
    std::unique_lock<std::mutex> lock(mtx_dispositivos);
    if (indice < 0 || indice >= static_cast<int>(dispositivosDisponiveis.size())) return;
    
    DispositivoBluetooth dispositivo = dispositivosDisponiveis[indice];
    lock.unlock(); // Destrava para a chamada de sistema

    definirMensagemStatus("Pareando dispositivo...");
    iniciarTarefaEmSegundoPlano([this, dispositivo]() {
        bluetooth_result pareamento = ::parear_bluetooth(dispositivo.endereco);
        if (!pareamento.ok) {
            sincronizarComHardware();
            definirMensagemStatus(pareamento.mensagem.empty() ? "Falha ao parear dispositivo." : pareamento.mensagem, true);
            return;
        }

        bluetooth_result confianca = ::confiar_bluetooth(dispositivo.endereco);
        if (!confianca.ok) {
            sincronizarComHardware();
            definirMensagemStatus(confianca.mensagem.empty() ? "Falha ao confiar no dispositivo." : confianca.mensagem, true);
            return;
        }

        bluetooth_result conexao = ::conectar_bluetooth_result(dispositivo.endereco);
        sincronizarComHardware();
        definirMensagemStatus(
            conexao.ok ? "Dispositivo pareado e conectado."
                       : (conexao.mensagem.empty() ? "Pareado com sucesso, mas a conexao falhou." : conexao.mensagem),
            !conexao.ok
        );
    });
}

/**
 * @brief Esquece um dispositivo pareado.
 */
void JanelaBluetooth::esquecerDispositivo(int indice) {
    std::unique_lock<std::mutex> lock(mtx_dispositivos);
    if (indice < 0 || indice >= static_cast<int>(dispositivosPareados.size())) return;
    
    std::string mac = dispositivosPareados[indice].endereco;
    lock.unlock();

    definirMensagemStatus("Removendo dispositivo...");
    iniciarTarefaEmSegundoPlano([this, mac]() {
        bluetooth_result remocao = ::remover_bluetooth(mac);
        sincronizarComHardware();
        definirMensagemStatus(
            remocao.ok ? "Dispositivo removido."
                       : (remocao.mensagem.empty() ? "Falha ao remover dispositivo." : remocao.mensagem),
            !remocao.ok
        );
    });
}

// NAVEGAÇÃO

/**
 * @brief Navega para o elemento acima na lista.
 */
void JanelaBluetooth::navegarParaCima() {
    if (!bluetoothAtivo) return;
    
    if (indiceFocado > -1) {
        indiceFocado--;
        atualizarScroll();
        gerAudio.tocarSom("navegacao.wav");
    }
}

/**
 * @brief Navega para o elemento abaixo na lista.
 */
void JanelaBluetooth::navegarParaBaixo() {
    if (!bluetoothAtivo) return;
    
    int totalElementos = calcularTotalElementosFocaveis();
    if (indiceFocado < totalElementos - 1) {
        indiceFocado++;
        atualizarScroll();
        gerAudio.tocarSom("navegacao.wav");
    }
}

/**
 * @brief Confirma a seleção do elemento focado.
 */
void JanelaBluetooth::confirmarSelecao() {
    if (!bluetoothAtivo) {
        toggleBluetooth();
        gerAudio.tocarSom("select.wav");
        return;
    }
    
    if (indiceFocado == -1) {
        toggleBluetooth();
        gerAudio.tocarSom("select.wav");
    } else {
        int idx = -1;
        bool isPareado = false;

        {
            std::lock_guard<std::mutex> lock(mtx_dispositivos);
            if (indiceFocado < static_cast<int>(dispositivosPareados.size())) {
                idx = indiceFocado;
                isPareado = true;
            } else {
                idx = indiceFocado - dispositivosPareados.size();
                isPareado = false;
            }
        }

        if (isPareado) {
            toggleConexaoDispositivo(idx);
        } else {
            parearDispositivo(idx);
        }
        gerAudio.tocarSom("select.wav");
    }
}

/**
 * @brief Atualiza o scroll das listas baseado no foco.
 */
void JanelaBluetooth::atualizarScroll() {
    std::lock_guard<std::mutex> lock(mtx_dispositivos);
    if (indiceFocado >= 0 && indiceFocado < static_cast<int>(dispositivosPareados.size())) {
        if (indiceFocado < scrollOffsetPareados) {
            scrollOffsetPareados = indiceFocado;
        } else if (indiceFocado >= scrollOffsetPareados + maxDispositivosVisiveis) {
            scrollOffsetPareados = indiceFocado - maxDispositivosVisiveis + 1;
        }
    }
    
    int inicioDisponiveis = dispositivosPareados.size();
    if (indiceFocado >= inicioDisponiveis) {
        int indiceRelativo = indiceFocado - inicioDisponiveis;
        
        if (indiceRelativo < scrollOffsetDisponiveis) {
            scrollOffsetDisponiveis = indiceRelativo;
        } else if (indiceRelativo >= scrollOffsetDisponiveis + maxDispositivosVisiveis) {
            scrollOffsetDisponiveis = indiceRelativo - maxDispositivosVisiveis + 1;
        }
    }
}

/**
 * @brief Calcula o total de elementos focáveis na interface.
 */
int JanelaBluetooth::calcularTotalElementosFocaveis() {
    std::lock_guard<std::mutex> lock(mtx_dispositivos);
    return 1 + dispositivosPareados.size() + dispositivosDisponiveis.size();
}

// EVENTOS

/**
 * @brief Processa eventos SDL (mouse, controle, etc.).
 */
bool JanelaBluetooth::processarEvento(SDL_Event& evento) {
    Uint32 tempoAtual = SDL_GetTicks();
    
    // Processa movimento do mouse para hover
    if (evento.type == SDL_MOUSEMOTION) {
        if (btnToggleBluetooth) {
            btnToggleBluetooth->handleMouseMotion(evento, 0, 0);
        }
        if (bluetoothAtivo && btnEscanear) {
            btnEscanear->handleMouseMotion(evento, 0, 0);
        }

        if (bluetoothAtivo) {
            bool pareado = false;
            int indice = -1;
            if (localizarDispositivoPorPonto(evento.motion.x, evento.motion.y, pareado, indice)) {
                std::lock_guard<std::mutex> lock(mtx_dispositivos);
                indiceFocado = pareado ? indice : static_cast<int>(dispositivosPareados.size()) + indice;
            }
        }
    }

    if (evento.type == SDL_MOUSEBUTTONDOWN) {
        int mx = evento.button.x;
        int my = evento.button.y;
        
        if (btnToggleBluetooth && btnToggleBluetooth->contemPonto(mx, my)) {
            // Apenas para feedback visual de pressionado
            return true;
        }
        if (bluetoothAtivo && btnEscanear && btnEscanear->contemPonto(mx, my)) {
            return true;
        }
    }

    if (evento.type == SDL_MOUSEBUTTONUP) {
        int mx = evento.button.x;
        int my = evento.button.y;
        
        if (btnToggleBluetooth && btnToggleBluetooth->contemPonto(mx, my)) {
            if (alternandoBluetooth) return true;
            toggleBluetooth();
            gerAudio.tocarSom("select.wav");
            return true;
        }

        if (bluetoothAtivo && btnEscanear && btnEscanear->contemPonto(mx, my)) {
            if (!escaneando) {
                iniciarEscaneamento();
                gerAudio.tocarSom("select.wav");
            }
            return true;
        }

        if (bluetoothAtivo) {
            bool pareado = false;
            int indice = -1;
            if (localizarDispositivoPorPonto(mx, my, pareado, indice)) {
                if (evento.button.button == SDL_BUTTON_LEFT) {
                    if (pareado) {
                        toggleConexaoDispositivo(indice);
                    } else {
                        parearDispositivo(indice);
                    }
                } else if (evento.button.button == SDL_BUTTON_RIGHT && pareado) {
                    esquecerDispositivo(indice);
                }
                gerAudio.tocarSom("select.wav");
                return true;
            }
        }
    }
    
    if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (evento.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                navegarParaCima();
                return true;
                
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                navegarParaBaixo();
                return true;
                
            case SDL_CONTROLLER_BUTTON_A:
                if (alternandoBluetooth && indiceFocado == -1) {
                    return true;
                }
                confirmarSelecao();
                return true;
                
            case SDL_CONTROLLER_BUTTON_X:
                if (bluetoothAtivo && indiceFocado >= 0) {
                    std::unique_lock<std::mutex> lock(mtx_dispositivos);
                    if (indiceFocado < (int)dispositivosPareados.size()) {
                        lock.unlock();
                        esquecerDispositivo(indiceFocado);
                        return true;
                    }
                }
                break;
                
            case SDL_CONTROLLER_BUTTON_Y:
                if (bluetoothAtivo) {
                    iniciarEscaneamento();
                    gerAudio.tocarSom("select.wav");
                    return true;
                }
                break;
        }
    }
    
    if (evento.type == SDL_CONTROLLERAXISMOTION) {
        if (tempoAtual - ultimoInputAnalogico < INTERVALO_ANALOGICO) {
            return false;
        }
        
        if (evento.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
            if (evento.caxis.value < -DEADZONE) {
                navegarParaCima();
                ultimoInputAnalogico = tempoAtual;
                return true;
            } else if (evento.caxis.value > DEADZONE) {
                navegarParaBaixo();
                ultimoInputAnalogico = tempoAtual;
                return true;
            }
        }
    }
    
    return false;
}

/**
 * @brief Reseta o estado da janela Bluetooth.
 */
void JanelaBluetooth::resetar() {
    encerrando = true;
    if (scanThread.joinable()) {
        scanThread.join();
    }
    if (workerThread.joinable()) {
        workerThread.join();
    }
    encerrando = false;
    sincronizarComHardware();
    indiceFocado = -1;
    scrollOffsetPareados = 0;
    scrollOffsetDisponiveis = 0;
    escaneando = false;
    operacaoEmAndamento = false;
    mensagemStatus.clear();
    mensagemErro = false;
}
