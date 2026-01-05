#include "JanelaJogo.hpp"
#include "GerenciadorFontes.hpp"
#include "GerenciadorImagens.hpp"
#include <iostream>
#include <sstream>
#include "Utils.hpp"
#include <SDL2/SDL.h>
// #include <cstdlib> // Para system()

using namespace MeuProjeto;

extern GerenciadorImagens gerImg;
// Variável estática não utilizada, mas mantida no escopo original
static GerenciadorFontes gerenciadorFontesJanela;

/**
 * @brief Construtor da Janela de Jogo.
 */
JanelaJogo::JanelaJogo(SDL_Renderer* renderer, const std::string& nomeJogo, const std::string& imagemPath,
                       const std::string& descricao, const std::vector<std::string>& capturasPaths)
    : renderer(renderer), imagemJogo(nullptr), nomeJogo(nomeJogo), descricao(descricao),
      botaoJogar(1080, 450, 770, 120, "JOGAR"),
      botaoFechar(1830, 10, 80, 80, "X"),
      capturaIndex(0),
      areaJanela{0, 0, 1920, 1080},
      areaTopo{0, 0, 1920, 100},
      areaFundo{50, 120, 1820, 560},
      areaImagem{70, 140, 990, 520},
      areaDescricao{1080, 140, 770, 350},
      areaFundoCapturas{0, 710, 1920, 370},
      setaEsquerda(30, CAPTURA_POS_Y, 120, CAPTURA_ALTURA, "<"),
      setaDireita(1770, CAPTURA_POS_Y, 120, CAPTURA_ALTURA, ">")
{
    // Carrega a imagem principal
    imagemJogo = gerImg.carregar(renderer, imagemPath.c_str());
    if (imagemJogo) {
        SDL_SetTextureBlendMode(imagemJogo, SDL_BLENDMODE_BLEND);
    }

    // Carrega as texturas das capturas de tela
    for (const auto& path : capturasPaths) {
        SDL_Texture* tex = gerImg.carregar(renderer, path.c_str());
        if (tex) {
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            capturasTexturas.push_back(tex);
        }
    }

    // Configurar cores dos botões
    botaoJogar.setCor({59, 59, 61, 255}, {95, 95, 97, 255}, {78, 78, 80, 255});
    botaoFechar.setCor({200, 20, 20, 255}, {250, 50, 50, 255}, {180, 0, 0, 255});
    setaEsquerda.setCor({119, 118, 123, 255}, {96, 97, 99, 255}, {131, 131, 133, 255});
    setaDireita.setCor({119, 118, 123, 255}, {96, 97, 99, 255}, {131, 131, 133, 133});

    // Configurar fontes dos botões
    botaoJogar.setFonte(TipoFonte::NEGRITO);
    botaoJogar.setTamanhoFonte(42);

    botaoFechar.setFonte(TipoFonte::NORMAL);
    botaoFechar.setTamanhoFonte(48);

    setaEsquerda.setFonte(TipoFonte::NORMAL);
    setaEsquerda.setTamanhoFonte(56);

    setaDireita.setFonte(TipoFonte::NORMAL);
    setaDireita.setTamanhoFonte(56);
}

/**
 * @brief Destrutor.
 */
JanelaJogo::~JanelaJogo() {
    std::cout << "[DEBUG JanelaJogo] Destruidor chamado." << std::endl;
}

/**
 * @brief Trata eventos de mouse e teclado.
 *
 * @param evento A estrutura SDL_Event.
 * @param offsetX Deslocamento X.
 * @param offsetY Deslocamento Y.
 * @return Retorna @c true se o evento indica que a janela deve ser fechada.
 */
bool JanelaJogo::tratarEvento(SDL_Event& evento, int offsetX, int offsetY) {
    // BOTÃO FECHAR (X)
    botaoFechar.tratarEvento(evento, 0, 0);
    if (evento.type == SDL_MOUSEBUTTONUP && evento.button.button == SDL_BUTTON_LEFT) {
        int mouseX = evento.button.x;
        int mouseY = evento.button.y;
        
        if (botaoFechar.contemPonto(mouseX, mouseY)) {
            std::cout << "  [DEBUG] ✓ Botão X clicado - fechando janela!" << std::endl;
            return true;
        }
    }
    
    // BOTÃO JOGAR 
    if (botaoJogar.tratarEvento(evento, 0, 0)) {
        if (evento.type == SDL_MOUSEBUTTONUP) {
            std::cout << "  [DEBUG] ✓ Botão JOGAR clicado!" << std::endl;
            // *** NOVA FUNCIONALIDADE: Executar o jogo ***
            executarJogo();
            // Não fecha a janela, apenas executa o jogo
        }
    }

    // SETAS (Carrossel)
    setaEsquerda.tratarEvento(evento, 0, 0);
    setaDireita.tratarEvento(evento, 0, 0);
    
    if (evento.type == SDL_MOUSEBUTTONUP) {
        int mouseX = evento.button.x;
        int mouseY = evento.button.y;
        
        int numCapturas = capturasTexturas.size();
        
        if (setaEsquerda.contemPonto(mouseX, mouseY) && numCapturas > 0) {
            // Navegação circular para a esquerda
            capturaIndex = (capturaIndex - 1 + numCapturas) % numCapturas;
        }
        else if (setaDireita.contemPonto(mouseX, mouseY) && numCapturas > 0) {
            // Navegação circular para a direita
            capturaIndex = (capturaIndex + 1) % numCapturas;
        }
    }

    // ===== TECLA ESC =====
    if (evento.type == SDL_KEYDOWN && evento.key.keysym.sym == SDLK_ESCAPE) {
        std::cout << "  [DEBUG] ESC pressionado - fechando janela!" << std::endl;
        return true;
    }

    return false;
}

/**
 * @brief Inicia o processo de execução do jogo.
 *
 * Esta função deve conter a lógica de chamada do sistema operacional
 * ou API de launcher para iniciar o jogo.
 */
void JanelaJogo::executarJogo() {
    std::cout << "[LAUNCHER] Tentando iniciar o jogo: " << nomeJogo << std::endl;
    
    // --- PONTO DE IMPLEMENTAÇÃO ---
    // A lógica de execução depende da sua plataforma e como os jogos estão
    // armazenados.
    
    /*
    Exemplo de execução de comando de sistema (usando <cstdlib>):
    
    std::string comando = "caminho/para/executavel/" + nomeJogo + ".exe";
    int resultado = system(comando.c_str());
    
    if (resultado != 0) {
        std::cerr << "[LAUNCHER ERRO] Falha ao executar o comando: " << resultado << std::endl;
    }
    
    Exemplo de uso de SDL_OpenURL (útil para links de launchers como Steam):
    
    std::string steamUrl = "steam://run/APPID_DO_JOGO"; 
    SDL_OpenURL(steamUrl.c_str());
    */
    
    std::cout << "[LAUNCHER] Jogo '" << nomeJogo << "' seria executado aqui." << std::endl;
    std::cout << "[LAUNCHER] Implemente a lógica de execução nesta função." << std::endl;
}

/**
 * @brief Desenha todos os elementos da janela.
 */
void JanelaJogo::desenhar(int offsetX, int offsetY) {
    desenharFundo();
    desenharTopo();
    desenharAreaFundo();
    desenharImagem(offsetX, offsetY);
    desenharDescricao(offsetX, offsetY);
    desenharCapturas(offsetX, offsetY);
    
    // Limpa o clip rect antes de desenhar os botões que ficam por cima
    SDL_RenderSetClipRect(renderer, NULL); 
    
    desenharBotoes(offsetX, offsetY);
}

/**
 * @brief Desenha o fundo da janela.
 */
void JanelaJogo::desenharFundo() {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, &areaJanela);
}

/**
 * @brief Desenha o cabeçalho e o nome do jogo.
 */
void JanelaJogo::desenharTopo() {
    SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
    SDL_RenderFillRect(renderer, &areaTopo);
    
    int fontSize = 48;
    // Posição ajustada para centralização aproximada do título
    int xPos = 960 - (nomeJogo.length() * 12); 
    MeuProjeto::desenharTexto(renderer, nomeJogo, xPos, 26, {255, 255, 255, 255}, fontSize, MeuProjeto::TipoFonte::NEGRITO);
}

/**
 * @brief Desenha a área de fundo do painel principal.
 */
void JanelaJogo::desenharAreaFundo() {
    SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
    SDL_RenderFillRect(renderer, &areaFundo);
}

/**
 * @brief Desenha a imagem principal/capa do jogo.
 */
void JanelaJogo::desenharImagem(int offsetX, int offsetY) {
    if (imagemJogo) {
        SDL_Rect dst = {areaImagem.x + offsetX, areaImagem.y + offsetY, areaImagem.w, areaImagem.h};
        SDL_RenderCopy(renderer, imagemJogo, nullptr, &dst);
    }
}

/**
 * @brief Desenha a descrição com quebra de linha.
 */
void JanelaJogo::desenharDescricao(int offsetX, int offsetY) {
    int fontSize = 24;
    int linhaAltura = 30;
    
    int x = areaDescricao.x + offsetX + 10;
    int y = areaDescricao.y + offsetY + 10;
    int maxWidth = areaDescricao.w - 20;

    std::stringstream ss(descricao);
    std::string linhaOriginal;

    // Processa linha a linha, respeitando quebras de linha manuais (\n)
    while (std::getline(ss, linhaOriginal, '\n')) {
        std::vector<std::string> linhasQuebradas;
        std::string palavra;
        std::stringstream linhaStream(linhaOriginal);
        std::string linhaAtual;
        
        // Quebra de linha automática (por palavra)
        while (std::getline(linhaStream, palavra, ' ')) {
            // Estimativa de largura usando proporção simples
            int larguraEstimada = (int)(linhaAtual.length() + palavra.length()) * (fontSize / 2);
            
            if (larguraEstimada > maxWidth && !linhaAtual.empty()) {
                linhasQuebradas.push_back(linhaAtual);
                linhaAtual = palavra;
            } else {
                if (!linhaAtual.empty()) linhaAtual += " ";
                linhaAtual += palavra;
            }
        }
        
        if (!linhaAtual.empty()) {
            linhasQuebradas.push_back(linhaAtual);
        }
        
        // Desenha as linhas resultantes
        for (const auto& textoLinha : linhasQuebradas) {
            if (y + linhaAltura > areaDescricao.y + areaDescricao.h) break;
         
            MeuProjeto::desenharTexto(renderer, textoLinha, x, y, {200, 200, 200, 255}, fontSize, MeuProjeto::TipoFonte::NORMAL);
            y += linhaAltura;
        }
    }
}

/**
 * @brief Desenha o carrossel circular de capturas de tela.
 */
void JanelaJogo::desenharCapturas(int offsetX, int offsetY) {
    int numCapturas = static_cast<int>(capturasTexturas.size());

    // Fundo cinza
    SDL_Rect bgCarrossel = {areaFundoCapturas.x + offsetX, areaFundoCapturas.y + offsetY, areaFundoCapturas.w, areaFundoCapturas.h};
    SDL_SetRenderDrawColor(renderer, 50, 50, 60, 255);
    SDL_RenderFillRect(renderer, &bgCarrossel);

    // Título
    MeuProjeto::desenharTexto(renderer, "Capturas de Tela", 50, areaFundoCapturas.y + 10, {255, 255, 255, 255}, 28 ,MeuProjeto::TipoFonte::NEGRITO);
    
    if (numCapturas == 0) {
        return;
    }

    const int MARGEM_SETAS = 160; // Espaço para as setas
    // Área visível do carrossel (recorte)
    SDL_Rect clipRect = {MARGEM_SETAS, CAPTURA_POS_Y, 1920 - (MARGEM_SETAS * 2), CAPTURA_ALTURA};
    SDL_RenderSetClipRect(renderer, &clipRect); // Aplica o recorte

    int centroX = 960; // Centro da tela
    
    for (int i = 0; i < numCapturas; ++i) {
        // Calcula posição relativa ao índice central
        int relPos = i - capturaIndex;
        
        // Ajuste para carrossel circular (mostra apenas vizinhos imediatos)
        if (relPos < -1) relPos += numCapturas;
        if (relPos > 1) relPos -= numCapturas;
        
        // Posição X centralizada com espaçamento
        int posX = centroX + (relPos * CAPTURA_ESPACAMENTO) - (CAPTURA_LARGURA / 2);
        
        // Desenha apenas as capturas visíveis (central, esquerda e direita)
        if (abs(relPos) <= 1) {
            SDL_Rect dst = {
                posX + offsetX,
                CAPTURA_POS_Y + offsetY,
                CAPTURA_LARGURA,
                CAPTURA_ALTURA
            };
            
            SDL_RenderCopy(renderer, capturasTexturas[i], nullptr, &dst);
        }
    }
    
    SDL_RenderSetClipRect(renderer, NULL); // Remove o recorte
}

/**
 * @brief Desenha todos os botões (Jogar, Fechar, Setas).
 */
void JanelaJogo::desenharBotoes(int offsetX, int offsetY) {
    botaoJogar.desenhar(renderer, offsetX, offsetY);
    botaoFechar.desenhar(renderer, offsetX, offsetY);
    setaEsquerda.desenhar(renderer, offsetX, offsetY);
    setaDireita.desenhar(renderer, offsetX, offsetY);
}