/**
 * @file Botao.cpp
 * @brief Implementação detalhada da classe Botao para interface gráfica com SDL2.
 * 
 * Este arquivo contém a implementação completa de um componente de botão altamente
 * configurável que suporta:
 * - Renderização de texto e imagens
 * - Sistema de temas dinâmicos (claro/escuro)
 * - Interação via mouse e gamepad
 * - Estados visuais (normal, hover, pressionado, focado)
 * - Formas geométricas variadas (retângulo, círculo, bordas arredondadas)
 * - Sistema de popup para informações detalhadas
 * - Gerenciamento eficiente de recursos através de cache
 */

#include "Botao.hpp"
#include "Janela.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorImagens.hpp"
#include "Utils.hpp" 
#include "GerenciadorTemas.hpp"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <iostream>

using namespace MeuProjeto;

/** 
 * @brief Referência ao gerenciador global de imagens definido no main.cpp.
 * 
 * Esta variável externa permite acesso ao sistema de cache de texturas,
 * evitando o recarregamento desnecessário de imagens idênticas e otimizando
 * o uso de memória da GPU.
 */
extern GerenciadorImagens gerImg;

/**
 * @brief Construtor para Botão de Texto.
 * 
 * Cria um botão focado na exibição de rótulos textuais. Este tipo de botão
 * é ideal para menus, barras de ferramentas e controles de interface.
 * 
 * O construtor inicializa o botão com configurações padrão que incluem:
 * - Integração automática com o sistema de temas (usaTemaPadrao = true)
 * - Cores transparentes iniciais que serão substituídas pelo tema
 * - Fonte em negrito com tamanho 32
 * - Estados de interação zerados (não hover, não pressionado, não focado)
 * 
 * @note Este construtor define usaTemaPadrao como true, o que significa que
 * as cores do botão se adaptarão automaticamente quando o usuário alternar
 * entre o tema Claro e Escuro através do GerenciadorTemas.
 * 
 * @param x Posição X absoluta do botão na tela (canto superior esquerdo)
 * @param y Posição Y absoluta do botão na tela (canto superior esquerdo)
 * @param largura Dimensão horizontal do botão em pixels
 * @param altura Dimensão vertical do botão em pixels
 * @param texto Conteúdo textual a ser exibido no centro do botão
 */
Botao::Botao(int x, int y, int largura, int altura, const std::string& texto) 
    : area{x, y, largura, altura}, texturaImagem(nullptr), texto(texto), 
      corPadrao({0,0,0,0}), corHover({0,0,0,0}), corPressionado({0,0,0,0}),
      usaTemaPadrao(true), 
      hover(false), pressionado(false), focadoPorControle(false), parentJanela(nullptr), nomeJogo(""), descJogo(""), 
      isRound(false), isDestaque(false), raioBordasArredondadas(0),
      tipoFonte(TipoFonte::NEGRITO), tamanhoFonte(32),
      onClickCallback(nullptr)
{
}

/**
 * @brief Construtor para Botão de Imagem (Capas/Banners de jogos).
 * 
 * Cria um botão que exibe uma textura gráfica, tipicamente usado para
 * representar jogos através de suas capas, banners ou ícones. Este tipo
 * de botão é comum em bibliotecas de jogos e galerias de mídia.
 * 
 * Características deste construtor:
 * - Carrega a imagem através do gerenciador de cache (gerImg)
 * - Configura blend mode para suportar transparência alpha
 * - Por padrão, o fundo é transparente para não interferir na arte
 * - usaTemaPadrao = false, pois botões de imagem mantêm identidade visual fixa
 * - Suporta modo "destaque" para banners com transparência especial
 * 
 * @note Este construtor define usaTemaPadrao como false, pois botões de imagem
 * geralmente possuem identidade visual própria ou dependem da arte do jogo,
 * não devendo mudar cores automaticamente com o tema do sistema.
 * 
 * @param renderer O renderizador SDL usado para criar texturas
 * @param x Posição X absoluta do botão
 * @param y Posição Y absoluta do botão
 * @param largura Largura do botão em pixels
 * @param altura Altura do botão em pixels
 * @param imagemPath Caminho completo do arquivo de imagem (PNG, JPG, etc.)
 * @param nome Nome do jogo a ser exibido no popup ao passar o mouse
 * @param desc Descrição detalhada do jogo exibida no popup
 * @param isDestaque Se true, aplica transparência especial de 50% (para banners)
 */
Botao::Botao(SDL_Renderer* renderer, int x, int y, int largura, int altura,
             const std::string& imagemPath, const std::string& nome,
             const std::string& desc, bool isDestaque)
    : area{x, y, largura, altura}, texturaImagem(nullptr), texto(""),
      corPadrao({0,0,0,0}), corHover({96, 97, 99, 255}), corPressionado({131, 131, 133, 255}),
      usaTemaPadrao(false), 
      hover(false), pressionado(false), focadoPorControle(false), parentJanela(nullptr),
      nomeJogo(nome), descJogo(desc), imagemPath(imagemPath), 
      isRound(false), isDestaque(isDestaque),raioBordasArredondadas(0),
      tipoFonte(TipoFonte::NEGRITO), tamanhoFonte(32),
      onClickCallback(nullptr)
{
    // Verifica se o renderer é válido e se o caminho da imagem foi fornecido
    if (renderer && !imagemPath.empty()) {
        // Solicita a textura ao cache global para evitar duplicação de recursos
        texturaImagem = gerImg.carregar(renderer, imagemPath);
        if (texturaImagem) {
            // Habilita o canal alpha para permitir efeitos de transparência
            // Essencial para suavização visual e feedback de interação
            SDL_SetTextureBlendMode(texturaImagem, SDL_BLENDMODE_BLEND);
        }
    }
}

/**
 * @brief Destrutor do botão.
 * 
 * Implementação vazia intencional - a textura não é destruída aqui pois
 * pertence ao gerenciador de cache global (gerImg). O cache é responsável
 * pelo ciclo de vida completo das texturas, permitindo compartilhamento
 * eficiente entre múltiplos botões que usam a mesma imagem.
 */
Botao::~Botao() {
    // A textura não é destruída aqui pois pertence ao cache global (gerImg)
}

/**
 * @brief Define uma cor de texto personalizada que sobrescreve o tema.
 * 
 * Esta função permite especificar uma cor de texto customizada que será
 * usada independentemente das configurações do tema ativo. Útil para
 * criar botões com identidade visual específica ou destacar elementos
 * importantes da interface.
 * 
 * @param cor Estrutura SDL_Color com os componentes RGBA (Red, Green, Blue, Alpha)
 */
void Botao::setCorTexto(const SDL_Color& cor) {
    corTextoCustom = cor;
    usaCorTextoCustom = true;
}

/**
 * @brief Define ou atualiza o texto exibido no botão.
 * 
 * Permite modificar dinamicamente o conteúdo textual do botão após
 * sua criação. Útil para botões que mudam de estado (ON/OFF, 
 * Ativar/Desativar, etc).
 * 
 * A renderização do novo texto aproveita o sistema de cache de texturas
 * do GerenciadorTexturasTexto, evitando recriação desnecessária de recursos.
 * 
 * @param novoTexto Nova string a ser exibida no centro do botão
 * 
 * @note Esta função apenas atualiza o atributo interno. A renderização
 * real ocorre no próximo frame ao chamar desenhar().
 */
void Botao::setTexto(const std::string& novoTexto) {
    texto = novoTexto;
}

/**
 * @brief Define manualmente as cores dos estados do botão.
 * 
 * Permite controle total sobre a aparência visual do botão ao especificar
 * cores distintas para cada estado de interação. Esta função desativa
 * automaticamente a integração com o sistema de temas.
 * 
 * Estados visuais:
 * - Padrão: Estado de repouso, sem interação do usuário
 * - Hover: Cursor do mouse sobre o botão
 * - Pressionado: Botão sendo clicado/ativado
 * 
 * @important Ao chamar esta função, a flag usaTemaPadrao é desativada.
 * O botão manterá estas cores permanentemente, ignorando mudanças
 * globais de tema (claro/escuro) realizadas pelo usuário.
 * 
 * @param padrao Cor do botão em estado normal (sem interação)
 * @param hover Cor do botão quando o mouse está sobre ele
 * @param press Cor do botão quando está sendo pressionado
 */
void Botao::setCor(const SDL_Color& padrao, const SDL_Color& hover, const SDL_Color& press) { 
    corPadrao = padrao; 
    corHover = hover; 
    corPressionado = press; 
    usaTemaPadrao = false; 
}

/**
 * @brief Configura o raio das bordas arredondadas do botão.
 * 
 * Define o arredondamento dos cantos do botão, criando uma aparência
 * mais moderna e suave. Um raio maior resulta em cantos mais arredondados.
 * 
 * @param raio Raio do arredondamento em pixels (0 = cantos retos, valores maiores = mais arredondado)
 */
void Botao::setRetanguloBordasArredondadas(int raio) {
    raioBordasArredondadas = raio;
}

/**
 * @brief Método de renderização simplificado sem offset.
 * 
 * Versão conveniente do método de desenho que assume offset zero,
 * útil para botões em posições fixas sem scroll ou movimento.
 * 
 * @param renderer O renderizador SDL usado para desenhar o botão
 */
void Botao::desenhar(SDL_Renderer* renderer) {
    desenhar(renderer, 0, 0);
}

/**
 * @brief Método principal de renderização do Botão.
 * 
 * Este é o núcleo visual do componente, responsável por renderizar todas
 * as camadas do botão em ordem específica para criar a aparência final.
 * 
 * PIPELINE DE RENDERIZAÇÃO (ordem de camadas):
 * 
 * 1. **Sincronização de Tema**: 
 *    - Se usaTemaPadrao = true, busca cores atualizadas do GerenciadorTemas
 *    - Permite mudança dinâmica entre tema claro/escuro
 * 
 * 2. **Fundo Geométrico (Background Layer)**:
 *    - Desenha forma base: retângulo, círculo ou retângulo arredondado
 *    - Cor determinada pelo estado atual (normal, hover, pressionado)
 *    - Suporta transparência através do canal alpha
 * 
 * 3. **Camada de Imagem (Arte do Jogo)**:
 *    - Se texturaImagem existe, renderiza sobre o fundo
 *    - Aplica modulação de Alpha baseada no estado:
 *      * Destaque (Banner): 128 (50% transparência)
 *      * Hover/Focado/Pressionado: 180 (levemente escurecido)
 *      * Normal: 255 (opaco)
 * 
 * 4. **Camada de Texto**:
 *    - Renderizada apenas se não houver imagem
 *    - Centralizada perfeitamente na área do botão
 *    - Usa fonte configurada (tipo e tamanho)
 *    - Cor do tema ou customizada
 * 
 * 5. **Feedback de Controle (Gamepad Focus)**:
 *    - Borda externa vibrante quando focado por controle
 *    - Múltiplas camadas para efeito anti-aliasing suave
 *    - Adapta-se à geometria do botão (círculo ou retângulo)
 * 
 * @param renderer O renderizador SDL usado para todas as operações de desenho
 * @param offsetX Deslocamento horizontal em pixels (usado para scroll ou câmera)
 * @param offsetY Deslocamento vertical em pixels (usado para scroll ou câmera)
 */
void Botao::desenhar(SDL_Renderer* renderer, int offsetX, int offsetY) {
    // Validação de segurança - evita crashes se renderer for nulo
    if (!renderer) return;

    // === FASE 1: SINCRONIZAÇÃO DE TEMA ===
    // Lógica de Temas: Atualiza cores em tempo real se não houver override manual
    if (usaTemaPadrao) {
        auto& tema = GerenciadorTemas::getInstance();
        corPadrao = tema.getCorBotaoNormal();
        corHover = tema.getCorBotaoHover();
        corPressionado = tema.getCorBotaoPressionado();
    }

    // Aplica o deslocamento de scroll à área física do botão
    // Permite que o botão se mova com a câmera/scroll da interface
    SDL_Rect adjustedArea = {area.x + offsetX, area.y + offsetY, area.w, area.h};
    
    // Determina qual cor usar com base no estado atual de interação
    SDL_Color corAtual = determineCurrentColor();

    // === FASE 2: RENDERIZAÇÃO DO FUNDO (Background Layer) ===
    // Apenas desenha se a cor tiver alguma opacidade (alpha > 0)
    if (corAtual.a > 0) {
        // Ativa blend mode para suportar transparência
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, corAtual.r, corAtual.g, corAtual.b, corAtual.a);
        
        if (isRound) {
            // GEOMETRIA CIRCULAR: Para botões redondos (ex: ícones circulares)
            // Calcula centro e raio baseado nas dimensões do botão
            int centroX = adjustedArea.x + adjustedArea.w / 2;
            int centroY = adjustedArea.y + adjustedArea.h / 2;
            int raio = (adjustedArea.w < adjustedArea.h) ? (adjustedArea.w / 2) : (adjustedArea.h / 2);
            // Usa SDL_gfx para desenhar círculo preenchido com anti-aliasing
            filledCircleRGBA(renderer, centroX, centroY, raio, corAtual.r, corAtual.g, corAtual.b, corAtual.a);
        } else if (raioBordasArredondadas > 0) {
            // GEOMETRIA ARREDONDADA: Retângulo com cantos suavizados
            // Usa SDL_gfx para criar aparência moderna e polida
            roundedBoxRGBA(renderer, 
                           adjustedArea.x, adjustedArea.y, 
                           adjustedArea.x + adjustedArea.w, adjustedArea.y + adjustedArea.h, 
                           raioBordasArredondadas, 
                           corAtual.r, corAtual.g, corAtual.b, corAtual.a); 
        } else {
            // GEOMETRIA PADRÃO: Retângulo simples com cantos retos
            SDL_SetRenderDrawColor(renderer, corAtual.r, corAtual.g, corAtual.b, corAtual.a);
            SDL_RenderFillRect(renderer, &adjustedArea);
        }
    }

    // === FASE 3: RENDERIZAÇÃO DA IMAGEM (Arte do Jogo) ===
    if (texturaImagem) {
        /**
         * @section Modulação_Alpha
         * 
         * O canal Alpha da textura é ajustado dinamicamente conforme o estado
         * de interação para fornecer feedback visual ao usuário:
         * 
         * - **Destaque (Banner)**: Alpha = 128 (50% transparência)
         *   Usado para banners de fundo que não devem chamar muita atenção
         * 
         * - **Hover/Focado/Pressionado**: Alpha = 180 (~70% opacidade)
         *   Escurece levemente a imagem para indicar interação ativa
         * 
         * - **Normal**: Alpha = 255 (100% opaco)
         *   Imagem exibida em sua forma original sem modificações
         */
        Uint8 alphaValue = (isDestaque) ? 128 : ((hover || pressionado || focadoPorControle) ? 180 : 255);
        SDL_SetTextureAlphaMod(texturaImagem, alphaValue);
        
        // Renderiza a textura preenchendo completamente a área do botão
        SDL_RenderCopy(renderer, texturaImagem, nullptr, &adjustedArea);
        
        // Restaura alpha para 255 para não afetar próximas renderizações
        SDL_SetTextureAlphaMod(texturaImagem, 255);
    }
    // === FASE 4: RENDERIZAÇÃO DO TEXTO ===
    // Texto é renderizado apenas se não houver imagem (prioridade para imagem)
    else if (!texto.empty()) {
        renderizarTextoCentralizado(renderer, adjustedArea);
    }
    
    // === FASE 5: BORDA DE DESTAQUE DO CONTROLE (Gamepad Focus) ===
    // Desenha uma borda colorida quando o botão está focado via gamepad/teclado
    if (focadoPorControle) {
        SDL_Color corDestaque = GerenciadorTemas::getInstance().getCorDestaque();
        
        if (isRound) {
            // BORDA CIRCULAR: Para botões redondos
            int centroX = adjustedArea.x + adjustedArea.w / 2;
            int centroY = adjustedArea.y + adjustedArea.h / 2;
            int raio = (adjustedArea.w < adjustedArea.h) ? (adjustedArea.w / 2) : (adjustedArea.h / 2);
            
            // Desenha 3 círculos concêntricos com raios incrementais
            // Simula uma borda espessa e suave com efeito anti-aliasing
            aacircleRGBA(renderer, centroX, centroY, raio, corDestaque.r, corDestaque.g, corDestaque.b, 255);
            aacircleRGBA(renderer, centroX, centroY, raio + 1, corDestaque.r, corDestaque.g, corDestaque.b, 255);
            aacircleRGBA(renderer, centroX, centroY, raio + 2, corDestaque.r, corDestaque.g, corDestaque.b, 255);
        } else {
            if (raioBordasArredondadas > 0) {
                // BORDA ARREDONDADA: Para botões com cantos suavizados
                int raioDestaque = raioBordasArredondadas + 2;

                // Correção de alpha: usa 255 se o fundo for transparente
                Uint8 alphaCorrigido = (corAtual.a == 0) ? 255 : corAtual.a;

                // Desenha duas bordas arredondadas ligeiramente deslocadas
                // Cria efeito de profundidade e melhor visibilidade
                roundedRectangleRGBA(renderer, 
                    adjustedArea.x - 3, adjustedArea.y - 3, 
                    adjustedArea.x + adjustedArea.w + 3, adjustedArea.y + adjustedArea.h + 3, 
                    raioDestaque, 
                    corDestaque.r, corDestaque.g, corDestaque.b, alphaCorrigido);
                
                roundedRectangleRGBA(renderer, 
                    adjustedArea.x - 2, adjustedArea.y - 2, 
                    adjustedArea.x + adjustedArea.w + 2, adjustedArea.y + adjustedArea.h + 2, 
                    raioDestaque - 1, 
                    corDestaque.r, corDestaque.g, corDestaque.b, alphaCorrigido);
            } else {
                // BORDA RETA: Para retângulos tradicionais
                // Desenha duas bordas retangulares para espessura visual
                rectangleRGBA(renderer, 
                    adjustedArea.x - 3, adjustedArea.y - 3, 
                    adjustedArea.x + adjustedArea.w + 3, adjustedArea.y + adjustedArea.h + 3, 
                    corDestaque.r, corDestaque.g, corDestaque.b, 255);

                rectangleRGBA(renderer, 
                    adjustedArea.x - 2, adjustedArea.y - 2, 
                    adjustedArea.x + adjustedArea.w + 2, adjustedArea.y + adjustedArea.h + 2, 
                    corDestaque.r, corDestaque.g, corDestaque.b, 255);
            }
        }
    }
}

/**
 * @brief Máquina de estados para determinar a cor de renderização no frame atual.
 * 
 * Implementa uma lógica de priorização clara para decidir qual cor usar
 * na renderização do botão baseada nos estados de interação ativos.
 * 
 * Ordem de prioridade (do maior para o menor):
 * 1. Pressionado: Quando o botão está sendo clicado/ativado
 * 2. Hover/Focado: Quando o mouse está sobre o botão OU está focado por controle
 * 3. Normal: Estado de repouso padrão
 * 
 * @return SDL_Color A cor apropriada para o estado atual do botão
 */
SDL_Color Botao::determineCurrentColor() {
    if (pressionado) return corPressionado;
    else if (hover || focadoPorControle) return corHover;
    return corPadrao;
}

/**
 * @brief Renderiza o texto perfeitamente centralizado na área do botão.
 * 
 * Executa o processo completo de renderização de texto usando SDL_ttf:
 * 1. Carrega a fonte através do gerenciador de cache
 * 2. Seleciona a cor apropriada (custom ou tema)
 * 3. Renderiza o texto em uma surface de alta qualidade (Blended)
 * 4. Converte a surface em textura GPU
 * 5. Calcula posição centralizada usando geometria
 * 6. Desenha a textura e libera recursos temporários
 * 
 * Fórmulas de centralização:
 * - X_centralizado = X_botao + (Largura_botao - Largura_texto) / 2
 * - Y_centralizado = Y_botao + (Altura_botao - Altura_texto) / 2
 * 
 * @param renderer O renderizador SDL para criar e desenhar a textura de texto
 * @param area Retângulo que define a área física do botão (já com offset aplicado)
 */
void Botao::renderizarTextoCentralizado(SDL_Renderer* renderer, const SDL_Rect& area) {
    // Gerenciador de fontes estático para cache e reutilização
    static GerenciadorFontes gerenciadorFontesBotoes;

    // Obtém o caminho da fonte baseado no tipo configurado
    const std::string& fontePath = ConfigFontes::obterCaminho(tipoFonte);
    
    // Carrega a fonte do cache (ou carrega se não estiver em cache)
    TTF_Font* font = gerenciadorFontesBotoes.carregar(fontePath, tamanhoFonte);
    if (!font) return; // Falha silenciosa se fonte não disponível

    // Seleção de cor: Usa cor customizada se definida, senão usa cor do tema
    SDL_Color corTexto = (usaCorTextoCustom) ? corTextoCustom : GerenciadorTemas::getInstance().getCorTextoNegrito();

    // Renderização Blended: Alta qualidade com suavização anti-aliasing
    // Suporta UTF-8 para caracteres internacionais e acentuação
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, texto.c_str(), corTexto);
    if (!surface) return;

    // Converte surface CPU para textura GPU para renderização acelerada
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) { 
        SDL_FreeSurface(surface); 
        return; 
    }

    // Obtém dimensões reais do texto renderizado
    int texW = surface->w, texH = surface->h;
    
    // Cálculo do retângulo de destino para centralização perfeita
    // Posiciona o texto exatamente no centro geométrico do botão
    SDL_Rect rectTexto = { 
        area.x + (area.w - texW) / 2,  // Centralização horizontal
        area.y + (area.h - texH) / 2,  // Centralização vertical
        texW,                          // Largura original do texto
        texH                           // Altura original do texto
    };

    // Renderiza a textura de texto na posição calculada
    SDL_RenderCopy(renderer, texture, nullptr, &rectTexto);
    
    // Liberação de recursos temporários (textura e surface não são cacheados)
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

/**
 * @brief Distribuidor central de eventos (Event Dispatcher) para o botão.
 * 
 * Implementa o padrão de design "Chain of Responsibility" para processar
 * diferentes tipos de eventos de entrada. Cada tipo de evento é delegado
 * para um handler especializado que conhece a lógica específica.
 * 
 * Eventos suportados:
 * - SDL_MOUSEMOTION: Movimento do mouse (hover)
 * - SDL_MOUSEBUTTONDOWN: Botão do mouse pressionado
 * - SDL_MOUSEBUTTONUP: Botão do mouse solto
 * - SDL_CONTROLLERBUTTONDOWN: Botão do gamepad pressionado
 * - SDL_CONTROLLERBUTTONUP: Botão do gamepad solto
 * 
 * @param evento Estrutura SDL_Event contendo os dados do evento capturado
 * @param offsetX Deslocamento horizontal do scroll/câmera (para ajuste de coordenadas)
 * @param offsetY Deslocamento vertical do scroll/câmera (para ajuste de coordenadas)
 * @return true se o botão interagiu com o evento e o "consumiu" (evita propagação)
 * @return false se o evento não foi tratado por este botão
 */
bool Botao::tratarEvento(SDL_Event& evento, int offsetX, int offsetY) {
    bool eventoConsumido = false;
    
    // Delega para o handler apropriado baseado no tipo de evento
    if (evento.type == SDL_MOUSEMOTION) 
        eventoConsumido = handleMouseMotion(evento, offsetX, offsetY);
    else if (evento.type == SDL_MOUSEBUTTONDOWN && evento.button.button == SDL_BUTTON_LEFT) 
        eventoConsumido = handleMouseButtonDown(evento, offsetX, offsetY);
    else if (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT) 
        eventoConsumido = handleMouseButtonUp(evento, offsetX, offsetY);
    else if (evento.type == SDL_CONTROLLERBUTTONDOWN || evento.type == SDL_CONTROLLERBUTTONUP) 
        eventoConsumido = handleGamepadButton(evento);
    
    return eventoConsumido;
}

/**
 * @brief Gerencia o movimento do mouse e a lógica de POPUP.
 * 
 * Este handler é responsável por detectar quando o cursor entra ou sai
 * da área do botão, atualizando o estado de hover e gerenciando a
 * exibição de popups informativos para botões de jogos.
 * 
 * Comportamento:
 * - **Mouse Entra**: Se for um botão de jogo (sem texto), solicita à
 *   janela pai que abra um popup com nome e descrição do jogo
 * - **Mouse Sai**: Fecha qualquer popup aberto e desativa hover
 * 
 * @note A detecção de "botão de jogo" é feita verificando se nomeJogo
 * está preenchido E texto está vazio (botão só com imagem).
 * 
 * @param evento Estrutura SDL_Event contendo coordenadas do mouse
 * @param offsetX Deslocamento horizontal para ajustar coordenadas de scroll
 * @param offsetY Deslocamento vertical para ajustar coordenadas de scroll
 * @return true se um popup foi aberto (evento consumido)
 * @return false se nenhum popup foi aberto
 */
bool Botao::handleMouseMotion(SDL_Event& evento, int offsetX, int offsetY) {
    // Converte coordenadas da tela para coordenadas locais (considerando scroll)
    int x = evento.motion.x - offsetX; 
    int y = evento.motion.y - offsetY;
    
    // Testa se o cursor está dentro da área do botão (usa geometria apropriada)
    bool mouseDentro = contemPonto(x, y);
    bool abriuPopup = false;

    // TRANSIÇÃO: Mouse entrando no botão
    if (mouseDentro && !hover) { 
        // Verifica se é um botão de jogo que deve mostrar popup
        if (parentJanela && !nomeJogo.empty() && texto.empty()) {
            // Chamada simplificada para abrir popup com informações do jogo
            parentJanela->abrirPopup(nomeJogo, descJogo, area.x, area.y, area.w, area.h);
            abriuPopup = true;
        }
    } 
    // TRANSIÇÃO: Mouse saindo do botão
    else if (!mouseDentro && hover) { 
        // Fecha popup se houver algum aberto
        if (parentJanela) parentJanela->fecharPopup();
    }
    
    // Atualiza estado de hover para o próximo frame
    hover = mouseDentro; 
    return abriuPopup;
}

/**
 * @brief Gerencia o evento de pressionar o botão do mouse.

 * 
 * Detecta quando o usuário pressiona o botão esquerdo do mouse sobre
 * este botão, marcando o início de uma interação de clique.
 * 
 * @param evento Estrutura SDL_Event contendo coordenadas do clique
 * @param offsetX Deslocamento horizontal para ajuste de scroll
 * @param offsetY Deslocamento vertical para ajuste de scroll
 * @return true se o clique ocorreu sobre este botão (evento consumido)
 * @return false se o clique foi fora da área do botão
 */
bool Botao::handleMouseButtonDown(SDL_Event& evento, int offsetX, int offsetY) {
    // Converte coordenadas do evento para espaço local do botão
    int x = evento.button.x - offsetX; 
    int y = evento.button.y - offsetY;
    
    // Testa colisão e marca como pressionado se positivo
    if (contemPonto(x, y)) { 
        pressionado = true; 
        return true; 
    }
    return false;
}

/**
 * @brief Gerencia o evento de soltar o botão do mouse (conclusão do clique).
 * 
 * Detecta quando o usuário solta o botão esquerdo do mouse. Se o mouse
 * ainda estiver sobre o botão E o botão estava pressionado, isso
 * configura um "clique completo" que dispara a ação do botão.
 * 
 * Padrão de "Clique Completo":
 * - Requer que o botão tenha sido pressionado inicialmente neste botão
 * - Requer que o mouse ainda esteja sobre o botão ao soltar
 * - Permite que o usuário cancele clicando e arrastando para fora
 * 
 * @param evento Estrutura SDL_Event contendo coordenadas ao soltar o botão
 * @param offsetX Deslocamento horizontal para ajuste de scroll
 * @param offsetY Deslocamento vertical para ajuste de scroll
 * @return true se um clique completo foi detectado e a ação foi disparada
 * @return false se não houve clique completo
 */
bool Botao::handleMouseButtonUp(SDL_Event& evento, int offsetX, int offsetY) {
    // Converte coordenadas do evento para espaço local
    int x = evento.button.x - offsetX; 
    int y = evento.button.y - offsetY;
    
    // Testa se é um clique completo (mouse sobre botão + estava pressionado)
    if (contemPonto(x, y) && pressionado) {
        pressionado = false;
        
        // Dispara a ação de callback associada ao botão (se existir)
        if (onClickCallback) onClickCallback();
        return true;
    }
    
    // Reseta estado de pressionado mesmo se o mouse saiu da área
    pressionado = false;
    return false;
}

/**
 * @brief Gerencia a interação via controle de jogo (Gamepad).
 * 
 * Processa eventos de botões do gamepad, mapeando o botão 'A' (ou Cross no
 * PlayStation) para executar a mesma ação que o clique esquerdo do mouse.
 * 
 * Este handler só processa eventos se o botão estiver focado via sistema
 * de navegação por controle. A navegação direcional (D-pad/Analógico) é
 * gerenciada pela janela pai.
 * 
 * Mapeamento:
 * - SDL_CONTROLLER_BUTTON_A: Botão 'A' (Xbox) / 'Cross' (PlayStation)
 * - Pressionar: Ativa estado visual de pressionado
 * - Soltar: Executa callback e retorna ao estado normal
 * 
 * @param evento Estrutura SDL_Event contendo dados do botão do controle
 * @return true se o botão focado processou o evento do controle
 * @return false se o botão não está focado ou evento não é relevante
 */
bool Botao::handleGamepadButton(SDL_Event& evento) {
    // Só processa se este botão está focado no momento
    if (!focadoPorControle) return false;
    
    if (evento.type == SDL_CONTROLLERBUTTONDOWN) {
        // Botão 'A' pressionado: Ativa feedback visual
        if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_A) { 
            pressionado = true; 
            return true; 
        }
    } else if (evento.type == SDL_CONTROLLERBUTTONUP) {
        // Botão 'A' solto: Executa ação se estava pressionado
        if (evento.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
            if (pressionado) {
                pressionado = false;
                
                // Dispara callback (equivalente ao clique do mouse)
                if (onClickCallback) onClickCallback();
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Define o estado de foco do controle e gerencia popups associados.
 * 
 * Esta função é chamada pelo sistema de navegação da janela pai quando
 * o foco do controle muda de um botão para outro. Gerencia tanto o
 * estado visual quanto a lógica de popup.
 * 
 * Comportamento:
 * - **Recebe foco**: Se for botão de jogo, abre popup com informações
 * - **Perde foco**: Fecha popup e reseta estado de pressionado
 * 
 * @note Funciona de forma análoga ao mouse motion, mas disparado pela
 * navegação direcional do controle (D-pad ou analógico).
 * 
 * @param focado true para focar este botão, false para remover foco
 */
void Botao::setFocado(bool focado) {
    // Armazena estado anterior para detectar transições
    bool estadoAnterior = focadoPorControle;
    focadoPorControle = focado;
    
    // TRANSIÇÃO: Recebendo foco
    if (focado && !estadoAnterior) {
        // Abre popup se for botão de jogo (sem texto)
        if (parentJanela && !nomeJogo.empty() && texto.empty()) {
            parentJanela->abrirPopup(nomeJogo, descJogo, area.x, area.y, area.w, area.h);
        }
    } 
    // TRANSIÇÃO: Perdendo foco
    else if (!focado && estadoAnterior) {
        // Limpa estado e fecha popup
        if (parentJanela) parentJanela->fecharPopup();
        pressionado = false; // Cancela qualquer pressão pendente
    }
}

/**
 * @brief Ativa visualmente o botão via controle.
 * 
 * Marca o botão como pressionado se ele estiver focado. Usado para
 * feedback visual quando o usuário mantém pressionado o botão do controle.
 */
void Botao::ativarPorControle() { 
    if (focadoPorControle) pressionado = true; 
}

/**
 * @brief Desativa o estado visual de pressionado.
 * 
 * Remove o feedback visual de pressão. Usado quando o usuário solta
 * o botão do controle ou cancela a ação.
 */
void Botao::desativarPorControle() { 
    pressionado = false; 
}

/**
 * @brief Executa a ação do botão imediatamente.
 * 
 * Dispara o callback associado ao botão se ele estiver focado.
 * Útil para implementar atalhos de teclado ou comandos programáticos.
 * 
 * @return true se a ação foi executada com sucesso
 * @return false se o botão não está focado ou não tem callback
 */
bool Botao::executarAcaoPorControle() {
    if (focadoPorControle && onClickCallback) { 
        onClickCallback(); 
        return true; 
    }
    return false;
}

/**
 * @brief Define a textura de imagem do botão manualmente.
 * 
 * Permite substituir ou definir a textura após a construção do botão.
 * Automaticamente configura o blend mode para suportar transparência.
 * 
 * @param textura Ponteiro para a textura SDL (gerenciada externamente)
 */
void Botao::setTexturaImagem(SDL_Texture* textura) { 
    texturaImagem = textura; 
    if(texturaImagem) {
        // Configura blend mode para suportar canal alpha
        SDL_SetTextureBlendMode(texturaImagem, SDL_BLENDMODE_BLEND); 
    }
}

/**
 * @brief Define a janela pai responsável por gerenciar popups.
 * 
 * Estabelece a conexão entre o botão e sua janela container,
 * permitindo que o botão solicite abertura/fechamento de popups.
 * 
 * @param janela Ponteiro para a janela que contém este botão
 */
void Botao::setParentJanela(Janela* janela) { 
    parentJanela = janela; 
}

/**
 * @brief Define se o botão deve ser renderizado como círculo.
 * 
 * Altera a geometria de renderização e detecção de colisão para
 * forma circular. Útil para botões de ícone redondos.
 * 
 * @param round true para círculo, false para retângulo
 */
void Botao::setIsRound(bool round) { 
    isRound = round; 
}

/**
 * @brief Detecção de colisão (Hitbox) adaptativa à geometria.
 * 
 * Implementa dois algoritmos de detecção de ponto conforme a geometria:
 * 
 * **Geometria Circular**:
 * Usa a fórmula da distância Euclidiana:
 * - Calcula: dx = x - centro_x; dy = y - centro_y
 * - Testa: (dx² + dy²) <= raio²
 * - Mais preciso para botões redondos
 * 
 * **Geometria Retangular**:
 * Usa comparação de limites (AABB - Axis-Aligned Bounding Box):
 * - Testa: x >= left && x <= right && y >= top && y <= bottom
 * - Mais eficiente computacionalmente
 * 
 * @param x Coordenada X do ponto a testar (já ajustada para scroll)
 * @param y Coordenada Y do ponto a testar (já ajustada para scroll)
 * @return true se o ponto está dentro da área do botão
 * @return false se o ponto está fora da área do botão
 */
bool Botao::contemPonto(int x, int y) {
    if (isRound) {
        // ALGORITMO CIRCULAR: Distância euclidiana
        int cx = area.x + area.w/2;  // Centro X
        int cy = area.y + area.h/2;  // Centro Y
        int r = area.w/2;            // Raio (assume largura mínima)
        
        // Calcula diferenças (vetores)
        int dx = x - cx;
        int dy = y - cy;
        
        // Testa se a distância ao quadrado é menor ou igual ao raio ao quadrado
        // (evita cálculo de raiz quadrada para performance)
        return (dx*dx + dy*dy) <= (r*r);
    }
    
    // ALGORITMO RETANGULAR: AABB (Axis-Aligned Bounding Box)
    return x >= area.x && x <= area.x + area.w && 
           y >= area.y && y <= area.y + area.h;
}