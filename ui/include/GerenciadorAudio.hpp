#ifndef GERENCIADOR_AUDIO_HPP
#define GERENCIADOR_AUDIO_HPP

#include <SDL2/SDL_mixer.h>
#include <string>
#include <map>
#include <iostream>

namespace MeuProjeto {

class GerenciadorAudio {
public:
    GerenciadorAudio();
    ~GerenciadorAudio();

    /**
     * @brief Toca um efeito sonoro curto (SFX).
     * Carrega automaticamente se ainda não estiver no cache.
     * 
     * @param nomeArquivo Nome do arquivo em assets/sounds/ (ex: "nav.wav")
     */
    void tocarSom(const std::string& nomeArquivo);

    /**
     * @brief Toca uma música de fundo (BGM).
     * 
     * @param nomeArquivo Nome do arquivo em assets/music/ (ex: "ambient.mp3")
     * @param loop Se true, toca em loop infinito.
     */
    void tocarMusica(const std::string& nomeArquivo, bool loop = true);

    void pausarMusica();
    void retomarMusica();
    void pararMusica();
    void liberarTudo();

    // Controle de volume (0 a 128)
    void setVolumeSFX(int volume);
    void setVolumeMusica(int volume);

private:
    std::map<std::string, Mix_Chunk*> cacheSfx;
    Mix_Music* musicaAtual;

    Mix_Chunk* carregarSfx(const std::string& path);
};

} // namespace MeuProjeto

// Declaração global para acesso fácil
extern MeuProjeto::GerenciadorAudio gerAudio;

#endif // GERENCIADOR_AUDIO_HPP
