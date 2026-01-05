CXX := g++

CXXFLAGS := -std=c++17 -Wall -Wextra -g -I. 
LDFLAGS := -pthread

VPATH = $(SRC_DIR):$(LOG_DIR)

# Diretório para os executáveis finais.
BIN_DIR := bin

# Diretório para os arquivos-objeto intermediários.
BUILD_DIR := build

# Diretórios de código-fonte.
SRC_DIR := src
LOG_DIR := logs

# Executáveis que serão gerados.
TARGET_PM := $(BIN_DIR)/gameman-pm
TARGET_LOG_SERVER := $(BIN_DIR)/gameman-log

# Fontes do serviço ProcessManager (gameman-pm).
PM_SRCS := \
    $(SRC_DIR)/main.cpp \
    $(SRC_DIR)/GameManager.cpp \
    $(LOG_DIR)/SocketLogger.cpp \
    $(LOG_DIR)/UnixSocketClient.cpp \
    $(LOG_DIR)/LogManager.cpp

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
