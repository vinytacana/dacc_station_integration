/**
 * @file JanelaBluetooth.cpp
 * @brief Versão Premium: Duas seções, visual polido e máxima estabilidade.
 */

#include "JanelaBluetooth.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include "GerenciadorAudio.hpp"
#include "GerenciadorImagens.hpp"
#include "GraphicsUtils.hpp"
#include "functions.hpp"
#include <iostream>
#include <algorithm>

using namespace MeuProjeto;

extern MeuProjeto::GerenciadorAudio gerAudio;
extern MeuProjeto::GerenciadorImagens gerImg;

JanelaBluetooth::JanelaBluetooth(GerenciadorImagens* gerImgLocal) : gerImgRef(gerImgLocal) {
    auto& tema = GerenciadorTemas::getInstance();
    
    btnPower = std::make_unique<Botao>(
        ConfigLayout::F(250), ConfigLayout::F(180),
        ConfigLayout::F(400), ConfigLayout::F(80),
        "Bluetooth"
    );
    btnPower->setCor(tema.getCorBotaoNormal(), tema.getCorBotaoHover(), tema.getCorBotaoPressionado());
    btnPower->setRetanguloBordasArredondadas(15);

    btnScan = std::make_unique<Botao>(
        ConfigLayout::F(670), ConfigLayout::F(180),
        ConfigLayout::F(400), ConfigLayout::F(80),
        "Escanear Novos"
    );
    btnScan->setCor(tema.getCorBotaoNormal(), tema.getCorBotaoHover(), tema.getCorBotaoPressionado());
    btnScan->setRetanguloBordasArredondadas(15);

    resetar();
}

JanelaBluetooth::~JanelaBluetooth() {

    executandoThread = false; 

    // Garante que a thread de background terminou antes de destruir o objeto

    if (workerThread.joinable()) {

        workerThread.join();

    }

}



void JanelaBluetooth::resetar() {

    focoIdx = -1;

    // Carrega status inicial - Única exceção que usamos thread temporária 

    // pois o objeto está sendo construído ou resetado totalmente.

    std::thread([this]() {

        this->bluetoothAtivo = ::obter_estado_bluetooth();

        this->atualizarListasSync();

    }).detach();

}



void JanelaBluetooth::executarComandoHardware(std::function<void()> func) {

    if (estadoAtual != BTState::IDLE) return;



    // Limpa thread anterior concluída

    if (workerThread.joinable()) {

        workerThread.join();

    }



    estadoAtual = BTState::BUSY;

    workerThread = std::thread([this, func]() {

        executandoThread = true;

        func();

        executandoThread = false;

        estadoAtual = BTState::IDLE;

    });

}



void JanelaBluetooth::atualizarListasSync() {
    if (!bluetoothAtivo) {
        std::lock_guard<std::mutex> lock(mtxDados);
        pareados.clear(); disponiveis.clear();
        return;
    }

    auto listaReal = ::get_list_device();
    std::vector<DispositivoBluetooth> tempP, tempD;

    for (const auto& d : listaReal) {
        TipoDispositivoBT t = TipoDispositivoBT::OUTRO;
        std::string n = d.nome;
        std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        if (n.find("controller") != std::string::npos || n.find("wireless") != std::string::npos) t = TipoDispositivoBT::CONTROLE;
        else if (n.find("audio") != std::string::npos || n.find("headset") != std::string::npos) t = TipoDispositivoBT::HEADPHONE;

        if (d.pareado) tempP.push_back(DispositivoBluetooth(d.nome, t, true, d.conectado, d.mac));
        else tempD.push_back(DispositivoBluetooth(d.nome, t, false, d.conectado, d.mac));
    }

    std::lock_guard<std::mutex> lock(mtxDados);
    pareados = std::move(tempP);
    disponiveis = std::move(tempD);
}

bool JanelaBluetooth::processarEvento(SDL_Event& e) {
    if (estadoAtual != BTState::IDLE) return false;

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x; int my = e.button.y;
        if (btnPower->contemPonto(mx, my)) {
            bool n = !bluetoothAtivo;
            executarComandoHardware([this, n]() {
                this->estadoAtual = n ? BTState::POWERING_ON : BTState::POWERING_OFF;
                ::definir_estado_bt(n);
                SDL_Delay(1000);
                this->bluetoothAtivo = ::obter_estado_bluetooth();
                this->atualizarListasSync();
            });
            return true;
        }
        if (bluetoothAtivo && btnScan->contemPonto(mx, my)) {
            executarComandoHardware([this]() {
                this->estadoAtual = BTState::SCANNING;
                ::scan_dispositivos_bluetooth(6);
                this->atualizarListasSync();
            });
            return true;
        }
    }

    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
            if (focoIdx > 0 && focoIdx < 100) focoIdx--;
            else if (focoIdx >= 100) focoIdx--;
            else if (focoIdx == 0) focoIdx = -2;
            else if (focoIdx == 100) {
                std::lock_guard<std::mutex> lock(mtxDados);
                focoIdx = pareados.empty() ? -2 : pareados.size() - 1;
            }
            else if (focoIdx == -2) focoIdx = -1;
            gerAudio.tocarSom("navegacao.wav");
        }
        else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
            int nP = 0, nD = 0; 
            { std::lock_guard<std::mutex> lock(mtxDados); nP = pareados.size(); nD = disponiveis.size(); }
            
            if (focoIdx == -1) focoIdx = -2;
            else if (focoIdx == -2) {
                if (nP > 0) focoIdx = 0;
                else if (nD > 0) focoIdx = 100;
            }
            else if (focoIdx >= 0 && focoIdx < nP - 1) focoIdx++;
            else if (focoIdx == nP - 1) { if (nD > 0) focoIdx = 100; }
            else if (focoIdx >= 100 && focoIdx < 100 + nD - 1) focoIdx++;
            
            gerAudio.tocarSom("navegacao.wav");
        }
        else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
            if (focoIdx == -1) {
                bool n = !bluetoothAtivo;
                executarComandoHardware([this, n]() {
                    this->estadoAtual = n ? BTState::POWERING_ON : BTState::POWERING_OFF;
                    ::definir_estado_bt(n);
                    SDL_Delay(1000);
                    this->bluetoothAtivo = ::obter_estado_bluetooth();
                    this->atualizarListasSync();
                });
            } else if (focoIdx == -2) {
                executarComandoHardware([this]() {
                    this->estadoAtual = BTState::SCANNING;
                    ::scan_dispositivos_bluetooth(6);
                    this->atualizarListasSync();
                });
            } else {
                std::string mac; bool c;
                {
                    std::lock_guard<std::mutex> lock(mtxDados);
                    if (focoIdx < 100) { mac = pareados[focoIdx].endereco; c = pareados[focoIdx].conectado; }
                    else { mac = disponiveis[focoIdx-100].endereco; c = false; }
                }
                macEmProcesso = mac;
                executarComandoHardware([this, mac, c]() {
                    if (c) ::desconectar_bluetooth(mac); else ::conectar_bluetooth(mac);
                    { std::lock_guard<std::mutex> lock(mtxDados); macEmProcesso = ""; }
                    this->atualizarListasSync();
                });
            }
            gerAudio.tocarSom("select.wav");
        }
        return true;
    }
    return false;
}

void JanelaBluetooth::desenhar(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    desenharTexto(renderer, "Configurações de Bluetooth", ConfigLayout::F(250), ConfigLayout::F(80), tema.getCorTextoNegrito(), ConfigLayout::F(42));

    btnPower->setTexto(bluetoothAtivo ? "Bluetooth: LIGADO" : "Bluetooth: DESLIGADO");
    btnPower->setFocado(focoIdx == -1);
    btnPower->desenhar(renderer);

    btnScan->setFocado(focoIdx == -2);
    if (bluetoothAtivo) btnScan->desenhar(renderer);

    desenharStatusHardware(renderer);

    if (bluetoothAtivo) {
        std::vector<DispositivoBluetooth> cP, cD;
        { std::lock_guard<std::mutex> lock(mtxDados); cP = pareados; cD = disponiveis; }

        // --- SESSÃO 1: PAREADOS ---
        int startX = ConfigLayout::F(250);
        int yP = ConfigLayout::F(350);
        desenharTexto(renderer, "APARELHOS CONHECIDOS", startX, yP - ConfigLayout::F(40), {120, 120, 120, 255}, ConfigLayout::F(18), TipoFonte::NEGRITO);
        
        if (cP.empty()) desenharTexto(renderer, "Nenhum aparelho pareado.", startX, yP, {80, 80, 80, 255}, ConfigLayout::F(20));
        for (size_t i = 0; i < cP.size() && i < 4; i++) {
            int x = startX + (i % 2) * ConfigLayout::F(520);
            int y = yP + (i / 2) * ConfigLayout::F(95);
            desenharCard(renderer, cP[i], x, y, (focoIdx == (int)i));
        }

        // --- SESSÃO 2: DISPONÍVEIS ---
        int yD = ConfigLayout::F(580);
        desenharTexto(renderer, "NOVOS APARELHOS ENCONTRADOS", startX, yD - ConfigLayout::F(40), {120, 120, 120, 255}, ConfigLayout::F(18), TipoFonte::NEGRITO);
        
        if (cD.empty() && estadoAtual != BTState::SCANNING) 
            desenharTexto(renderer, "Clique em 'Escanear' para buscar.", startX, yD, {80, 80, 80, 255}, ConfigLayout::F(20));
        
        for (size_t i = 0; i < cD.size() && i < 6; i++) {
            int x = startX + (i % 2) * ConfigLayout::F(520);
            int y = yD + (i / 2) * ConfigLayout::F(95);
            desenharCard(renderer, cD[i], x, y, (focoIdx == (int)(100 + i)));
        }
    } else {
        desenharTexto(renderer, "Ative o Bluetooth para gerenciar dispositivos.", ConfigLayout::F(250), ConfigLayout::F(450), {100, 100, 100, 255}, ConfigLayout::F(24));
    }

    desenharImagemExplicativa(renderer);
}

void JanelaBluetooth::desenharCard(SDL_Renderer* renderer, const DispositivoBluetooth& d, int x, int y, bool focado) {
    auto& tema = GerenciadorTemas::getInstance();
    SDL_Rect rect = {x, y, ConfigLayout::F(500), ConfigLayout::F(85)};
    bool proc = (macEmProcesso == d.endereco);

    SDL_Color bg = d.conectado ? SDL_Color{30, 70, 30, 255} : (proc ? SDL_Color{70, 70, 20, 255} : tema.getCorRetangulos());
    if (focado) bg = tema.getCorBotaoHover();

    GraphicsUtils::drawRoundedRect(renderer, rect, 15, bg);
    if (focado) GraphicsUtils::drawRoundedRectOutline(renderer, rect, 15, tema.getCorDestaque());
    else GraphicsUtils::drawRoundedRectOutline(renderer, rect, 15, {100, 100, 100, 50});

    std::string pre = (d.tipo == TipoDispositivoBT::CONTROLE) ? "CTR " : (d.tipo == TipoDispositivoBT::HEADPHONE ? "AUD " : "DEV ");
    desenharTexto(renderer, pre + d.nome, x + 20, y + 25, tema.getCorTextoNormal(), ConfigLayout::F(22), TipoFonte::NEGRITO);
    
    if (proc) desenharTexto(renderer, "AGUARDE...", x + rect.w - 120, y + 30, {255, 255, 0, 255}, ConfigLayout::F(16), TipoFonte::NEGRITO);
    else if (d.conectado) desenharTexto(renderer, "CONECTADO", x + rect.w - 130, y + 30, {100, 255, 100, 255}, ConfigLayout::F(16), TipoFonte::NEGRITO);
}

void JanelaBluetooth::desenharStatusHardware(SDL_Renderer* renderer) {
    if (estadoAtual == BTState::IDLE) return;
    std::string msg = (estadoAtual == BTState::SCANNING) ? "Buscando novos aparelhos..." : "Comunicando com o hardware...";
    SDL_Rect barra = {ConfigLayout::F(250), ConfigLayout::F(280), ConfigLayout::F(820), ConfigLayout::F(4)};
    SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
    SDL_RenderFillRect(renderer, &barra);
    desenharTexto(renderer, msg, ConfigLayout::F(250), ConfigLayout::F(295), {0, 200, 255, 255}, ConfigLayout::F(16), TipoFonte::NORMAL);
}

void JanelaBluetooth::desenharImagemExplicativa(SDL_Renderer* renderer) {
    if (!texExplicacao && gerImgRef) {
        auto& tema = GerenciadorTemas::getInstance();
        std::string p = (tema.getTemaAtual() == TipoTema::CLARO) ? "assets/images/light/explicacaoBotoesJanelaBluetoothClaro.jpg" : "assets/images/dark/explicacaoBotoesJanelaBluetoothEscuro.jpg";
        texExplicacao = gerImgRef->carregar(renderer, p);
    }
    if (texExplicacao) {
        SDL_Rect dst = { 0, ConfigLayout::F(1080) - ConfigLayout::F(30), ConfigLayout::F(1525), ConfigLayout::F(30) };
        SDL_RenderCopy(renderer, texExplicacao, nullptr, &dst);
    }
}

void JanelaBluetooth::carregarTexturas(SDL_Renderer* r) { desenharImagemExplicativa(r); }
