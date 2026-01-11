# TODO AUDIO 01: AMBIENTE E BUILD SYSTEM

## OBJETIVO
Preparar o ambiente de desenvolvimento e o sistema de build para suportar a biblioteca `SDL2_mixer`, criando também a estrutura de diretórios necessária para os assets de áudio.

## REFERÊNCIA TÉCNICA
- **Biblioteca Alvo:** `SDL2_mixer` (Suporte a WAV, MP3, OGG).
- **Arquivos Alvo:** `ui/Makefile`, `ui/src/main.cpp`.

## ESPECIFICAÇÃO DE IMPLEMENTAÇÃO

### 1. Estrutura de Diretórios
Criar a taxonomia para organizar os arquivos de áudio separando efeitos curtos de músicas longas.
```text
ui/assets/
├── sounds/     (Efeitos sonoros: .wav)
│   ├── navigation.wav
│   ├── select.wav
│   └── error.wav
└── music/      (Música de fundo: .mp3 / .ogg)
    └── ambient.mp3
```

### 2. Atualização do Makefile
O compilador precisa saber onde encontrar os headers e qual biblioteca linkar.
- **Flags de Linkagem (LDFLAGS):** Adicionar `-lSDL2_mixer` ao final da lista de bibliotecas no `ui/Makefile`.

### 3. Inicialização no Main
O subsistema de áudio precisa ser iniciado explicitamente.
- **Arquivo:** `ui/src/main.cpp`
- **Código:**
  ```cpp
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
      std::cerr << "Erro ao iniciar SDL_mixer: " << Mix_GetError() << std::endl;
  }
  // ... Loop principal ...
  Mix_Quit();
  ```

## PASSO A PASSO
1. [ ] Criar pastas `ui/assets/sounds` e `ui/assets/music`.
2. [ ] Editar `ui/Makefile` adicionando a flag `-lSDL2_mixer`.
3. [ ] Adicionar include `<SDL2/SDL_mixer.h>` e inicialização em `ui/src/main.cpp`.
4. [ ] Verificar se o sistema compila sem erros (mesmo sem tocar som ainda).
