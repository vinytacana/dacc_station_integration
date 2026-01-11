# 📋 TODO 08: Integração de Dados de Sistema (Header)

**Objetivo:** Implementar o provedor de dados para o cabeçalho (Relógio, Wifi, Bateria) de forma eficiente.

**Arquivos Afetados:**
- `include/SystemStatus.hpp` (Novo)
- `src/SystemStatus.cpp` (Novo)
- `src/main.cpp`

**Especificações Técnicas:**

1.  **Estrutura de Dados:**
    ```cpp
    struct SystemData {
        std::string currentTime; // "14:30"
        bool wifiConnected;
        int batteryLevel;        // 0-100, ou -1 se desktop
    };
    ```

2.  **Classe Provider (Singleton ou Estática):**
    Criar uma classe que atualiza esses dados periodicamente.
    - `void update()`: Chamado no loop principal.
    - **Timer Interno:** Só recupere dados reais do sistema (que podem ser lentos, ex: ler `/sys/class/power_supply`) a cada 2 ou 5 segundos.
    - `SystemData getCachedData()`: Retorna os dados para a UI desenhar a cada frame (sem lag).

3.  **Implementação Mock (Fase 1):**
    - `currentTime`: Usar `std::time`.
    - `wifi`: Retornar `true` fixo.
    - `battery`: Retornar `100` fixo.

**Critério de Sucesso:**
O relógio no canto da tela atualiza os minutos corretamente sem derrubar o FPS da renderização.
