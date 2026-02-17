/**
 * @file JanelaRede.cpp
 * @brief Implementação da interface de configurações de rede funcional e segura.
 */

#include "JanelaRede.hpp"
#include "Utils.hpp"
#include "GerenciadorTemas.hpp"
#include "GerenciadorAudio.hpp"
#include "GerenciadorImagens.hpp"
#include "TecladoVirtual.hpp"
#include "functions.hpp"
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

using namespace MeuProjeto;

extern GerenciadorAudio gerAudio;
extern GerenciadorImagens gerImg;

namespace MeuProjeto { 
class StringInputAdapter : public BotaoPesquisa {
public:
    StringInputAdapter(std::string& str) : BotaoPesquisa(nullptr, 0, 0, 0, 0), texto(str) {}
    void adicionarTexto(const std::string& car) override { texto += car; }
    void limparTexto() override { texto.clear(); }
    void apagarTexto() override { if (!texto.empty()) texto.pop_back(); }
    void resetarSolicitacaoTeclado() override {}
    void resetarFocoResultados() override {}
    void cancelarBusca() override {}
private:
    std::string& texto;
};
}

JanelaRede::JanelaRede(GerenciadorImagens* gerImgLocal) : gerImgRef(gerImgLocal) {
    wifiAtivo = true;
    inicializarRedes();
    inicializarBotoes();
}

JanelaRede::~JanelaRede() {
    // Flag para avisar threads em andamento para não tocarem no 'this'
    buscandoRedes = false; 
    if (texturaExplicacao) {
        texturaExplicacao = nullptr;
    }
}

void JanelaRede::inicializarRedes() {
    if (buscandoRedes) return;
    buscandoRedes = true;
    
    std::thread([this]() {
        auto redesReais = ::listar_wifi_parsed();
        
        // Proteção contra objeto destruído
        if (!this || !buscandoRedes) return;

        std::lock_guard<std::mutex> lock(mutexRedes);
        redesDisponiveis.clear();
        for (const auto& r : redesReais) {
            redesDisponiveis.push_back(RedeInfo(r.ssid, false, r.sinal, r.em_uso));
            if (r.em_uso) redeConectada = r.ssid;
        }
        buscandoRedes = false;
    }).detach();
}

void JanelaRede::inicializarBotoes() {
    auto& tema = GerenciadorTemas::getInstance();
    btnToggleWifi = std::make_unique<Botao>(
        ConfigLayout::F(1100), ConfigLayout::F(280),
        ConfigLayout::F(200), ConfigLayout::F(60),
        wifiAtivo ? "ON" : "OFF"
    );
    btnToggleWifi->setCor(tema.getCorBotaoNormal(), tema.getCorBotaoHover(), tema.getCorBotaoPressionado());
}

void JanelaRede::desenhar(SDL_Renderer* renderer) {
    if (!renderer) return;
    desenharCabecalho(renderer);
    desenharToggleWifi(renderer);
    desenharListaRedes(renderer);
    if (tecladoVisivel) desenharTelasenha(renderer);
}

void JanelaRede::desenharCabecalho(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    desenharTexto(renderer, "Configurações de Rede", ConfigLayout::F(600), ConfigLayout::F(150), tema.getCorTextoNegrito(), ConfigLayout::F(48));
}

void JanelaRede::desenharToggleWifi(SDL_Renderer* renderer) {
    btnToggleWifi->setTexto(wifiAtivo ? "ON" : "OFF");
    btnToggleWifi->desenhar(renderer);
}

void JanelaRede::desenharListaRedes(SDL_Renderer* renderer) {
    auto& tema = GerenciadorTemas::getInstance();
    std::lock_guard<std::mutex> lock(mutexRedes);
    
    if (botoesRedes.size() != redesDisponiveis.size()) {
        botoesRedes.clear();
        for (size_t i = 0; i < redesDisponiveis.size(); i++) {
            auto btn = std::make_unique<Botao>(ConfigLayout::F(250), ConfigLayout::F(450 + i*90), ConfigLayout::F(1000), ConfigLayout::F(70), redesDisponiveis[i].nome);
            btn->setCor(tema.getCorBotaoNormal(), tema.getCorBotaoHover(), tema.getCorBotaoPressionado());
            botoesRedes.push_back(std::move(btn));
        }
    }

    for (size_t i = 0; i < botoesRedes.size() && i < 4; i++) {
        botoesRedes[i]->desenhar(renderer);
    }
}

void JanelaRede::confirmarSenha() {
    if (indiceRedeSelecionada < 0) return;
    std::string ssid = redesDisponiveis[indiceRedeSelecionada].nome;
    std::string senha = senhaAtual;
    std::thread([this, ssid, senha]() {
        ::conectar_wifi(ssid, senha);
        this->inicializarRedes();
    }).detach();
    fecharTecladoSenha();
}

void JanelaRede::desenharTelasenha(SDL_Renderer* renderer) {
    // Overlay escuro
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, nullptr);
    
    desenharTexto(renderer, "Senha para " + redesDisponiveis[indiceRedeSelecionada].nome, ConfigLayout::F(400), ConfigLayout::F(200), {255,255,255,255}, ConfigLayout::F(32));
    desenharTexto(renderer, "Senha: " + std::string(senhaAtual.length(), '*'), ConfigLayout::F(400), ConfigLayout::F(300), {255,255,0,255}, ConfigLayout::F(28));
    tecladoVirtual.desenhar(renderer);
    
    desenharTexto(renderer, "Pressione START (ou ENTER) para confirmar", ConfigLayout::F(400), ConfigLayout::F(500), {200,200,200,255}, ConfigLayout::F(20));
}

void JanelaRede::toggleWifi() {
    wifiAtivo = !wifiAtivo;
    std::string cmd = wifiAtivo ? "nmcli radio wifi on" : "nmcli radio wifi off";
    system(cmd.c_str());
    if (wifiAtivo) inicializarRedes();
}

void JanelaRede::selecionarRede(int i) {
    indiceRedeSelecionada = i;
    tecladoVisivel = true;
    tecladoVirtual.abrir();
}

void JanelaRede::abrirTecladoSenha(int i) { selecionarRede(i); }
void JanelaRede::fecharTecladoSenha() { tecladoVisivel = false; tecladoVirtual.fechar(); }
void JanelaRede::navegarParaCima() {}
void JanelaRede::navegarParaBaixo() {}
void JanelaRede::confirmarSelecao() {}
void JanelaRede::atualizarScroll() {}
void JanelaRede::carregarImagemExplicativa(SDL_Renderer*) {}
void JanelaRede::liberarImagemExplicativa() {}
void JanelaRede::resetar() { inicializarRedes(); }

bool JanelaRede::processarEvento(SDL_Event& e) {
    if (tecladoVisivel) {
        MeuProjeto::StringInputAdapter adapter(senhaAtual);
        
        // Prioridade: Teclado Virtual processa os botões do controle (A, D-Pad)
        bool processado = tecladoVirtual.processarControle(e, &adapter);
        if (processado) return true;

        // Se o teclado não processou, verificamos se o usuário quer confirmar (START ou ENTER)
        if ((e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_START) ||
            (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN)) {
            confirmarSenha();
            return true;
        }
        
        if (e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
            fecharTecladoSenha();
            return true;
        }
        return false;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        if (btnToggleWifi->contemPonto(e.button.x, e.button.y)) toggleWifi();
        for(size_t i=0; i<botoesRedes.size(); i++) {
            if (botoesRedes[i]->contemPonto(e.button.x, e.button.y)) selecionarRede(i);
        }
    }
    return false;
}
