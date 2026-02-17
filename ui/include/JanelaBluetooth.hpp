/**
 * @file JanelaBluetooth.hpp
 * @brief Arquitetura Estável com Duas Listas: Pareados e Disponíveis.
 */

#ifndef JANELA_BLUETOOTH_HPP
#define JANELA_BLUETOOTH_HPP

#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include "Botao.hpp"

namespace MeuProjeto {

class GerenciadorImagens;

enum class BTState {
    IDLE,
    BUSY,
    SCANNING,
    POWERING_ON,
    POWERING_OFF
};

enum class TipoDispositivoBT {
    CONTROLE,
    HEADPHONE,
    OUTRO
};

struct DispositivoBluetooth {
    std::string nome;
    TipoDispositivoBT tipo;
    bool pareado;
    bool conectado;
    std::string endereco;
    
    DispositivoBluetooth(const std::string& n, TipoDispositivoBT t, bool p, bool c, const std::string& e)
        : nome(n), tipo(t), pareado(p), conectado(c), endereco(e) {}
};

class JanelaBluetooth {
public:
    JanelaBluetooth(GerenciadorImagens* gerImgLocal);
    ~JanelaBluetooth();

    void desenhar(SDL_Renderer* renderer);
    bool processarEvento(SDL_Event& evento);
    void resetar();
    void carregarTexturas(SDL_Renderer* renderer);

private:
    // Estado e Controle
    std::atomic<BTState> estadoAtual{BTState::IDLE};
    std::atomic<bool> bluetoothAtivo{false};
    std::atomic<bool> executandoThread{false};
    std::string macEmProcesso = "";
    GerenciadorImagens* gerImgRef = nullptr;

    // Dados (Protegidos por Mutex único para evitar Deadlocks)
    std::mutex mtxDados;
    std::vector<DispositivoBluetooth> pareados;
    std::vector<DispositivoBluetooth> disponiveis;
    
    // UI
    std::unique_ptr<Botao> btnPower;
    std::unique_ptr<Botao> btnScan;
    SDL_Texture* texExplicacao = nullptr;
    
    // Threads Gerenciadas
    std::thread workerThread;

    // Foco: -1: Power, -2: Scan, 0-99: Pareados, 100+: Disponíveis
    int focoIdx = -1; 

    // Métodos Internos
    void atualizarListasSync();
    void executarComandoHardware(std::function<void()> func);

    // Renderização
    void desenharCard(SDL_Renderer* renderer, const DispositivoBluetooth& d, int x, int y, bool focado);
    void desenharStatusHardware(SDL_Renderer* renderer);
    void desenharImagemExplicativa(SDL_Renderer* renderer);
};

} // namespace MeuProjeto

#endif
