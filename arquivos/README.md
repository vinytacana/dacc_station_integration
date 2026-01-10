# Documentação de Recursos do DACC Station

Este documento serve como guia técnico para a padronização de ativos visuais, formatação de dados e dependências necessárias para o projeto.

## 1. Dimensões das Imagens

Para garantir a integridade visual da interface, é fundamental que as imagens sigam estritamente as resoluções especificadas abaixo.

**Interface do Usuário (UI):**
O **Botão de Configurações**, localizado no canto superior esquerdo (ex: ConfigClaro.png), deve ter a dimensão de **130 pixels de largura por 180 pixels de altura**. O **Botão de Mudar Tema**, posicionado no canto superior direito (ex: mudarTema.png), deve ter **120 pixels de largura por 120 pixels de altura**.

Para os ícones funcionais, a **Lupa de Pesquisa**, que é renderizada dentro da barra de busca, deve ter **100 pixels por 100 pixels**. O ícone do **Dado (Aleatório)**, exibido ao lado do botão de escolha aleatória, deve ter **150 pixels por 150 pixels**. A imagem de **Overlay de Botões** (explicacaoBotoesClaro.jpg), que aparece no rodapé centralizado, tem um tamanho sugerido de aproximadamente **1000 pixels de largura por 150 pixels de altura**.

As Janelas Pop-up (utilizadas para exibir os Detalhes do Jogo e o menu de Configurações) possuem uma dimensão total de 1525 pixels de largura por 1116 pixels de altura.

**Ativos dos Jogos:**
Para os jogos em destaque no topo da tela, a **Capa de Destaque** deve ser quadrada, com **300 pixels por 300 pixels**. O **Fundo de Destaque**, que é o wallpaper panorâmico exibido atrás dos menus, deve ter a resolução de **1862 pixels de largura por 597 pixels de altura**.

Na lista de grade inferior, a **Capa do Jogo (Lista)** deve ser retangular, com **384 pixels de largura por 328 pixels de altura**. Já na janela de detalhes do jogo, a **Capa Principal** (imagem grande à esquerda) deve ter **990 pixels de largura por 520 pixels de altura**. Por fim, as **Capturas de Tela** (screenshots) exibidas no carrossel devem ter **480 pixels de largura por 270 pixels de altura**.

---

## 2. Formatação de Dados (jogos.txt)

O sistema carrega as informações dos jogos lendo o arquivo `jogos.txt` sequencialmente em blocos. A estrutura é rígida: cada jogo deve ocupar exatamente **8 linhas**, seguidas imediatamente pelo próximo bloco ou fim de arquivo.

A ordem das linhas para cada jogo é a seguinte:
* **Linha 1:** Deve conter o **Nome do Jogo**.
* **Linha 2:** Deve conter a **Categoria** do jogo. As categorias aceitas pelo sistema são: Aventura, Acao, RPG, Casual, Estrategia e Esportes.
* **Linha 3:** Deve conter o caminho relativo para a imagem da **Capa de Detalhes** (dimensão 990x520).
* **Linha 4:** Deve conter o caminho relativo para a imagem da **Capa de Lista/Destaque** (dimensão 300x300 ou 384x328).
* **Linha 5:** Deve conter o caminho relativo para a imagem do **Fundo de Destaque** (dimensão 1862x597).
* **Linha 6:** Deve conter a **Descrição ou Sinopse** do jogo. Este deve ser um texto corrido em uma única linha, sem quebras manuais (Enter).
* **Linha 7:** Deve conter os caminhos para as **Screenshots**, separados apenas por ponto e vírgula (;), sem espaços adicionais entre eles.
* **Linha 8:** Deve conter um **Separador**, geralmente representado por três traços (---) ou uma linha indicativa de fim de bloco.

Exemplo de estrutura de um bloco:
God of War
Acao
arquivos/gow_detalhes.jpg
arquivos/gow_capa.jpg
arquivos/gow_fundo.jpg
Kratos vive agora como um homem no reino dos deuses e monstros nordicos.
arquivos/gow_shot1.jpg;arquivos/gow_shot2.jpg
---

---

## 3. Dependências e Compilação

Para compilar e executar o sistema corretamente, é necessário ter instalado o pacote de desenvolvimento da biblioteca **SDL2** e suas extensões oficiais.

No ambiente Linux (Debian/Ubuntu), os pacotes necessários são:
1.  **libsdl2-dev**: O núcleo da biblioteca, responsável pelo gerenciamento de janelas e eventos.
2.  **libsdl2-image-dev**: Extensão necessária para carregar formatos de imagem como PNG e JPG.
3.  **libsdl2-ttf-dev**: Extensão utilizada para renderizar fontes de texto TrueType.
4.  **libsdl2-gfx-dev**: Extensão utilizada para desenhar formas geométricas primitivas e avançadas.