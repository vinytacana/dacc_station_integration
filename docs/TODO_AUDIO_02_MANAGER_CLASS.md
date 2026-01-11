# TODO AUDIO 02: CLASSE GERENCIADOR DE ÁUDIO

## OBJETIVO
Criar uma classe centralizada (`GerenciadorAudio`) responsável por carregar, cachear e reproduzir sons. Ela deve abstrair a complexidade da `SDL2_mixer` e seguir o padrão de design dos outros gerenciadores do projeto (`GerenciadorImagens`, `GerenciadorFontes`).

## REFERÊNCIA TÉCNICA
- **Arquivos Alvo:** `ui/include/GerenciadorAudio.hpp`, `ui/src/GerenciadorAudio.cpp`.
- **Tipos SDL:** 
  - `Mix_Chunk*`: Para efeitos sonoros curtos (SFX).
  - `Mix_Music*`: Para música de fundo (BGM).

## ESPECIFICAÇÃO DE IMPLEMENTAÇÃO

### 1. Interface da Classe (Header)
A classe deve ser um Singleton ou Instância Global (extern) para fácil acesso em qualquer janela.

```cpp
class GerenciadorAudio {
public:
    GerenciadorAudio();
    ~GerenciadorAudio();

    // Carrega e toca um efeito sonoro (cache automático)
    void tocarSom(const std::string& nomeArquivo);
    
    // Toca música de fundo (loop -1 = infinito)
    void tocarMusica(const std::string& nomeArquivo, bool loop = true);
    
    // Controle
    void pausarMusica();
    void pararMusica();
    void setVolumeSFX(int volume); // 0-128
    void setVolumeMusic(int volume);

private:
    std::map<std::string, Mix_Chunk*> cacheSfx;
    Mix_Music* musicaAtual = nullptr;
    
    Mix_Chunk* carregarSfx(const std::string& path);
};
```

### 2. Gerenciamento de Memória (RAII)
- No destrutor, iterar sobre o mapa `cacheSfx` e chamar `Mix_FreeChunk`.
- Se houver música carregada, chamar `Mix_FreeMusic`.

### 3. Tratamento de Erros
- Se o arquivo não existir, logar erro no console (`std::cerr`) mas **não crashar** a aplicação. O jogo deve continuar mudo.

## PASSO A PASSO
1. [ ] Criar arquivo header `.hpp`.
2. [ ] Implementar lógica de cache (`std::map`) no `.cpp`.
3. [ ] Implementar funções de reprodução e limpeza.
4. [ ] Instanciar objeto global `gerAudio` (similar a `gerImg`).
