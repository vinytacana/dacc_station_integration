# MÓDULO: USER INTERFACE (dacc-ui)
# RESPONSÁVEL TÉCNICO: João Eduardo (Frontend & UX)

## 1. ESCOPO E OBJETIVO
Este diretório contém o código da Interface Gráfica do DACC Station.
A UI é a "camada base" do sanduíche arquitetural. Ela roda continuamente quando nenhum jogo está aberto.

**RESPONSABILIDADES:**
1.  **Renderização:** Desenhar menus, capas de jogos (box art) e animações na tela usando aceleração de hardware.
2.  **Input:** Ler entradas do controle/teclado para navegação nos menus.
3.  **Configuração:** Gerar o arquivo `../config.json` com as preferências do usuário (resolução, volume).
4.  **Lançamento:** Invocar o `gameman-pm` (ou sinalizar o daemon) quando o usuário escolhe "JOGAR".

**RESTRIÇÕES RÍGIDAS:**
- **NUNCA** renderize o jogo dentro de uma janela da UI. A UI deve sair da frente ou ficar em segundo plano.
- **NUNCA** altere configurações de driver de vídeo diretamente.

---

## 2. TECH STACK & REQUISITOS
- **Linguagem:** C++ (Padrão C++17).
- **Graphics Lib:** SDL2 (Simple DirectMedia Layer).
- **Extension Libs:** - `SDL2_image` (Carregamento de PNG/JPG).
  - `SDL2_ttf` (Renderização de fontes TrueType).
  - `SDL2_mixer` (Efeitos sonoros de navegação).
- **Hardware Alvo:** Raspberry Pi 4 (GPU VideoCore VI).
  - *Dica:* Use sempre `SDL_Texture` (VRAM) em vez de `SDL_Surface` (RAM) para renderização.

---

## 3. ARQUITETURA E PADRÕES (SDL FOCUSED)

### O Loop Principal (Game Loop)
A estrutura deve seguir o padrão clássico:
1.  `HandleEvents()`: Processa fila de input (`SDL_PollEvent`).
2.  `Update()`: Atualiza lógica de animação e transição de estados.
3.  `Render()`: Limpa a tela (`SDL_RenderClear`), desenha (`SDL_RenderCopy`), atualiza (`SDL_RenderPresent`).

### Gerenciamento de Recursos (Resource Managers)
Carregar texturas e fontes é custoso (I/O de disco).
- **Singleton:** Use `TextureManager` e `FontManager`.
- **Cache:** Carregue os assets na inicialização ou sob demanda e mantenha em cache (`std::map<string, SDL_Texture*>`).
- **RAII:** Garanta que `SDL_DestroyTexture` seja chamado nos destrutores. O vazamento de VRAM trava o Pi rapidamente.

### Navegação (State Machine)
- Use o padrão **State** para telas (ex: `MenuState`, `SettingsState`, `GameState`).
- Evite `if/else` gigantes dentro do `main loop`.

---

## 4. MODOS DE OPERAÇÃO (COMANDOS)

### Se eu digitar `/plan` (Modo UX Designer):
- Descreva o fluxo de telas.
- Defina o layout (posicionamento X,Y relativos, nunca absolutos fixos, para suportar 720p/1080p).
- Sugira animações de transição (ex: fade-in, slide).

### Se eu digitar `/code` (Modo SDL Developer):
- Escreva código C++ focado em SDL2.
- Verifique sempre se ponteiros (ex: `SDL_Window*`) não são nulos após a criação.
- Use `SDL_Rect` para cálculos de geometria.
- **Snippet Obrigatório:** Ao criar texturas, verifique erros:
  ```cpp
  if (!texture) { SDL_Log("Failed to load texture: %s", IMG_GetError()); }