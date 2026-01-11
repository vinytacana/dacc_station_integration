# 📋 TODO 06: Padronização de Assets e Filesystem

**Objetivo:** Implementar a estrutura de diretórios "Hero" recomendada e garantir carregamento resiliente.

**Arquivos Afetados:**
- `src/GerenciadorImagens.cpp`
- `src/GerenciadorJogos.cpp`

**Especificações Técnicas:**

1.  **Nova Estrutura de Diretórios:**
    O sistema deve buscar os assets na seguinte hierarquia:
    ```text
    /dacc/games/
    ├── [ID_DO_JOGO]/
    │   ├── rom.bin            (Executável/ROM)
    │   ├── assets/
    │   │   ├── cover_portrait.jpg  (Capa Vertical: 600x900)
    │   │   ├── hero_bg.jpg         (Wallpaper: 1920x1080, preferencialmente c/ blur)
    │   │   └── logo.png            (Logo transparente, opcional)
    ```

2.  **Lógica de Fallback de Imagem:**
    Ao solicitar `hero_bg`:
    1. Tenta carregar `/assets/hero_bg.jpg`.
    2. Se falhar, tenta `/assets/cover_portrait.jpg` (e será esticada/cortada pela UI).
    3. Se falhar, retorna textura "PLACEHOLDER".

3.  **Otimização de Memória:**
    - Use `IMG_Load` para carregar JPGs (menor uso de disco) mas converta para textura nativa da GPU (`SDL_CreateTextureFromSurface`).
    - Não mantenha *todos* os wallpapers carregados. Implemente um cache simples ou carregue sob demanda no evento de troca de seleção.

**Critério de Sucesso:**
O sistema reconhece a nova estrutura de pastas. Jogos sem a pasta `assets` ainda funcionam (usando placeholder).