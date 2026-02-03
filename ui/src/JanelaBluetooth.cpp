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
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include "GerenciadorAudio.hpp"
#include "GerenciadorImagens.hpp"
#include <iostream>
#include <algorithm>

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
    inicializarBotoes();
    inicializarDispositivosPareados();
    inicializarDispositivosDisponiveis();
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
 * As cores são obtidas do gerenciador de temas e as dimensões do ConfigLayout.
 */
void JanelaBluetooth::inicializarBotoes() {
    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color btnNormal = tema.getCorBotaoNormal();
    SDL_Color btnHover = tema.getCorBotaoHover();
    SDL_Color btnPress = tema.getCorBotaoPressionado();

    btnToggleBluetooth = std::make_unique<Botao>(
        ConfigLayout::X(513), ConfigLayout::Y(180),
        ConfigLayout::X(500), ConfigLayout::Y(80),
        bluetoothAtivo ? "Bluetooth: ON" : "Bluetooth: OFF"
    );
    btnToggleBluetooth->setCor(btnNormal, btnHover, btnPress);
    btnToggleBluetooth->setRetanguloBordasArredondadas(15);
}

/**
 * @brief Inicializa a lista de dispositivos pareados.
 * 
 * @details Cria uma lista inicial de dispositivos Bluetooth pareados (hardcoded para demonstração).
 * Cada dispositivo contém nome, tipo, status de pareamento, status de conexão e endereço MAC.
 */
void JanelaBluetooth::inicializarDispositivosPareados() {
    dispositivosPareados.clear();
    
    dispositivosPareados.push_back(DispositivoBluetooth(
        "DualShock 4", TipoDispositivoBT::CONTROLE, true, true, "A1:B2:C3:D4:E5:F6"
    ));
    
    dispositivosPareados.push_back(DispositivoBluetooth(
        "Sony WH-1000XM4", TipoDispositivoBT::HEADPHONE, true, false, "11:22:33:44:55:66"
    ));
    
    dispositivosPareados.push_back(DispositivoBluetooth(
        "Galaxy S21", TipoDispositivoBT::SMARTPHONE, true, false, "AA:BB:CC:DD:EE:FF"
    ));
}

/**
 * @brief Inicializa a lista de dispositivos disponíveis.
 * 
 * @details Cria uma lista inicial de dispositivos Bluetooth disponíveis para pareamento
 * (hardcoded para demonstração).
 */
void JanelaBluetooth::inicializarDispositivosDisponiveis() {
    dispositivosDisponiveis.clear();
    
    dispositivosDisponiveis.push_back(DispositivoBluetooth(
        "JBL Flip 5", TipoDispositivoBT::HEADPHONE, false, false, "12:34:56:78:90:AB"
    ));
    
    dispositivosDisponiveis.push_back(DispositivoBluetooth(
        "Samsung TV", TipoDispositivoBT::TV, false, false, "98:76:54:32:10:FE"
    ));
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
    
    if (!texturaExplicacao) {
        SDL_Log("[ERRO] Imagem explicativa não encontrada: %s", caminhoExplicacao.c_str());
        SDL_Log("[INFO] Certifique-se de que o arquivo existe no caminho correto");
    } else {
        SDL_Log("[INFO] Imagem explicativa carregada com sucesso: %s", caminhoExplicacao.c_str());
    }
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
        
        if (escaneando) {
            desenharMensagemEscaneamento(renderer);
        }
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
 * @brief Desenha a seção de dispositivos pareados.
 * 
 * @details Renderiza a lista de dispositivos Bluetooth pareados.
 * Inclui título, lista de dispositivos e indicadores de scroll.
 * 
 * @param renderer Ponteiro para o renderizador SDL.
 */
void JanelaBluetooth::desenharDispositivosPareados(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    int baseY = ConfigLayout::Y(320);
    
    desenharTexto(renderer, "Dispositivos Pareados",
                  ConfigLayout::X(217), baseY,
                  tema.getCorTextoNegrito(), ConfigLayout::F(32));
    
    if (dispositivosPareados.empty()) {
        desenharTexto(renderer, "Nenhum dispositivo pareado",
                      ConfigLayout::X(250), baseY + ConfigLayout::Y(60),
                      tema.getCorTextoNormal(), ConfigLayout::F(24));
        return;
    }
    
    int offsetY = baseY + ConfigLayout::Y(60);
    int espacamento = ConfigLayout::Y(90);
    
    size_t maxIndex = static_cast<size_t>(scrollOffsetPareados + maxDispositivosVisiveis);
    for (size_t i = scrollOffsetPareados; 
         i < dispositivosPareados.size() && i < maxIndex;
         ++i) {
        
        int indiceLista = i - scrollOffsetPareados;
        int posY = offsetY + (indiceLista * espacamento);
        bool focado = (indiceFocado == static_cast<int>(i));
        
        desenharDispositivo(renderer, dispositivosPareados[i], 
                           ConfigLayout::X(250), posY, focado);
    }
    
    if (scrollOffsetPareados > 0) {
        desenharTexto(renderer, "▲ Mais acima",
                      ConfigLayout::X(250), baseY + ConfigLayout::Y(40),
                      tema.getCorTextoNormal(), ConfigLayout::F(18));
    }
    
    if (static_cast<size_t>(scrollOffsetPareados + maxDispositivosVisiveis) < dispositivosPareados.size()) {
        int posIndicador = offsetY + (maxDispositivosVisiveis * espacamento) - ConfigLayout::Y(30);
        desenharTexto(renderer, "▼ Mais abaixo",
                      ConfigLayout::X(250), posIndicador,
                      tema.getCorTextoNormal(), ConfigLayout::F(18));
    }
}

/**
 * @brief Desenha a seção de dispositivos disponíveis.
 * 
 * @details Renderiza a lista de dispositivos Bluetooth disponíveis para pareamento.
 * Inclui título, lista de dispositivos e indicadores de scroll.
 * 
 * @param renderer Ponteiro para o renderizador SDL.
 */
void JanelaBluetooth::desenharDispositivosDisponiveis(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    
    int numPareadosVisiveis = std::min(
        static_cast<int>(dispositivosPareados.size()), 
        maxDispositivosVisiveis
    );
    
    int baseYPareados = ConfigLayout::Y(320);
    int espacamentoPareados = ConfigLayout::Y(90);
    int alturaSecaoPareados = ConfigLayout::Y(60) + (numPareadosVisiveis * espacamentoPareados);
    int baseY = baseYPareados + alturaSecaoPareados + ConfigLayout::Y(40);
    
    std::string textoDisponiveis = escaneando ? 
        "Dispositivos Disponíveis (Escaneando...)" : 
        "Dispositivos Disponíveis";
    
    desenharTexto(renderer, textoDisponiveis,
                  ConfigLayout::X(217), baseY,
                  tema.getCorTextoNegrito(), ConfigLayout::F(32));
    
    if (dispositivosDisponiveis.empty()) {
        std::string mensagem = escaneando ? 
            "Buscando dispositivos..." : 
            "Nenhum dispositivo encontrado (Pressione Y para escanear)";
            
        desenharTexto(renderer, mensagem,
                      ConfigLayout::X(250), baseY + ConfigLayout::Y(60),
                      tema.getCorTextoNormal(), ConfigLayout::F(24));
        return;
    }
    
    int offsetY = baseY + ConfigLayout::Y(60);
    int espacamento = ConfigLayout::Y(90);
    
    size_t maxIndex = static_cast<size_t>(scrollOffsetDisponiveis + maxDispositivosVisiveis);
    for (size_t i = scrollOffsetDisponiveis; 
         i < dispositivosDisponiveis.size() && i < maxIndex;
         ++i) {
        
        int indiceLista = i - scrollOffsetDisponiveis;
        int posY = offsetY + (indiceLista * espacamento);
        int indiceGlobal = dispositivosPareados.size() + i;
        bool focado = (indiceFocado == indiceGlobal);
        
        desenharDispositivo(renderer, dispositivosDisponiveis[i], 
                           ConfigLayout::X(250), posY, focado);
    }
    
    if (scrollOffsetDisponiveis > 0) {
        desenharTexto(renderer, "▲ Mais acima",
                      ConfigLayout::X(250), baseY + ConfigLayout::Y(40),
                      tema.getCorTextoNormal(), ConfigLayout::F(18));
    }
    
    if (static_cast<size_t>(scrollOffsetDisponiveis + maxDispositivosVisiveis) < dispositivosDisponiveis.size()) {
        int posIndicador = offsetY + (maxDispositivosVisiveis * espacamento) - ConfigLayout::Y(30);
        desenharTexto(renderer, "▼ Mais abaixo",
                      ConfigLayout::X(250), posIndicador,
                      tema.getCorTextoNormal(), ConfigLayout::F(18));
    }
}

/**
 * @brief Desenha um dispositivo individual na lista.
 * 
 * @details Renderiza as informações de um dispositivo Bluetooth, incluindo nome,
 * status (conectado/pareado/disponível) e destaque visual quando focado.
 * 
 * @param renderer Ponteiro para o renderizador SDL.
 * @param dispositivo Referência constante ao dispositivo a ser desenhado.
 * @param x Posição horizontal (em coordenadas ajustadas pelo ConfigLayout).
 * @param y Posição vertical (em coordenadas ajustadas pelo ConfigLayout).
 * @param focado Indica se o dispositivo está atualmente focado (selecionado).
 */
void JanelaBluetooth::desenharDispositivo(SDL_Renderer* renderer, 
                                          const DispositivoBluetooth& dispositivo,
                                          int x, int y, bool focado) {
    auto& tema = GerenciadorTemas::getInstance();
    
    // Fundo do dispositivo (se focado)
    if (focado) {
        SDL_Rect fundoFoco = {
            x - ConfigLayout::X(10),
            y - ConfigLayout::Y(5),
            ConfigLayout::X(1000),
            ConfigLayout::Y(70)
        };
        SDL_Color corFoco = tema.getCorDestaque();
        SDL_SetRenderDrawColor(renderer, corFoco.r, corFoco.g, corFoco.b, corFoco.a);
        SDL_RenderFillRect(renderer, &fundoFoco);
    }
    
    
    // Nome do dispositivo
    SDL_Color corNome = focado ? tema.getCorTextoNegrito() : tema.getCorTextoNormal();
    desenharTexto(renderer, dispositivo.nome, 
                  x + ConfigLayout::X(60), y,
                  corNome, ConfigLayout::F(28));
    
    // Status (Conectado/Pareado/Disponível)
    std::string status;
    SDL_Color corStatus;
    
    if (dispositivo.conectado) {
        status = "● Conectado";
        corStatus = {0, 200, 0, 255}; // Verde
    } else if (dispositivo.pareado) {
        status = "○ Pareado";
        corStatus = tema.getCorTextoNormal();
    } else {
        status = "○ Disponível";
        corStatus = tema.getCorTextoNormal();
    }
    
    desenharTexto(renderer, status,
                  x + ConfigLayout::X(60), y + ConfigLayout::Y(35),
                  corStatus, ConfigLayout::F(20));
    
}

/**
 * @brief Desenha a mensagem de escaneamento em andamento.
 * 
 * @details Renderiza uma mensagem animada com barra de progresso durante o
 * escaneamento de dispositivos Bluetooth. A animação dura DURACAO_ESCANEAMENTO ms.
 * 
 * @param renderer Ponteiro para o renderizador SDL.
 */
void JanelaBluetooth::desenharMensagemEscaneamento(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    Uint32 tempoAtual = SDL_GetTicks();
    
    if (tempoAtual - tempoInicioEscanear >= DURACAO_ESCANEAMENTO) {
        escaneando = false;
        return;
    }
    
    int posX = ConfigLayout::X(500);
    int posY = ConfigLayout::Y(900);
    
    float progresso = static_cast<float>(tempoAtual - tempoInicioEscanear) / DURACAO_ESCANEAMENTO;
    
    // Animação de pontos
    int numPontos = static_cast<int>(progresso * 10) % 4;
    std::string pontos(numPontos, '.');
    std::string mensagem = "Escaneando" + pontos;
    
    SDL_Rect fundoMsg = {
        posX - ConfigLayout::X(50),
        posY - ConfigLayout::Y(20),
        ConfigLayout::X(400),
        ConfigLayout::Y(60)
    };
    SDL_Color corFundo = tema.getCorRetangulos();
    SDL_SetRenderDrawColor(renderer, corFundo.r, corFundo.g, corFundo.b, 230);
    SDL_RenderFillRect(renderer, &fundoMsg);
    
    desenharTexto(renderer, mensagem, posX, posY,
                  tema.getCorTextoNegrito(), ConfigLayout::F(28));
    
    // Barra de progresso
    int larguraBarra = ConfigLayout::X(300);
    int alturaBarra = ConfigLayout::Y(8);
    int posBarraX = posX + ConfigLayout::X(20);
    int posBarraY = posY + ConfigLayout::Y(40);
    
    // Fundo da barra (cor de retângulos mais escura)
    SDL_Rect fundoBarra = {posBarraX, posBarraY, larguraBarra, alturaBarra};
    SDL_Color corFundoBarra = tema.getCorFundo();
    SDL_SetRenderDrawColor(renderer, corFundoBarra.r, corFundoBarra.g, corFundoBarra.b, 255);
    SDL_RenderFillRect(renderer, &fundoBarra);
    
    // Barra de progresso (cor de destaque do tema)
    int larguraProgresso = static_cast<int>(larguraBarra * progresso);
    SDL_Rect barraProgresso = {posBarraX, posBarraY, larguraProgresso, alturaBarra};
    SDL_Color corProgresso = tema.getCorDestaque();
    SDL_SetRenderDrawColor(renderer, corProgresso.r, corProgresso.g, corProgresso.b, 255);
    SDL_RenderFillRect(renderer, &barraProgresso);
}

/**
 * @brief Desenha a imagem explicativa no rodapé da tela.
 * 
 * @details Renderiza uma imagem com explicações dos controles na parte inferior da tela.
 * A imagem ocupa toda a largura da tela e tem altura fixa de 30 pixels.
 * 
 * @param renderer Ponteiro para o renderizador SDL.
 */
void JanelaBluetooth::desenharImagemExplicativa(SDL_Renderer* renderer) {
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


// AÇÕES

/**
 * @brief Alterna o estado do Bluetooth (ON/OFF).
 * 
 * @details Ativa ou desativa o Bluetooth. Quando desativado, limpa o estado de foco
 * e para qualquer escaneamento em andamento.
 */
void JanelaBluetooth::toggleBluetooth() {
    bluetoothAtivo = !bluetoothAtivo;
    
    if (!bluetoothAtivo) {
        indiceFocado = -1;
        escaneando = false;
    }
    
    std::cout << "[BLUETOOTH] Estado alterado: " 
              << (bluetoothAtivo ? "ON" : "OFF") << std::endl;
}

/**
 * @brief Inicia o escaneamento de dispositivos Bluetooth.
 * 
 * @details Inicia um processo de escaneamento por novos dispositivos.
 * Só funciona se o Bluetooth estiver ativado.
 * O escaneamento dura DURACAO_ESCANEAMENTO milissegundos.
 */
void JanelaBluetooth::iniciarEscaneamento() {
    if (!bluetoothAtivo) return;
    
    escaneando = true;
    tempoInicioEscanear = SDL_GetTicks();
    
    std::cout << "[BLUETOOTH] Iniciando escaneamento..." << std::endl;
}

/**
 * @brief Alterna a conexão de um dispositivo pareado.
 * 
 * @details Conecta ou desconecta um dispositivo já pareado.
 * 
 * @param indice Índice do dispositivo na lista de dispositivos pareados.
 */
void JanelaBluetooth::toggleConexaoDispositivo(int indice) {
    if (indice < 0 || indice >= static_cast<int>(dispositivosPareados.size())) {
        return;
    }
    
    DispositivoBluetooth& dispositivo = dispositivosPareados[indice];
    dispositivo.conectado = !dispositivo.conectado;
    
    std::cout << "[BLUETOOTH] " 
              << (dispositivo.conectado ? "Conectando" : "Desconectando") 
              << " dispositivo: " << dispositivo.nome << std::endl;
}

/**
 * @brief Pareia um dispositivo disponível.
 * 
 * @details Move um dispositivo da lista de disponíveis para a lista de pareados.
 * 
 * @param indice Índice do dispositivo na lista de dispositivos disponíveis.
 */
void JanelaBluetooth::parearDispositivo(int indice) {
    if (indice < 0 || indice >= static_cast<int>(dispositivosDisponiveis.size())) {
        return;
    }
    
    DispositivoBluetooth dispositivo = dispositivosDisponiveis[indice];
    dispositivo.pareado = true;
    
    dispositivosPareados.push_back(dispositivo);
    dispositivosDisponiveis.erase(dispositivosDisponiveis.begin() + indice);
    
    if (indiceFocado >= static_cast<int>(dispositivosPareados.size())) {
        indiceFocado = dispositivosPareados.size() - 1;
    }
    
    atualizarScroll();
    
    std::cout << "[BLUETOOTH] Pareando dispositivo: " << dispositivo.nome << std::endl;
}

/**
 * @brief Esquece um dispositivo pareado.
 * 
 * @details Remove um dispositivo da lista de pareados e o move de volta para
 * a lista de disponíveis (com status de pareamento resetado).
 * 
 * @param indice Índice do dispositivo na lista de dispositivos pareados.
 */
void JanelaBluetooth::esquecerDispositivo(int indice) {
    if (indice < 0 || indice >= static_cast<int>(dispositivosPareados.size())) {
        return;
    }
    
    DispositivoBluetooth dispositivo = dispositivosPareados[indice];
    dispositivo.pareado = false;
    dispositivo.conectado = false;
    
    dispositivosDisponiveis.push_back(dispositivo);
    dispositivosPareados.erase(dispositivosPareados.begin() + indice);
    
    if (indiceFocado >= static_cast<int>(dispositivosPareados.size()) && 
        indiceFocado > 0) {
        indiceFocado--;
    }
    
    atualizarScroll();
    
    std::cout << "[BLUETOOTH] Esquecendo dispositivo: " << dispositivo.nome << std::endl;
    
    gerAudio.tocarSom("navegacao.wav");
}

// NAVEGAÇÃO

/**
 * @brief Navega para o elemento acima na lista.
 * 
 * @details Move o foco para o elemento anterior na lista de elementos focáveis.
 * Só funciona se o Bluetooth estiver ativado.
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
 * 
 * @details Move o foco para o próximo elemento na lista de elementos focáveis.
 * Só funciona se o Bluetooth estiver ativado.
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
 * 
 * @details Executa a ação apropriada para o elemento atualmente focado:
 * - Se o botão toggle estiver focado: alterna o estado do Bluetooth
 * - Se um dispositivo pareado estiver focado: alterna a conexão
 * - Se um dispositivo disponível estiver focado: pareia o dispositivo
 */
void JanelaBluetooth::confirmarSelecao() {
    if (!bluetoothAtivo && indiceFocado == -1) {
        toggleBluetooth();
        gerAudio.tocarSom("select.wav");
        return;
    }
    
    if (indiceFocado == -1) {
        toggleBluetooth();
        gerAudio.tocarSom("select.wav");
    } else if (indiceFocado < static_cast<int>(dispositivosPareados.size())) {
        toggleConexaoDispositivo(indiceFocado);
        gerAudio.tocarSom("select.wav");
    } else {
        int indiceDisponivel = indiceFocado - dispositivosPareados.size();
        parearDispositivo(indiceDisponivel);
        gerAudio.tocarSom("select.wav");
    }
}

/**
 * @brief Atualiza os offsets de scroll conforme a navegação.
 * 
 * @details Ajusta as variáveis de scroll (scrollOffsetPareados e scrollOffsetDisponiveis)
 * para garantir que o elemento focado esteja sempre visível na tela.
 */
void JanelaBluetooth::atualizarScroll() {
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
 * 
 * @details Inclui o botão toggle + dispositivos pareados + dispositivos disponíveis.
 * 
 * @return Número total de elementos que podem receber foco.
 */
int JanelaBluetooth::calcularTotalElementosFocaveis() {
    return 1 + dispositivosPareados.size() + dispositivosDisponiveis.size();
}

// EVENTOS

/**
 * @brief Processa eventos SDL (mouse, controle, etc.).
 * 
 * @details Gerencia a interação do usuário com a interface, incluindo cliques do mouse,
 * botões do controle e movimentos do analógico.
 * 
 * @param evento Referência ao evento SDL a ser processado.
 * @return true se o evento foi processado, false caso contrário.
 */
bool JanelaBluetooth::processarEvento(SDL_Event& evento) {
    Uint32 tempoAtual = SDL_GetTicks();
    
    if (evento.type == SDL_MOUSEBUTTONDOWN) {
        int mx = evento.button.x;
        int my = evento.button.y;
        
        if (btnToggleBluetooth && btnToggleBluetooth->contemPonto(mx, my)) {
            toggleBluetooth();
            gerAudio.tocarSom("select.wav");
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
                if (bluetoothAtivo && 
                    indiceFocado >= 0 && 
                    indiceFocado < static_cast<int>(dispositivosPareados.size())) {
                    esquecerDispositivo(indiceFocado);
                    return true;
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
 * 
 * @details Retorna a interface ao estado inicial, removendo foco,
 * resetando scrolls e parando escaneamentos.
 */
void JanelaBluetooth::resetar() {
    indiceFocado = -1;
    scrollOffsetPareados = 0;
    scrollOffsetDisponiveis = 0;
    escaneando = false;
}