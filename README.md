Launcher de Biblioteca de Jogos (DACC Station)

Uma interface gráfica moderna e funcional desenvolvida em C++ e SDL2, projetada para servir como uma central de jogos (Launcher). O projeto foca em uma experiência de usuário (UX) fluida, inspirada no modo "Big Picture", oferecendo suporte completo para controles, navegação híbrida e personalização visual.

Funcionalidades Principais

Navegação Híbrida (Mouse & Gamepad): Sistema inteligente que permite alternar entre mouse, teclado e controles (Xbox/PlayStation/Genéricos) de forma transparente.

Experiência "Big Picture": Interface otimizada para telas grandes com navegação baseada em zonas e foco visual claro.

Gestão de Temas Dinâmicos: Troca instantânea entre os modos Claro e Escuro, afetando cores, ícones e guias de interface.

Busca em Tempo Real: Barra de pesquisa com resultados dinâmicos e suporte a Teclado Virtual integrado para uso com joysticks.

Catálogo Organizado: Filtragem por categorias (Ação, RPG, Casual, etc.) e seleção de jogo aleatório.

Visualização de Detalhes: "Páginas de Produto" individuais com sinopse, carrossel de capturas de tela e execução direta do jogo.

Lançamento de Processos: Capacidade de executar os jogos diretamente da interface através de chamadas de sistema assíncronas.

Persistência de Dados: Carregamento de biblioteca via arquivos de texto externos, facilitando a adição de novos títulos.

Arquitetura do Projeto

O sistema foi desenvolvido seguindo princípios de modularidade e padrões de projeto consolidados:

Padrão Singleton: Utilizado no GerenciadorTemas para manter uma paleta de cores consistente e global.

Padrão Flyweight (Cache): Gerenciadores de Imagens e Fontes que evitam o consumo excessivo de RAM/VRAM ao reutilizar ativos já carregados.

Programação Orientada a Objetos (POO): Extensivo uso de herança (como na classe Botao e BotaoPesquisa) e encapsulamento para separação de lógica e interface.

Gerenciamento de Memória: Implementação de práticas modernas de C++ para evitar vazamentos de memória (RAII).

Tecnologias Utilizadas

Linguagem: C++ (Padrão 11/14/17).

Biblioteca Base: SDL2 (Simple DirectMedia Layer).

Ativos e Fontes: SDL2_image e SDL2_ttf.

Primitivas Gráficas: SDL2_gfx.

Documentação: Doxygen.

Estrutura de Pastas

/cabecalhos: Contém as definições de API (.hpp).

/implementacao: Contém a lógica de funcionamento e algoritmos (.cpp).

/arquivos: Diretório destinado aos dados dos jogos (.txt) e ativos visuais (.png, .jpg).

/Fontes: Arquivos de fonte TrueType (.ttf).

main.cpp: Ponto de entrada e coordenação do loop principal do sistema.

🕹️ Controles
Dispositivo	Ação
Mouse	Clique para interagir, Roda para Scroll.
D-Pad / Analógico	Navegação de foco entre botões.
Botão A (Controle)	Confirmar / Abrir Detalhes (Clique Duplo: Inicia o Jogo).
Botão B (Controle)	Voltar / Sair.
LB / RB	Trocar Categorias.
LT / RT	Trocar Banners de Destaque.
Botão X (Controle)	Abrir Busca.
ESC	Fechar janelas ou sair do programa.
Documentação Técnica

O código está inteiramente documentado no padrão Doxygen.