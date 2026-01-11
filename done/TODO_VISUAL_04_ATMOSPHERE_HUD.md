# TODO VISUAL 04: ATMOSFERA E HUD (GLOBAL)

## OBJETIVO
Unificar a interface com um acabamento profissional. Criar contraste suficiente para que o texto branco seja legível sobre qualquer background de jogo. Limpar o HUD (cabeçalho/rodapé) removendo ruído visual.

## REFERÊNCIA TÉCNICA
- **Arquivo Alvo:** `src/JanelaJogo.cpp`, `src/GraphicsUtils.cpp`
- **Libs:** `SDL2_gfx` (para gradientes e formas).

## ESPECIFICAÇÃO DE IMPLEMENTAÇÃO

### 1. Overlay Gradiente (Vignette/Shadow)
O background do jogo pode ser muito claro, dificultando a leitura.
- **Implementação:** Desenhar um retângulo preto com gradiente vertical (Alpha 0 -> Alpha 200) sobre o background, mas *atrás* do texto e das capas.
- **Área:** Começar do meio da tela (Y=540) até o final (Y=1080).
- **Técnica:** Usar uma textura PNG semi-transparente (`assets/ui/gradient_overlay.png`) é mais rápido que calcular gradiente por pixel. Se não houver asset, usar `boxRGBA` preto com Alpha 180 na metade inferior como fallback temporário.

### 2. Rodapé Sólido (Footer)
Substituir a atual faixa de logos ou transparência por uma barra sólida ou quase sólida.
- **Altura:** 60px a 80px na base da tela.
- **Cor:** Preto quase total (10, 10, 10) ou Gradiente muito escuro.
- **Conteúdo:** Apenas os prompts de botão ("B - Voltar", "Menu - Opções"). Alinhados à direita.

### 3. Cabeçalho (Header)
Simplificar a barra de status.
- **Avatar:** Melhorar o desenho do círculo do usuário. Adicionar uma borda branca de 2px (`aacircleRGBA` de SDL2_gfx).
- **Relógio:** Fonte Monospace ou numérica tabular para evitar que o relógio "pule" quando os segundos mudam.

### 4. Remoção de Ruído
- **Remover:** A imagem estática de logos de empresas (Marvel, etc.) no rodapé. Isso polui o visual e raramente bate com o jogo selecionado.

## PASSO A PASSO
1. [ ] Criar/Gerar asset `gradient_overlay.png` (ou usar `boxRGBA` com Alpha como placeholder).
2. [ ] Inserir desenho do Overlay em `JanelaJogo::desenhar` logo após desenhar o background e antes de desenhar textos/capas.
3. [ ] Atualizar `GraphicsUtils::drawBottomNavHints` para desenhar um fundo retangular preto antes dos textos.
4. [ ] Remover renderização da textura de logos de empresas.
