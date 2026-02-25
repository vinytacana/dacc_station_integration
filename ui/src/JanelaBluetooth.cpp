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
    std::lock_guard<std::mutex> lock(mtx_dispositivos);
    
    this->bluetoothAtivo = ::obter_estado_bluetooth();
    
    if (bluetoothAtivo) {
        dispositivosPareados.clear();
        auto reais = ::get_list_device(); // Pega dispositivos pareados/conhecidos
        
        for (const auto& d : reais) {
            dispositivosPareados.push_back(DispositivoBluetooth(
                d.nome, stringParaTipoBT(d.icon), true, d.conectado, d.mac
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

    // Posições relativas à janela de configurações (1525x1116)
    // Usamos F() apenas para escala, sem o offset centralizador global do X() e Y()
    int btnW = ConfigLayout::F(500);
    int btnH = ConfigLayout::F(80);
    int btnX = (ConfigLayout::F(1525) - btnW) / 2; // Centralizado na janela
    int btnY = ConfigLayout::F(180);

    btnToggleBluetooth = std::make_unique<Botao>(
        btnX, btnY, btnW, btnH,
        bluetoothAtivo ? "Bluetooth: ON" : "Bluetooth: OFF"
    );
    btnToggleBluetooth->setCor(btnNormal, btnHover, btnPress);
    btnToggleBluetooth->setRetanguloBordasArredondadas(15);

    // --- Botão Escanear (Rodapé) ---
    int scanW = ConfigLayout::F(350); 
    int scanH = ConfigLayout::F(60);
    int scanX = (ConfigLayout::F(1525) - scanW) / 2;
    int scanY = ConfigLayout::F(980); 

    btnEscanear = std::make_unique<Botao>(
        scanX, scanY, scanW, scanH,
        "Escanear"
    );
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
    std::string caminhoExplicacao;
    
    if (tema.getTemaAtual() == TipoTema::CLARO) {
        caminhoExplicacao = "assets/images/light/explicacaoBotoesJanelaBluetoothClaro.jpg";
    } else {
        caminhoExplicacao = "assets/images/dark/explicacaoBotoesJanelaBluetoothEscuro.jpg";
    }
    
    texturaExplicacao = gerImg.carregar(renderer, caminhoExplicacao);
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

    auto& tema = GerenciadorTemas::getInstance();

    carregarTexturas(renderer);
    
    desenharTexto(renderer, "Configuração Bluetooth",
                  ConfigLayout::X(462), ConfigLayout::Y(80),
                  tema.getCorTextoNegrito(), ConfigLayout::F(48));
    
    // Desenha toggle
    desenharToggleBluetooth(renderer);
    
    if (bluetoothAtivo) {
        desenharDispositivosPareados(renderer);
        desenharDispositivosDisponiveis(renderer);
        
        // Desenha o novo botão aqui
        desenharBotaoEscanear(renderer);
    } else {
        // Mensagem centralizada quando Bluetooth está desligado
        desenharTexto(renderer, "Bluetooth desativado. Ative para ver dispositivos.",
                      ConfigLayout::X(270), ConfigLayout::Y(400),
                      tema.getCorTextoNormal(), ConfigLayout::F(28));
    }

    // Desenha imagem explicativa no rodapé
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

    if (escaneando) {
        btnEscanear->setTexto("Escaneando...");
    } else {
        btnEscanear->setTexto("Escanear (Y)");
    }

    btnEscanear->desenhar(renderer);
}

/**
 * @brief Desenha a seção de dispositivos pareados.
 */
void JanelaBluetooth::desenharDispositivosPareados(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    int baseY = ConfigLayout::F(320);
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
        
        desenharDispositivo(renderer, dispositivosPareados[i], 
                           ConfigLayout::F(250), posY, focado);
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
    
    int baseYPareados = ConfigLayout::F(320);
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
        
        desenharDispositivo(renderer, dispositivosDisponiveis[i], 
                           ConfigLayout::F(250), posY, focado);
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
    desenharTexto(renderer, dispositivo.nome, 
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
        posY - ConfigLayout::F(20),
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


// AÇÕES

/**
 * @brief Alterna o estado do Bluetooth (ON/OFF).
 */
void JanelaBluetooth::toggleBluetooth() {
    bluetoothAtivo = !bluetoothAtivo;
    ::definir_estado_bt(bluetoothAtivo);
    
    // Pequeno delay para permitir que o comando do sistema seja processado antes de ler o estado
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Sincroniza para garantir que a variável bluetoothAtivo reflita a realidade
    this->bluetoothAtivo = ::obter_estado_bluetooth();

    if (btnToggleBluetooth) {
        btnToggleBluetooth->setTexto(bluetoothAtivo ? "Bluetooth: ON" : "Bluetooth: OFF");
    }
    
    if (!bluetoothAtivo) {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        indiceFocado = -1;
        escaneando = false;
        dispositivosDisponiveis.clear();
    } else {
        sincronizarComHardware();
    }
}

/**
 * @brief Inicia o escaneamento de dispositivos Bluetooth.
 */
void JanelaBluetooth::iniciarEscaneamento() {
    if (!bluetoothAtivo || escaneando) return;
    
    escaneando = true;
    tempoInicioEscanear = SDL_GetTicks();
    
    std::thread([this]() {
        auto detectados = ::scan_dispositivos_bluetooth(5); // Scan de 5 segundos
        
        std::lock_guard<std::mutex> lock(this->mtx_dispositivos);
        this->dispositivosDisponiveis.clear();
        for (const auto& d : detectados) {
            // Verifica se já está nos pareados
            bool ja_pareado = false;
            for (const auto& p : this->dispositivosPareados) {
                if (p.endereco == d.mac) {
                    ja_pareado = true;
                    break;
                }
            }
            if (!ja_pareado) {
                this->dispositivosDisponiveis.push_back(DispositivoBluetooth(
                    d.nome, stringParaTipoBT(d.icon), false, false, d.mac
                ));
            }
        }
        this->escaneando = false;
    }).detach();
}

/**
 * @brief Alterna a conexão de um dispositivo pareado.
 */
void JanelaBluetooth::toggleConexaoDispositivo(int indice) {
    std::lock_guard<std::mutex> lock(mtx_dispositivos);
    if (indice < 0 || indice >= static_cast<int>(dispositivosPareados.size())) return;
    
    DispositivoBluetooth& dispositivo = dispositivosPareados[indice];
    if (dispositivo.conectado) {
        if (::desconectar_bluetooth(dispositivo.endereco)) {
            dispositivo.conectado = false;
        }
    } else {
        // Conectar também tenta parear se necessário no bluetoothctl
        if (::conectar_bluetooth(dispositivo.endereco)) {
            dispositivo.conectado = true;
        }
    }
}

/**
 * @brief Pareia um dispositivo disponível.
 */
void JanelaBluetooth::parearDispositivo(int indice) {
    std::unique_lock<std::mutex> lock(mtx_dispositivos);
    if (indice < 0 || indice >= static_cast<int>(dispositivosDisponiveis.size())) return;
    
    DispositivoBluetooth dispositivo = dispositivosDisponiveis[indice];
    lock.unlock(); // Destrava para a chamada de sistema

    if (::conectar_bluetooth(dispositivo.endereco)) {
        sincronizarComHardware(); // Recarrega listas
    }
}

/**
 * @brief Esquece um dispositivo pareado.
 */
void JanelaBluetooth::esquecerDispositivo(int indice) {
    std::unique_lock<std::mutex> lock(mtx_dispositivos);
    if (indice < 0 || indice >= static_cast<int>(dispositivosPareados.size())) return;
    
    std::string mac = dispositivosPareados[indice].endereco;
    lock.unlock();

    // Executa comando de remoção via bluetoothctl
    std::string cmd = "bluetoothctl remove " + mac + " > /dev/null 2>&1";
    if (system(cmd.c_str()) == 0) {
        sincronizarComHardware();
    }
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
    sincronizarComHardware();
    indiceFocado = -1;
    scrollOffsetPareados = 0;
    scrollOffsetDisponiveis = 0;
    escaneando = false;
}