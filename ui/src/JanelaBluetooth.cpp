/**
 * @file JanelaBluetooth.cpp
 * @brief Implementação da interface Bluetooth com seções exclusivas e lista rolável única.
 */

#include "JanelaBluetooth.hpp"
#include "config-dacc/functions.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include "GerenciadorAudio.hpp"
#include "GerenciadorImagens.hpp"

#include <SDL2/SDL2_gfxPrimitives.h>
#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>
#include <thread>
#include <unordered_set>
#include <utility>

using namespace MeuProjeto;

extern GerenciadorAudio gerAudio;
extern GerenciadorImagens gerImg;

namespace {

constexpr int LISTA_X = 220;
constexpr int LISTA_Y = 285;
constexpr int LISTA_W = 1085;
constexpr int LISTA_H = 745;
constexpr int CARD_H = 78;
constexpr int CARD_GAP = 14;
constexpr int SECAO_GAP = 28;
constexpr int HEADER_H = 42;
constexpr int EMPTY_H = 58;

std::string limitarTexto(const std::string& texto, size_t limite) {
    if (texto.size() <= limite) return texto;
    if (limite <= 3) return texto.substr(0, limite);
    return texto.substr(0, limite - 3) + "...";
}

SDL_Color comAlpha(SDL_Color cor, Uint8 alpha) {
    cor.a = alpha;
    return cor;
}

std::string normalizarMac(const std::string& mac) {
    std::string out = mac;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return out;
}

bool ordenarPorNome(const DispositivoBluetooth& a, const DispositivoBluetooth& b) {
    std::string nomeA = a.nome.empty() ? a.endereco : a.nome;
    std::string nomeB = b.nome.empty() ? b.endereco : b.nome;
    return nomeA < nomeB;
}

TipoDispositivoBT stringParaTipoBT(const std::string& icon) {
    if (icon == "input-gaming" || icon == "input-keyboard" || icon == "input-mouse") {
        return TipoDispositivoBT::CONTROLE;
    }
    if (icon == "audio-headset" || icon == "audio-card" || icon == "audio-headphones") {
        return TipoDispositivoBT::HEADPHONE;
    }
    if (icon == "phone" || icon == "smartphone") {
        return TipoDispositivoBT::SMARTPHONE;
    }
    if (icon == "tv" || icon == "video-display") {
        return TipoDispositivoBT::TV;
    }
    if (icon == "computer" || icon == "laptop") {
        return TipoDispositivoBT::COMPUTADOR;
    }
    return TipoDispositivoBT::OUTRO;
}

DispositivoBluetooth paraDispositivoUi(const device_bt& d) {
    std::string nome = d.nome.empty() ? d.mac : d.nome;
    return DispositivoBluetooth(nome, stringParaTipoBT(d.icon), d.pareado, d.conectado, d.mac);
}

std::string labelTipo(TipoDispositivoBT tipo) {
    switch (tipo) {
        case TipoDispositivoBT::CONTROLE: return "[Controle]";
        case TipoDispositivoBT::HEADPHONE: return "[Fone]";
        case TipoDispositivoBT::SMARTPHONE: return "[Celular]";
        case TipoDispositivoBT::TV: return "[TV]";
        case TipoDispositivoBT::COMPUTADOR: return "[PC]";
        case TipoDispositivoBT::OUTRO:
        default: return "[Outro]";
    }
}

std::string mensagemAmigavelBluetooth(const system_result& resultado, const std::string& fallback) {
    if (resultado.ok) return resultado.mensagem;
    if (resultado.codigo == "authentication_failed") return "Falha de autenticacao no dispositivo.";
    if (resultado.codigo == "operation_timeout" || resultado.codigo == "bluetoothctl_timeout") return "O dispositivo demorou para responder.";
    if (resultado.codigo == "adapter_unavailable") return "Nenhum adaptador Bluetooth disponivel.";
    if (resultado.codigo == "adapter_blocked" || resultado.codigo == "adapter_soft_blocked" || resultado.codigo == "adapter_hard_blocked") {
        return "Bluetooth bloqueado no sistema.";
    }
    if (resultado.codigo == "device_unavailable") return "Dispositivo Bluetooth indisponivel.";
    if (resultado.codigo == "device_not_connected") return "Dispositivo nao esta conectado.";
    return resultado.mensagem.empty() ? fallback : resultado.mensagem;
}

void logErroBluetooth(const std::string& operacao, const system_result& resultado) {
    if (resultado.ok) return;
    std::cerr << "[BLUETOOTH][" << operacao << "] codigo=" << resultado.codigo
              << " mensagem=" << resultado.mensagem
              << " detalhes=" << resultado.detalhes << std::endl;
}

} // namespace

JanelaBluetooth::JanelaBluetooth() {
    sincronizarComHardware();
    inicializarBotoes();
}

JanelaBluetooth::~JanelaBluetooth() {
    encerrando = true;
    if (scanThread.joinable()) {
        scanThread.join();
    }
    if (workerThread.joinable()) {
        workerThread.join();
    }
    texturaExplicacao = nullptr;
}

void JanelaBluetooth::sincronizarComHardware() {
    bluetooth_ui_snapshot snapshot = ::obter_estado_bluetooth_ui();
    bluetoothAtivo = snapshot.adapter.powered;
    adaptadorDisponivel = snapshot.adapter.controller_disponivel;
    adaptadorBloqueado = snapshot.adapter.soft_blocked || snapshot.adapter.hard_blocked;

    std::vector<DispositivoBluetooth> desconhecidos;
    std::vector<DispositivoBluetooth> pareados;
    std::vector<DispositivoBluetooth> escaneados;

    if (bluetoothAtivo && adaptadorDisponivel) {
        for (const auto& d : snapshot.desconhecidos_conectados) {
            desconhecidos.push_back(paraDispositivoUi(d));
        }
        for (const auto& d : snapshot.pareados) {
            pareados.push_back(paraDispositivoUi(d));
        }
        for (const auto& d : snapshot.escaneados_filtrados) {
            escaneados.push_back(paraDispositivoUi(d));
        }
    }

    std::stable_sort(desconhecidos.begin(), desconhecidos.end(), ordenarPorNome);
    std::stable_sort(pareados.begin(), pareados.end(), [](const DispositivoBluetooth& a, const DispositivoBluetooth& b) {
        if (a.conectado != b.conectado) return a.conectado > b.conectado;
        return ordenarPorNome(a, b);
    });

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        dispositivosDesconhecidosConectados = std::move(desconhecidos);
        dispositivosPareados = std::move(pareados);
        dispositivosEscaneados = std::move(escaneados);
    }

    if (snapshot.vindo_do_cache && !snapshot.erro_codigo.empty()) {
        std::cerr << "[BLUETOOTH][snapshot_cache] codigo=" << snapshot.erro_codigo
                  << " mensagem=" << snapshot.erro_mensagem
                  << " detalhes=" << snapshot.erro_detalhes << std::endl;
    }

    ajustarFocoAposMudancaListas();
}

void JanelaBluetooth::inicializarBotoes() {
    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color btnNormal = tema.getCorBotaoNormal();
    SDL_Color btnHover = tema.getCorBotaoHover();
    SDL_Color btnPress = tema.getCorBotaoPressionado();

    int espacamento = ConfigLayout::F(30);
    int btnToggleW = ConfigLayout::F(380);
    int btnScanW = ConfigLayout::F(320);
    int btnH = ConfigLayout::F(64);
    int btnY = ConfigLayout::F(165);
    int larguraTotal = btnToggleW + espacamento + btnScanW;
    int startX = (ConfigLayout::F(1525) - larguraTotal) / 2;

    btnToggleBluetooth = std::make_unique<Botao>(
        startX, btnY, btnToggleW, btnH,
        bluetoothAtivo ? "Bluetooth: ON" : "Bluetooth: OFF"
    );
    btnToggleBluetooth->setCor(btnNormal, btnHover, btnPress);
    btnToggleBluetooth->setRetanguloBordasArredondadas(12);

    btnEscanear = std::make_unique<Botao>(
        startX + btnToggleW + espacamento, btnY, btnScanW, btnH,
        "Escanear (Y)"
    );
    btnEscanear->setCor(btnNormal, btnHover, btnPress);
    btnEscanear->setRetanguloBordasArredondadas(12);
}

void JanelaBluetooth::inicializarDispositivosPareados() {}

void JanelaBluetooth::inicializarDispositivosDisponiveis() {}

void JanelaBluetooth::carregarTexturas(SDL_Renderer* renderer) {
    if (!renderer) return;

    auto& tema = GerenciadorTemas::getInstance();
    TipoTema temaAtual = tema.getTemaAtual();

    static TipoTema ultimoTema = static_cast<TipoTema>(-1);
    if (temaAtual == ultimoTema && texturaExplicacao != nullptr) return;

    std::string caminhoExplicacao = (temaAtual == TipoTema::CLARO)
        ? "assets/images/light/explicacaoBotoesJanelaBluetoothClaro.jpg"
        : "assets/images/dark/explicacaoBotoesJanelaBluetoothEscuro.jpg";

    texturaExplicacao = gerImg.carregar(renderer, caminhoExplicacao);
    ultimoTema = temaAtual;
}

void JanelaBluetooth::desenhar(SDL_Renderer* renderer) {
    if (!renderer) return;

    carregarTexturas(renderer);
    auto& tema = GerenciadorTemas::getInstance();

    desenharTexto(renderer, "Configuração Bluetooth",
                  ConfigLayout::X(462), ConfigLayout::Y(78),
                  tema.getCorTextoNegrito(), ConfigLayout::F(48));

    desenharStatusOperacional(renderer);
    desenharToggleBluetooth(renderer);

    if (!adaptadorDisponivel) {
        desenharPainelVazio(renderer, ConfigLayout::X(250), ConfigLayout::Y(360),
                            ConfigLayout::X(1000), "Nenhum adaptador Bluetooth detectado no sistema.");
    } else if (bluetoothAtivo) {
        desenharBotaoEscanear(renderer);
        desenharListaDispositivos(renderer);
    } else {
        desenharPainelVazio(renderer, ConfigLayout::X(250), ConfigLayout::Y(390),
                            ConfigLayout::X(1000), "Bluetooth desativado. Ative para ver dispositivos.");
    }

    desenharImagemExplicativa(renderer);
}

void JanelaBluetooth::desenharToggleBluetooth(SDL_Renderer* renderer) {
    if (!btnToggleBluetooth) return;

    btnToggleBluetooth->setTexto(bluetoothAtivo ? "Bluetooth: ON" : "Bluetooth: OFF");
    btnToggleBluetooth->setFocado(indiceFocado == -1);
    btnToggleBluetooth->desenhar(renderer);
}

void JanelaBluetooth::desenharBotaoEscanear(SDL_Renderer* renderer) {
    if (!btnEscanear || !bluetoothAtivo) return;

    if (escaneando) {
        btnEscanear->setTexto("Escaneando...");
    } else if (operacaoEmAndamento) {
        btnEscanear->setTexto("Ocupado...");
    } else {
        btnEscanear->setTexto("Escanear (Y)");
    }

    btnEscanear->desenhar(renderer);
}

void JanelaBluetooth::desenharListaDispositivos(SDL_Renderer* renderer) {
    std::vector<DispositivoBluetooth> desconhecidos;
    std::vector<DispositivoBluetooth> pareados;
    std::vector<DispositivoBluetooth> escaneados;

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        desconhecidos = dispositivosDesconhecidosConectados;
        pareados = dispositivosPareados;
        escaneados = dispositivosEscaneados;
        itensFocaveis.clear();
    }

    SDL_Rect clip{
        ConfigLayout::X(LISTA_X - 20),
        ConfigLayout::Y(LISTA_Y - 10),
        ConfigLayout::X(LISTA_W + 40),
        ConfigLayout::Y(LISTA_H)
    };
    SDL_RenderSetClipRect(renderer, &clip);

    int yAtual = ConfigLayout::Y(LISTA_Y) - scrollY;
    yAtual = desenharSecao(renderer, "Desconhecidos Conectados", SecaoBluetooth::DESCONHECIDO_CONECTADO,
                           desconhecidos, yAtual, "Nenhum dispositivo conectado sem pareamento.");
    yAtual = desenharSecao(renderer, "Pareados", SecaoBluetooth::PAREADO,
                           pareados, yAtual, "Nenhum dispositivo pareado salvo.");
    yAtual = desenharSecao(renderer, escaneando ? "Escaneados (buscando...)" : "Escaneados",
                           SecaoBluetooth::ESCANEADO, escaneados, yAtual,
                           escaneando ? "Buscando dispositivos proximos..." : "Nenhum dispositivo novo escaneado.");

    SDL_RenderSetClipRect(renderer, nullptr);

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        alturaConteudoLista = (yAtual + scrollY) - ConfigLayout::Y(LISTA_Y);
        int maxScroll = std::max(0, alturaConteudoLista - ConfigLayout::Y(LISTA_H - 20));
        if (scrollY > maxScroll) scrollY = maxScroll;
    }

    auto& tema = GerenciadorTemas::getInstance();
    if (scrollY > 0) {
        desenharTexto(renderer, "▲",
                      ConfigLayout::X(LISTA_X + LISTA_W - 25), ConfigLayout::Y(LISTA_Y - 22),
                      tema.getCorTextoNormal(), ConfigLayout::F(18));
    }

    int maxScroll = std::max(0, alturaConteudoLista - ConfigLayout::Y(LISTA_H - 20));
    if (scrollY < maxScroll) {
        desenharTexto(renderer, "▼",
                      ConfigLayout::X(LISTA_X + LISTA_W - 25), ConfigLayout::Y(LISTA_Y + LISTA_H - 20),
                      tema.getCorTextoNormal(), ConfigLayout::F(18));
    }
}

int JanelaBluetooth::desenharSecao(
    SDL_Renderer* renderer,
    const std::string& titulo,
    SecaoBluetooth secao,
    const std::vector<DispositivoBluetooth>& dispositivos,
    int yAtual,
    const std::string& mensagemVazia
) {
    auto& tema = GerenciadorTemas::getInstance();
    int x = ConfigLayout::X(LISTA_X);
    int w = ConfigLayout::X(LISTA_W);
    int limiteSuperior = ConfigLayout::Y(LISTA_Y - 30);
    int limiteInferior = ConfigLayout::Y(LISTA_Y + LISTA_H);

    if (yAtual + ConfigLayout::Y(HEADER_H) >= limiteSuperior && yAtual <= limiteInferior) {
        std::string contador = " (" + std::to_string(dispositivos.size()) + ")";
        desenharTexto(renderer, titulo + contador, x, yAtual,
                      tema.getCorTextoNegrito(), ConfigLayout::F(28), TipoFonte::NEGRITO);
    }
    yAtual += ConfigLayout::Y(HEADER_H);

    if (dispositivos.empty()) {
        if (yAtual + ConfigLayout::Y(EMPTY_H) >= limiteSuperior && yAtual <= limiteInferior) {
            desenharPainelVazio(renderer, x + ConfigLayout::X(30), yAtual, w - ConfigLayout::X(60), mensagemVazia);
        }
        yAtual += ConfigLayout::Y(EMPTY_H + SECAO_GAP);
        return yAtual;
    }

    for (size_t i = 0; i < dispositivos.size(); ++i) {
        int cardY = yAtual;
        SDL_Rect area{x + ConfigLayout::X(30), cardY, w - ConfigLayout::X(60), ConfigLayout::Y(CARD_H)};

        {
            std::lock_guard<std::mutex> lock(mtx_dispositivos);
            itensFocaveis.push_back(ItemBluetoothFocavel{secao, static_cast<int>(i), area});
        }

        bool focado = indiceFocado == static_cast<int>(itensFocaveis.size()) - 1;
        if (cardY + ConfigLayout::Y(CARD_H) >= limiteSuperior && cardY <= limiteInferior) {
            desenharDispositivo(renderer, dispositivos[i], area.x, area.y, focado, secao);
        }

        yAtual += ConfigLayout::Y(CARD_H + CARD_GAP);
    }

    yAtual += ConfigLayout::Y(SECAO_GAP);
    return yAtual;
}

void JanelaBluetooth::desenharDispositivo(
    SDL_Renderer* renderer,
    const DispositivoBluetooth& dispositivo,
    int x,
    int y,
    bool focado,
    SecaoBluetooth secao
) {
    auto& tema = GerenciadorTemas::getInstance();

    int cardW = ConfigLayout::X(LISTA_W - 60);
    int cardH = ConfigLayout::Y(CARD_H);
    int raio = ConfigLayout::F(10);

    SDL_Color corBase = tema.getCorRetangulos();
    SDL_Color corBorda = focado ? tema.getCorDestaque() : comAlpha(tema.getCorTextoNormal(), 80);
    if (dispositivo.conectado && dispositivo.pareado) {
        corBase = SDL_Color{28, 120, 82, 255};
        corBorda = SDL_Color{85, 255, 170, 255};
    } else if (secao == SecaoBluetooth::DESCONHECIDO_CONECTADO) {
        corBase = SDL_Color{120, 85, 30, 255};
        corBorda = SDL_Color{255, 190, 75, 255};
    }

    roundedBoxRGBA(renderer, x, y, x + cardW, y + cardH, raio,
                   corBase.r, corBase.g, corBase.b, 220);
    roundedRectangleRGBA(renderer, x, y, x + cardW, y + cardH, raio,
                         corBorda.r, corBorda.g, corBorda.b, 255);

    if (focado) {
        SDL_Color foco = tema.getCorDestaque();
        roundedRectangleRGBA(renderer, x - ConfigLayout::X(4), y - ConfigLayout::Y(4),
                             x + cardW + ConfigLayout::X(4), y + cardH + ConfigLayout::Y(4),
                             raio, foco.r, foco.g, foco.b, 255);
    }

    std::string nome = limitarTexto(labelTipo(dispositivo.tipo) + " " +
                                    (dispositivo.nome.empty() ? dispositivo.endereco : dispositivo.nome), 34);
    std::string mac = limitarTexto(dispositivo.endereco, 20);

    desenharTexto(renderer, nome, x + ConfigLayout::X(22), y + ConfigLayout::Y(10),
                  tema.getCorTextoNegrito(), ConfigLayout::F(24), TipoFonte::NEGRITO);
    desenharTexto(renderer, mac, x + ConfigLayout::X(430), y + ConfigLayout::Y(13),
                  tema.getCorTextoNormal(), ConfigLayout::F(17));

    std::string status;
    SDL_Color corStatus = tema.getCorTextoNormal();
    if (dispositivo.conectado && dispositivo.pareado) {
        status = "● Conectado";
        corStatus = SDL_Color{120, 255, 170, 255};
    } else if (secao == SecaoBluetooth::DESCONHECIDO_CONECTADO) {
        status = "● Conectado sem pareamento";
        corStatus = SDL_Color{255, 215, 125, 255};
    } else if (dispositivo.pareado) {
        status = "○ Pareado";
    } else {
        status = "○ Novo";
    }

    desenharTexto(renderer, status, x + ConfigLayout::X(22), y + ConfigLayout::Y(45),
                  corStatus, ConfigLayout::F(18), TipoFonte::NEGRITO);

    std::string acao;
    if (secao == SecaoBluetooth::PAREADO) {
        acao = dispositivo.conectado ? "A/Clique: desconectar   X/Direito: desconectar"
                                     : "A/Clique: conectar      X/Direito: sem acao";
    } else if (secao == SecaoBluetooth::ESCANEADO) {
        acao = "A/Clique: parear e conectar";
    } else {
        acao = "A/Clique: parear   X/Direito: desconectar";
    }

    desenharTexto(renderer, acao, x + ConfigLayout::X(520), y + ConfigLayout::Y(47),
                  tema.getCorTextoNormal(), ConfigLayout::F(16));
}

void JanelaBluetooth::desenharMensagemEscaneamento(SDL_Renderer* renderer) {
    (void)renderer;
}

void JanelaBluetooth::desenharImagemExplicativa(SDL_Renderer* renderer) {
    if (!texturaExplicacao) return;

    int larguraTela = ConfigLayout::F(1525);
    int alturaImagem = ConfigLayout::F(30);
    int posY = ConfigLayout::F(1080) - alturaImagem;
    SDL_Rect destExplicacao = {0, posY, larguraTela, alturaImagem};
    SDL_RenderCopy(renderer, texturaExplicacao, nullptr, &destExplicacao);
}

void JanelaBluetooth::desenharStatusOperacional(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    std::string mensagem;
    bool erro = false;
    int qtdDesconhecidos = 0;
    int qtdPareados = 0;
    int qtdConectadosPareados = 0;
    int qtdEscaneados = 0;

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        mensagem = mensagemStatus;
        erro = mensagemErro;
        qtdDesconhecidos = static_cast<int>(dispositivosDesconhecidosConectados.size());
        qtdPareados = static_cast<int>(dispositivosPareados.size());
        qtdEscaneados = static_cast<int>(dispositivosEscaneados.size());
        for (const auto& d : dispositivosPareados) {
            if (d.conectado) ++qtdConectadosPareados;
        }
    }

    std::string resumo;
    if (!adaptadorDisponivel) {
        resumo = "Adaptador: indisponivel";
    } else if (adaptadorBloqueado && !bluetoothAtivo) {
        resumo = "Adaptador: bloqueado";
    } else {
        resumo = bluetoothAtivo ? "Adaptador: ativo" : "Adaptador: desligado";
    }

    if (operacaoEmAndamento || escaneando || alternandoBluetooth) {
        resumo += " | ocupando";
    } else if (bluetoothAtivo && adaptadorDisponivel) {
        resumo += " | conectados: " + std::to_string(qtdConectadosPareados + qtdDesconhecidos);
        resumo += " | pareados: " + std::to_string(qtdPareados);
        resumo += " | novos: " + std::to_string(qtdEscaneados);
    }

    desenharTexto(renderer, resumo, ConfigLayout::X(330), ConfigLayout::Y(132),
                  tema.getCorTextoNormal(), ConfigLayout::F(22));

    if (!mensagem.empty()) {
        SDL_Color cor = erro ? SDL_Color{255, 120, 120, 255}
                             : SDL_Color{120, 255, 150, 255};
        desenharTexto(renderer, mensagem, ConfigLayout::X(250), ConfigLayout::Y(245),
                      cor, ConfigLayout::F(20));
    }
}

void JanelaBluetooth::ajustarFocoAposMudancaListas() {
    std::lock_guard<std::mutex> lock(mtx_dispositivos);
    int total = static_cast<int>(
        dispositivosDesconhecidosConectados.size() +
        dispositivosPareados.size() +
        dispositivosEscaneados.size()
    );

    if (!bluetoothAtivo || total == 0) {
        indiceFocado = -1;
    } else if (indiceFocado >= total) {
        indiceFocado = total - 1;
    }

    if (scrollY < 0) scrollY = 0;
    int maxScroll = std::max(0, alturaConteudoLista - ConfigLayout::Y(LISTA_H - 20));
    if (scrollY > maxScroll) scrollY = maxScroll;
}

void JanelaBluetooth::ajustarScrollAoFoco() {
    std::lock_guard<std::mutex> lock(mtx_dispositivos);
    if (indiceFocado < 0 || indiceFocado >= static_cast<int>(itensFocaveis.size())) {
        return;
    }

    SDL_Rect area = itensFocaveis[indiceFocado].area;
    int topoVisivel = ConfigLayout::Y(LISTA_Y);
    int baseVisivel = ConfigLayout::Y(LISTA_Y + LISTA_H - 40);

    if (area.y < topoVisivel) {
        scrollY = std::max(0, scrollY - (topoVisivel - area.y) - ConfigLayout::Y(20));
    } else if (area.y + area.h > baseVisivel) {
        scrollY += (area.y + area.h - baseVisivel) + ConfigLayout::Y(20);
    }

    int maxScroll = std::max(0, alturaConteudoLista - ConfigLayout::Y(LISTA_H - 20));
    if (scrollY > maxScroll) scrollY = maxScroll;
}

void JanelaBluetooth::filtrarEscaneadosBloqueado() {
    std::unordered_set<std::string> macsBloqueados;
    for (const auto& d : dispositivosDesconhecidosConectados) {
        macsBloqueados.insert(normalizarMac(d.endereco));
    }
    for (const auto& d : dispositivosPareados) {
        macsBloqueados.insert(normalizarMac(d.endereco));
    }

    dispositivosEscaneados.erase(
        std::remove_if(dispositivosEscaneados.begin(), dispositivosEscaneados.end(),
                       [&macsBloqueados](const DispositivoBluetooth& d) {
                           return macsBloqueados.find(normalizarMac(d.endereco)) != macsBloqueados.end();
                       }),
        dispositivosEscaneados.end()
    );
}

void JanelaBluetooth::desenharPainelVazio(SDL_Renderer* renderer, int x, int y, int w, const std::string& mensagem) {
    auto& tema = GerenciadorTemas::getInstance();
    SDL_Color fundo = tema.getCorRetangulos();
    SDL_Color borda = comAlpha(tema.getCorTextoNormal(), 80);
    int h = ConfigLayout::Y(EMPTY_H);
    int raio = ConfigLayout::F(10);

    roundedBoxRGBA(renderer, x, y, x + w, y + h, raio, fundo.r, fundo.g, fundo.b, 145);
    roundedRectangleRGBA(renderer, x, y, x + w, y + h, raio, borda.r, borda.g, borda.b, 180);
    desenharTexto(renderer, mensagem, x + ConfigLayout::X(22), y + ConfigLayout::Y(15),
                  tema.getCorTextoNormal(), ConfigLayout::F(20));
}

bool JanelaBluetooth::localizarDispositivoPorPonto(int x, int y, SecaoBluetooth& secao, int& indiceLocal) {
    std::lock_guard<std::mutex> lock(mtx_dispositivos);
    for (size_t i = 0; i < itensFocaveis.size(); ++i) {
        const auto& item = itensFocaveis[i];
        const SDL_Rect& area = item.area;
        if (x >= area.x && x <= area.x + area.w && y >= area.y && y <= area.y + area.h) {
            secao = item.secao;
            indiceLocal = item.indice;
            indiceFocado = static_cast<int>(i);
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
        try {
            tarefa();
        } catch (const std::exception& e) {
            definirMensagemStatus(std::string("Operacao Bluetooth falhou: ") + e.what(), true);
        } catch (...) {
            definirMensagemStatus("Operacao Bluetooth falhou inesperadamente.", true);
        }
        alternandoBluetooth = false;
        operacaoEmAndamento = false;
    });
    return true;
}

bool JanelaBluetooth::obterItemFocado(SecaoBluetooth& secao, int& indiceLocal) {
    std::lock_guard<std::mutex> lock(mtx_dispositivos);
    if (indiceFocado < 0 || indiceFocado >= static_cast<int>(itensFocaveis.size())) {
        return false;
    }
    secao = itensFocaveis[indiceFocado].secao;
    indiceLocal = itensFocaveis[indiceFocado].indice;
    return true;
}

void JanelaBluetooth::acionarItem(SecaoBluetooth secao, int indiceLocal, bool acaoSecundaria) {
    if (secao == SecaoBluetooth::PAREADO) {
        if (acaoSecundaria) {
            std::string endereco;
            bool conectado = false;
            {
                std::lock_guard<std::mutex> lock(mtx_dispositivos);
                if (indiceLocal < 0 || indiceLocal >= static_cast<int>(dispositivosPareados.size())) return;
                endereco = dispositivosPareados[indiceLocal].endereco;
                conectado = dispositivosPareados[indiceLocal].conectado;
            }
            if (!conectado) {
                definirMensagemStatus("Dispositivo ja esta desconectado.");
                return;
            }
            definirMensagemStatus("Desconectando dispositivo...");
            iniciarTarefaEmSegundoPlano([this, endereco]() {
                system_result resultado = ::desconectar_bluetooth_result(endereco);
                logErroBluetooth("desconectar_pareado_secundario", resultado);
                sincronizarComHardware();
                definirMensagemStatus(
                    resultado.ok ? "Dispositivo desconectado."
                                 : mensagemAmigavelBluetooth(resultado, "Falha ao desconectar dispositivo."),
                    !resultado.ok
                );
            });
        } else {
            toggleConexaoDispositivo(indiceLocal);
        }
        return;
    }

    if (secao == SecaoBluetooth::ESCANEADO && !acaoSecundaria) {
        parearDispositivo(indiceLocal);
        return;
    }

    if (secao == SecaoBluetooth::DESCONHECIDO_CONECTADO) {
        if (acaoSecundaria) {
            desconectarDispositivoDesconhecido(indiceLocal);
        } else {
            parearDispositivoDesconhecido(indiceLocal);
        }
    }
}

void JanelaBluetooth::toggleBluetooth() {
    if (alternandoBluetooth.exchange(true)) return;

    bool estadoDesejado = !bluetoothAtivo.load();
    definirMensagemStatus(estadoDesejado ? "Ativando Bluetooth..." : "Desativando Bluetooth...");

    if (!iniciarTarefaEmSegundoPlano([this, estadoDesejado]() {
        system_result resultado = ::definir_estado_bt_result(estadoDesejado);
        sincronizarComHardware();

        if (!bluetoothAtivo) {
            std::lock_guard<std::mutex> lock(mtx_dispositivos);
            indiceFocado = -1;
            scrollY = 0;
            dispositivosEscaneados.clear();
        }

        definirMensagemStatus(
            resultado.ok
                ? (estadoDesejado ? "Bluetooth ativado." : "Bluetooth desativado.")
                : (resultado.mensagem.empty() ? "Falha ao alterar o estado do Bluetooth." : resultado.mensagem),
            !resultado.ok
        );
        alternandoBluetooth = false;
    })) {
        alternandoBluetooth = false;
    }
}

void JanelaBluetooth::iniciarEscaneamento() {
    if (!bluetoothAtivo || escaneando) return;
    if (operacaoEmAndamento.exchange(true)) {
        definirMensagemStatus("Ha uma operacao Bluetooth em andamento.", true);
        return;
    }

    if (scanThread.joinable()) {
        scanThread.join();
    }

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        escaneando = true;
        mensagemStatus = "Escaneando dispositivos proximos...";
        mensagemErro = false;
        dispositivosEscaneados.clear();
    }
    tempoInicioEscanear = SDL_GetTicks();

    scanThread = std::thread([this]() {
        bluetooth_ui_snapshot snapshot;
        try {
            snapshot = ::obter_estado_bluetooth_ui_com_scan(5);
        } catch (const std::exception& e) {
            definirMensagemStatus(std::string("Scan Bluetooth falhou: ") + e.what(), true);
            escaneando = false;
            operacaoEmAndamento = false;
            return;
        } catch (...) {
            definirMensagemStatus("Scan Bluetooth falhou inesperadamente.", true);
            escaneando = false;
            operacaoEmAndamento = false;
            return;
        }

        if (encerrando) {
            escaneando = false;
            operacaoEmAndamento = false;
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mtx_dispositivos);
            dispositivosDesconhecidosConectados.clear();
            dispositivosPareados.clear();
            dispositivosEscaneados.clear();
            for (const auto& d : snapshot.desconhecidos_conectados) {
                dispositivosDesconhecidosConectados.push_back(paraDispositivoUi(d));
            }
            for (const auto& d : snapshot.pareados) {
                dispositivosPareados.push_back(paraDispositivoUi(d));
            }
            for (const auto& d : snapshot.escaneados_filtrados) {
                dispositivosEscaneados.push_back(paraDispositivoUi(d));
            }
            escaneando = false;
        }

        ajustarFocoAposMudancaListas();
        definirMensagemStatus(
            snapshot.escaneados_filtrados.empty() ? "Scan concluido. Nenhum dispositivo novo encontrado."
                                                  : "Scan concluido. Dispositivos novos atualizados."
        );
        operacaoEmAndamento = false;
    });
}

void JanelaBluetooth::toggleConexaoDispositivo(int indice) {
    std::string endereco;
    bool conectado = false;

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        if (indice < 0 || indice >= static_cast<int>(dispositivosPareados.size())) return;
        endereco = dispositivosPareados[indice].endereco;
        conectado = dispositivosPareados[indice].conectado;
    }

    definirMensagemStatus(conectado ? "Desconectando dispositivo..." : "Conectando dispositivo...");
    iniciarTarefaEmSegundoPlano([this, endereco, conectado]() {
        system_result resultado = conectado ? ::desconectar_bluetooth_result(endereco)
                                               : ::conectar_bluetooth_result(endereco);
        logErroBluetooth(conectado ? "desconectar_pareado" : "conectar_pareado", resultado);
        sincronizarComHardware();
        definirMensagemStatus(
            resultado.ok
                ? (conectado ? "Dispositivo desconectado." : "Dispositivo conectado.")
                : mensagemAmigavelBluetooth(resultado, conectado ? "Falha ao desconectar dispositivo." : "Falha ao conectar dispositivo."),
            !resultado.ok
        );
    });
}

void JanelaBluetooth::parearDispositivoDesconhecido(int indice) {
    DispositivoBluetooth dispositivo("", TipoDispositivoBT::OUTRO);

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        if (indice < 0 || indice >= static_cast<int>(dispositivosDesconhecidosConectados.size())) return;
        dispositivo = dispositivosDesconhecidosConectados[indice];
    }

    definirMensagemStatus("Pareando dispositivo conectado...");
    iniciarTarefaEmSegundoPlano([this, dispositivo]() {
        system_result resultado = ::parear_confiar_conectar_bluetooth(dispositivo.endereco);
        logErroBluetooth("parear_confiar_conectar_desconhecido", resultado);
        sincronizarComHardware();
        definirMensagemStatus(
            resultado.ok ? "Dispositivo pareado e conectado."
                         : mensagemAmigavelBluetooth(resultado, "Falha ao parear dispositivo conectado."),
            !resultado.ok
        );
    });
}

void JanelaBluetooth::desconectarDispositivoDesconhecido(int indice) {
    std::string endereco;

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        if (indice < 0 || indice >= static_cast<int>(dispositivosDesconhecidosConectados.size())) return;
        endereco = dispositivosDesconhecidosConectados[indice].endereco;
    }

    definirMensagemStatus("Desconectando dispositivo...");
    iniciarTarefaEmSegundoPlano([this, endereco]() {
        system_result resultado = ::desconectar_bluetooth_result(endereco);
        logErroBluetooth("desconectar_desconhecido_conectado", resultado);
        sincronizarComHardware();
        definirMensagemStatus(
            resultado.ok ? "Dispositivo desconectado."
                         : mensagemAmigavelBluetooth(resultado, "Falha ao desconectar dispositivo."),
            !resultado.ok
        );
    });
}

void JanelaBluetooth::parearDispositivo(int indice) {
    DispositivoBluetooth dispositivo("", TipoDispositivoBT::OUTRO);

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        if (indice < 0 || indice >= static_cast<int>(dispositivosEscaneados.size())) return;
        dispositivo = dispositivosEscaneados[indice];
    }

    definirMensagemStatus("Pareando e conectando dispositivo...");
    iniciarTarefaEmSegundoPlano([this, dispositivo]() {
        system_result resultado = ::parear_confiar_conectar_bluetooth(dispositivo.endereco);
        logErroBluetooth("parear_confiar_conectar_escaneado", resultado);
        sincronizarComHardware();
        definirMensagemStatus(
            resultado.ok ? "Dispositivo pareado e conectado."
                         : mensagemAmigavelBluetooth(resultado, "Falha ao parear e conectar dispositivo."),
            !resultado.ok
        );
    });
}

void JanelaBluetooth::navegarParaCima() {
    if (indiceFocado > -1) {
        --indiceFocado;
        ajustarScrollAoFoco();
        gerAudio.tocarSom("navegacao.wav");
    }
}

void JanelaBluetooth::navegarParaBaixo() {
    int total = calcularTotalElementosFocaveis();
    if (indiceFocado < total - 1) {
        ++indiceFocado;
        ajustarScrollAoFoco();
        gerAudio.tocarSom("navegacao.wav");
    }
}

void JanelaBluetooth::confirmarSelecao() {
    if (!bluetoothAtivo || indiceFocado == -1) {
        toggleBluetooth();
        gerAudio.tocarSom("select.wav");
        return;
    }

    SecaoBluetooth secao;
    int indiceLocal = -1;
    if (obterItemFocado(secao, indiceLocal)) {
        acionarItem(secao, indiceLocal, false);
        gerAudio.tocarSom("select.wav");
    }
}

void JanelaBluetooth::atualizarScroll() {
    ajustarScrollAoFoco();
}

int JanelaBluetooth::calcularTotalElementosFocaveis() {
    std::lock_guard<std::mutex> lock(mtx_dispositivos);
    return static_cast<int>(
        dispositivosDesconhecidosConectados.size() +
        dispositivosPareados.size() +
        dispositivosEscaneados.size()
    );
}

bool JanelaBluetooth::processarEvento(SDL_Event& evento) {
    Uint32 tempoAtual = SDL_GetTicks();

    if (evento.type == SDL_MOUSEMOTION) {
        if (btnToggleBluetooth) {
            btnToggleBluetooth->handleMouseMotion(evento, 0, 0);
        }
        if (bluetoothAtivo && btnEscanear) {
            btnEscanear->handleMouseMotion(evento, 0, 0);
        }

        SecaoBluetooth secao;
        int indice = -1;
        if (bluetoothAtivo && localizarDispositivoPorPonto(evento.motion.x, evento.motion.y, secao, indice)) {
            return true;
        }
    }

    if (evento.type == SDL_MOUSEBUTTONDOWN) {
        int mx = evento.button.x;
        int my = evento.button.y;
        if ((btnToggleBluetooth && btnToggleBluetooth->contemPonto(mx, my)) ||
            (bluetoothAtivo && btnEscanear && btnEscanear->contemPonto(mx, my))) {
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
            iniciarEscaneamento();
            gerAudio.tocarSom("select.wav");
            return true;
        }

        if (bluetoothAtivo) {
            SecaoBluetooth secao;
            int indice = -1;
            if (localizarDispositivoPorPonto(mx, my, secao, indice)) {
                bool acaoSecundaria = evento.button.button == SDL_BUTTON_RIGHT;
                acionarItem(secao, indice, acaoSecundaria);
                gerAudio.tocarSom("select.wav");
                return true;
            }
        }
    }

    if (evento.type == SDL_MOUSEWHEEL && bluetoothAtivo) {
        scrollY -= evento.wheel.y * ConfigLayout::Y(55);
        if (scrollY < 0) scrollY = 0;
        int maxScroll = std::max(0, alturaConteudoLista - ConfigLayout::Y(LISTA_H - 20));
        if (scrollY > maxScroll) scrollY = maxScroll;
        return true;
    }

    if (evento.type == SDL_KEYDOWN) {
        switch (evento.key.keysym.sym) {
            case SDLK_UP:
                navegarParaCima();
                return true;
            case SDLK_DOWN:
                navegarParaBaixo();
                return true;
            case SDLK_RETURN:
            case SDLK_SPACE:
                confirmarSelecao();
                return true;
            case SDLK_x: {
                SecaoBluetooth secao;
                int indice = -1;
                if (obterItemFocado(secao, indice)) {
                    acionarItem(secao, indice, true);
                    return true;
                }
                break;
            }
            case SDLK_y:
                iniciarEscaneamento();
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
            case SDL_CONTROLLER_BUTTON_X: {
                SecaoBluetooth secao;
                int indice = -1;
                if (obterItemFocado(secao, indice)) {
                    acionarItem(secao, indice, true);
                    return true;
                }
                break;
            }
            case SDL_CONTROLLER_BUTTON_Y:
                iniciarEscaneamento();
                gerAudio.tocarSom("select.wav");
                return true;
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
            }
            if (evento.caxis.value > DEADZONE) {
                navegarParaBaixo();
                ultimoInputAnalogico = tempoAtual;
                return true;
            }
        }
    }

    return false;
}

void JanelaBluetooth::resetar() {
    encerrando = true;
    if (scanThread.joinable()) {
        scanThread.join();
    }
    if (workerThread.joinable()) {
        workerThread.join();
    }
    encerrando = false;

    {
        std::lock_guard<std::mutex> lock(mtx_dispositivos);
        indiceFocado = -1;
        scrollY = 0;
        alturaConteudoLista = 0;
        dispositivosEscaneados.clear();
        itensFocaveis.clear();
        escaneando = false;
        operacaoEmAndamento = false;
        mensagemStatus.clear();
        mensagemErro = false;
    }

    sincronizarComHardware();
}
