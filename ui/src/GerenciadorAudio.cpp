/**
 * @file GerenciadorAudio.cpp
 * @brief Implementação do sistema centralizado de gerenciamento de áudio do projeto.
 * 
 * Este arquivo implementa um gerenciador de áudio completo utilizando SDL_mixer,
 * fornecendo funcionalidades para:
 * - Reprodução de efeitos sonoros (SFX) com sistema de cache
 * - Reprodução de música de fundo com controles de playback
 * - Gerenciamento de volume independente para SFX e música
 * - Carregamento otimizado de recursos de áudio
 * - Controle de loop e estado de reprodução
 * 
 * O sistema implementa o padrão Singleton através de uma instância global
 * declarada fora do namespace para facilitar acesso em todo o projeto.
 */

#include "GerenciadorAudio.hpp"
#include "Utils.hpp"

/**
 * @brief Instância Global do Gerenciador de Áudio (Singleton).
 * 
 * Declarada fora do namespace MeuProjeto para facilitar o acesso global
 * em todo o projeto. Esta instância é referenciada através da declaração
 * 'extern' no arquivo header correspondente.
 * 
 * @note A inicialização ocorre antes da função main() devido à natureza
 * de variáveis globais em C++.
 */
MeuProjeto::GerenciadorAudio gerAudio;

namespace MeuProjeto {

/**
 * @brief Construtor do Gerenciador de Áudio.
 * 
 * Inicializa o gerenciador em um estado limpo, pronto para carregar
 * recursos de áudio. A inicialização real do SDL_mixer é feita no
 * main.cpp através de Mix_OpenAudio(), este construtor apenas prepara
 * as estruturas de dados internas.
 * 
 * Inicializações:
 * - musicaAtual = nullptr (nenhuma música carregada inicialmente)
 * - cacheSfx vazio (será populado sob demanda)
 * 
 * @note O SDL_mixer deve estar inicializado antes de usar este gerenciador.
 */
GerenciadorAudio::GerenciadorAudio() : musicaAtual(nullptr) {
    // Mixer inicializado no main.cpp, aqui apenas gerenciamos os recursos
}

/**
 * @brief Destrutor do Gerenciador de Áudio.
 * 
 * Realiza a limpeza completa de todos os recursos de áudio carregados
 * durante a execução do programa. Este processo é crucial para evitar
 * vazamentos de memória e liberar recursos do sistema.
 * 
 * Processo de limpeza:
 * 1. **Efeitos Sonoros (SFX)**:
 *    - Itera sobre todos os chunks em cache
 *    - Libera cada chunk com Mix_FreeChunk()
 *    - Limpa o mapa de cache
 * 
 * 2. **Música**:
 *    - Verifica se há música carregada
 *    - Libera com Mix_FreeMusic()
 *    - Reseta ponteiro para nullptr
 * 
 * @note Este destrutor é chamado automaticamente ao final do programa
 * quando a instância global gerAudio é destruída.
 */
GerenciadorAudio::~GerenciadorAudio() {
    liberarTudo();
}

void GerenciadorAudio::liberarTudo() {
    // Libera todos os chunks de efeitos sonoros do cache
    for (auto& par : cacheSfx) {
        if (par.second) {
            Mix_FreeChunk(par.second);
            par.second = nullptr;
        }
    }
    cacheSfx.clear();

    // A música é liberada apenas se estiver carregada
    if (musicaAtual) {
        Mix_FreeMusic(musicaAtual);
        musicaAtual = nullptr;
    }
}

/**
 * @brief Reproduz um efeito sonoro (SFX) com sistema de cache inteligente.
 * 
 * Este método implementa um sistema de cache para otimizar a reprodução
 * de efeitos sonoros frequentes. O fluxo de execução é:
 * 
 * 1. **Construção do Caminho**: Adiciona o prefixo "assets/sounds/"
 * 2. **Busca no Cache**: Verifica se o som já foi carregado anteriormente
 * 3. **Carregamento sob Demanda**: Se não estiver em cache, carrega o arquivo
 * 4. **Armazenamento**: Adiciona ao cache para reutilização futura
 * 5. **Reprodução**: Toca o som em qualquer canal disponível
 * 
 * Vantagens do sistema de cache:
 * - Reduz operações de I/O (disco)
 * - Melhora performance em sons repetitivos
 * - Elimina stuttering causado por carregamento em tempo real
 * 
 * @param nomeArquivo Nome do arquivo de som (sem caminho, ex: "click.wav")
 * 
 * @note O som é reproduzido no primeiro canal disponível (-1 em Mix_PlayChannel).
 * @note Se o carregamento falhar, a função retorna silenciosamente (erro já logado).
 */
void GerenciadorAudio::tocarSom(const std::string& nomeArquivo) {
    // Constrói o caminho completo do arquivo de som
    std::string caminho = caminho_absoluto_projeto("assets/sounds/" + nomeArquivo);
    
    // Tenta encontrar o som no cache usando o nome como chave
    if (cacheSfx.find(nomeArquivo) == cacheSfx.end()) {
        // Som não está em cache, precisa carregar do disco
        Mix_Chunk* som = carregarSfx(caminho);
        if (som) {
            // Carregamento bem-sucedido, adiciona ao cache
            cacheSfx[nomeArquivo] = som;
        } else {
            // Falha silenciosa (erro já foi logado na função carregarSfx)
            return;
        }
    }

    // Toca o som no primeiro canal disponível
    // Parâmetros: Mix_PlayChannel(canal, chunk, loops)
    // -1 = primeiro canal livre, 0 = sem repetição
    Mix_PlayChannel(-1, cacheSfx[nomeArquivo], 0);
}

/**
 * @brief Carrega um efeito sonoro do disco para a memória.
 * 
 * Função auxiliar privada que realiza o carregamento físico do arquivo
 * de áudio. Suporta múltiplos formatos através do SDL_mixer:
 * - WAV (sem compressão)
 * - OGG (compressão Vorbis)
 * - MP3 (se suportado pela build do SDL_mixer)
 * 
 * Em caso de erro:
 * - Registra mensagem detalhada no stderr
 * - Inclui caminho do arquivo e mensagem de erro do SDL_mixer
 * - Retorna nullptr para indicar falha
 * 
 * @param path Caminho completo do arquivo de som (incluindo "assets/sounds/")
 * @return Mix_Chunk* Ponteiro para o chunk carregado, ou nullptr em caso de erro
 */
Mix_Chunk* GerenciadorAudio::carregarSfx(const std::string& path) {
    // Tenta carregar o arquivo de áudio
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    
    if (!chunk) {
        // Carregamento falhou, registra erro com detalhes
        std::cerr << "[AUDIO] Erro ao carregar SFX (" << path << "): " 
                  << Mix_GetError() << std::endl;
    }
    
    return chunk;
}

/**
 * @brief Carrega e reproduz uma música de fundo.
 * 
 * Este método gerencia a música principal do jogo/aplicação. Implementa
 * um sistema de substituição automática onde apenas uma música pode
 * tocar por vez.
 * 
 * Fluxo de execução:
 * 1. **Construção do Caminho**: Adiciona prefixo "assets/music/"
 * 2. **Limpeza de Música Anterior**:
 *    - Para a reprodução atual (Mix_HaltMusic)
 *    - Libera a música da memória (Mix_FreeMusic)
 * 3. **Carregamento da Nova Música**: Mix_LoadMUS() suporta vários formatos
 * 4. **Reprodução**: Inicia playback com ou sem loop
 * 
 * Diferença entre Música e SFX:
 * - Música: Streaming do disco, baixo uso de RAM, apenas 1 por vez
 * - SFX: Totalmente em RAM, múltiplos simultâneos, sistema de canais
 * 
 * @param nomeArquivo Nome do arquivo de música (ex: "menu_theme.ogg")
 * @param loop true para repetir infinitamente, false para tocar uma vez
 * 
 * @note SDL_mixer faz streaming da música, não carrega completamente em RAM.
 * @note Mix_PlayMusic com -1 = loop infinito, 0 = toca uma vez apenas.
 */
void GerenciadorAudio::tocarMusica(const std::string& nomeArquivo, bool loop) {
    // Constrói o caminho completo do arquivo de música
    std::string caminho = caminho_absoluto_projeto("assets/music/" + nomeArquivo);

    // Se já existe música carregada, limpa antes de carregar nova
    if (musicaAtual) {
        Mix_HaltMusic();           // Para a reprodução imediatamente
        Mix_FreeMusic(musicaAtual); // Libera recursos da música anterior
    }

    // Carrega a nova música do disco (streaming, não carrega tudo em RAM)
    musicaAtual = Mix_LoadMUS(caminho.c_str());
    if (!musicaAtual) {
        // Carregamento falhou, registra erro detalhado
        std::cerr << "[AUDIO] Erro ao carregar Musica (" << caminho << "): " 
                  << Mix_GetError() << std::endl;
        return;
    }

    // Inicia a reprodução da música
    // -1 = loop infinito, 0 = toca uma vez
    Mix_PlayMusic(musicaAtual, loop ? -1 : 0);
}

/**
 * @brief Pausa a música atual se estiver tocando.
 * 
 * Congela a reprodução da música no ponto atual, permitindo que seja
 * retomada posteriormente do mesmo ponto através de retomarMusica().
 * 
 * Diferença entre Pausar e Parar:
 * - **Pausar**: Mantém posição, pode retomar do mesmo ponto
 * - **Parar**: Volta ao início, próximo play recomeça a música
 * 
 * @note Se nenhuma música estiver tocando, a função não faz nada (seguro chamar).
 * @see retomarMusica() Para retomar a reprodução pausada
 */
void GerenciadorAudio::pausarMusica() {
    if (Mix_PlayingMusic()) {
        Mix_PauseMusic();
    }
}

/**
 * @brief Retoma a reprodução de uma música pausada.
 * 
 * Continua a reprodução do ponto exato onde a música foi pausada.
 * Apenas funciona se a música estiver no estado "pausado".
 * 
 * Estados da música:
 * - **Tocando**: Mix_PlayingMusic() retorna true
 * - **Pausado**: Mix_PausedMusic() retorna true
 * - **Parado**: Ambas as funções retornam false
 * 
 * @note Se a música não estiver pausada, a função não faz nada (seguro chamar).
 * @see pausarMusica() Para pausar a música atualmente tocando
 */
void GerenciadorAudio::retomarMusica() {
    if (Mix_PausedMusic()) {
        Mix_ResumeMusic();
    }
}

/**
 * @brief Para completamente a reprodução da música.
 * 
 * Interrompe a música imediatamente e reseta a posição de reprodução
 * para o início. A próxima chamada a tocarMusica() ou Mix_PlayMusic()
 * começará do início da faixa.
 * 
 * Diferença para pausarMusica():
 * - **pararMusica()**: Perde a posição, volta ao início
 * - **pausarMusica()**: Mantém posição para retomar depois
 * 
 * @note A música carregada (musicaAtual) permanece na memória,
 * apenas a reprodução é interrompida.
 */
void GerenciadorAudio::pararMusica() {
    Mix_HaltMusic();
}

/**
 * @brief Define o volume global de todos os efeitos sonoros (SFX).
 * 
 * Ajusta o volume de todos os canais de efeitos sonoros simultaneamente.
 * Este volume é multiplicativo com o volume individual de cada chunk.
 * 
 * Comportamento do SDL_mixer:
 * - Volume 0 = Silêncio completo
 * - Volume 128 = Volume padrão/máximo do SDL_mixer (MIX_MAX_VOLUME)
 * - Valores acima de 128 podem causar distorção/clipping
 * 
 * @param volume Nível de volume (0-128, onde 128 é o máximo recomendado)
 * 
 * @note Mix_Volume(-1, volume) aplica a todos os canais simultaneamente.
 * @note Para ajustar apenas um canal específico, use Mix_Volume(canal, volume).
 */
void GerenciadorAudio::setVolumeSFX(int volume) {
    // Define volume para todos os canais de SFX
    // -1 = aplica a todos os canais simultaneamente
    Mix_Volume(-1, volume);
}

/**
 * @brief Define o volume da música de fundo.
 * 
 * Ajusta o volume apenas da música, sem afetar os efeitos sonoros.
 * Permite controle independente entre música e SFX, comum em configurações
 * de áudio de jogos.
 * 
 * Escala de volume:
 * - 0 = Música silenciada
 * - 128 = Volume máximo padrão (MIX_MAX_VOLUME)
 * - Valores intermediários = Proporcionais
 * 
 * @param volume Nível de volume (0-128, onde 128 é o máximo recomendado)
 * 
 * @note Diferente do volume de SFX, o volume da música é global e afeta
 * apenas a trilha sonora de fundo carregada via Mix_LoadMUS().
 */
void GerenciadorAudio::setVolumeMusica(int volume) {
    Mix_VolumeMusic(volume);
}

} // namespace MeuProjeto
