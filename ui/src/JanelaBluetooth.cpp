/**
 * @file JanelaBluetooth.cpp
 * @brief Implementação da classe JanelaBluetooth segura e funcional.
 */

#include "JanelaBluetooth.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "ConfigLayout.hpp"
#include "GerenciadorAudio.hpp"
#include "GerenciadorImagens.hpp"
#include "functions.hpp"
#include <iostream>
#include <algorithm>

using namespace MeuProjeto;

extern GerenciadorAudio gerAudio;
extern GerenciadorImagens gerImg;

JanelaBluetooth::JanelaBluetooth() {
    inicializarBotoes();
    inicializarDispositivosPareados();
}

JanelaBluetooth::~JanelaBluetooth() {
    // Flag de destruição
    escaneando = false; 
    if (texturaExplicacao) {
        texturaExplicacao = nullptr;
    }
}

void JanelaBluetooth::inicializarBotoes() {
    auto& tema = GerenciadorTemas::getInstance();
    btnToggleBluetooth = std::make_unique<Botao>(
        ConfigLayout::X(513), ConfigLayout::Y(180),
        ConfigLayout::X(500), ConfigLayout::Y(80),
        bluetoothAtivo ? "Bluetooth: ON" : "Bluetooth: OFF"
    );
    btnToggleBluetooth->setCor(tema.getCorBotaoNormal(), tema.getCorBotaoHover(), tema.getCorBotaoPressionado());
}

void JanelaBluetooth::inicializarDispositivosPareados() {
    auto dispositivosReais = ::get_list_device();
    
    std::lock_guard<std::mutex> lock(mutexBT);
    dispositivosPareados.clear();
    for (const auto& d : dispositivosReais) {
        dispositivosPareados.push_back(DispositivoBluetooth(
            d.nome, TipoDispositivoBT::OUTRO, true, false, d.mac
        ));
    }
}

void JanelaBluetooth::toggleBluetooth() {
    bluetoothAtivo = !bluetoothAtivo;
    ::definir_estado_bt(bluetoothAtivo);
    
    if (!bluetoothAtivo) {
        indiceFocado = -1;
        escaneando = false;
    } else {
        inicializarDispositivosPareados();
    }
}

void JanelaBluetooth::iniciarEscaneamento() {
    if (!bluetoothAtivo || escaneando) return;
    
    escaneando = true;
    tempoInicioEscanear = SDL_GetTicks();
    
    std::thread([this]() {
        auto novos = ::scan_dispositivos_bluetooth(10);
        
        // Verifica se a janela ainda existe
        if (!this || !escaneando) return;

        std::lock_guard<std::mutex> lock(mutexBT);
        dispositivosDisponiveis.clear();
        for (const auto& d : novos) {
            dispositivosDisponiveis.push_back(DispositivoBluetooth(
                d.nome, TipoDispositivoBT::OUTRO, false, false, d.mac
            ));
        }
        escaneando = false;
    }).detach();
}

void JanelaBluetooth::toggleConexaoDispositivo(int indice) {
    if (indice < 0 || (size_t)indice >= dispositivosPareados.size()) return;
    
    // Capturamos o MAC, não o índice, para a thread
    std::string mac = dispositivosPareados[indice].endereco;
    bool deveConectar = !dispositivosPareados[indice].conectado;

    std::thread([this, mac, deveConectar]() {
        bool sucesso = false;
        if (deveConectar) sucesso = ::conectar_bluetooth(mac);
        else sucesso = ::desconectar_bluetooth(mac);
        
        if (sucesso && this && bluetoothAtivo) {
            std::lock_guard<std::mutex> lock(mutexBT);
            // Procura o dispositivo pelo MAC na lista atualizada
            for (auto& d : dispositivosPareados) {
                if (d.endereco == mac) {
                    d.conectado = deveConectar;
                    break;
                }
            }
        }
    }).detach();
}

void JanelaBluetooth::parearDispositivo(int indice) {
    if (indice < 0 || (size_t)indice >= dispositivosDisponiveis.size()) return;
    
    std::string mac = dispositivosDisponiveis[indice].endereco;

    std::thread([this, mac]() {
        if (::conectar_bluetooth(mac)) {
            this->inicializarDispositivosPareados();
        }
    }).detach();
}

void JanelaBluetooth::esquecerDispositivo(int indice) {
    if (indice < 0 || (size_t)indice >= dispositivosPareados.size()) return;
    std::string mac = dispositivosPareados[indice].endereco;
    std::thread([this, mac]() {
        std::string cmd = "bluetoothctl remove " + mac;
        system(cmd.c_str());
        this->inicializarDispositivosPareados();
    }).detach();
}

void JanelaBluetooth::desenhar(SDL_Renderer* renderer) {
    if (!renderer) return;
    auto& tema = GerenciadorTemas::getInstance();
    
    desenharTexto(renderer, "Configuração Bluetooth", ConfigLayout::X(462), ConfigLayout::Y(80), tema.getCorTextoNegrito(), ConfigLayout::F(48));
    desenharToggleBluetooth(renderer);
    
    if (bluetoothAtivo) {
        desenharDispositivosPareados(renderer);
        desenharDispositivosDisponiveis(renderer);
        if (escaneando) {
            desenharTexto(renderer, "Escaneando...", ConfigLayout::X(500), ConfigLayout::Y(900), {255,255,255,255}, 28);
        }
    }
}

void JanelaBluetooth::desenharToggleBluetooth(SDL_Renderer* renderer) {
    btnToggleBluetooth->setTexto(bluetoothAtivo ? "Bluetooth: ON" : "Bluetooth: OFF");
    btnToggleBluetooth->setFocado(indiceFocado == -1);
    btnToggleBluetooth->desenhar(renderer);
}

void JanelaBluetooth::desenharDispositivosPareados(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    std::lock_guard<std::mutex> lock(mutexBT);
    int baseY = ConfigLayout::Y(320);
    for (size_t i = 0; i < dispositivosPareados.size() && i < 3; ++i) {
        desenharDispositivo(renderer, dispositivosPareados[i], ConfigLayout::X(250), baseY + i*90, indiceFocado == (int)i);
    }
}

void JanelaBluetooth::desenharDispositivosDisponiveis(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    std::lock_guard<std::mutex> lock(mutexBT);
    int baseY = ConfigLayout::Y(650);
    for (size_t i = 0; i < dispositivosDisponiveis.size() && i < 3; ++i) {
        desenharDispositivo(renderer, dispositivosDisponiveis[i], ConfigLayout::X(250), baseY + i*90, indiceFocado == (int)(dispositivosPareados.size() + i));
    }
}

void JanelaBluetooth::desenharDispositivo(SDL_Renderer* renderer, const DispositivoBluetooth& d, int x, int y, bool f) {
    auto& tema = GerenciadorTemas::getInstance();
    if (f) {
        SDL_Rect r = {x - 10, y - 5, ConfigLayout::X(1000), ConfigLayout::Y(70)};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);
        SDL_RenderFillRect(renderer, &r);
    }
    desenharTexto(renderer, d.nome + (d.conectado ? " [CONECTADO]" : ""), x + 20, y + 20, tema.getCorTextoNormal(), 24);
}

void JanelaBluetooth::navegarParaCima() { if (indiceFocado > -1) indiceFocado--; }
void JanelaBluetooth::navegarParaBaixo() { 
    if (indiceFocado < (int)(dispositivosPareados.size() + dispositivosDisponiveis.size()) - 1) indiceFocado++; 
}

void JanelaBluetooth::confirmarSelecao() {
    if (indiceFocado == -1) toggleBluetooth();
    else if (indiceFocado < (int)dispositivosPareados.size()) toggleConexaoDispositivo(indiceFocado);
    else parearDispositivo(indiceFocado - dispositivosPareados.size());
}

bool JanelaBluetooth::processarEvento(SDL_Event& e) {
    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) navegarParaCima();
        if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) navegarParaBaixo();
        if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A) confirmarSelecao();
        if (e.cbutton.button == SDL_CONTROLLER_BUTTON_Y) iniciarEscaneamento();
        return true;
    }
    return false;
}

void JanelaBluetooth::resetar() { inicializarDispositivosPareados(); }
void JanelaBluetooth::carregarTexturas(SDL_Renderer*) {}
void JanelaBluetooth::atualizarScroll() {}
int JanelaBluetooth::calcularTotalElementosFocaveis() { return 1 + dispositivosPareados.size() + dispositivosDisponiveis.size(); }
