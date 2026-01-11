# TODO VISUAL 02: ÁREA DE AÇÃO (BOTÃO JOGAR)

## OBJETIVO
Redesenhar o botão de ação principal ("JOGAR") para se integrar à área de informações do jogo, abandonando o estilo "retângulo gigante no canto direito". Ele deve parecer um botão de UI moderna (arredondado/pílula) e incluir a dica de atalho.

## REFERÊNCIA TÉCNICA
- **Arquivo Alvo:** `src/JanelaJogo.cpp`
- **Método:** `desenharDetalhesJogo()`

## ESPECIFICAÇÃO DE IMPLEMENTAÇÃO

### 1. Reposicionamento
Mover o botão para a esquerda, alinhado verticalmente com o título e a descrição.
- **Posição Y:** Logo abaixo da descrição do jogo (aprox. Y = 450px).
- **Posição X:** Alinhado à margem esquerda do texto (aprox. X = 100px).

### 2. Estilo Visual (Pílula)
Em vez de `SDL_RenderFillRect`:
- Usar `roundedBoxRGBA` (da lib `SDL2_gfx`) com raio alto (ex: 25px) para criar uma forma de pílula.
- **Cor:** Azul Vibrante (Steam Blue: `rgb(26, 140, 255)`) se estado for "JOGAR".
- **Cor:** Verde ou Cinza se estado for "RODANDO".

### 3. Conteúdo do Botão
O botão deve conter:
1. **Ícone do Input:** Um círculo branco pequeno com a letra 'A' ou texto 'Enter' vazado, ou desenhado à esquerda do texto.
2. **Texto de Ação:** "JOGAR", "EM EXECUÇÃO", ou "INSTALAR".

### 4. Hierarquia
Se o foco estiver no carrossel (`focoAtual == FOCO_CARROSSEL`), o botão deve ter opacidade reduzida ou cor desaturada.
Se o foco estiver no botão (`focoAtual == FOCO_ACOES`), ele deve brilhar ou ficar branco com texto azul (inversão).

## PASSO A PASSO
1. [ ] Remover o código antigo que desenha o botão azul no canto direito inferior (`1500, 800`).
2. [ ] Criar função helper `desenharBotaoPilula(Renderer, x, y, texto, cor)`.
3. [ ] Integrar chamada dentro de `desenharDetalhesJogo`, posicionado abaixo do texto de descrição.
4. [ ] Adicionar lógica visual para estado "Bloqueado/Rodando" (cor de fundo diferente).
