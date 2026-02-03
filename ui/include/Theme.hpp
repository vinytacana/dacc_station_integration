#ifndef THEME_HPP
#define THEME_HPP

#include <SDL2/SDL.h>

namespace MeuProjeto {

/**
 * @struct Theme
 * @brief Centraliza todos os tokens de design visual (cores, tamanhos, espaçamentos)
 * 
 * Esta estrutura define um sistema de design consistente para toda a aplicação,
 * fornecendo constantes para cores, raios de borda e espaçamentos. Permite fácil
 * personalização visual ("theming") e garante consistência estética em todos os
 * componentes da interface.
 * 
 * Todos os valores são constantes em tempo de compilação (constexpr) para máxima
 * eficiência. A estrutura é organizada em categorias lógicas: backgrounds, acentos,
 * botões, texto e geometria.
 */
struct Theme {
    // --- Palette: Backgrounds ---
    
    /**
     * @brief Cor de fundo primária da aplicação
     * 
     * Cinza profundo (30, 30, 35) usado como plano de fundo principal da aplicação.
     * Fornece uma base escura e neutra que reduz fadiga visual e destaca o conteúdo.
     */
    static constexpr SDL_Color BG_PRIMARY   = {30, 30, 35, 255};
    
    /**
     * @brief Cor de fundo para painéis e cards
     * 
     * Cinza mais claro (40, 42, 48) com transparência (alpha=230) usado para
     * painéis, cards e elementos de UI elevados. Cria contraste sutil sobre
     * o fundo primário mantendo hierarquia visual clara.
     */
    static constexpr SDL_Color BG_PANEL     = {40, 42, 48, 230};
    
    /**
     * @brief Cor de fundo para overlays e dimming
     * 
     * Quase preto (10, 10, 12) com alta transparência (alpha=220) usado para
     * escurecer o fundo quando modais, popups ou menus estão abertos. Cria
     * efeito de sobreposição que mantém contexto visual do que está por trás.
     */
    static constexpr SDL_Color BG_OVERLAY   = {10, 10, 12, 220};

    // --- Palette: Accents & States ---
    
    /**
     * @brief Cor de acento principal da aplicação
     * 
     * Azul profissional (0, 122, 204) usado para indicar foco, seleção e
     * elementos interativos ativos. Fornece contraste forte contra os tons
     * de cinza, guiando a atenção do usuário para áreas de interação.
     */
    static constexpr SDL_Color ACCENT_COLOR = {0, 122, 204, 255};
    
    /**
     * @brief Cor para ações destrutivas ou de perigo
     * 
     * Vermelho intenso (220, 50, 50) usado em botões de fechar, deletar ou
     * outras ações que requerem atenção especial do usuário. Transmite
     * urgência e potencial para ações irreversíveis.
     */
    static constexpr SDL_Color DANGER_COLOR = {220, 50, 50, 255};
    
    // --- Palette: Buttons ---
    
    /**
     * @brief Cor padrão de botões no estado normal (repouso)
     * 
     * Cinza médio (55, 55, 60) usado quando o botão não está sendo
     * interagido. Estado base que se integra com o design geral.
     */
    static constexpr SDL_Color BTN_NORMAL   = {55, 55, 60, 255};
    
    /**
     * @brief Cor de botões no estado hover (mouse sobre o elemento)
     * 
     * Cinza mais claro (75, 75, 85) usado quando o cursor está sobre o botão.
     * Fornece feedback visual imediato de que o elemento é interativo.
     */
    static constexpr SDL_Color BTN_HOVER    = {75, 75, 85, 255};
    
    /**
     * @brief Cor de botões no estado ativo (sendo pressionado/selecionado)
     * 
     * Cinza ainda mais claro (90, 90, 100) usado quando o botão está sendo
     * pressionado ou está no estado selecionado. Indica claramente a ação
     * atual do usuário.
     */
    static constexpr SDL_Color BTN_ACTIVE   = {90, 90, 100, 255};

    // --- Palette: Text ---
    
    /**
     * @brief Cor primária para texto principal
     * 
     * Branco levemente atenuado (240, 240, 245) usado para títulos, labels
     * e texto de alta importância. Off-white ao invés de branco puro reduz
     * contraste excessivo e fadiga visual em fundos escuros.
     */
    static constexpr SDL_Color TEXT_PRIMARY = {240, 240, 245, 255};
    
    /**
     * @brief Cor secundária para texto de apoio
     * 
     * Branco levemente atenuado (240, 240, 245) usado para descrições,
     * subtítulos e texto de menor hierarquia visual. Atualmente idêntico
     * ao TEXT_PRIMARY, mas separado para permitir fácil diferenciação futura.
     */
    static constexpr SDL_Color TEXT_SECONDARY= {240, 240, 245, 255};

    // --- Geometry ---
    
    /**
     * @brief Raio de borda pequeno para cantos levemente arredondados
     * 
     * 5 pixels de raio, usado em elementos compactos ou detalhes sutis
     * onde arredondamento mínimo é desejado.
     */
    static constexpr int RADIUS_SMALL = 5;
    
    /**
     * @brief Raio de borda médio para arredondamento padrão
     * 
     * 10 pixels de raio, usado na maioria dos botões, cards e painéis.
     * Fornece aparência moderna e amigável sem ser excessivo.
     */
    static constexpr int RADIUS_MEDIUM = 10;
    
    /**
     * @brief Raio de borda grande para elementos com destaque
     * 
     * 20 pixels de raio, usado em elementos grandes como modais principais
     * ou painéis de destaque onde arredondamento pronunciado é desejado.
     */
    static constexpr int RADIUS_LARGE = 20;

    /**
     * @brief Padding pequeno para espaçamento compacto
     * 
     * 10 pixels de espaçamento interno, usado em botões pequenos, tags
     * ou elementos onde espaço é limitado mas algum padding é necessário.
     */
    static constexpr int PADDING_SMALL = 10;
    
    /**
     * @brief Padding normal para espaçamento padrão
     * 
     * 20 pixels de espaçamento interno, usado na maioria dos elementos
     * como botões, cards e painéis. Fornece respiração adequada ao conteúdo
     * sem desperdiçar espaço excessivo.
     */
    static constexpr int PADDING_NORMAL = 20;
};

} // namespace MeuProjeto

#endif // THEME_HPP