#ifndef CONFIG_LAYOUT_HPP
#define CONFIG_LAYOUT_HPP

#pragma once

#include <iostream>
#include <algorithm> 

namespace MeuProjeto {

/**
 * @brief Estrutura central de configuração de layout responsiva para a interface do jogo
 * 
 * Esta estrutura gerencia todo o sistema de layout da aplicação, incluindo dimensionamento,
 * posicionamento e escalonamento de elementos da interface. Implementa um sistema de layout
 * responsivo que adapta elementos desenhados para uma resolução nativa (1920x1280) para
 * qualquer resolução de tela, mantendo proporções e centralizando o conteúdo.
 * 
 * O sistema funciona através de:
 * - Constantes de design em resolução nativa (1920x1280)
 * - Cálculo automático de escala baseado na resolução atual
 * - Funções helper para converter coordenadas nativas em coordenadas reais
 * 
 * Todos os elementos da interface são definidos em coordenadas nativas e depois
 * convertidos para a tela real através das funções X(), Y() e F().
 */
struct ConfigLayout {
    // VARIÁVEIS DE ESTADO (Dinâmicas)
    
    /** @brief Largura atual da janela/tela em pixels */
    static int larguraTela;
    
    /** @brief Altura atual da janela/tela em pixels */
    static int alturaTela;
    
    /** @brief Fator de escala aplicado a todos os elementos da interface
     *  Calculado como o menor entre (larguraAtual/nativa) e (alturaAtual/nativa)
     *  para manter as proporções sem distorção */
    static float escalaGlobal; 
    
    /** @brief Deslocamento horizontal para centralizar o conteúdo quando há espaço extra */
    static int offsetX;        
    
    /** @brief Deslocamento vertical para centralizar o conteúdo quando há espaço extra */
    static int offsetY;        

    // CONSTANTES DE DESIGN (Base: 1920x1280)

    /** @brief Largura da resolução nativa de referência para o design */
    static constexpr int LARGURA_NATIVA = 1920;
    
    /** @brief Altura da resolução nativa de referência para o design */
    static constexpr int ALTURA_NATIVA  = 1280;

    /** @brief Margem horizontal padrão das bordas da tela */
    static constexpr int MARGEM_TELA_X = 32;
    
    /** @brief Margem vertical padrão das bordas da tela */
    static constexpr int MARGEM_TELA_Y = 30;
    
    // Espaçamentos Genéricos
    /** @brief Espaçamento pequeno entre elementos (20px) */
    static constexpr int ESPACO_PEQUENO = 20;
    
    /** @brief Espaçamento médio entre elementos (50px) */
    static constexpr int ESPACO_MEDIO   = 50;
    
    /** @brief Espaçamento grande entre elementos (80px) */
    static constexpr int ESPACO_GRANDE  = 80;

    // 1. CABEÇALHO
    
        // BOTÃO CONFIG (esquerda)
        /** @brief Largura do botão de configurações no canto superior esquerdo */
        static constexpr int BOTAO_CONFIG_LARGURA = 80;
        
        /** @brief Altura do botão de configurações no canto superior esquerdo */
        static constexpr int BOTAO_CONFIG_ALTURA  = 80;
        
        /** @brief Posição X do botão de configurações (alinhado à margem esquerda) */
        static constexpr int BOTAO_CONFIG_POS_X   = MARGEM_TELA_X;
        
        /** @brief Posição Y do botão de configurações */
        static constexpr int BOTAO_CONFIG_POS_Y   = 29;

        // BOTÃO TEMA (direita)
        /** @brief Largura do botão de tema no canto superior direito */
        static constexpr int BOTAO_TEMA_LARGURA = 80;
        
        /** @brief Altura do botão de tema no canto superior direito */
        static constexpr int BOTAO_TEMA_ALTURA  = 80;
        
        /** @brief Posição X do botão de tema (alinhado à margem direita) */
        static constexpr int BOTAO_TEMA_POS_X   = LARGURA_NATIVA - MARGEM_TELA_X - BOTAO_TEMA_LARGURA;
        
        /** @brief Posição Y do botão de tema */
        static constexpr int BOTAO_TEMA_POS_Y   = 29;

        // PESQUISA (centralizada)
        /** @brief Largura da barra de pesquisa centralizada no topo */
        static constexpr int PESQUISA_LARGURA = 600;
        
        /** @brief Altura da barra de pesquisa */
        static constexpr int PESQUISA_ALTURA  = 80;
        
        /** @brief Posição X da barra de pesquisa (centralizada horizontalmente) */
        static constexpr int PESQUISA_POS_X   = (LARGURA_NATIVA - PESQUISA_LARGURA) / 2;
        
        /** @brief Posição Y da barra de pesquisa */
        static constexpr int PESQUISA_POS_Y   = 29;

        // ÍCONE LUPA
        /** @brief Margem interna do ícone de lupa em relação à borda da barra de pesquisa */
        static constexpr int MARGEM_LUPA        = 14;
        
        /** @brief Tamanho do ícone de lupa (quadrado) */
        static constexpr int ICONE_LUPA_TAMANHO = 50;
        
        /** @brief Posição X do ícone de lupa (alinhado à direita dentro da barra de pesquisa) */
        static constexpr int ICONE_LUPA_POS_X   = PESQUISA_POS_X + PESQUISA_LARGURA - ICONE_LUPA_TAMANHO - MARGEM_LUPA;
        
        /** @brief Posição Y do ícone de lupa (centralizado verticalmente na barra de pesquisa) */
        static constexpr int ICONE_LUPA_POS_Y   = PESQUISA_POS_Y + (PESQUISA_ALTURA - ICONE_LUPA_TAMANHO) / 2;

        /** @brief Largura do fundo decorativo do topo da tela */
        static constexpr int FUNDO_TOPO_LARGURA = 1920;
        
        /** @brief Altura do fundo decorativo do topo da tela */
        static constexpr int FUNDO_TOPO_ALTURA  = 140;

    // 2. ÁREA DE DESTAQUE (HERO)
    
    /** @brief Posição X do painel de destaque principal (hero banner) */
    static constexpr int DESTAQUE_PRINCIPAL_POS_X     = 30;
    
    /** @brief Posição Y do painel de destaque principal */
    static constexpr int DESTAQUE_PRINCIPAL_POS_Y     = 180; 
    
    /** @brief Largura do painel de destaque principal */
    static constexpr int DESTAQUE_PRINCIPAL_LARGURA   = 1862;
    
    /** @brief Altura do painel de destaque principal */
    static constexpr int DESTAQUE_PRINCIPAL_ALTURA    = 597;


    // Miniaturas do carrossel de destaques
    /** @brief Tamanho de cada miniatura no carrossel de destaques (quadrado) */
    static constexpr int DESTAQUE_MINI_TAMANHO     = 320;
    
    /** @brief Espaçamento horizontal entre miniaturas do carrossel */
    static constexpr int DESTAQUE_MINI_ESPACO      = 80; 

    /** @brief Quantidade de miniaturas visíveis simultaneamente no carrossel */
    static constexpr int DESTAQUE_MINI_QTD = 5;

    /** @brief Largura total ocupada pelas miniaturas incluindo espaçamentos */
    static constexpr int DESTAQUE_MINI_LARGURA_TOTAL = DESTAQUE_MINI_QTD * DESTAQUE_MINI_TAMANHO +
    (DESTAQUE_MINI_QTD - 1) * DESTAQUE_MINI_ESPACO;

    /** @brief Posição X inicial do primeiro item do carrossel (centralizado) */
    static constexpr int DESTAQUE_MINI_POS_X_INICIAL = (LARGURA_NATIVA - DESTAQUE_MINI_LARGURA_TOTAL) / 2;
    
    /** @brief Posição Y das miniaturas do carrossel */
    static constexpr int DESTAQUE_MINI_POS_Y = 597;
    
    /** @brief Distância total entre o início de uma miniatura e a próxima (tamanho + espaço) */
    static constexpr int DESTAQUE_MINI_PASSO_TOTAL =
    DESTAQUE_MINI_TAMANHO + DESTAQUE_MINI_ESPACO;

    // 3. CATEGORIAS E BOTÃO ALEATÓRIO

    /** @brief Distância vertical entre a área de destaque (hero) e o painel de categorias */
    static constexpr int DISTANCIA_HERO_CATEGORIA = 80;

    /** @brief Largura do painel que contém os botões de categorias */
    static constexpr int PAINEL_CATEGORIA_LARGURA = 1300;  
    
    /** @brief Altura do painel de categorias */
    static constexpr int PAINEL_CATEGORIA_ALTURA  = 110; 

    /** @brief Posição X do painel de categorias (centralizado horizontalmente) */
    static constexpr int PAINEL_CATEGORIA_POS_X = (LARGURA_NATIVA - PAINEL_CATEGORIA_LARGURA) / 2;

    /** @brief Posição Y do painel de categorias (abaixo das miniaturas + distância) */
    static constexpr int PAINEL_CATEGORIA_POS_Y = DESTAQUE_MINI_POS_Y + DESTAQUE_MINI_TAMANHO + DISTANCIA_HERO_CATEGORIA;

    /** @brief Quantidade de categorias visíveis simultaneamente no painel */
    static constexpr int CATEGORIA_QTD_VISIVEL = 5;

    /** @brief Largura de cada botão de categoria */
    static constexpr int ITEM_CATEGORIA_LARGURA = 200; 
    
    /** @brief Altura de cada botão de categoria */
    static constexpr int ITEM_CATEGORIA_ALTURA  = 80;  

    /** @brief Padding horizontal interno do painel de categorias */
    static constexpr int PAINEL_PADDING_HORIZONTAL = 50;

    /** @brief Largura útil disponível para os botões dentro do painel (descontando padding) */
    static constexpr int PAINEL_CONTEUDO_LARGURA =
        PAINEL_CATEGORIA_LARGURA - (PAINEL_PADDING_HORIZONTAL * 2);

    /** @brief Espaçamento entre botões de categoria (calculado para distribuição uniforme) */
    static constexpr int ITEM_CATEGORIA_ESPACO = (PAINEL_CONTEUDO_LARGURA - (CATEGORIA_QTD_VISIVEL * ITEM_CATEGORIA_LARGURA)) /
        (CATEGORIA_QTD_VISIVEL - 1);

    /** @brief Posição X base do primeiro botão de categoria (relativa ao painel) */
    static constexpr int ITEM_CATEGORIA_POS_X_BASE = PAINEL_CATEGORIA_POS_X + PAINEL_PADDING_HORIZONTAL;

    /** @brief Posição Y dos botões de categoria (centralizados verticalmente no painel) */
    static constexpr int ITEM_CATEGORIA_POS_Y = PAINEL_CATEGORIA_POS_Y + (PAINEL_CATEGORIA_ALTURA - ITEM_CATEGORIA_ALTURA) / 2;

    /** @brief Posição X do título "Categorias" (centralizado) */
    static constexpr int TITULO_CATEGORIA_POS_X = (LARGURA_NATIVA / 2) - 70;
    
    /** @brief Posição Y do título "Categorias" (acima do painel) */
    static constexpr int TITULO_CATEGORIA_POS_Y = PAINEL_CATEGORIA_POS_Y - 60;

    /** @brief Largura das setas de navegação do carrossel de categorias */
    static constexpr int SETA_CATEGORIA_LARGURA = 50;
    
    /** @brief Altura das setas de navegação do carrossel de categorias */
    static constexpr int SETA_CATEGORIA_ALTURA  = 90;

    /** @brief Posição X da seta esquerda (à esquerda do painel com margem) */
    static constexpr int SETA_CATEGORIA_ESQ_POS_X = PAINEL_CATEGORIA_POS_X - SETA_CATEGORIA_LARGURA - 10;

    /** @brief Posição X da seta direita (à direita do painel com margem) */
    static constexpr int SETA_CATEGORIA_DIR_POS_X = PAINEL_CATEGORIA_POS_X + PAINEL_CATEGORIA_LARGURA + 10;

    /** @brief Posição Y das setas de navegação (centralizadas verticalmente no painel) */
    static constexpr int SETA_CATEGORIA_POS_Y = PAINEL_CATEGORIA_POS_Y + (PAINEL_CATEGORIA_ALTURA - SETA_CATEGORIA_ALTURA) / 2;

    /** @brief Largura do botão de jogo aleatório */
    static constexpr int BOTAO_ALEATORIO_LARGURA = 290;
    
    /** @brief Altura do botão de jogo aleatório */
    static constexpr int BOTAO_ALEATORIO_ALTURA  = 435;

    // 4. GRADE DE JOGOS
    
    /** @brief Distância vertical entre o painel de categorias e a grade de jogos */
    static constexpr int DISTANCIA_CATEGORIA_GRADE = 50;
    
    /** @brief Posição Y inicial da grade de jogos */
    static constexpr int GRADE_JOGOS_Y_INICIAL = PAINEL_CATEGORIA_POS_Y + PAINEL_CATEGORIA_ALTURA + DISTANCIA_CATEGORIA_GRADE;
    
    /** @brief Largura de cada item (card) na grade de jogos */
    static constexpr int GRADE_JOGOS_LARGURA_ITEM = 290;
    
    /** @brief Altura de cada item (card) na grade de jogos */
    static constexpr int GRADE_JOGOS_ALTURA_ITEM  = 435;
    
    /** @brief Espaçamento horizontal entre itens da grade */
    static constexpr int GRADE_JOGOS_ESPACO_X     = 72;
    
    /** @brief Espaçamento vertical entre linhas da grade */
    static constexpr int GRADE_JOGOS_ESPACO_Y     = 60;
    
    /** @brief Número de colunas na grade de jogos */
    static constexpr int GRADE_JOGOS_COLUNAS      = 5;

    // 5. JANELA DE DETALHES (JanelaJogo)
    
    // Dimensões e Gaps
    /** @brief Altura da barra de título da janela de detalhes do jogo */
    static constexpr int JANELA_TOPO_ALTURA      = 100;
    
    /** @brief Largura do painel principal da janela de detalhes */
    static constexpr int JANELA_PAINEL_LARGURA   = 1920;
    
    /** @brief Altura do painel principal da janela de detalhes */
    static constexpr int JANELA_PAINEL_ALTURA    = 560;
    
    /** @brief Largura da capa do jogo exibida na janela de detalhes */
    static constexpr int JANELA_CAPA_LARGURA     = 960;
    
    /** @brief Altura da capa do jogo exibida na janela de detalhes */
    static constexpr int JANELA_CAPA_ALTURA      = 540;
    
    /** @brief Espaçamento entre a coluna da capa e a coluna de descrição */
    static constexpr int JANELA_COLUNA_GAP       = 20; 
    
    /** @brief Largura da área de descrição do jogo */
    static constexpr int JANELA_DESC_LARGURA     = 770;
    
    /** @brief Altura da área de descrição do jogo */
    static constexpr int JANELA_DESC_ALTURA      = 350;
    
    /** @brief Largura do botão "Jogar" na janela de detalhes */
    static constexpr int JANELA_JOGAR_LARGURA    = 770;
    
    /** @brief Altura do botão "Jogar" na janela de detalhes */
    static constexpr int JANELA_JOGAR_ALTURA     = 120;
    
    // Posições Relativas
    /** @brief Posição X do painel principal da janela (alinhado à esquerda) */
    static constexpr int JANELA_PAINEL_POS_X     = 0;
    
    /** @brief Posição Y do painel principal da janela */
    static constexpr int JANELA_PAINEL_POS_Y     = 120;
    
    /** @brief Deslocamento horizontal da capa em relação ao painel */
    static constexpr int JANELA_CAPA_OFFSET_X    = 8; 
    
    /** @brief Deslocamento vertical da capa em relação ao painel */
    static constexpr int JANELA_CAPA_OFFSET_Y    = 8;
    
    /** @brief Posição X da capa do jogo (com offset) */
    static constexpr int JANELA_CAPA_POS_X       = JANELA_PAINEL_POS_X + JANELA_CAPA_OFFSET_X; 
    
    /** @brief Posição Y da capa do jogo (com offset) */
    static constexpr int JANELA_CAPA_POS_Y       = JANELA_PAINEL_POS_Y + JANELA_CAPA_OFFSET_Y; 

    /** @brief Posição X da área de descrição (à direita da capa + gap) */
    static constexpr int JANELA_DESC_POS_X       = JANELA_CAPA_POS_X + JANELA_CAPA_LARGURA + JANELA_COLUNA_GAP; 
    
    /** @brief Posição Y da área de descrição (alinhada ao topo da capa) */
    static constexpr int JANELA_DESC_POS_Y       = JANELA_CAPA_POS_Y; 

    /** @brief Posição X do botão "Jogar" (alinhado à área de descrição) */
    static constexpr int JANELA_JOGAR_POS_X      = JANELA_DESC_POS_X; 
    
    /** @brief Posição Y do botão "Jogar" */
    static constexpr int JANELA_JOGAR_POS_Y      = 500; 

    /** @brief Posição Y do título do jogo na janela de detalhes */
    static constexpr int JANELA_TITULO_POS_Y     = 26;
    
    /** @brief Tamanho da fonte do título do jogo */
    static constexpr int JANELA_TITULO_FONT      = 48;

    /** @brief Tamanho do botão de fechar (X) na janela de detalhes */
    static constexpr int JANELA_FECHAR_TAM       = 80;
    
    /** @brief Posição X do botão de fechar (canto superior direito) */
    static constexpr int JANELA_FECHAR_POS_X     = 1830;
    
    /** @brief Posição Y do botão de fechar */
    static constexpr int JANELA_FECHAR_POS_Y     = 10;
    
    /** @brief Tamanho da fonte do símbolo "X" no botão de fechar */
    static constexpr int JANELA_FECHAR_FONT      = 48;

    /** @brief Tamanho da fonte da descrição do jogo */
    static constexpr int JANELA_DESC_FONT        = 24;
    
    /** @brief Altura de linha do texto da descrição */
    static constexpr int JANELA_DESC_LINE_H      = 30;
    
    /** @brief Padding interno da área de descrição */
    static constexpr int JANELA_DESC_PAD         = 10;
    
    /** @brief Tamanho da fonte do botão "Jogar" */
    static constexpr int JANELA_JOGAR_FONT       = 42;

    /** @brief Espaçamento vertical entre o painel e o carrossel de capturas */
    static constexpr int JANELA_CARROSSEL_GAP_Y  = 30; 
    
    /** @brief Posição Y do fundo do carrossel de capturas de tela */
    static constexpr int JANELA_CARROSSEL_BG_POS_Y = JANELA_PAINEL_POS_Y + JANELA_PAINEL_ALTURA + JANELA_CARROSSEL_GAP_Y; 
    
    /** @brief Altura do fundo do carrossel de capturas */
    static constexpr int JANELA_CARROSSEL_BG_ALTURA = 370;

    /** @brief Largura de cada captura de tela no carrossel */
    static constexpr int JANELA_CAPTURA_LARGURA  = 500;
    
    /** @brief Altura de cada captura de tela no carrossel */
    static constexpr int JANELA_CAPTURA_ALTURA   = 250;
    
    /** @brief Offset vertical base das capturas de tela (não usado diretamente) */
    static constexpr int JANELA_CAPTURA_OFFSET_Y = 80; 
    
    /** @brief Offset vertical das setas de navegação do carrossel */
    static constexpr int JANELA_SETA_OFFSET_Y    = 80; 
    
    /** @brief Posição Y base calculada para elementos do carrossel (setas) */
    static constexpr int JANELA_BASE_Y           = JANELA_CARROSSEL_BG_POS_Y + JANELA_SETA_OFFSET_Y;

    /** @brief Posição Y das capturas de tela (centralizadas em relação às setas, +10px) */
    static constexpr int JANELA_CAPTURA_POS_Y    = JANELA_BASE_Y + 10; 
    
    /** @brief Espaçamento horizontal entre capturas de tela no carrossel */
    static constexpr int JANELA_CAPTURA_GAP      = 520; 

    /** @brief Offset horizontal do título do carrossel em relação à borda */
    static constexpr int JANELA_CARROSSEL_TIT_OFFSET_X = 50;
    
    /** @brief Offset vertical do título do carrossel em relação ao fundo */
    static constexpr int JANELA_CARROSSEL_TIT_OFFSET_Y = 10;
    
    /** @brief Posição X do título do carrossel */
    static constexpr int JANELA_CARROSSEL_TIT_POS_X = JANELA_CARROSSEL_TIT_OFFSET_X; 
    
    /** @brief Posição Y do título do carrossel */
    static constexpr int JANELA_CARROSSEL_TIT_POS_Y = JANELA_CARROSSEL_TIT_OFFSET_Y; 
    
    /** @brief Tamanho da fonte do título do carrossel */
    static constexpr int JANELA_CARROSSEL_TIT_FONT  = 28;

    // Setas de Navegação do carrossel
    /** @brief Largura das setas de navegação do carrossel de capturas */
    static constexpr int JANELA_SETA_LARGURA     = 120;
    
    /** @brief Altura das setas de navegação do carrossel de capturas */
    static constexpr int JANELA_SETA_ALTURA      = 270;
    
    /** @brief Posição Y das setas de navegação (na posição base) */
    static constexpr int JANELA_SETA_POS_Y       = JANELA_BASE_Y; 
    
    /** @brief Posição X da seta esquerda do carrossel */
    static constexpr int JANELA_SETA_ESQ_POS_X   = 30;
    
    /** @brief Posição X da seta direita do carrossel */
    static constexpr int JANELA_SETA_DIR_POS_X   = 1770;
    
    /** @brief Tamanho da fonte dos símbolos das setas */
    static constexpr int JANELA_SETA_FONT        = 56;
    
    /** @brief Margem de segurança para as setas */
    static constexpr int JANELA_SETA_MARGEM      = 160;

    /** @brief Posição X do label da seta esquerda (LB = Left Button) */
    static constexpr int JANELA_LABEL_POS_X_LB   = 55;
    
    /** @brief Posição X do label da seta direita (RB = Right Button) */
    static constexpr int JANELA_LABEL_POS_X_RB   = 1785;
    
    /** @brief Offset vertical dos labels das setas em relação às setas */
    static constexpr int JANELA_LABEL_OFFSET_Y   = 35;
    
    /** @brief Tamanho da fonte dos labels das setas */
    static constexpr int JANELA_LABEL_FONT       = 20;

    // TECLADO VIRTUAL
    
    /** @brief Posição X base do teclado virtual na tela */
    static constexpr int TECLADO_BASE_POS_X = 460;
    
    /** @brief Posição Y base do teclado virtual na tela */
    static constexpr int TECLADO_BASE_POS_Y = 600;
    
    /** @brief Tamanho de cada tecla do teclado virtual (quadrado) */
    static constexpr int TECLADO_TECLA_TAMANHO = 60;
    
    /** @brief Espaçamento entre teclas do teclado virtual */
    static constexpr int TECLADO_TECLA_ESPACAMENTO = 10;

    // FUNÇÕES DE CÁLCULO
    
    /**
     * @brief Inicializa o sistema de layout responsivo com a resolução atual da tela
     * 
     * Esta função deve ser chamada uma vez no início da aplicação ou sempre que a
     * resolução da janela mudar. Ela calcula:
     * - O fator de escala global baseado na menor proporção entre largura e altura
     * - Os offsets de centralização para quando a tela for maior que o necessário
     * 
     * O cálculo mantém a proporção de aspecto original (1920x1280) sem distorção,
     * adicionando barras pretas (letterboxing) quando necessário.
     * 
     * @param larguraAtual Largura atual da janela/tela em pixels
     * @param alturaAtual Altura atual da janela/tela em pixels
     */
    static void inicializar(int larguraAtual, int alturaAtual) {
        larguraTela = larguraAtual;
        alturaTela = alturaAtual;
        
        // Calcula proporções em ambas as dimensões
        float ratioX = (float)larguraAtual / LARGURA_NATIVA;
        float ratioY = (float)alturaAtual / ALTURA_NATIVA;
        
        // Usa a menor escala para evitar que conteúdo ultrapasse os limites
        escalaGlobal = std::min(ratioX, ratioY);
        
        // Calcula quanto espaço o conteúdo escalonado ocupará
        int larguraOcupada = (int)(LARGURA_NATIVA * escalaGlobal);
        int alturaOcupada  = (int)(ALTURA_NATIVA * escalaGlobal);
        
        // Centraliza o conteúdo no espaço disponível
        offsetX = (larguraAtual - larguraOcupada) / 2;
        offsetY = (alturaAtual - alturaOcupada) / 2;

        std::cout << "[LAYOUT] Escala: " << escalaGlobal << " Offset: " << offsetX << "," << offsetY << std::endl;
    }

    /**
     * @brief Converte uma coordenada X nativa para a coordenada X real na tela atual
     * 
     * @param valorNativo Coordenada X na resolução nativa (1920x1280)
     * @return Coordenada X escalonada e deslocada para a tela atual
     */
    static int X(int valorNativo) { return (int)(valorNativo * escalaGlobal) + offsetX; }
    
    /**
     * @brief Converte uma coordenada Y nativa para a coordenada Y real na tela atual
     * 
     * @param valorNativo Coordenada Y na resolução nativa (1920x1280)
     * @return Coordenada Y escalonada e deslocada para a tela atual
     */
    static int Y(int valorNativo) { return (int)(valorNativo * escalaGlobal) + offsetY; }
    
    /**
     * @brief Converte um tamanho (largura, altura ou fonte) nativo para o tamanho real na tela atual
     * 
     * @param tamanhoNativo Tamanho na resolução nativa (1920x1280)
     * @return Tamanho escalonado para a tela atual (sem offset, apenas escala)
     */
    static int F(int tamanhoNativo) { return (int)(tamanhoNativo * escalaGlobal); }
};

} // namespace MeuProjeto

#endif