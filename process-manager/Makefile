CXX := g++

# Includes: 
# -Iinclude: Cabeçalhos locais
# -Ithird-party: Bibliotecas externas (json.hpp)
# -I../logs/include: Cabeçalhos do sistema de log (Corrigido para singular)
CXXFLAGS := -std=c++17 -Wall -Wextra -g -Iinclude -Ithird-party -I../logs/include
LDFLAGS := -pthread

# Diretórios
BIN_DIR := bin
BUILD_DIR := build
SRC_DIR := src
LOGS_SRC_DIR := ../logs/src

# Executável
TARGET_PM := $(BIN_DIR)/process-manager

# Fontes
# Incluímos os arquivos locais e os arquivos necessários do projeto de Logs
PM_SRCS := \
    $(SRC_DIR)/main.cpp \
    $(SRC_DIR)/ProcessManager.cpp \
    $(LOGS_SRC_DIR)/SocketLogger.cpp \
    $(LOGS_SRC_DIR)/UnixSocketClient.cpp \
    $(LOGS_SRC_DIR)/LogManager.cpp

# Objetos
# Usamos notdir para pegar apenas o nome do arquivo (ex: main.o, SocketLogger.o)
# Isso exige que não existam arquivos com mesmo nome em pastas diferentes
PM_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(notdir $(PM_SRCS)))

# VPATH permite que o make encontre os .cpp listados acima
VPATH := $(SRC_DIR):$(LOGS_SRC_DIR)

# Alvo Padrão
all: $(TARGET_PM)

# Regra de Linkagem
$(TARGET_PM): $(PM_OBJS)
	@echo "==> Ligando (linking): $@"
	@mkdir -p $(@D)
	$(CXX) $(PM_OBJS) -o $@ $(LDFLAGS)

# Regra de Compilação
$(BUILD_DIR)/%.o: %.cpp
	@echo "==> Compilando: $<"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MP

# Include de dependências
-include $(BUILD_DIR)/*.d

.PHONY: clean all

clean:
	@echo "==> Limpando..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Fontes do servidor de log (gameman-log).
LOG_SERVER_SRCS := \
    $(LOG_DIR)/LogServer.cpp \
    $(LOG_DIR)/LogManager.cpp \

# Converte a lista de fontes .cpp para uma lista de arquivos-objeto .o no diretório de build.
PM_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(notdir $(PM_SRCS)))
LOG_SERVER_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(notdir $(LOG_SERVER_SRCS)))

# Inclui todos os arquivos de dependência (.d) gerados automaticamente.
DEPS := $(PM_OBJS:.o=.d) $(LOG_SERVER_OBJS:.o=.d)

# O alvo padrão, que é executado quando você digita apenas 'make'.
.PHONY: all
all: $(TARGET_PM) $(TARGET_LOG_SERVER)

# Regra para linkar o executável 'gameman-pm'.
$(TARGET_PM): $(PM_OBJS)
	@echo "==> Ligando (linking) o alvo: $@"
	@mkdir -p $(@D)
	$(CXX) $(PM_OBJS) -o $@ $(LDFLAGS)

# Regra para linkar o executável 'gameman-log'.
$(TARGET_LOG_SERVER): $(LOG_SERVER_OBJS)
	@echo "==> Ligando (linking) o alvo: $@"
	@mkdir -p $(@D)
	$(CXX) $(LOG_SERVER_OBJS) -o $@ $(LDFLAGS)

# Regra genérica para compilar arquivos .cpp para .o no diretório de build.
# -MMD -MP: Gera automaticamente os arquivos de dependência de cabeçalho (.d).
$(BUILD_DIR)/%.o: %.cpp
	@echo "==> Compilando: $<"
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MP

# Inclui os arquivos de dependência gerados para que o 'make' saiba quando recompilar.
-include $(DEPS)

# Alvo para limpar todos os arquivos gerados (executáveis e objetos).
.PHONY: clean
clean:
	@echo "==> Limpando diretórios de build e bin..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)
