#include "GerenciadorAudio.hpp"

// Instância Global (Fora do namespace para casar com o extern global do header)
MeuProjeto::GerenciadorAudio gerAudio;

namespace MeuProjeto {

GerenciadorAudio::GerenciadorAudio() : musicaAtual(nullptr) {
    // Mixer inicializado no main.cpp, aqui apenas gerenciamos os recursos
}

GerenciadorAudio::~GerenciadorAudio() {
    // Libera todos os chunks de efeitos sonoros
    for (auto& par : cacheSfx) {
        if (par.second) {
            Mix_FreeChunk(par.second);
        }
    }
    cacheSfx.clear();

    // A música é liberada apenas se estiver carregada
    if (musicaAtual) {
        Mix_FreeMusic(musicaAtual);
        musicaAtual = nullptr;
    }
}

void GerenciadorAudio::tocarSom(const std::string& nomeArquivo) {
    std::string caminho = "assets/sounds/" + nomeArquivo;
    
    // Tenta encontrar no cache
    if (cacheSfx.find(nomeArquivo) == cacheSfx.end()) {
        Mix_Chunk* som = carregarSfx(caminho);
        if (som) {
            cacheSfx[nomeArquivo] = som;
        } else {
            // Falha silenciosa (erro já logado no carregamento)
            return;
        }
    }

    // Toca o som no primeiro canal disponível (-1)
    Mix_PlayChannel(-1, cacheSfx[nomeArquivo], 0);
}

Mix_Chunk* GerenciadorAudio::carregarSfx(const std::string& path) {
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        std::cerr << "[AUDIO] Erro ao carregar SFX (" << path << "): " << Mix_GetError() << std::endl;
    }
    return chunk;
}

void GerenciadorAudio::tocarMusica(const std::string& nomeArquivo, bool loop) {
    std::string caminho = "assets/music/" + nomeArquivo;

    // Se já existe música carregada, libera
    if (musicaAtual) {
        Mix_HaltMusic();
        Mix_FreeMusic(musicaAtual);
    }

    musicaAtual = Mix_LoadMUS(caminho.c_str());
    if (!musicaAtual) {
        std::cerr << "[AUDIO] Erro ao carregar Musica (" << caminho << "): " << Mix_GetError() << std::endl;
        return;
    }

    Mix_PlayMusic(musicaAtual, loop ? -1 : 0);
}

void GerenciadorAudio::pausarMusica() {
    if (Mix_PlayingMusic()) {
        Mix_PauseMusic();
    }
}

void GerenciadorAudio::retomarMusica() {
    if (Mix_PausedMusic()) {
        Mix_ResumeMusic();
    }
}

void GerenciadorAudio::pararMusica() {
    Mix_HaltMusic();
}

void GerenciadorAudio::setVolumeSFX(int volume) {
    // Define volume para todos os canais (-1)
    Mix_Volume(-1, volume);
}

void GerenciadorAudio::setVolumeMusica(int volume) {
    Mix_VolumeMusic(volume);
}

} // namespace MeuProjeto
