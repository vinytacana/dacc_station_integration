#ifndef CONFIG_LAYOUT_HPP
#define CONFIG_LAYOUT_HPP

#pragma once

#include <iostream>
#include <algorithm> 

namespace MeuProjeto {

struct ConfigLayout {
    // =============================================================
    // VARIÁVEIS DE ESTADO (Dinâmicas)
    // =============================================================
    static int larguraTela;
    static int alturaTela;
    static float escalaGlobal; 
    static int offsetX;        
    static int offsetY;        

    // =============================================================
    // CONSTANTES DE DESIGN (Base: 1920x1280)
    // =============================================================

    static constexpr int LARGURA_NATIVA = 1920;
    static constexpr int ALTURA_NATIVA  = 1280;

    static constexpr int MARGEM_TELA_X = 35;
    static constexpr int MARGEM_TELA_Y = 30;
    
    // Espaçamentos Genéricos
    static constexpr int ESPACO_PEQUENO = 20;
    static constexpr int ESPACO_MEDIO   = 50;
    static constexpr int ESPACO_GRANDE  = 80;

    // -------------------------------------------------------------
    // 1. CABEÇALHO
    // -------------------------------------------------------------
    static constexpr int BOTAO_CONFIG_POS_X   = MARGEM_TELA_X;
    static constexpr int BOTAO_CONFIG_POS_Y   = 29; 
    static constexpr int BOTAO_CONFIG_LARGURA = 130;
    static constexpr int BOTAO_CONFIG_ALTURA  = 130; 
    static constexpr int DISTANCIA_CONFIG_PESQUISA = 290; 
    
    static constexpr int PESQUISA_POS_X   = BOTAO_CONFIG_POS_X + BOTAO_CONFIG_LARGURA + DISTANCIA_CONFIG_PESQUISA;
    static constexpr int PESQUISA_POS_Y   = 45; 
    static constexpr int PESQUISA_LARGURA = 1008;
    static constexpr int PESQUISA_ALTURA  = 98;

    static constexpr int MARGEM_LUPA        = 14; 
    static constexpr int ICONE_LUPA_TAMANHO = 100;
    static constexpr int ICONE_LUPA_POS_X   = PESQUISA_POS_X + PESQUISA_LARGURA - ICONE_LUPA_TAMANHO - MARGEM_LUPA;
    static constexpr int ICONE_LUPA_POS_Y   = PESQUISA_POS_Y + (PESQUISA_ALTURA - ICONE_LUPA_TAMANHO)/2 + 3;

    static constexpr int DISTANCIA_PESQUISA_TEMA = 266; 
    static constexpr int BOTAO_TEMA_POS_X   = PESQUISA_POS_X + PESQUISA_LARGURA + DISTANCIA_PESQUISA_TEMA;
    static constexpr int BOTAO_TEMA_POS_Y   = 29; 
    static constexpr int BOTAO_TEMA_LARGURA = 130;
    static constexpr int BOTAO_TEMA_ALTURA  = 130; 

    // -------------------------------------------------------------
    // 2. ÁREA DE DESTAQUE (HERO)
    // -------------------------------------------------------------
    static constexpr int DESTAQUE_PRINCIPAL_POS_X     = 30;
    static constexpr int DESTAQUE_PRINCIPAL_POS_Y     = 180; 
    static constexpr int DESTAQUE_PRINCIPAL_LARGURA   = 1862;
    static constexpr int DESTAQUE_PRINCIPAL_ALTURA    = 597;

    // Miniaturas
    static constexpr int DESTAQUE_MINI_POS_X_INICIAL  = 70;
    static constexpr int DESTAQUE_MINI_POS_Y          = 597; 
    
    static constexpr int DESTAQUE_MINI_TAMANHO     = 300;
    static constexpr int DESTAQUE_MINI_ESPACO      = 72; 
    static constexpr int DESTAQUE_MINI_PASSO_TOTAL = DESTAQUE_MINI_TAMANHO + DESTAQUE_MINI_ESPACO;

    // -------------------------------------------------------------
    // 3. CATEGORIAS E BOTÃO ALEATÓRIO
    // -------------------------------------------------------------
    static constexpr int DISTANCIA_HERO_CATEGORIA = 23;

    static constexpr int PAINEL_CATEGORIA_POS_X   = 30;
    static constexpr int PAINEL_CATEGORIA_POS_Y   = DESTAQUE_MINI_POS_Y + DESTAQUE_MINI_TAMANHO + DISTANCIA_HERO_CATEGORIA; 
    
    static constexpr int PAINEL_CATEGORIA_LARGURA = 980;
    static constexpr int PAINEL_CATEGORIA_ALTURA  = 100;

    static constexpr int ITEM_CATEGORIA_LARGURA = 140;
    static constexpr int ITEM_CATEGORIA_ALTURA  = 80;
    static constexpr int ITEM_CATEGORIA_ESPACO  = 6;
    static constexpr int ITEM_CATEGORIA_PASSO   = ITEM_CATEGORIA_LARGURA + ITEM_CATEGORIA_ESPACO;
    
    static constexpr int ITEM_CATEGORIA_POS_Y   = PAINEL_CATEGORIA_POS_Y + (PAINEL_CATEGORIA_ALTURA - ITEM_CATEGORIA_ALTURA)/2;
    static constexpr int ITEM_CATEGORIA_POS_X_BASE = 320; 
    static constexpr int CATEGORIA_QTD_VISIVEL  = 4;

    static constexpr int TITULO_CATEGORIA_POS_X = PAINEL_CATEGORIA_POS_X + 10;
    static constexpr int TITULO_CATEGORIA_POS_Y = PAINEL_CATEGORIA_POS_Y + 30;

    static constexpr int DISTANCIA_CATEGORIA_ALEATORIO = 130;
    static constexpr int BOTAO_ALEATORIO_POS_X = PAINEL_CATEGORIA_POS_X + PAINEL_CATEGORIA_LARGURA + DISTANCIA_CATEGORIA_ALEATORIO;
    static constexpr int BOTAO_ALEATORIO_POS_Y = PAINEL_CATEGORIA_POS_Y;
    static constexpr int BOTAO_ALEATORIO_LARGURA = 700;
    static constexpr int BOTAO_ALEATORIO_ALTURA  = 100;

    // -------------------------------------------------------------
    // 4. GRADE DE JOGOS
    // -------------------------------------------------------------
    static constexpr int DISTANCIA_CATEGORIA_GRADE = 50;
    static constexpr int GRADE_JOGOS_Y_INICIAL = PAINEL_CATEGORIA_POS_Y + PAINEL_CATEGORIA_ALTURA + DISTANCIA_CATEGORIA_GRADE;
    
    static constexpr int GRADE_JOGOS_LARGURA_ITEM = 290;
    static constexpr int GRADE_JOGOS_ALTURA_ITEM  = 435;
    static constexpr int GRADE_JOGOS_ESPACO_X     = 72;
    static constexpr int GRADE_JOGOS_ESPACO_Y     = 60;
    static constexpr int GRADE_JOGOS_COLUNAS      = 5;

    // -------------------------------------------------------------
    // 5. JANELA DE DETALHES (JanelaJogo)
    // -------------------------------------------------------------
    
    // Dimensões e Gaps
    static constexpr int JANELA_TOPO_ALTURA      = 100;
    static constexpr int JANELA_PAINEL_LARGURA   = 1920;
    static constexpr int JANELA_PAINEL_ALTURA    = 560;
    
    static constexpr int JANELA_CAPA_LARGURA     = 960;
    static constexpr int JANELA_CAPA_ALTURA      = 540;
    
    static constexpr int JANELA_COLUNA_GAP       = 20; 
    static constexpr int JANELA_DESC_LARGURA     = 770;
    static constexpr int JANELA_DESC_ALTURA      = 350;
    
    static constexpr int JANELA_JOGAR_LARGURA    = 770;
    static constexpr int JANELA_JOGAR_ALTURA     = 120;
    
    // Posições Relativas
    static constexpr int JANELA_PAINEL_POS_X     = 0;
    static constexpr int JANELA_PAINEL_POS_Y     = 120;
    
    static constexpr int JANELA_CAPA_OFFSET_X    = 8; 
    static constexpr int JANELA_CAPA_OFFSET_Y    = 8;
    static constexpr int JANELA_CAPA_POS_X       = JANELA_PAINEL_POS_X + JANELA_CAPA_OFFSET_X; 
    static constexpr int JANELA_CAPA_POS_Y       = JANELA_PAINEL_POS_Y + JANELA_CAPA_OFFSET_Y; 

    static constexpr int JANELA_DESC_POS_X       = JANELA_CAPA_POS_X + JANELA_CAPA_LARGURA + JANELA_COLUNA_GAP; 
    static constexpr int JANELA_DESC_POS_Y       = JANELA_CAPA_POS_Y; 

    static constexpr int JANELA_JOGAR_POS_X      = JANELA_DESC_POS_X; 
    static constexpr int JANELA_JOGAR_POS_Y      = 450; 

    static constexpr int JANELA_TITULO_POS_Y     = 26;
    static constexpr int JANELA_TITULO_FONT      = 48;

    static constexpr int JANELA_FECHAR_TAM       = 80;
    static constexpr int JANELA_FECHAR_POS_X     = 1830;
    static constexpr int JANELA_FECHAR_POS_Y     = 10;
    static constexpr int JANELA_FECHAR_FONT      = 48;

    static constexpr int JANELA_DESC_FONT        = 24;
    static constexpr int JANELA_DESC_LINE_H      = 30;
    static constexpr int JANELA_DESC_PAD         = 10;
    static constexpr int JANELA_JOGAR_FONT       = 42;

    static constexpr int JANELA_CARROSSEL_GAP_Y  = 30; 
    static constexpr int JANELA_CARROSSEL_BG_POS_Y = JANELA_PAINEL_POS_Y + JANELA_PAINEL_ALTURA + JANELA_CARROSSEL_GAP_Y; 
    static constexpr int JANELA_CARROSSEL_BG_ALTURA = 370;

    static constexpr int JANELA_CAPTURA_LARGURA  = 500;
    static constexpr int JANELA_CAPTURA_ALTURA   = 250;
    static constexpr int JANELA_CAPTURA_OFFSET_Y = 80; 
    
    // 1. Definição da Posição Base (usada pelas setas, que são maiores)
    static constexpr int JANELA_SETA_OFFSET_Y    = 80; 
    static constexpr int JANELA_BASE_Y           = JANELA_CARROSSEL_BG_POS_Y + JANELA_SETA_OFFSET_Y;

    // 2. Cálculo Centralizado para a Imagem
    // A seta tem 270px e a imagem 250px. Diferença de 20px.
    // Para centralizar a imagem dentro da altura da seta: descemos 10px.
    static constexpr int JANELA_CAPTURA_POS_Y    = JANELA_BASE_Y + 10; 
    
    static constexpr int JANELA_CAPTURA_GAP      = 520; 

    static constexpr int JANELA_CARROSSEL_TIT_OFFSET_X = 50;
    static constexpr int JANELA_CARROSSEL_TIT_OFFSET_Y = 10;
    static constexpr int JANELA_CARROSSEL_TIT_POS_X = JANELA_CARROSSEL_TIT_OFFSET_X; 
    static constexpr int JANELA_CARROSSEL_TIT_POS_Y = JANELA_CARROSSEL_TIT_OFFSET_Y; 
    static constexpr int JANELA_CARROSSEL_TIT_FONT  = 28;

    // Setas de Navegação
    static constexpr int JANELA_SETA_LARGURA     = 120;
    static constexpr int JANELA_SETA_ALTURA      = 270;
    
    // 3. A seta usa a posição Base sem o deslocamento de +10
    static constexpr int JANELA_SETA_POS_Y       = JANELA_BASE_Y; 
    
    static constexpr int JANELA_SETA_ESQ_POS_X   = 30;
    static constexpr int JANELA_SETA_DIR_POS_X   = 1770;
    static constexpr int JANELA_SETA_FONT        = 56;
    static constexpr int JANELA_SETA_MARGEM      = 160;

    static constexpr int JANELA_LABEL_POS_X_LB   = 55;
    static constexpr int JANELA_LABEL_POS_X_RB   = 1785;
    static constexpr int JANELA_LABEL_OFFSET_Y   = 35;
    static constexpr int JANELA_LABEL_FONT       = 20;

    // -------------------------------------------------------------
    // TECLADO VIRTUAL
    // -------------------------------------------------------------
    static constexpr int TECLADO_BASE_POS_X = 460;
    static constexpr int TECLADO_BASE_POS_Y = 600;
    static constexpr int TECLADO_TECLA_TAMANHO = 60;
    static constexpr int TECLADO_TECLA_ESPACAMENTO = 10;

    // =============================================================
    // FUNÇÕES DE CÁLCULO
    // =============================================================
    
    static void inicializar(int larguraAtual, int alturaAtual) {
        larguraTela = larguraAtual;
        alturaTela = alturaAtual;
        
        float ratioX = (float)larguraAtual / LARGURA_NATIVA;
        float ratioY = (float)alturaAtual / ALTURA_NATIVA;
        escalaGlobal = std::min(ratioX, ratioY);
        
        int larguraOcupada = (int)(LARGURA_NATIVA * escalaGlobal);
        int alturaOcupada  = (int)(ALTURA_NATIVA * escalaGlobal);
        
        offsetX = (larguraAtual - larguraOcupada) / 2;
        offsetY = (alturaAtual - alturaOcupada) / 2;

        std::cout << "[LAYOUT] Escala: " << escalaGlobal << " Offset: " << offsetX << "," << offsetY << std::endl;
    }

    static int X(int valorNativo) { return (int)(valorNativo * escalaGlobal) + offsetX; }
    static int Y(int valorNativo) { return (int)(valorNativo * escalaGlobal) + offsetY; }
    static int F(int tamanhoNativo) { return (int)(tamanhoNativo * escalaGlobal); }
};

} // namespace MeuProjeto

#endif