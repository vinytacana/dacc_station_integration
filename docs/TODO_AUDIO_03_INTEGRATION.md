# TODO AUDIO 03: INTEGRAÇÃO E UX

## OBJETIVO
Integrar o sistema de áudio à lógica de navegação existente na Interface Gráfica. O objetivo é fornecer feedback auditivo imediato para as ações do usuário (mover cursor, selecionar, erro).

## REFERÊNCIA TÉCNICA
- **Arquivo Alvo:** `ui/src/JanelaJogo.cpp`, `ui/src/Botao.cpp`.
- **Eventos Chave:** `SDL_KEYDOWN`, `SDL_MOUSEMOTION`.

## ESPECIFICAÇÃO DE IMPLEMENTAÇÃO

### 1. Mapeamento de Sons (Sound Design)
Definir quais arquivos .wav correspondem a quais ações:
- **Navegação (Setas/Mouse Hover):** `click_short.wav` (Som curto, seco, baixo volume).
- **Seleção/Confirmar (Enter/A):** `confirm_chime.wav` (Som positivo, harmônico).
- **Voltar/Cancelar (Esc/B):** `cancel_whoosh.wav` (Som de fechamento).
- **Erro/Bloqueado:** `error_buzz.wav` (Som grave).

### 2. Integração na Janela Principal (`JanelaJogo`)
No método `tratarEvento`:
- **Ao mover índice (Setas):** Chamar `gerAudio.tocarSom("navigation.wav")`.
- **Ao confirmar (Enter):** Chamar `gerAudio.tocarSom("select.wav")`.
- **Ao iniciar a janela:** Chamar `gerAudio.tocarMusica("ambient_loop.mp3")`.

### 3. Integração nos Botões (`Botao`)
No método `handleMouseMotion` e `handleMouseButtonDown`:
- Tocar som de *Hover* apenas quando o estado muda de `!hover` para `hover` (Edge detection) para evitar metralhadora de sons.

### 4. Placeholder Assets
Como não temos os arquivos de som reais ainda, a implementação deve usar nomes genéricos.
- **Ação:** Criar arquivos vazios ou baixar placeholders CC0 para testar a integração.

## PASSO A PASSO
1. [ ] Adicionar chamadas `tocarSom` nos `case` do switch de eventos em `JanelaJogo.cpp`.
2. [ ] Adicionar lógica de som no `Botao.cpp` (cuidado com repetição em loop).
3. [ ] Configurar música de fundo para iniciar no construtor da `JanelaJogo`.
